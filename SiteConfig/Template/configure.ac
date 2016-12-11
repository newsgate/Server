# @file   configure.ac
# @author Karen Arutyunov
# $Id:$

AC_INIT([configure.ac])
m4_include([OpenSBE.ac])

OSBE_ENABLE_FEATURE([Deployment])

AC_ARG_VAR([proddir],[Product directory. By default is /opt/NewsGate.])

if test -z $proddir; then
  proddir="/opt/NewsGate"
fi

dnl AC_ARG_ENABLE([proddir],
dnl               AC_HELP_STRING([--proddir],
dnl                              [Product directory. By default is /opt/NewsGate.]),
dnl               [],[
dnl proddir="/opt/NewsGate"
dnl ])

interface_langs="eng rus"

# Callbacks

host_preconfig()
{
  if test -n "$hosts_config"; then
    hosts_config="$hosts_config
"
  fi

  hosts_config="$hosts_config<host name=\"$1\">
"
}

host_postconfig()
{
  hosts_config="$hosts_config</host>
"
}

host_service()
{
  varname=""

  case $2 in
    RSSPullerManager)   varname="server_host_rss_puller_manager";;
    MessageBankManager) varname="server_host_message_bank_manager";;
    WordManager)        varname="server_host_word_manager";;
    Segmentor)          varname="server_host_segmentor";;
    EventBankManager)   varname="server_host_event_bank_manager";;
    FeedManager)        varname="server_host_feed_manager";;
    ModeratorManager)   varname="server_host_moderator_manager";;
    CustomerManager)    varname="server_host_customer_manager";;
    AdManager)          varname="server_host_ad_manager";;
    CategoryManager)    varname="server_host_category_manager";;
    SearchFrontend)     varname="server_host_search_frontend"
                        search_frontend_count=`expr $search_frontend_count + 1`
                        search_frontend_endpoints=\
"$search_frontend_endpoints $1:$service_frontend_search_port";;
    StatProcessor)      varname="server_host_stat_processor";;
    SearchMailer)       varname="server_host_search_mailer";;
    LimitChecker)       varname="server_host_limit_checker";;
    AdServer)           varname="server_host_ad_server";;
    LimitedFrontend)    limited_frontend_count=`expr $limited_frontend_count + 1`
                        limited_frontend_endpoints=\
"$limited_frontend_endpoints $1:$service_frontend_limited_port";;
    MessageBank)        message_bank_count=`expr $message_bank_count + 1`;;
  esac

  if test -n "$varname"; then
    eval srv_host=\$$varname

    if test "$varname" != "server_host_search_frontend" -a -n "$srv_host"; then
      AC_MSG_ERROR([Only 1 instance of $2 is allowed in a cluster; already assigned to $srv_host host])
    fi

    eval $varname="$1"
  fi

  hosts_config="$hosts_config  <service name=\"$2\"/>
"
}

corbaloc_url()
{
  case $orb_id in
    ORBacus) url="corbaloc:iiop:$1:$2";;
    MICO)    url="corbaloc:iiop:$1:$2/Default/PersistentIdPOA";;
    TAO)     url="corbaloc:iiop:$1:$2";;
  esac

  echo "$url"
}

current_host=`hostname`
search_frontend_count="0"
limited_frontend_count="0"
message_bank_count="0"

osbe_config_mech_write_comment "NewsGate Core subsystem includes Message Bank Manager, Message Bank, 
# Word Manager, Segmentor, Search Frontend services."

# Configuring NewsGate optional subsystems set

osbe_config_mech_write_nl
osbe_config_mech_write_comment "NewsGate Optional Subsystems.
#"

feed_moderating="1"
client_moderating="0"
customer_moderating="0"
feed_pulling="1"
event_detection="1"
stat_processing="1"
fraud_prevention="0"
ad_serving="0"
ad_management="0"
limited_frontend="0"
search_mailing="0"
tests="0"

osbe_add_option feed_moderating \
                "$feed_moderating" \
                "subsystem.feed_moderating" \
                "Feed Moderating" \
                "" \
                "0" \
"If 1, then Feed Moderating subsystem will be enabled.
# Implements interface for moderator accounts, feeds and messages management.
# Includes Moderator Manager, Feed Manager, Moderator Frontend services."

osbe_add_option client_moderating \
                "$client_moderating" \
                "subsystem.client_moderating" \
                "Client Moderating" \
                "" \
                "0" \
"If 1, then Client Moderating subsystems will be enabled.
# Implements means for client management.
# Includes Client Manager, Client Controller services."

osbe_add_option customer_moderating \
                "$customer_moderating" \
                "subsystem.customer_moderating" \
                "Customer Moderating" \
                "" \
                "0" \
"If 1, then Customer Moderating subsystems will be enabled.
# Implements means for client management.
# Includes Customer Manager service."

osbe_add_option ad_management \
                "$ad_management" \
                "subsystem.ad_management" \
                "Ad Management" \
                "" \
                "0" \
"If 1, then Ad Management subsystems will be enabled.
# Implements means for ad management.
# Includes Ad Manager service."

osbe_add_option feed_pulling \
                "$feed_pulling" \
                "subsystem.feed_pulling" \
                "Feed Pulling" \
                "" \
                "0" \
"If 1, then Feed Pulling subsystem will be enabled.
# Responsible for requesting items from feeds, collecting feed statistics.
# Includes RSS Puller Manager, RSS Puller services."

osbe_add_option event_detection \
                "$event_detection" \
                "subsystem.event_detection" \
                "Event Detection" \
                "" \
                "0" \
"If 1, then Event Detection subsystem will be enabled.
# Responsible for consolidating similar news items into groups called 'events'.
# Includes Event Bank service."

osbe_add_option stat_processing \
                "$stat_processing" \
                "subsystem.stat_processing" \
                "Stat Processing" \
                "" \
                "0" \
"If 1, then internal Stat Processing subsystem will be enabled,
# If 2, then external Stat Processing service will be used.
# Implements interface for statistics gathering.
# Includes Stat Processor service."

osbe_add_option fraud_prevention \
                "$fraud_prevention" \
                "subsystem.fraud_prevention" \
                "Fraud Prevention" \
                "" \
                "0" \
"If 1, then internal Fraud Prevention subsystem will be enabled,
# If 2, then external Fraud Prevention services will be used.
# Implements interface for fraud prevention actions.
# Includes Limit Checker service."

osbe_add_option ad_serving \
                "$ad_serving" \
                "subsystem.ad_serving" \
                "Ad Serving" \
                "" \
                "0" \
"If 1, then internal Ad Serving subsystem will be enabled,
# If 2, then external Ad Serving services will be used.
# Implements interface for ad serving.
# Includes Ad Server service."

osbe_add_option limited_frontend \
                "$limited_frontend" \
                "subsystem.limited_frontend" \
                "Limited Frontend" \
                "" \
                "0" \
"If 1, then LimitedFrontend will be enabled.
# Responsible for handling restricted number of 'safe' urls."

osbe_add_option search_mailing \
                "$search_mailing" \
                "subsystem.search_mailing" \
                "Search Mailing" \
                "" \
                "0" \
"If 1, then Search Mailing subsystem will be enabled,
# If 2, then external SearchMailer service will be used.
# Includes SearchMailer service."

osbe_add_option tests \
                "$tests" \
                "subsystem.tests" \
                "Tests" \
                "" \
                "0" \
"If 1, then test applications will be configured.
# Includes RSS Feed Provider, RSS Feed Frontend service." 

osbe_ask_for_options "NewsGate Optional Subsystems"

if test $feed_moderating -eq 1; then
  feed_moderating_enabled="true"
else
  feed_moderating_enabled="false"
  client_moderating="0"
fi

if test $client_moderating -eq 1; then
  client_moderating_enabled="true"
else
  client_moderating_enabled="false"
fi

if test $customer_moderating -eq 1; then
  customer_moderating_enabled="true"
else
  customer_moderating_enabled="false"
fi

if test $ad_management -eq 1; then
  ad_management_enabled="true"
else
  ad_management_enabled="false"
fi

if test $feed_pulling -eq 1; then
  feed_pulling_enabled="true"
else
  feed_pulling_enabled="false"
fi

if test $event_detection -eq 1; then
  event_detection_enabled="true"
else
  event_detection_enabled="false"
fi

if test $limited_frontend -eq 1; then
  limited_frontend_enabled="true"
  search_frontend_limited_option=""
else
  limited_frontend_enabled="false"
  search_frontend_limited_option="-D frontend-limited"
fi

if test $stat_processing -eq 1; then
  stat_processing_enabled="true"
else
  stat_processing_enabled="false"
fi

if test $search_mailing -eq 1; then
  search_mailing_enabled="true"
else
  search_mailing_enabled="false"
fi

if test $fraud_prevention -eq 1; then
  limit_checker_enabled="true"
else
  limit_checker_enabled="false"
fi

if test $ad_serving -eq 0; then
  ad_server_enabled="false"
else
  ad_server_enabled="true"
fi

dnl AC_SUBST([proddir])
AC_SUBST([customer_moderating])
AC_SUBST([client_moderating])
AC_SUBST([feed_moderating_enabled])
AC_SUBST([customer_moderating_enabled])
AC_SUBST([ad_management_enabled])
AC_SUBST([client_moderating_enabled])
AC_SUBST([feed_pulling_enabled])
AC_SUBST([event_detection_enabled])
AC_SUBST([stat_processing_enabled])
AC_SUBST([search_mailing_enabled])
AC_SUBST([limit_checker_enabled])
AC_SUBST([ad_server_enabled])
AC_SUBST([ad_management])
AC_SUBST([limited_frontend_enabled])
AC_SUBST([search_mailing])
AC_SUBST([tests])

# Configuring Elements library paths

osbe_config_mech_write_nl
osbe_config_mech_write_comment "Elements Library Paths
#"

osbe_config_mech_write_comment "Elements root directory path."
osbe_ask_for_ne_string "Please enter Elements Library Root Directory Path" \
                       "Elements Root Path" \
                        elements_root \ 
                        "elements.paths.root" \
                        "1"

eval elements_root="$elements_root"
AC_SUBST([elements_root])

osbe_add_option elements_bin \
                "$elements_root/bin" \
                "elements.paths.bin" \
                "Binaries" \
                "" \
                "0" \
"Elements executable binary files directory path."

osbe_add_option elements_lib \
                "$elements_root/lib" \
                "elements.paths.lib" \
                "Libraries" \
                "" \
                "0" \
"Elements shared library files directory path."

osbe_add_option elements_scripts \
                "$elements_root/bin" \
                "elements.paths.scripts" \
                "Scripts" \
                "" \
                "0" \
"Elements script files directory path."


osbe_add_option elements_xsd \
                "$elements_root/xsd/El" \
                "elements.paths.xsd" \
                "XSD" \
                "" \
                "0" \
"Elements XML-schema definition files directory path."

osbe_add_option elements_loc \
                "$elements_root/etc/El/loc" \
                "elements.paths.loc" \
                "Localization" \
                "" \
                "0" \
"Elements localization files directory path."

osbe_add_option elements_dict \
                "$elements_root/etc/El/dict" \
                "elements.paths.dict" \
                "Dictionary" \
                "" \
                "0" \
"Elements dictionary files directory path."

osbe_ask_for_options "Elements Library Paths"

eval elements_bin="$elements_bin"
eval elements_lib="$elements_lib"
eval elements_scripts="$elements_scripts"
eval elements_xsd="$elements_xsd"
eval elements_loc="$elements_loc"
eval elements_dict="$elements_dict"

AC_SUBST([elements_bin])
AC_SUBST([elements_lib])
AC_SUBST([elements_scripts])
AC_SUBST([elements_xsd])
AC_SUBST([elements_loc])
AC_SUBST([elements_dict])

# Configuring bits info

osbe_config_mech_write_nl
osbe_config_mech_write_comment "Bits info
#"

lib_path="$elements_lib/libElements.so"

if test -e "$lib_path"; then

  lib_info=`file -L "$lib_path" 2>/dev/null`
  bit_number=`echo "$lib_info" | grep "32-bit"`

  if test -n "$bit_number"; then
    bit_number="32"
  else
    bit_number="64"
  fi

else

  arc_info=`arch`  
  bit_number=`echo "$arc_info" | grep "x86_64"`

  if test -n "$bit_number"; then
    bit_number="64"
  else
    bit_number="32"
  fi

fi

osbe_config_mech_write_comment "Bit number."
osbe_ask_for_ne_string "Please enter application bit number" \
                       "Bit Number" \
                        bit_number \
                        "bit_number" \
                        "1"

libdir_suffix=""

case $bit_number in
  64) libdir_suffix="64"
      message_bank_capacity="100000000"
      ;;
  32) message_bank_capacity="950000"
      ;;
  *) AC_MSG_ERROR([Unexpected bit number: $bit_number]);;
esac

apache_modules_path="/usr/lib$libdir_suffix/httpd/modules"
apache_app_path="/usr/sbin/httpd.worker."

case $bit_number in
  64) apache_app_path="${apache_app_path}x86_64";;
  32) apache_app_path="${apache_app_path}i386";;
esac

AC_SUBST([apache_modules_path])
AC_SUBST([apache_app_path])

# Configuring ORB info

osbe_config_mech_write_nl
osbe_config_mech_write_comment "ORB info
#"

corba_linkage=`ldd "$elements_lib/libElCorba.so" 2>/dev/null`
orb_id=`echo "$corba_linkage" | grep "libOB.so"`

if test -n "$orb_id"; then
  orb_id="ORBacus"
fi

if test -z "$orb_id"; then
  orb_id=`echo "$corba_linkage" | grep "libmico2."`

  if test -n "$orb_id"; then
    orb_id="MICO"
  fi
fi

if test -z "$orb_id"; then
  orb_id=`echo "$corba_linkage" | grep "libTAO.so"`

  if test -n "$orb_id"; then
    orb_id="TAO"
  fi
fi

osbe_config_mech_write_comment "ORB identifier."
osbe_ask_for_ne_string "Please enter ORB Identifier" \
                       "ORB Id" \
                        orb_id \
                        "orb.id" \
                        "1"

orb_lib=""

case $orb_id in
  ORBacus) orb_lib="/usr/lib$libdir_suffix";;
  MICO)    orb_lib="/opt/mico/lib$libdir_suffix";;
  TAO)     orb_lib="/usr/lib$libdir_suffix";;

  *)       AC_MSG_ERROR([CORBA orb $orb_id is not recognized])
           ;;
esac

# Configuring ORB lib path

osbe_config_mech_write_comment "ORB library directory path."
osbe_ask_for_ne_string "Please enter ORB Library Directory Path" \
                       "ORB Library Directory Path" \
                        orb_lib \
                        "orb.paths.library" \
                        "1"

eval orb_lib="$orb_lib"
AC_SUBST([orb_lib])

# Configuring NewsGate paths

osbe_config_mech_write_nl
osbe_config_mech_write_comment "NewsGate Server Paths
#"

osbe_config_mech_write_comment "NewsGate root directory path."
osbe_ask_for_ne_string "Please enter NewsGate Server Root Directory Path" \
                       "NewsGate Root Path" \
                        server_root \
                        "server.paths.root" \
                        "1"

eval server_root="$server_root"
AC_SUBST([server_root])

osbe_add_option server_bin \
                "$server_root/bin" \
                "server.paths.bin" \
                "Binaries" \
                "" \
                "0" \
"NewsGate executable binary files directory path."

osbe_add_option server_lib \
                "$server_root/lib" \
                "server.paths.lib" \
                "Libraries" \
                "" \
                "0" \
"NewsGate shared library files directory path."

if test -z "$server_plugins"; then
  server_plugins="$server_root/plugins"
fi

osbe_add_option server_plugins \
                "$server_plugins" \
                "server.paths.plugins" \
                "Plugins" \
                "Plugin Directory Space-separated List" \
                "1" \
"NewsGate plugin files directory path."

osbe_add_option server_scripts \
                "$server_root/bin" \
                "server.paths.scripts" \
                "Scripts" \
                "" \
                "0" \
"NewsGate script files directory path."

osbe_add_option server_xsd \
                "$server_root/xsd/NewsGate" \
                "server.paths.xsd" \
                "XSD" \
                "" \
                "0" \
"NewsGate XML-schema definition files directory path."

osbe_add_option site_config_root \
                "$server_root/etc/NewsGate" \
                "server.paths.site_config" \
                "Site Config" \
                "" \
                "0" \
"NewsGate configuration files directory path."

osbe_add_option data_root \
                "$server_root/var/lib" \
                "server.paths.data" \
                "Data" \
                "" \
                "0" \
"NewsGate data files directory path."

osbe_add_option www_root \
                "$server_root/www/NewsGate" \
                "server.paths.www" \
                "WWW" \
                "" \
                "0" \
"NewsGate WEB-server files directory path."

osbe_add_option run_root \
                "$server_root/var/run" \
                "server.paths.run" \
                "Run" \
                "" \
                "0" \
"NewsGate run-time files directory path."

osbe_add_option cache_root \
                "$server_root/var/cache" \
                "server.paths.cache" \
                "Cache" \
                "" \
                "0" \
"NewsGate cache files directory path."

osbe_add_option log_root \
                "$server_root/var/log" \
                "server.paths.log" \
                "Log" \
                "" \
                "0" \
"NewsGate log files directory path."

osbe_add_option tmp_root \
                "$server_root/var/tmp" \
                "server.paths.tmp" \
                "Tmp" \
                "" \
                "0" \
"NewsGate temporary files directory path."

osbe_add_option ssh_key_file \
                "~/.ssh/adkey" \
                "server.paths.sshkey" \
                "SSH Key" \
                "" \
                "0" \
"Path to ssh private key used by NewsGate scripts for executing commands
# on server machines."

osbe_ask_for_options "NewsGate Server Paths"

eval server_bin="$server_bin"
eval server_lib="$server_lib"
eval server_scripts="$server_scripts"
eval server_xsd="$server_xsd"
eval site_config_root="$site_config_root"
eval data_root="$data_root"
eval www_root="$www_root"
eval run_root="$run_root"
eval cache_root="$cache_root"
eval log_root="$log_root"
eval tmp_root="$tmp_root"
eval ssh_key_file="$ssh_key_file"
www_templates="$www_root/templates"

plugin_paths=""

for path in $server_plugins; do
  eval path="$path"
  plugin_paths="$plugin_paths:$path"
done

AC_SUBST([server_bin])
AC_SUBST([server_lib])
AC_SUBST([server_scripts])
AC_SUBST([server_xsd])
AC_SUBST([site_config_root])
AC_SUBST([data_root])
AC_SUBST([www_root])
AC_SUBST([run_root])
AC_SUBST([cache_root])
AC_SUBST([log_root])
AC_SUBST([tmp_root])
AC_SUBST([ssh_key_file])
AC_SUBST([www_templates])
AC_SUBST([plugin_paths])

# Configuring NewsGate Server base port

osbe_config_mech_write_nl
osbe_config_mech_write_comment "NewsGate Server Base Port.
# Assigns services TCP port default values in range [base_port, base_port+99]"

server_port_base=7100

osbe_ask_for_ne_string "Please enter NewsGate Server Port Base Number" \
                       "NewsGate Server Port Base" \
                        server_port_base \ 
                        "server.ports.base" \
                        "1"

AC_SUBST([server_port_base])

# Configuring NewsGate Services settings

osbe_config_mech_write_nl
osbe_config_mech_write_comment "NewsGate Services Settings.
# Log level defines maximum severity of records to be logged. Possible values:
# 0 (EMERGENCY), 1 (ALERT), 2 (ERROR), 3 (CRITICAL), 4 (WARNING), 5 (NOTICE), 
# 6 (INFO), 7 (DEBUG), 8 (LOW TRACE), 9 (MIDDLE TRACE), 10 (HIGH TRACE).
# Aspects specify (separated by comma) which service subsystems should 
# write trace records. * character mean all subsystems."

service_moderator_manager_port=`expr $server_port_base + 0`
service_moderator_manager_log_level="10"
service_moderator_manager_log_aspects="*"

service_customer_manager_port=`expr $server_port_base + 12`
service_customer_manager_log_level="10"
service_customer_manager_log_aspects="*"

service_ad_manager_port=`expr $server_port_base + 15`
service_ad_manager_log_level="10"
service_ad_manager_log_aspects="*"

service_feed_manager_port=`expr $server_port_base + 1`
service_feed_manager_log_level="10"
service_feed_manager_log_aspects="*"

service_rss_puller_manager_port=`expr $server_port_base + 2`
service_rss_puller_manager_log_level="10"
service_rss_puller_manager_log_aspects="*"

service_rss_puller_port=`expr $server_port_base + 3`
service_rss_puller_log_level="10"
service_rss_puller_log_aspects="*"

service_message_bank_manager_port=`expr $server_port_base + 4`
service_message_bank_manager_log_level="10"
service_message_bank_manager_log_aspects="*"

service_message_bank_port=`expr $server_port_base + 5`
service_message_bank_log_level="10"
service_message_bank_log_aspects="*"

service_word_manager_port=`expr $server_port_base + 6`
service_word_manager_log_level="10"
service_word_manager_log_aspects="*"

service_segmentor_port=`expr $server_port_base + 7`
service_segmentor_log_level="10"
service_segmentor_log_aspects="*"

service_event_bank_manager_port=`expr $server_port_base + 8`
service_event_bank_manager_log_level="10"
service_event_bank_manager_log_aspects="*"

service_event_bank_port=`expr $server_port_base + 9`
service_event_bank_log_level="10"
service_event_bank_log_aspects="*"

service_category_manager_port=`expr $server_port_base + 10`
service_category_manager_log_level="10"
service_category_manager_log_aspects="*"

service_stat_processor_port=`expr $server_port_base + 11`
service_stat_processor_log_level="10"
service_stat_processor_log_aspects="*"
service_stat_processor_raw_stat_keep_days="0"

service_limit_checker_port=`expr $server_port_base + 13`
service_limit_checker_log_level="10"
service_limit_checker_log_aspects="*"

service_ad_server_port=`expr $server_port_base + 14`
service_ad_server_log_level="10"
service_ad_server_log_aspects="*"

service_search_mailer_port=`expr $server_port_base + 16`
service_search_mailer_log_level="10"
service_search_mailer_log_aspects="*"

service_db_port=`expr $server_port_base + 36`

service_frontend_search_port=`expr $server_port_base + 80`
service_frontend_search_hostname="$current_host:$service_frontend_search_port"
service_frontend_search_log_level="10"
service_frontend_search_log_aspects="*"

service_frontend_moderator_port=`expr $server_port_base + 81`
service_frontend_moderator_hostname="$current_host:$service_frontend_moderator_port"
service_frontend_moderator_log_level="10"
service_frontend_moderator_log_aspects="*"
service_frontend_moderator_ssl_use="0"

service_frontend_limited_port=`expr $server_port_base + 82`
service_frontend_limited_hostname="$current_host:$service_frontend_limited_port"
service_frontend_limited_log_level="10"
service_frontend_limited_log_aspects="*"

if test $feed_moderating -eq 1; then

  osbe_add_option service_moderator_manager_port \
                  "$service_moderator_manager_port" \
                  "service.moderator_manager.port" \
                  "Moderator Manager Port" \
                  "" \
                  "0" \
  "
# Moderator Manager service provide means for moderator accounts management.
#
# Moderator Manager service TCP port."

  osbe_add_option service_moderator_manager_log_level \
                  "$service_moderator_manager_log_level" \
                  "service.moderator_manager.log.level" \
                  "Moderator Manager Log Level" \
                  "" \
                  "0" \
  "Moderator Manager service log level."

  osbe_add_option service_moderator_manager_log_aspects \
                  "$service_moderator_manager_log_aspects" \
                  "service.moderator_manager.log.aspects" \
                  "Moderator Manager Log Aspects" \
                  "" \
                  "0" \
  "Moderator Manager service log aspects. Available: Application, State."

  osbe_add_option service_feed_manager_port \
                  "$service_feed_manager_port" \
                  "service.feed_manager.port" \
                  "Feed Manager Port" \
                  "" \
                  "0" \
  "
# Feed Manager service provide means for feed and message management.
#
# Feed Manager service TCP port."

  osbe_add_option service_feed_manager_log_level \
                  "$service_feed_manager_log_level" \
                  "service.feed_manager.log.level" \
                  "Feed Manager Log Level" \
                  "" \
                  "0" \
  "Feed Manager service log level."

  osbe_add_option service_feed_manager_log_aspects \
                  "$service_feed_manager_log_aspects" \
                  "service.feed_manager.log.aspects" \
                  "Feed Manager Log Aspects" \
                  "" \
                  "0" \
  "Feed Manager service log aspects. Available: Application, State."

  osbe_add_option service_category_manager_port \
                  "$service_category_manager_port" \
                  "service.category_manager.port" \
                  "Category Manager Port" \
                  "" \
                  "0" \
  "
# Category Manager service provide means for message category management.
#
# Category Manager service TCP port."

  osbe_add_option service_category_manager_log_level \
                  "$service_category_manager_log_level" \
                  "service.category_manager.log.level" \
                  "Category Manager Log Level" \
                  "" \
                  "0" \
  "Category Manager service log level."

  osbe_add_option service_category_manager_log_aspects \
                  "$service_category_manager_log_aspects" \
                  "service.category_manager.log.aspects" \
                  "Category Manager Log Aspects" \
                  "" \
                  "0" \
  "Category Manager service log aspects. Available: Application, State."
fi

if test $customer_moderating -eq 1; then

  osbe_add_option service_customer_manager_port \
                  "$service_customer_manager_port" \
                  "service.customer_manager.port" \
                  "Customer Manager Port" \
                  "" \
                  "0" \
  "
# Customer Manager service provide means for customer info management.
#
# Customer Manager service TCP port."

  osbe_add_option service_customer_manager_log_level \
                  "$service_customer_manager_log_level" \
                  "service.customer_manager.log.level" \
                  "Customer Manager Log Level" \
                  "" \
                  "0" \
  "Customer Manager service log level."

  osbe_add_option service_customer_manager_log_aspects \
                  "$service_customer_manager_log_aspects" \
                  "service.customer_manager.log.aspects" \
                  "Customer Manager Log Aspects" \
                  "" \
                  "0" \
  "Customer Manager service log aspects. Available: Application, State."
fi

if test $ad_management -eq 1; then

  osbe_add_option service_ad_manager_port \
                  "$service_ad_manager_port" \
                  "service.ad_manager.port" \
                  "Ad Manager Port" \
                  "" \
                  "0" \
  "
# Ad Manager service provide means for ad info management.
#
# Ad Manager service TCP port."

  osbe_add_option service_ad_manager_log_level \
                  "$service_ad_manager_log_level" \
                  "service.ad_manager.log.level" \
                  "Ad Manager Log Level" \
                  "" \
                  "0" \
  "Ad Manager service log level."

  osbe_add_option service_ad_manager_log_aspects \
                  "$service_ad_manager_log_aspects" \
                  "service.ad_manager.log.aspects" \
                  "Ad Manager Log Aspects" \
                  "" \
                  "0" \
  "Ad Manager service log aspects. Available: Application, State."
fi

if test $feed_pulling -eq 1; then
  osbe_add_option service_rss_puller_manager_port \
                  "$service_rss_puller_manager_port" \
                  "service.rss_puller_manager.port" \
                  "RSS Puller Manager Port" \
                  "" \
                  "0" \
  "
# RSS Puller Manager service controls one or more RSS Puller services.
#
# RSS Puller Manager service TCP port."

  osbe_add_option service_rss_puller_manager_log_level \
                  "$service_rss_puller_manager_log_level" \
                  "service.rss_puller_manager.log.level" \
                  "RSS Puller Manager Log Level" \
                  "" \
                  "0" \
  "RSS Puller Manager service log level."

  osbe_add_option service_rss_puller_manager_log_aspects \
                  "$service_rss_puller_manager_log_aspects" \
                  "service.rss_puller_manager.log.aspects" \
                  "RSS Puller Manager Log Aspects" \
                  "" \
                  "0" \
  "RSS Puller Manager service log aspects.
# Available: Application, State, PullingFeeds, FeedManagement."

  osbe_add_option service_rss_puller_port \
                  "$service_rss_puller_port" \
                  "service.rss_puller.port" \
                  "RSS Puller Port" \
                  "" \
                  "0" \
  "
# RSS Puller service request items from feeds pushing new items to
# Message Bank services.
#
# RSS Puller service TCP port."

  osbe_add_option service_rss_puller_log_level \
                  "$service_rss_puller_log_level" \
                  "service.rss_puller.log.level" \
                  "RSS Puller Log Level" \
                  "" \
                  "0" \
  "RSS Puller service log level."

  osbe_add_option service_rss_puller_log_aspects \
                  "$service_rss_puller_log_aspects" \
                  "service.rss_puller.log.aspects" \
                  "RSS Puller Log Aspects" \
                  "" \
                  "0" \
  "RSS Puller service log aspects.
# Available: Application, State, PullingFeeds, MessageManagement."

fi

if test $stat_processing -eq 1; then

  osbe_add_option service_stat_processor_port \
                  "$service_stat_processor_port" \
                  "service.stat_processor.port" \
                  "Stat Processor Port" \
                  "" \
                  "0" \
  "
# Stat Processor service provide means for statistics gathering.
#
# Stat Processor service TCP port."

  osbe_add_option service_stat_processor_log_level \
                  "$service_stat_processor_log_level" \
                  "service.stat_processor.log.level" \
                  "Stat Processor Log Level" \
                  "" \
                  "0" \
  "Stat Processor service log level."

  osbe_add_option service_stat_processor_log_aspects \
                  "$service_stat_processor_log_aspects" \
                  "service.stat_processor.log.aspects" \
                  "Stat Processor Log Aspects" \
                  "" \
                  "0" \
  "Stat Processor service log aspects. Available: Application, State."

  osbe_add_option service_stat_processor_raw_stat_keep_days \
                  "$service_stat_processor_raw_stat_keep_days" \
                  "service.stat_processor.raw_stat_keep_days" \
                  "Stat Processor Days To Keep Raw Stat" \
                  "" \
                  "0" \
  "Stat Processor service days to keep raw stat."

elif test $stat_processing -eq 2; then

  osbe_add_option corbaloc_stat_processor_ref \
                  "" \
                  "service.stat_processor.external_ref" \
                  "Stat Processor External Reference (CORBA ref)" \
                  "" \
                  "1" \
  "
# Stat Processor service provide means for statistics gathering.
#
# External Stat Processor reference."

fi

service_search_mailer_trust_timeout="0"

if test $search_mailing -eq 1; then

  osbe_add_option service_search_mailer_port \
                  "$service_search_mailer_port" \
                  "service.search_mailer.port" \
                  "Search Mailer Port" \
                  "" \
                  "0" \
  "
# Search Mailer service provide means for mailing search results.
#
# Search Mailer service TCP port."

  osbe_add_option service_search_mailer_log_level \
                  "$service_search_mailer_log_level" \
                  "service.search_mailer.log.level" \
                  "Search Mailer Log Level" \
                  "" \
                  "0" \
  "Search Mailer service log level."

  osbe_add_option service_search_mailer_log_aspects \
                  "$service_search_mailer_log_aspects" \
                  "service.search_mailer.log.aspects" \
                  "Search Mailer Log Aspects" \
                  "" \
                  "0" \
  "Search Mailer service log aspects. Available: Application, State."

  osbe_add_option service_search_mailer_email \
                  "" \
                  "service.search_mailer.email" \
                  "Search Mailer Email" \
                  "" \
                  "0" \
  "Search Mailer email set to From header of outgoing emails."

  osbe_add_option service_search_mailer_debug_email \
                  "" \
                  "service.search_mailer.debug_email" \
                  "Search Mailer Debug Email" \
                  "" \
                  "0" \
  "Search Mailer email where outgoing emails will be copied."

  osbe_add_option service_search_mailer_recipient_blacklist \
                  "" \
                  "service.search_mailer.recipient_blacklist" \
                  "Search Mailer Email Blacklist" \
                  "" \
                  "0" \
  "Search Mailer emails which messages will not be sent."

  osbe_add_option service_search_mailer_reserve_message_bank_manager_ref \
                  "" \
                  "service.search_mailer.reserve_message_bank_manager" \
                  "Search Mailer Reserve Message Bank Manager Ref" \
                  "" \
                  "0" \
  "Search Mailer Reserve Message Bank Manager reference."

elif test $search_mailing -eq 2; then

  osbe_add_option corbaloc_search_mailer_ref \
                  "" \
                  "service.search_mailer.external_ref" \
                  "Search Mailer External Reference (CORBA ref)" \
                  "" \
                  "1" \
  "
# Search Mailer service provide means for mailing search results.
#
# External Search Mailer reference."

fi

if test $search_mailing -ne 0; then

  osbe_add_option service_search_mailer_trust_timeout \
                  "0" \
                  "service.search_mailer.trust_timeout" \
                  "Search Mailer Trust Timeout" \
                  "" \
                  "0" \
  "Search Mailer uid+ip+ua+session trust timeout."

  osbe_add_option service_search_mailer_recaptcha_client_key \
                  "" \
                  "service.search_mailer.recaptcha.client_key" \
                  "Search Mailer Recaptcha Client Key" \
                  "" \
                  "0" \
  "Search Mailer Recaptcha key used client side."

  osbe_add_option service_search_mailer_recaptcha_server_key \
                  "" \
                  "service.search_mailer.recaptcha.server_key" \
                  "Search Mailer Recaptcha Server Key" \
                  "" \
                  "0" \
  "Search Mailer Recaptcha key used server side."

fi

if test $fraud_prevention -eq 1; then

  osbe_add_option service_limit_checker_port \
                  "$service_limit_checker_port" \
                  "service.limit_checker.port" \
                  "Limit Checker Port" \
                  "" \
                  "0" \
  "
# Limit Checker service provide means for events frequency check and limitation.
#
# Limit Checker service TCP port."

  osbe_add_option service_limit_checker_log_level \
                  "$service_limit_checker_log_level" \
                  "service.limit_checker.log.level" \
                  "Limit Checker Log Level" \
                  "" \
                  "0" \
  "Limit Checker service log level."

  osbe_add_option service_limit_checker_log_aspects \
                  "$service_limit_checker_log_aspects" \
                  "service.limit_checker.log.aspects" \
                  "Limit Checker Log Aspects" \
                  "" \
                  "0" \
  "Limit Checker service log aspects. Available: Application, State."

elif test $fraud_prevention -eq 2; then

  osbe_add_option corbaloc_limit_checker_ref \
                  "" \
                  "service.limit_checker.external_ref" \
                  "Limit Checker External Reference (CORBA ref)" \
                  "" \
                  "1" \
  "
# Limit Checker service provide means for events frequency check and limitation.
#
# External Limit Checker reference."

fi

if test $ad_serving -ne 0; then

  osbe_add_option service_ad_server_port \
                  "$service_ad_server_port" \
                  "service.ad_server.port" \
                  "Ad Server Port" \
                  "" \
                  "0" \
  "
# Ad Server service provide means for ad serving.
#
# Ad Server service TCP port."

  osbe_add_option service_ad_server_log_level \
                  "$service_ad_server_log_level" \
                  "service.ad_server.log.level" \
                  "Ad Server Log Level" \
                  "" \
                  "0" \
  "Ad Server service log level."

  osbe_add_option service_ad_server_log_aspects \
                  "$service_ad_server_log_aspects" \
                  "service.ad_server.log.aspects" \
                  "Ad Server Log Aspects" \
                  "" \
                  "0" \
  "Ad Server service log aspects. Available: Application, State."

  if test $ad_serving -eq 2; then

    osbe_add_option corbaloc_ad_server_ext_ref \
                    "" \
                    "service.ad_server.external_ref" \
                    "Ad Server External Reference (CORBA ref)" \
                    "" \
                    "1" \
    "
# Ad Server service provide means for ad serving.
#
# External Ad Server reference."
  fi

fi

osbe_add_option service_message_bank_manager_port \
                "$service_message_bank_manager_port" \
                "service.message_bank_manager.port" \
                "Message Bank Manager Port" \
                 "" \
                 "0" \
"
# Message Bank Manager service controls one or more Message Bank services.
#
# Message Bank Manager service TCP port."

osbe_add_option service_message_bank_manager_log_level \
                "$service_message_bank_manager_log_level" \
                "service.message_bank_manager.log.level" \
                "Message Bank Manager Log Level" \
                "" \
                "0" \
"Message Bank Manager service log level."

osbe_add_option service_message_bank_manager_log_aspects \
                "$service_message_bank_manager_log_aspects" \
                "service.message_bank_manager.log.aspects" \
                "Message Bank Manager Log Aspects" \
                "" \
                "0" \
"Message Bank Manager service log aspects.
# Available: Application, State, MessageSharing, 
# MessageFilterDistribution."

osbe_add_option service_message_bank_port \
                "$service_message_bank_port" \
                "service.message_bank.port" \
                "Message Bank Port" \
                 "" \
                 "0" \
"
# Message Bank service stores feed items and maintain search index.
#
#Message Bank service TCP port."

osbe_add_option service_message_bank_log_level \
                "$service_message_bank_log_level" \
                "service.message_bank.log.level" \
                "Message Bank Log Level" \
                "" \
                "0" \
"Message Bank service log level."

osbe_add_option service_message_bank_log_aspects \
                "$service_message_bank_log_aspects" \
                "service.message_bank.log.aspects" \
                "Message Bank Log Aspects" \
                "" \
                "0" \
"Message Bank service log aspects.
# Available: Application, State, MessageSharing, MessageManagement,
# DBPerformance, MessageCategorization."

osbe_add_option service_word_manager_port \
                "$service_word_manager_port" \
                "service.word_manager.port" \
                "Word Manager Port" \
                 "" \
                 "0" \
"
# Word Manager service provides clients with morphology information.
#
# Word Manager service TCP port."

osbe_add_option service_word_manager_log_level \
                "$service_word_manager_log_level" \
                "service.word_manager.log.level" \
                "Word Manager Log Level" \
                "" \
                "0" \
"Word Manager service log level."

osbe_add_option service_word_manager_log_aspects \
                "$service_word_manager_log_aspects" \
                "service.word_manager.log.aspects" \
                "Word Manager Log Aspects" \
                "" \
                "0" \
"Word Manager service log aspects. Available: Application, State."

osbe_add_option service_segmentor_port \
                "$service_segmentor_port" \
                "service.segmentor.port" \
                "Segmentor Port" \
                 "" \
                 "0" \
"
# Segmentor service performs segmentation of feed items and queries written
# in languages such as Korean, Chinese, Japanese.
#
# Segmentor service TCP port."

osbe_add_option service_segmentor_log_level \
                "$service_segmentor_log_level" \
                "service.segmentor.log.level" \
                "Segmentor Log Level" \
                "" \
                "0" \
"Segmentor service log level."

osbe_add_option service_segmentor_log_aspects \
                "$service_segmentor_log_aspects" \
                "service.segmentor.log.aspects" \
                "Segmentor Log Aspects" \
                "" \
                "0" \
"Segmentor service log aspects. Available: Application, State."

osbe_add_option service_segmentor_factories \
                "$service_segmentor_factories" \
                "service.segmentor.factories" \
                "Segmentor factory functions" \
                "Segmentor factory functions in (func_name@func_args@lib_path )* form" \
                "1" \
"Segmentor service factories.
# Space-separated tripples of func_name@func_args@lib_path form."

if test $event_detection -eq 1; then

  osbe_add_option service_event_bank_manager_port \
                  "$service_event_bank_manager_port" \
                  "service.event_bank_manager.port" \
                  "Event Bank Manager Port" \
                   "" \
                   "0" \
"
# Event Bank Manager service controls one or more Event Bank services.
#
# Event Bank Manager service TCP port."

  osbe_add_option service_event_bank_manager_log_level \
                  "$service_event_bank_manager_log_level" \
                  "service.event_bank_manager.log.level" \
                  "Event Bank Manager Log Level" \
                  "" \
                  "0" \
"Event Bank Manager service log level."

  osbe_add_option service_event_bank_manager_log_aspects \
                  "$service_event_bank_manager_log_aspects" \
                  "service.event_bank_manager.log.aspects" \
                  "Event Bank Manager Log Aspects" \
                  "" \
                  "0" \
"Event Bank Manager service log aspects.
# Available: Application, State."

  osbe_add_option service_event_bank_port \
                  "$service_event_bank_port" \
                  "service.event_bank.port" \
                  "Event Bank Port" \
                   "" \
                   "0" \
  "
# Event Bank service groups similar messages into events.
#
# Event Bank service TCP port."

  osbe_add_option service_event_bank_log_level \
                  "$service_event_bank_log_level" \
                  "service.event_bank.log.level" \
                  "Event Bank Log Level" \
                  "" \
                  "0" \
  "Event Bank service log level."

  osbe_add_option service_event_bank_log_aspects \
                  "$service_event_bank_log_aspects" \
                  "service.event_bank.log.aspects" \
                  "Event Bank Log Aspects" \
                  "" \
                  "0" \
  "Event Bank service log aspects. Available: Application, State, EventManagement."

fi

osbe_add_option service_db_port \
                "$service_db_port" \
                "service.db.port" \
                "Data Base Port" \
                "" \
                "0" \
"
# MySQL DB instance running on each machine in cluster holds different objects
# depending on configured service set.
#
# MySQL DB instance TCP port."

osbe_add_option service_frontend_username \
                "$USER" \
                "service.frontend.username" \
                "Frontend User Name" \
                "" \
                "0" \
"
# Frontend is a WEB server providing system access to end-users and
# third-party applications.
#
# User name under which HTTP frontend is running."

osbe_add_option service_frontend_usergroup \
                "users" \
                "service.frontend.usergroup" \
                "Frontend User Group" \
                "" \
                "0" \
"User group under which HTTP frontend is running."

osbe_add_option service_frontend_development_mode \
                "0" \
                "service.frontend.development_mode" \
                "Frontend Development Mode" \
                "" \
                "0" \
"If 1, then HTTP frontend refers original (not compound) resources."

osbe_add_option service_frontend_debug_info \
                "0" \
                "service.frontend.debug_info" \
                "Frontend Debug Info Availability" \
                "" \
                "0" \
"If 1, then HTTP frontend provide controls for debug information management."

osbe_add_option service_frontend_error_in_response \
                "0" \
                "service.frontend.error_in_response" \
                "Frontend Error Description In Response Availability" \
                "" \
                "0" \
"If 1, then HTTP frontend outputs comprehensive error descriptions into
# browser. Use with care as it breaches security practices exposing server
# internals."

osbe_add_option service_frontend_search_port \
                "$service_frontend_search_port" \
                "service.frontend.search.port" \
                "Search Frontend Port" \
                "" \
                "0" \
"
# Search Frontend is a group of WEB services providing search functionality
# to end-users and third-party applications.
#
# Search Frontend TCP port."

osbe_add_option service_frontend_search_hostname \
                "$service_frontend_search_hostname" \
                "service.frontend.search.hostname" \
                "Search Frontend Host[:Port]" \
                "" \
                "0" \
"Search Frontend endpoint available to end-users in host or host:port forms."

osbe_add_option service_frontend_search_log_level \
                "$service_frontend_search_log_level" \
                "service.frontend.search.log.level" \
                "Search Frontend Log Level" \
                "" \
                "0" \
"Search Frontend log level."

osbe_add_option service_frontend_search_log_aspects \
                "$service_frontend_search_log_aspects" \
                "service.frontend.search.log.aspects" \
                "Search Frontend Log Aspects" \
                "" \
                "0" \
"Search Frontend log aspects.
# Available: Search, SearchEngine, Message, WordEngine, ThumbnailModule,
# PSP_Module."

osbe_add_option service_frontend_search_ensure_canonical \
                "0" \
                "service.frontend.search.ensure_canonical" \
                "Search Frontend Ensure Canonical" \
                "" \
                "0" \
"If 1 then sets PSP_CanonicalEndpoint to Search Frontend endpoint value"

osbe_add_option service_frontend_search_interceptor \
                "" \
                "service.frontend.search.interceptor" \
                "Search Frontend Interceptor File" \
                "" \
                "0" \
"Search Frontend Interceptor File Path."

osbe_add_option service_frontend_search_robots_txt \
                "" \
                "service.frontend.search.robots_txt" \
                "Search Frontend robots.txt Directives File" \
                "" \
                "0" \
"File path to directives to be incorporated into robots.txt."

osbe_add_option service_frontend_search_www_extension \
                "" \
                "service.frontend.search.www_extension" \
                "Search Frontend WWW Extension" \
                "" \
                "0" \
"Directory to appear under search frontend root path."

osbe_add_option service_frontend_search_psp_extension \
                "" \
                "service.frontend.search.psp_extension" \
                "Search Frontend PSP Extension" \
                "" \
                "0" \
"Directory with psp extensions."

if test $limited_frontend -eq 1; then
  osbe_add_option service_frontend_limited_port \
                  "$service_frontend_limited_port" \
                  "service.frontend.limited.port" \
                  "Limited Frontend Port" \
                  "" \
                  "0" \
"
# Limited Frontend is a group of WEB services providing thumbnail, message and 
# statistics logging requests processing functionality
#
# Limited Frontend TCP port."

  osbe_add_option service_frontend_limited_hostname \
                  "$service_frontend_limited_hostname" \
                  "service.frontend.limited.hostname" \
                  "Limited Frontend Host[:Port]" \
                  "" \
                  "0" \
"Limited Frontend endpoint available to end-users in host or host:port forms."

  osbe_add_option service_frontend_limited_log_level \
                  "$service_frontend_limited_log_level" \
                  "service.frontend.limited.log.level" \
                  "Limited Frontend Log Level" \
                  "" \
                  "0" \
"Limited Frontend log level."

  osbe_add_option service_frontend_limited_log_aspects \
                  "$service_frontend_limited_log_aspects" \
                  "service.frontend.limited.log.aspects" \
                  "Limited Frontend Log Aspects" \
                  "" \
                  "0" \
"Limited Frontend log aspects.
# Available: Message, ThumbnailModule, PSP_Module."

fi

if test $feed_moderating -eq 1; then
  osbe_add_option service_frontend_moderator_port \
                  "$service_frontend_moderator_port" \
                  "service.frontend.moderator.port" \
                  "Moderator Frontend Port" \
                  "" \
                  "0" \
  "
# Moderator Frontend is a group of WEB services providing feed management
# functionality to dedicated users - moderators.
#
# Moderator Frontend TCP port."

  osbe_add_option service_frontend_moderator_hostname \
                  "$service_frontend_moderator_hostname" \
                  "service.frontend.moderator.hostname" \
                  "Moderator Frontend Host[:Port]" \
                  "" \
                  "0" \
  "Moderator Frontend endpoint available to end-users in host or host:port forms."

  osbe_add_option service_frontend_moderator_log_level \
                  "$service_frontend_moderator_log_level" \
                  "service.frontend.moderator.log.level" \
                  "Moderator Frontend Log Level" \
                  "" \
                  "0" \
  "Moderator Frontend log level."

  osbe_add_option service_frontend_moderator_log_aspects \
                  "$service_frontend_moderator_log_aspects" \
                  "service.frontend.moderator.log.aspects" \
                  "Moderator Frontend Log Aspects" \
                  "" \
                  "0" \
  "Moderator Frontend log aspects. Available: Moderation, PSP_Module."

  osbe_add_option service_frontend_moderator_ssl_use \
                  "0" \
                  "service.frontend.moderator.ssl.use" \
                  "Moderator Frontend SSL Usage" \
                  "" \
                  "0" \
  "If 1, then Moderator Frontend will be available through HTTPS but not HTTP."

  osbe_add_option service_frontend_moderator_ssl_cert \
                  "" \
                  "service.frontend.moderator.ssl.cert" \
                  "Moderator Frontend SSL Certificate File" \
                  "" \
                  "1" \
  "Moderator Frontend SSL Certificate File path."

  osbe_add_option service_frontend_moderator_ssl_key \
                  "" \
                  "service.frontend.moderator.ssl.key" \
                  "Moderator Frontend SSL Certificate Key File" \
                  "" \
                  "1" \
  "Moderator Frontend SSL Certificate Key File path."

  osbe_add_option service_frontend_moderator_ssl_chain \
                  "" \
                  "service.frontend.moderator.ssl.chain" \
                  "Moderator Frontend SSL Certificate Chain File" \
                  "" \
                  "1" \
  "Moderator Frontend SSL Certificate Chain File path."

  osbe_add_option service_frontend_moderator_ssl_ca \
                  "" \
                  "service.frontend.moderator.ssl.ca" \
                  "Moderator Frontend SSL Certificate Authority Certificate File" \
                  "" \
                  "1" \
  "Moderator Frontend SSL Certificate Authority Certificate File path."
fi

osbe_ask_for_options "NewsGate Services Settings"

service_frontend_search_robots_directives=""

if test -n "$service_frontend_search_robots_txt"; then
  service_frontend_search_robots_directives=`cat "$service_frontend_search_robots_txt"`

  if test $? -ne 0; then
    AC_MSG_ERROR([Failed to read robots.txt directives])
  fi
fi

doc_root="$www_root/htdocs/WEB"
config_doc_root="\$(destdir)\$(proddir)/etc/NewsGate/www/htdocs/WEB"
config_psp_root="\$(destdir)\$(proddir)/etc/NewsGate/www/templates"

extra_installs=\
"  mkdir -p $config_doc_root \\
  && ln -s $data_root/comp $config_doc_root/comp \\
  && ln -s $doc_root/fixed $config_doc_root/fixed \\
  && ln -s $doc_root/sysmon $config_doc_root/sysmon"

if test -n "$service_frontend_search_www_extension"; then

  extra_installs="$extra_installs \\
  && \$(INSTALL_DIR) \"$service_frontend_search_www_extension\" \"$config_doc_root\" \\
\"\" -not \( -name \".svn\" -prune \)"

fi

if test -n "$service_frontend_search_psp_extension"; then

  extra_installs="$extra_installs \\
  && \$(INSTALL_DIR) \"$service_frontend_search_psp_extension\" \"$config_psp_root\" \\
\"\" -not \( -name \".svn\" -prune \)"

  psp_extras="$site_config_root/www/templates"
else
  psp_extras=""
fi

search_interceptor_config=""

if test -n "$service_frontend_search_interceptor"; then
  interceptor_dest="$config_psp_root/py/search_interceptor.py"

  extra_installs="$extra_installs \\
  && \$(INSTALL_DATA) $service_frontend_search_interceptor $interceptor_dest \\
  && ln -s $www_templates/py $config_psp_root/srv-py"

  for l in $interface_langs; do

    extra_installs="$extra_installs \\
  && \$(INSTALL_DATA) $service_frontend_search_interceptor.$l $interceptor_dest.$l"

  done

  search_interceptor_config="PSP_SearchPath $site_config_root/www/templates/py"
fi

service_frontend_moderator_ssl_config=""
service_frontend_moderator_schema="http"

if test $feed_moderating -eq 1 -a $service_frontend_moderator_ssl_use -gt 0; then

  if test -z "$service_frontend_moderator_ssl_cert"; then
    AC_MSG_ERROR([SSL certificate path not specified])
  fi

  service_frontend_moderator_schema="https"
  ssl_cert_name=`basename $service_frontend_moderator_ssl_cert`
  ssl_cert_ext=`echo "$ssl_cert_name" | sed -n -e "s/.*\.\(.*\)/\1/p"`
  ssl_cert_path="$site_config_root/cert/$ssl_cert_name"
  ssl_cert_path2="\$(destdir)\$(proddir)/etc/NewsGate/cert/$ssl_cert_name"

  extra_installs="$extra_installs \\
  && \$(INSTALL_DATA) $service_frontend_moderator_ssl_cert $ssl_cert_path2"

  service_frontend_moderator_ssl_cert_config="SSLCertificateFile $ssl_cert_path"

  if test -n "$service_frontend_moderator_ssl_key"; then
    ssl_key_name=`basename $service_frontend_moderator_ssl_key`
    ssl_key_path="$site_config_root/cert/$ssl_key_name"
    ssl_key_path2="\$(destdir)\$(proddir)/etc/NewsGate/cert/$ssl_key_name"

    extra_installs="$extra_installs \\
  && \$(INSTALL_DATA) $service_frontend_moderator_ssl_key $ssl_key_path2"

    service_frontend_moderator_ssl_cert_config="$service_frontend_moderator_ssl_cert_config
SSLCertificateKeyFile $ssl_key_path"
  fi

  if test -n "$service_frontend_moderator_ssl_chain"; then
    ssl_chain_name=`basename $service_frontend_moderator_ssl_chain`
    ssl_chain_path="$site_config_root/cert/$ssl_chain_name"
    ssl_chain_path2="\$(destdir)\$(proddir)/etc/NewsGate/cert/$ssl_chain_name"

    extra_installs="$extra_installs \\
  && \$(INSTALL_DATA) $service_frontend_moderator_ssl_chain $ssl_chain_path2"

    service_frontend_moderator_ssl_cert_config="$service_frontend_moderator_ssl_cert_config
SSLCertificateChainFile $ssl_chain_path"
  fi

  if test -n "$service_frontend_moderator_ssl_ca"; then
    ssl_ca_name=`basename $service_frontend_moderator_ssl_ca`
    ssl_ca_path="$site_config_root/cert/$ssl_ca_name"
    ssl_ca_path2="\$(destdir)\$(proddir)/etc/NewsGate/cert/$ssl_ca_name"

    extra_installs="$extra_installs \\
  && \$(INSTALL_DATA) $service_frontend_moderator_ssl_ca $ssl_ca_path2"

    service_frontend_moderator_ssl_cert_config="$service_frontend_moderator_ssl_cert_config
SSLCACertificateFile $ssl_ca_path"
  fi

  service_frontend_moderator_ssl_config="LoadModule ssl_module  /usr/lib$libdir_suffix/httpd/modules/mod_ssl.so
<IfDefine ap_2_4>
  LoadModule socache_shmcb_module  /usr/lib64/httpd/modules/mod_socache_shmcb.so
</IfDefine>
AddType application/x-x509-ca-cert .crt
AddType application/x-pkcs7-crl    .crl
SSLPassPhraseDialog  builtin
SSLSessionCache         shmcb:$cache_root/ModeratorFe/scache(512000)
SSLSessionCacheTimeout  300
<IfDefine ap_2_4>
  Mutex default
</IfDefine>
<IfDefine !ap_2_4>
  SSLMutex default
</IfDefine>
SSLRandomSeed startup file:/dev/urandom  256
SSLRandomSeed connect builtin
SSLCryptoDevice builtin
SSLEngine on
$service_frontend_moderator_ssl_cert_config"

fi

if test $service_frontend_search_ensure_canonical -eq 1; then
  service_frontend_search_canonical_endpoint="$service_frontend_search_hostname"
else
  service_frontend_search_canonical_endpoint=""
fi

image_extension_whitelist=`cat "$srcdir/image_extension_whitelist"`
image_prefix_blacklist=`cat "$srcdir/image_prefix_blacklist"`

AC_SUBST([service_customer_manager_port])
AC_SUBST([service_customer_manager_log_level])
AC_SUBST([service_customer_manager_log_aspects])

AC_SUBST([service_ad_manager_port])
AC_SUBST([service_ad_manager_log_level])
AC_SUBST([service_ad_manager_log_aspects])

AC_SUBST([service_moderator_manager_port])
AC_SUBST([service_moderator_manager_log_level])
AC_SUBST([service_moderator_manager_log_aspects])

AC_SUBST([service_feed_manager_port])
AC_SUBST([service_feed_manager_log_level])
AC_SUBST([service_feed_manager_log_aspects])

AC_SUBST([service_category_manager_port])
AC_SUBST([service_category_manager_log_level])
AC_SUBST([service_category_manager_log_aspects])

AC_SUBST([service_rss_puller_manager_port])
AC_SUBST([service_rss_puller_manager_log_level])
AC_SUBST([service_rss_puller_manager_log_aspects])

AC_SUBST([service_rss_puller_port])
AC_SUBST([service_rss_puller_log_level])
AC_SUBST([service_rss_puller_log_aspects])

AC_SUBST([service_message_bank_manager_port])
AC_SUBST([service_message_bank_manager_log_level])
AC_SUBST([service_message_bank_manager_log_aspects])

AC_SUBST([service_message_bank_port])
AC_SUBST([service_message_bank_log_level])
AC_SUBST([service_message_bank_log_aspects])

AC_SUBST([service_word_manager_port])
AC_SUBST([service_word_manager_log_level])
AC_SUBST([service_word_manager_log_aspects])

AC_SUBST([service_segmentor_port])
AC_SUBST([service_segmentor_log_level])
AC_SUBST([service_segmentor_log_aspects])

AC_SUBST([service_event_bank_manager_port])
AC_SUBST([service_event_bank_manager_log_level])
AC_SUBST([service_event_bank_manager_log_aspects])

AC_SUBST([service_event_bank_port])
AC_SUBST([service_event_bank_log_level])
AC_SUBST([service_event_bank_log_aspects])

AC_SUBST([service_stat_processor_port])
AC_SUBST([service_stat_processor_log_level])
AC_SUBST([service_stat_processor_log_aspects])
AC_SUBST([service_stat_processor_raw_stat_keep_days])

AC_SUBST([service_limit_checker_port])
AC_SUBST([service_limit_checker_log_level])
AC_SUBST([service_limit_checker_log_aspects])

AC_SUBST([service_ad_server_port])
AC_SUBST([service_ad_server_log_level])
AC_SUBST([service_ad_server_log_aspects])

AC_SUBST([service_search_mailer_port])
AC_SUBST([service_search_mailer_log_level])
AC_SUBST([service_search_mailer_log_aspects])
AC_SUBST([service_search_mailer_email])
AC_SUBST([service_search_mailer_debug_email])
AC_SUBST([service_search_mailer_recipient_blacklist])
AC_SUBST([service_search_mailer_trust_timeout])
AC_SUBST([service_search_mailer_reserve_message_bank_manager_ref])
AC_SUBST([service_search_mailer_recaptcha_client_key])
AC_SUBST([service_search_mailer_recaptcha_server_key])

AC_SUBST([service_db_port])

AC_SUBST([service_frontend_username])
AC_SUBST([service_frontend_usergroup])
AC_SUBST([service_frontend_development_mode])
AC_SUBST([service_frontend_debug_info])
AC_SUBST([service_frontend_error_in_response])

AC_SUBST([service_frontend_search_port])
AC_SUBST([service_frontend_search_hostname])
AC_SUBST([service_frontend_search_log_level])
AC_SUBST([service_frontend_search_log_aspects])
AC_SUBST([service_frontend_search_canonical_endpoint])
AC_SUBST([search_interceptor_config])
OSBE_SUBST_VIA_FILE([service_frontend_search_robots_directives])

AC_SUBST([service_frontend_moderator_port])
AC_SUBST([service_frontend_moderator_hostname])
AC_SUBST([service_frontend_moderator_log_level])
AC_SUBST([service_frontend_moderator_log_aspects])
AC_SUBST([service_frontend_moderator_schema])
OSBE_SUBST_VIA_FILE([service_frontend_moderator_ssl_config])

AC_SUBST([service_frontend_limited_port])
AC_SUBST([service_frontend_limited_hostname])
AC_SUBST([service_frontend_limited_log_level])
AC_SUBST([service_frontend_limited_log_aspects])

AC_SUBST([psp_extras])

OSBE_SUBST_VIA_FILE([image_extension_whitelist])
OSBE_SUBST_VIA_FILE([image_prefix_blacklist])

# Configuring NewsGate Maintanance Tasks

osbe_config_mech_write_nl
osbe_config_mech_write_comment "NewsGate Maintanance Tasks.
# Remove obsolete information and ensure data safety.
#"

osbe_add_option maintanance_username \
                "$USER" \
                "maintanance.username" \
                "User Name" \
                "" \
                "0" \
"User name under which cron will launch maintanance tasks."

osbe_add_option maintanance_log_cleanup_time \
                "0 5 * * *" \
                "maintanance.log.cleanup_crontime" \
                "Logs Cleanup Cron Time" \
                "Logs Cleanup Cron Time Specification (like 0 5 * * *)" \
                "1" \
"Logs Cleanup Cron Time Specification (like 0 5 * * *)."

osbe_add_option maintanance_log_age \
                "10080" \
                "maintanance.log.age" \
                "Logs Max Age (days)" \
                "" \
                "0" \
"Number of minutes log files will be kept."

osbe_add_option maintanance_webcache_cleanup_time \
                "0 5 * * *" \
                "maintanance.webcache.cleanup_crontime" \
                "Web Cache Cleanup Cron Time" \
                "Web Cache Cleanup Cron Time Specification (like 0 5 * * *)" \
                "1" \
"Web Cache Cleanup Cron Time Specification (like 0 5 * * *)."

osbe_add_option maintanance_backup_copy_cmd \
                "" \
                "maintanance.backup.copy_cmd" \
                "Backup Copy Transport" \
                "Backup Copy Transport (use =BACKUP= to denote archive file name)" \
                "1" \
"Backup Copy Transport command (use =BACKUP= to denote archive file name).
# Specify if want backups to be copied from origin machine to some safe place."

osbe_add_option maintanance_backup_cleanup_time \
                "0 5 * * *" \
                "maintanance.backup.cleanup_crontime" \
                "Backups Cleanup Cron Time" \
                "Backups Cleanup Cron Time Specification (like 0 5 * * *)" \
                "1" \
"Backups Cleanup Cron Time Specification (like 0 5 * * *)."

osbe_add_option maintanance_backup_age \
                "10080" \
                "maintanance.backup.age" \
                "Backups Max Age (days)" \
                "" \
                "0" \
"Number of minutes backup files will be kept on origin machine."

osbe_add_option maintanance_backup_full_args \
                "" \
                "maintanance.backup.full.args" \
                "Full Backup Arguments" \
                "Full Backup Additional Arguments" \
                "1" \
"Additional arguments to pass to full backup operation."

osbe_add_option maintanance_full_backup_time \
                "10 5 * * *" \
                "maintanance.backup.full.crontime" \
                "Full Backup Cron Time" \
                "Full Backup Cron Time Specification (like 0 5 * * *)" \
                "1" \
"Full Backup Cron Time Specification (like 0 5 * * *)."

osbe_add_option maintanance_part_backup_time \
                "30 * * * *" \
                "maintanance.backup.part.crontime" \
                "Partial Backup Cron Time" \
                "Partial Backup Cron Time Specification (like 0 * * * *)" \
                "1" \
"Partial Backup Cron Time Specification (like 0 * * * *)."

osbe_add_option zenoss_syslog_endpoint \
                "zenoss:1514" \
                "maintanance.zenoss.syslog.endpoint" \
                "ZenOSS Syslog Endpoint" \
                "ZenOSS Syslog Endpoint (like zenoss:1514)" \
                "1" \
"ZenOSS Syslog Endpoint specification (like zenoss:1514)."

syslog_host=`echo "$zenoss_syslog_endpoint" | sed -n -e "s/\(.*\):.*/\1/p"`
syslog_port=`echo "$zenoss_syslog_endpoint" | sed -n -e "s/.*:\(.*\)/\1/p"`

osbe_ask_for_options "NewsGate Maintanance Tasks"

if test -n "$maintanance_log_cleanup_time"; then
  maintanance_log_cleanup_command=\
"$maintanance_log_cleanup_time $maintanance_username /usr/bin/find $log_root/ -name '*log.*' -type f -mmin +$maintanance_log_age -delete"
fi

if test -n "$maintanance_webcache_cleanup_time"; then
  maintanance_webcache_cleanup_command=\
"$maintanance_webcache_cleanup_time $maintanance_username /usr/bin/find $data_root/Frontends/Thumbnail/ImgThumb/ -maxdepth 1 -name '????????' -not -name \`date -u +\%Y\%m\%d\` -not -name ImgThumb -type d -exec rm -r -f '{}' \\;"
fi

if test -n "$maintanance_backup_cleanup_time"; then
  maintanance_backup_cleanup_command=\
"$maintanance_backup_cleanup_time $maintanance_username /usr/bin/find $data_root/archives/ -maxdepth 1 -mmin +$maintanance_backup_age -print0 | xargs -0 rm -rf"
fi

if test -n "$maintanance_full_backup_time"; then
  maintanance_full_backup_command=\
"$maintanance_full_backup_time $maintanance_username source $site_config_root/environment.sh && ng-backup $maintanance_backup_full_args --copy '$maintanance_backup_copy_cmd' >> $log_root/Backup/Backup.log.\`date -u +\\%Y\\%m\\%d\` 2>&1"
fi

if test -n "$maintanance_part_backup_time"; then
  maintanance_part_backup_command=\
"$maintanance_part_backup_time $maintanance_username source $site_config_root/environment.sh && ng-backup part --copy '$maintanance_backup_copy_cmd' >> $log_root/Backup/Backup.log.\`date -u +\\%Y\\%m\\%d\` 2>&1"
fi

AC_SUBST([maintanance_username])
AC_SUBST([maintanance_log_cleanup_command])
AC_SUBST([maintanance_webcache_cleanup_command])
AC_SUBST([maintanance_backup_copy_cmd])
AC_SUBST([maintanance_backup_cleanup_command])
AC_SUBST([maintanance_backup_full_args])
AC_SUBST([maintanance_full_backup_command])
AC_SUBST([maintanance_part_backup_command])

# Configuring cluster hosts

hosts_config=""
search_frontend_endpoints=""
limited_frontend_endpoints=""

services="DB \
  Configurator \
  MessageBankManager \
  MessageBank \
  WordManager \
  Segmentor \
  SearchFrontend"

if test $customer_moderating -eq 1; then
  services="$services \
  CustomerManager"
fi

if test $ad_management -eq 1; then
  services="$services \
  AdManager"
fi

if test $feed_moderating -eq 1; then
  services="$services \
  ModeratorManager \
  FeedManager \
  CategoryManager \
  ModeratorFrontend"
fi

if test $feed_pulling -eq 1; then
  services="$services \
  RSSPullerManager \
  RSSPuller"
fi

if test $event_detection -eq 1; then
  services="$services EventBankManager EventBank"
fi

if test $stat_processing -eq 1; then
  services="$services StatProcessor"
fi

if test $fraud_prevention -eq 1; then
  services="$services LimitChecker"
fi

if test $ad_serving -ne 0; then
  services="$services AdServer"
fi

if test $search_mailing -eq 1; then
  services="$services SearchMailer"
fi

osbe_config_mech_write_nl
osbe_config_mech_write_comment "Cluster deployment structure.
# Record for each host should be specified in the following form:
# cluster.host.<number> = <hostname> : service1 service2 ... 
# where <number> start from 0.
# cluster.host.count denotes number of hosts in cluster."

osbe_configure_hosts \
  "$services" \
  "host_preconfig" \
  "host_postconfig" \
  "host_service" \
  "cluster"

frontend_endpoints=""

for ep in $search_frontend_endpoints; do
  frontend_endpoints="$frontend_endpoints
      <frontend endpoint=\"$ep\" type=\"search\"/>"
done

if test $limited_frontend -ne 1; then
  limited_frontend_endpoints="$search_frontend_endpoints"
fi

for ep in $limited_frontend_endpoints; do
  frontend_endpoints="$frontend_endpoints
      <frontend endpoint=\"$ep\" type=\"limited\"/>"
done

if test $customer_moderating -eq 1; then
  if test -z "$server_host_customer_manager"; then
    AC_MSG_ERROR([CustomerManager host not specified])
  fi
else
  server_host_customer_manager=""
fi

if test $ad_management -eq 1; then
  if test -z "$server_host_ad_manager"; then
    AC_MSG_ERROR([AdManager host not specified])
  fi
else
  server_host_ad_manager=""
fi
  
if test $feed_moderating -eq 1; then

  if test -z "$server_host_feed_manager"; then
    AC_MSG_ERROR([FeedManager host not specified])
  fi
  
  if test -z "$server_host_moderator_manager"; then
    AC_MSG_ERROR([ModeratorManager host not specified])
  fi
  
  if test -z "$server_host_category_manager"; then
    AC_MSG_ERROR([CategoryManager host not specified])
  fi
  
else
  server_host_feed_manager=""
  server_host_moderator_manager=""
  server_host_category_manager=""
fi

if test $feed_pulling -eq 1; then

  if test -z "$server_host_rss_puller_manager"; then
    AC_MSG_ERROR([RSSPullerManager host not specified])
  fi
  
else
  server_host_rss_puller_manager=""
fi

if test $event_detection -eq 1; then

  if test -z "$server_host_event_bank_manager"; then
    AC_MSG_ERROR([EventBankManager host not specified])
  fi
  
else
  server_host_event_bank_manager=""
fi

if test $stat_processing -eq 1; then

  if test -z "$server_host_stat_processor"; then
    AC_MSG_ERROR([StatProcessor host not specified])
  fi
  
  corbaloc_stat_processor_ref=`corbaloc_url "$server_host_stat_processor" \
                              "$service_stat_processor_port"`"/StatProcessor"

elif test $stat_processing -eq 2; then
  server_host_stat_processor=""

  if test -z "$corbaloc_stat_processor_ref"; then
    AC_MSG_ERROR([StatProcessor external reference not specified])
  fi
else
  server_host_stat_processor=""
  corbaloc_stat_processor_ref=""
fi

if test $search_mailing -eq 1; then

  if test -z "$server_host_search_mailer"; then
    AC_MSG_ERROR([SearchMailer host not specified])
  fi
  
  corbaloc_search_mailer_ref=`corbaloc_url "$server_host_search_mailer" \
                             "$service_search_mailer_port"`"/SearchMailer"

elif test $search_mailing -eq 2; then
  server_host_search_mailer=""

  if test -z "$corbaloc_search_mailer_ref"; then
    AC_MSG_ERROR([SearchMailer external reference not specified])
  fi
else
  server_host_search_mailer=""
  corbaloc_search_mailer_ref=""
fi

if test $fraud_prevention -eq 1; then

  if test -z "$server_host_limit_checker"; then
    AC_MSG_ERROR([LimitChecker host not specified])
  fi
  
  corbaloc_limit_checker_ref=`corbaloc_url "$server_host_limit_checker" \
                             "$service_limit_checker_port"`"/LimitChecker"

elif test $fraud_prevention -eq 2; then
  server_host_limit_checker=""

  if test -z "$corbaloc_limit_checker_ref"; then
    AC_MSG_ERROR([LimitChecker external reference not specified])
  fi
else
  server_host_limit_checker=""
  corbaloc_limit_checker_ref=""
fi

if test $ad_serving -ne 0; then

  if test -z "$server_host_ad_server"; then
    AC_MSG_ERROR([AdServer host not specified])
  fi

  corbaloc_ad_server_ref=`corbaloc_url "$server_host_ad_server" \
                          "$service_ad_server_port"`"/AdServer"
  
  if test $ad_serving -eq 2; then
    if test -z "$corbaloc_ad_server_ext_ref"; then
      AC_MSG_ERROR([AdServer external reference not specified])
    fi
  else
    corbaloc_ad_server_ext_ref=""
  fi
else
  server_host_ad_server=""
  corbaloc_ad_server_ref=""
  corbaloc_ad_server_ext_ref=""
fi

if test -z "$server_host_message_bank_manager"; then
  AC_MSG_ERROR([MessageBankManager host not specified])
fi  

if test -z "$server_host_word_manager"; then
  AC_MSG_ERROR([WordManager host not specified])
fi  

if test -z "$server_host_segmentor"; then
  AC_MSG_ERROR([Segmentor host not specified])
fi  

if test -z "$server_host_search_frontend"; then
  AC_MSG_ERROR([SearchFrontend host not specified])
fi

if test $limited_frontend -eq 1; then
  if test $limited_frontend_count -eq 0; then
    AC_MSG_ERROR([LimitedFrontend host not specified])
  fi
else
  service_frontend_limited_hostname=$service_frontend_search_hostname
fi

OSBE_SUBST_VIA_FILE([hosts_config])
OSBE_SUBST_VIA_FILE([frontend_endpoints])

AC_SUBST([server_host_rss_puller_manager])
AC_SUBST([server_host_message_bank_manager])
AC_SUBST([server_host_word_manager])
AC_SUBST([server_host_segmentor])
AC_SUBST([server_host_stat_processor])
AC_SUBST([server_host_search_mailer])
AC_SUBST([server_host_limit_checker])
AC_SUBST([server_host_ad_server])
AC_SUBST([server_host_event_bank_manager])
AC_SUBST([server_host_feed_manager])
AC_SUBST([server_host_moderator_manager])
AC_SUBST([server_host_customer_manager])
AC_SUBST([server_host_category_manager])
AC_SUBST([server_host_search_frontend])

corbaloc_word_manager_proc_ctl=`corbaloc_url "\\\\HOST\\\\" \
                                "$service_word_manager_port"`"/ProcessControl"

corbaloc_word_manager_ref=`corbaloc_url "$server_host_word_manager" \
                           "$service_word_manager_port"`"/WordManager"

corbaloc_message_bank_proc_ctl=`corbaloc_url "\\\\HOST\\\\" \
                                "$service_message_bank_port"`"/ProcessControl"

corbaloc_segmentor_proc_ctl=`corbaloc_url "\\\\HOST\\\\" \
                            "$service_segmentor_port"`"/ProcessControl"

corbaloc_rss_puller_manager_proc_ctl=`corbaloc_url "\\\\HOST\\\\" \
                          "$service_rss_puller_manager_port"`"/ProcessControl"

corbaloc_rss_puller_manager_ref=`corbaloc_url "$server_host_rss_puller_manager" \
                            "$service_rss_puller_manager_port"`"/PullerManager"

corbaloc_rss_puller_proc_ctl=`corbaloc_url "\\\\HOST\\\\" \
                             "$service_rss_puller_port"`"/ProcessControl"

corbaloc_message_bank_manager_proc_ctl=`corbaloc_url "\\\\HOST\\\\" \
                        "$service_message_bank_manager_port"`"/ProcessControl"

corbaloc_message_bank_manager_ref=`corbaloc_url "$server_host_message_bank_manager" \
                            "$service_message_bank_manager_port"`"/BankManager"

corbaloc_event_bank_manager_proc_ctl=`corbaloc_url "\\\\HOST\\\\" \
                        "$service_event_bank_manager_port"`"/ProcessControl"

corbaloc_event_bank_proc_ctl=`corbaloc_url "\\\\HOST\\\\" \
                             "$service_event_bank_port"`"/ProcessControl"

corbaloc_stat_processor_proc_ctl=`corbaloc_url "\\\\HOST\\\\" \
                             "$service_stat_processor_port"`"/ProcessControl"

corbaloc_search_mailer_proc_ctl=`corbaloc_url "\\\\HOST\\\\" \
                             "$service_search_mailer_port"`"/ProcessControl"

corbaloc_limit_checker_proc_ctl=`corbaloc_url "\\\\HOST\\\\" \
                             "$service_limit_checker_port"`"/ProcessControl"

corbaloc_ad_server_proc_ctl=`corbaloc_url "\\\\HOST\\\\" \
                             "$service_ad_server_port"`"/ProcessControl"

corbaloc_moderator_manager_proc_ctl=`corbaloc_url "\\\\HOST\\\\" \
                            "$service_moderator_manager_port"`"/ProcessControl"

corbaloc_moderator_manager_ref=`corbaloc_url "$server_host_moderator_manager" \
                          "$service_moderator_manager_port"`"/ModeratorManager"

corbaloc_customer_manager_proc_ctl=`corbaloc_url "\\\\HOST\\\\" \
                            "$service_customer_manager_port"`"/ProcessControl"

corbaloc_customer_manager_ref=`corbaloc_url "$server_host_customer_manager" \
                          "$service_customer_manager_port"`"/CustomerManager"

corbaloc_ad_manager_proc_ctl=`corbaloc_url "\\\\HOST\\\\" \
                             "$service_ad_manager_port"`"/ProcessControl"

corbaloc_ad_manager_ref=`corbaloc_url "$server_host_ad_manager" \
                        "$service_ad_manager_port"`"/AdManager"

corbaloc_feed_manager_proc_ctl=`corbaloc_url "\\\\HOST\\\\" \
                            "$service_feed_manager_port"`"/ProcessControl"

corbaloc_feed_manager_ref=`corbaloc_url "$server_host_feed_manager" \
                          "$service_feed_manager_port"`"/FeedManager"

if test $feed_moderating -eq 1; then
  corbaloc_feed_stat_sink_ref=`corbaloc_url "$server_host_feed_manager" \
                              "$service_feed_manager_port"`"/FeedStatSink"

else
  corbaloc_feed_stat_sink_ref=""
fi

corbaloc_category_manager_proc_ctl=`corbaloc_url "\\\\HOST\\\\" \
                            "$service_category_manager_port"`"/ProcessControl"

corbaloc_category_manager_ref=`corbaloc_url "$server_host_category_manager" \
                          "$service_category_manager_port"`"/CategoryManager"

AC_SUBST([corbaloc_word_manager_proc_ctl])
AC_SUBST([corbaloc_word_manager_ref])
AC_SUBST([corbaloc_message_bank_proc_ctl])
AC_SUBST([corbaloc_segmentor_proc_ctl])
AC_SUBST([corbaloc_rss_puller_manager_proc_ctl])
AC_SUBST([corbaloc_rss_puller_manager_ref])
AC_SUBST([corbaloc_rss_puller_proc_ctl])
AC_SUBST([corbaloc_message_bank_manager_proc_ctl])
AC_SUBST([corbaloc_message_bank_manager_ref])
AC_SUBST([corbaloc_event_bank_manager_proc_ctl])
AC_SUBST([corbaloc_event_bank_proc_ctl])
AC_SUBST([corbaloc_stat_processor_proc_ctl])
AC_SUBST([corbaloc_stat_processor_ref])
AC_SUBST([corbaloc_search_mailer_proc_ctl])
AC_SUBST([corbaloc_search_mailer_ref])
AC_SUBST([corbaloc_limit_checker_proc_ctl])
AC_SUBST([corbaloc_limit_checker_ref])
AC_SUBST([corbaloc_ad_server_proc_ctl])
AC_SUBST([corbaloc_ad_server_ref])
AC_SUBST([corbaloc_ad_server_ext_ref])
AC_SUBST([corbaloc_moderator_manager_proc_ctl])
AC_SUBST([corbaloc_moderator_manager_ref])
AC_SUBST([corbaloc_customer_manager_proc_ctl])
AC_SUBST([corbaloc_customer_manager_ref])
AC_SUBST([corbaloc_ad_manager_proc_ctl])
AC_SUBST([corbaloc_ad_manager_ref])
AC_SUBST([corbaloc_feed_manager_proc_ctl])
AC_SUBST([corbaloc_feed_manager_ref])
AC_SUBST([corbaloc_category_manager_proc_ctl])
AC_SUBST([corbaloc_category_manager_ref])
AC_SUBST([corbaloc_feed_stat_sink_ref])

# Configuring server behavior

osbe_config_mech_write_nl
osbe_config_mech_write_comment "NewsGate Server Behavior.
# Settings controlling NewsGate server functional behavior.
#"

osbe_add_option server_instance_name \
                "NewsGate" \
                "server.behaviour.server_instance_name" \
                "Server Instance Name" \
                "" \
                "0" \
"Server instance name will be visible in User Interface."

osbe_add_option server_instance_email \
                "" \
                "server.behaviour.server_instance_email" \
                "Server Instance EMail" \
                "" \
                "0" \
"Server instance email will be visible in User Interface."

osbe_add_option copyright_note \
                "Copyright &copy; 2014 DSN." \
                "server.behaviour.copyright_note" \
                "Server Copyright Note" \
                "" \
                "0" \
"Copyright Note will be visible in User Interface."

osbe_add_option feed_request_min_period \
                "600" \
                "server.behaviour.feed.request.min_period" \
                "Feed Request Min Period" \
                "" \
                "0" \
"Min feed request period in seconds"

osbe_add_option feed_request_max_period \
                "43200" \
                "server.behaviour.feed.request.max_period" \
                "Feed Request Max Period" \
                "" \
                "0" \
"Max feed request period in seconds"

osbe_add_option message_max_age \
                "1209600" \
                "server.behaviour.message.maxage" \
                "Message Maximum Age" \
                "" \
                "0" \
"Maximum message age in seconds.
# Message is deleted from the system when expire."

osbe_add_option store_duplicate_messages \
                "0" \
                "server.behaviour.message.store_duplicate" \
                "Store Duplicate Messages" \
                "" \
                "0" \
"Specifies if duplicate messages should be stored."

osbe_add_option category_name_as_last_resort \
                "1" \
                "server.behaviour.message.categorizer.category_name_as_last_resort" \
                "Category Name As Localization Last Resort" \
                "" \
                "0" \
"If true, then category name will be used if localized name not found."

osbe_add_option shared_message_source \
                "" \
                "server.behaviour.message.shared_message_source" \
                "Shared Messages Source (CORBA ref; none if empty)" \
                "" \
                "1" \
"If set to external Message Bank Manager CORBA reference, then external
#cluster can be used as a message source and for some requests delegation
#depending from mirroring and proxing policies"

osbe_add_option message_mirroring \
                "0" \
                "server.behaviour.message.mirroring" \
                "Message Mirroring" \
                "" \
                "0" \
"If 0 no messages will be pulled. If 1 then messages will be pulled from the 
#master cluster with some aspects inherited while others made specific for 
#the current cluster: core words, word normal forms, categories, 
#event clustering ... If 2 then cluster will behave as an absolute mirror 
#of the master cluster (used to create a  secondary NewsGate cluster to 
#improve system overall performance and fault tolerance capabilities)."

osbe_add_option message_proxing \
                "0" \
                "server.behaviour.message.proxing" \
                "Message Proxing" \
                "" \
                "0" \
"If 0 then no message requests will be proxied. If 1 then requests for 
#unexistent message thumbnails will be symmetrically proxied"

osbe_add_option message_max_core_words \
                "20" \
                "server.behaviour.message.max_core_words" \
                "Message Max Core Words Count" \
                "" \
                "0" \
"Maximum number of message words marked as core words."

osbe_add_option message_core_words_prc \
                "75" \
                "server.behaviour.message.core_words_prc" \
                "Message Core Words Percentage" \
                "" \
                "0" \
"Percentage of message words marked as core words."

osbe_add_option word_pair_counter_type \
                "" \
                "server.behaviour.message.word_pair_counter.type" \
                "Message Word Pair Counter Type" \
                "" \
                "0" \
"Should be one of mem, hash, btree, db or empty to disable"

osbe_add_option word_pair_counter_filename \
                "$cache_root/Message/Bank/MessageWordPairs" \
                "server.behaviour.message.word_pair_counter.filename" \
                "Message Word Pair Counter DB File Name" \
                "" \
                "0" \
"Word pair counter DB file path"

osbe_add_option word_pair_interval_groups \
                "10" \
                "server.behaviour.message.word_pair_counter.interval_groups" \
                "Message Word Pair Counter Interval Groups" \
                "" \
                "0" \
"Number of word pair counter time interval groups"

osbe_add_option word_pair_counter_bucket_factor \
                "0.2" \
                "server.behaviour.message.word_pair_counter.bucket_factor" \
                "Message Word Pair Counter Bucket Factor" \
                "" \
                "0" \
"Multiplies default word pair counter bucket number"

osbe_add_option word_pair_counter_cache_size \
                "10000000" \
                "server.behaviour.message.word_pair_counter.cache_size" \
                "Message Word Pair Counter Cache Size" \
                "" \
                "0" \
"Max number for word pair counter cache records"

osbe_add_option message_image_max_width \
                "400" \
                "server.behaviour.message.image.max_width" \
                "Message Image Max Width" \
                "" \
                "0" \
"Maximum width of message image in pixels.
# Wider images will be resized to fit that maximum."

osbe_add_option message_image_max_height \
                "500" \
                "server.behaviour.message.image.max_height" \
                "Message Image Max Height" \
                "" \
                "0" \
"Maximum height of message image in pixels.
# Higher images will be resized to fit that maximum."

osbe_add_option message_description_max_chars \
                "2048" \
                "server.behaviour.message.description.max_chars" \
                "Message Description Max Chars" \
                "" \
                "0" \
"Maximum count of message description characters."

osbe_add_option message_image_max_count \
                "101" \
                "server.behaviour.message.image.max_count" \
                "Message Image Max Count" \
                "" \
                "0" \
"Maximum count of message images."

osbe_add_option message_image_thumbnails \
                "" \
                "server.behaviour.message.image.thumbnails" \
                "Message Image Thumbnail Sizes" \
                "Message Image Thumbnail Sizes In (<width>x<height(C)? )* Form" \
                "1" \
"Space separated list of thumbnail sizes of <width>x<height> or 
# <width>x<height>C forms. System automatically produces thumbnails of 
# specified characteristics. C character means that cropping should be used 
# in thumbnail creation. If at least one thumnail specified, then first 
# thumbnail will be displayed on Search Results Page instead of original image.
# All thumbnails are available in XSearch API responces."

osbe_add_option message_image_mob_thumb \
                "0x0" \
                "server.behaviour.message.image.mob_thumbnail" \
                "Message Image Mobile Thumbnail" \
                "" \
                "0" \
"Message image thumbnail for mobile devices."

osbe_add_option message_image_referer_check \
                "1" \
                "server.behaviour.message.image.referer.check" \
                "Message Image Max Count" \
                "" \
                "0" \
"If to check if image loads with referer."

osbe_add_option message_bank_capacity \
                "$message_bank_capacity" \
                "server.behaviour.message.bank.capacity" \
                "Message Bank Capacity" \
                "" \
                "0" \
"Max number of messages can be stored by single Message Bank instance."

osbe_add_option impression_respected_level \
                "100" \
                "server.behaviour.message.stat.impression_respected_level" \
                "Message Impression Respected Level" \
                "" \
                "0" \
"Number of impressions considered reasonable for CTR calculation."

osbe_add_option stat_uid_cookie \
                "1" \
                "server.behaviour.message.stat.uid_cookie" \
                "Use uid cookie for stat purposes" \
                "" \
                "0" \
"Use uid cookie for statistics counting purposes."

osbe_add_option event_bank_backets \
                "4" \
                "server.behaviour.event.bank.backets" \
                "Event Bank Backets" \
                "" \
                "0" \
"Max number of event backets can be used by Event Bank instance."

osbe_add_option event_max_size \
                "10000" \
                "server.behaviour.event.max_size" \
                "Event Max Size" \
                "" \
                "0" \
"Max number of messages single event can encapsulate."

osbe_add_option event_max_time_range \
                "8640000" \
                "server.behaviour.event.max_time_range" \
                "Event Max Time Range" \
                "" \
                "0" \
"Max time range event can cover."

osbe_add_option event_merge_deny_size_factor \
                "360" \
                "server.behaviour.event.merge.deny.size_factor" \
                "Event Merge Deny Size Factor" \
                "" \
                "0" \
"Event merge denial size-based factor."

osbe_add_option event_merge_deny_max_time \
                "43200" \
                "server.behaviour.event.merge.deny.max_time" \
                "Event Merge Deny Max Time" \
                "" \
                "0" \
"Event merge denial maximum timeout."

osbe_add_option event_min_rift_time \
                "86400" \
                "server.behaviour.event.min_rift_time" \
                "Event Min Rift Time" \
                "" \
                "0" \
"Minimum time difference between event 2 parts which can be considered as 
# a place to split event."

osbe_add_option event_merge_max_time_diff \
                "4320000" \
                "server.behaviour.event.merge.max_time_diff" \
                "Event Merge Max Time Difference" \
                "" \
                "0" \
"Maximum time difference between events to be merged."

osbe_add_option event_merge_strain \
                "10" \
                "server.behaviour.event.merge.max_strain" \
                "Event Merge Max Strain" \
                "" \
                "0" \
"Maximum strain event can have to be merged."

osbe_add_option event_merge_level_base \
                "43" \
                "server.behaviour.event.merge.level.base" \
                "Event Merge Level Base" \
                "" \
                "0" \
"Events base overlap level to merge."

osbe_add_option event_merge_level_min \
                "40" \
                "server.behaviour.event.merge.level.min" \
                "Event Merge Level Min" \
                "" \
                "0" \
"Events minimum overlap level to merge."

osbe_add_option event_merge_level_time_based_increment_step \
                "0.5" \
                "server.behaviour.event.merge.level.time_based_increment_step" \
                "Event Merge Level Time-based Increment Step" \
                "" \
                "0" \
"Events merge level increment per 1 day of time difference."

osbe_add_option event_merge_level_range_based_increment_step \
                "0.0333" \
                "server.behaviour.event.merge.level.range_based_increment_step" \
                "Event Merge Level Timerange-based Increment Step" \
                "" \
                "0" \
"Events merge level increment per 1 day of time range."

osbe_add_option event_merge_level_strain_based_increment_step \
                "0.25" \
                "server.behaviour.event.merge.level.strain_based_increment_step" \
                "Event Merge Level Strain-based Increment Step" \
                "" \
                "0" \
"Events merge level increment per 1% of strain."

osbe_add_option merge_level_size_based_decrement_step \
                "0.02" \
                "server.behaviour.event.merge.level.size_based_decrement_step" \
                "Event Merge Level Size-based Decrement Step" \
                "" \
                "0" \
"Events merge level decrement per 100 messages of size."

osbe_add_option frontend_lang_last_resort \
                "" \
                "server.behaviour.frontend.lang.last_resort" \
                "Search Frontend Lang Last Resort" \
                "" \
                "0" \
"Search Frontend language calculation last resort rules."

osbe_add_option frontend_home_category_columns \
                "5" \
                "server.behaviour.frontend.home.category_columns" \
                "Search Frontend Home Page Category Columns" \
                "" \
                "0" \
"Search Frontend Home Page category columns count."

osbe_add_option frontend_home_subcategory_count \
                "-1" \
                "server.behaviour.frontend.home.subcategory_count" \
                "Search Frontend Home Page Subcategory Count" \
                "" \
                "0" \
"Search Frontend Home Page subcategory count."

osbe_add_option cookie_domain_offset \
                "0" \
                "server.behaviour.frontend.cookie.domain.offset" \
                "Set Cookie Domain Offset" \
                "" \
                "0" \
"Defines HTTP cookie setting domain offset."

osbe_add_option informer_enabled \
                "1" \
                "server.behaviour.frontend.informer.enable" \
                "Enable Informer Creation/Serving" \
                "" \
                "0" \
"Specifies if to enable informer creation/serving."

osbe_add_option informer_thumbnails \
                "60x60C" \
                "server.behaviour.frontend.informer.thumbnails" \
                "Informer thumbnail sizes" \
                "" \
                "0" \
"Specifies informer image thumbnails sizes to use."

osbe_add_option informer_default_thumbnail \
                "60x60C" \
                "server.behaviour.frontend.informer.default_thumbnail" \
                "Informer default thumbnail size" \
                "" \
                "0" \
"Specifies informer default image thumbnails size."

osbe_add_option informer_cache_timeout \
                "0" \
                "server.behaviour.frontend.informer.cache.timeout" \
                "Enable Cache Entry timeout (sec)" \
                "" \
                "0" \
"Specifies for how long informer response should be cached server side."

osbe_add_option trace_search_duration \
                "10" \
                "server.behaviour.frontend.search.trace_duration" \
                "Trace Search Request Durations (sec)" \
                "" \
                "0" \
"Specifies execution time threshold when search requests will be logged."

osbe_add_option frontend_canonize_event \
                "0" \
                "server.behaviour.frontend.canonize_event" \
                "Message age to canonize event (sec)" \
                "" \
                "0" \
"Specifies on which message age (sec) to canonize event search page."

osbe_add_option xsearch_client_ips \
                "" \
                "server.behaviour.frontend.xsearch.client_ips" \
                "XSearch Client IPs" \
                "" \
                "0" \
"XSearch client IPs space separated list (can be empty) or *"

osbe_add_option fraud_prevention_click_user \
                "" \
                "server.behaviour.frontend.fraud_prevention.click.user" \
                "User Click Limitations" \
                "" \
                "0" \
"User Click Limitations like: 100/86400 500/864000 ..."

osbe_add_option fraud_prevention_click_user_msg \
                "" \
                "server.behaviour.frontend.fraud_prevention.click.user_msg" \
                "User Message Click Limitations" \
                "" \
                "0" \
"User Message Click Limitations like: 1/864000 3/8640000 ..."

osbe_add_option fraud_prevention_click_ip \
                "" \
                "server.behaviour.frontend.fraud_prevention.click.ip" \
                "IP Click Limitations" \
                "" \
                "0" \
"IP Click Limitations like: 100/86400 500/864000 ..."

osbe_add_option fraud_prevention_click_ip_msg \
                "" \
                "server.behaviour.frontend.fraud_prevention.click.ip_msg" \
                "IP Message Click Limitations" \
                "" \
                "0" \
"IP Message Click Limitations like: 1/864000 3/8640000 ..."

osbe_add_option fraud_prevention_add_search_mail_user \
                "" \
                "server.behaviour.frontend.fraud_prevention.add_mail.user" \
                "User Add Search Mail Limitations" \
                "" \
                "0" \
"User Add Search Mail Limitations like: 100/86400 10/60 ..."

osbe_add_option fraud_prevention_add_search_mail_email \
                "" \
                "server.behaviour.frontend.fraud_prevention.add_mail.email" \
                "Email Add Search Mail Limitations" \
                "" \
                "0" \
"Email Add Search Mail Limitations like: 100/86400 10/60 ..."

osbe_add_option fraud_prevention_add_search_mail_ip \
                "" \
                "server.behaviour.frontend.fraud_prevention.add_mail.ip" \
                "IP Add Search Mail Limitations" \
                "" \
                "0" \
"IP Add Search Mail Limitations like: 300/86400 30/60 ..."

osbe_add_option fraud_prevention_update_search_mail_user \
                "" \
                "server.behaviour.frontend.fraud_prevention.update_mail.user" \
                "User Update Search Mail Limitations" \
                "" \
                "0" \
"User Update Search Mail Limitations like: 100/86400 10/60 ..."

osbe_add_option fraud_prevention_update_search_mail_email \
                "" \
                "server.behaviour.frontend.fraud_prevention.update_mail.email" \
                "Email Update Search Mail Limitations" \
                "" \
                "0" \
"Email Update Search Mail Limitations like: 100/86400 10/60 ..."

osbe_add_option fraud_prevention_update_search_mail_ip \
                "" \
                "server.behaviour.frontend.fraud_prevention.update_mail.ip" \
                "IP Update Search Mail Limitations" \
                "" \
                "0" \
"IP Update Search Mail Limitations like: 300/86400 30/60 ..."

osbe_add_option frontend_translator_default \
                "" \
                "server.behaviour.frontend.language.translator.default" \
                "Frontend Default Translator" \
                "" \
                "0" \
"Specifies default translation engine to use (can be empty or google)"

osbe_add_option frontend_translator_google_id \
                "" \
                "server.behaviour.frontend.language.translator.google.id" \
                "Frontend Translator Google Id" \
                "" \
                "0" \
"Specifies Content Id identifying NewsGate instance obtained from Google"

osbe_add_option frontend_language_filters \
                "" \
                "server.behaviour.frontend.language.filters" \
                "Language filter mapping" \
                "" \
                "0" \
"Specifies user language to message language filter mapping 
# (empty or smth. like: bel:rus ukr:rus rus:rus *:eng)."

osbe_add_option frontend_country_filters \
                "" \
                "server.behaviour.frontend.country.filters" \
                "Country filter mapping" \
                "" \
                "0" \
"Specifies message language to country filter mapping 
# (empty or smth. like: rus:RUS,UKR eng:USA,GBR)."

osbe_add_option crawler_googlebot_message_max_age \
                "-1" \
                "server.behaviour.frontend.crawler.googlebot.message_max_age" \
                "Googlebot message max age (sec)" \
                "" \
                "0" \
"Specifies for how long message should be available for Googlebot."

osbe_add_option crawler_googlebot_results_per_page \
                "-1" \
                "server.behaviour.frontend.crawler.googlebot.results_per_page" \
                "Googlebot results per page" \
                "" \
                "0" \
"Specifies how many messages per page to be shown for Googlebot."

osbe_add_option crawler_yandex_message_max_age \
                "-1" \
                "server.behaviour.frontend.crawler.yandex.message_max_age" \
                "Yandex crawler message max age (sec)" \
                "" \
                "0" \
"Specifies for how long message should be available for Yandex crawler."

osbe_add_option crawler_yandex_results_per_page \
                "-1" \
                "server.behaviour.frontend.crawler.yandex.results_per_page" \
                "Yandex crawler results per page" \
                "" \
                "0" \
"Specifies how many messages per page to be shown for Yandex crawler."

osbe_add_option crawler_other_message_max_age \
                "-1" \
                "server.behaviour.frontend.crawler.other.message_max_age" \
                "Other crawler message max age (sec)" \
                "" \
                "0" \
"Specifies for how long message should be available for other crawler."

osbe_add_option crawler_other_results_per_page \
                "-1" \
                "server.behaviour.frontend.crawler.other.results_per_page" \
                "Other crawler results per page" \
                "" \
                "0" \
"Specifies how many messages per page to be shown for other crawler."

#Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 5.1; .NET CLR 1.1.4322; .NET CLR 2.0.50727; .NET CLR 3.0.04506.30)

osbe_add_option request_user_agent \
                "Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; Trident/5.0)" \
                "server.behaviour.request.user_agent" \
                "Request User-Agent" \
                "" \
                "0" \
"Specifies User-Agent header for outcoming requests."

osbe_ask_for_options "NewsGate Server Behaviour"

content_main_languages=""

if test -z "$frontend_language_filters"; then
  frontend_language_filters="none"
else

  for val in $frontend_language_filters; do
    lang=`echo "$val" | sed -n -e "s/.*:\(.\+\)\$/\1/p"`
    content_main_languages="$content_main_languages $lang"
  done

  content_main_languages=\
`echo "$content_main_languages"|tr ' ' '\n' | sort | uniq | tr '\n' ' '|sed -e 's/ \+$//;s/^ \+//'`

fi

if test -z "$frontend_country_filters"; then
  frontend_country_filters="none"
fi

if test -z "$word_pair_counter_filename"; then
  word_pair_counter_filename="$cache_root/Message/Bank/MessageWordPairs"
fi

if test $message_mirroring -eq 0; then
  message_mirroring="none"
else  

  if test -z "$shared_message_source"; then
    AC_MSG_ERROR([shared message source should be specified])
  fi

  if test $message_mirroring -eq 1; then
    message_mirroring="sync"

  elif test $message_mirroring -eq 2; then

    if test $feed_moderating -ne 0; then
      AC_MSG_ERROR([feed moderating should be disabled])
    fi

    if test $feed_pulling -ne 0; then
      AC_MSG_ERROR([feed pulling should be disabled])
    fi

    if test $event_detection -ne 0; then
      AC_MSG_ERROR([event detection should be disabled])
    fi

    if test $stat_processing -eq 1; then
      AC_MSG_ERROR([stat processing should be disabled or delegated])
    fi

    if test $search_mailing -eq 1; then
      AC_MSG_ERROR([search mailing should be disabled or delegated])
    fi

    if test $fraud_prevention -eq 1; then
      AC_MSG_ERROR([fraud prevention should be disabled or delegated])
    fi

    if test $ad_serving -eq 1; then
      AC_MSG_ERROR([ad serving should be disabled or delegated])
    fi

    message_mirroring="absolute"
  else
    AC_MSG_ERROR([message_mirroring can be any of 0, 1, 2 values only])
  fi
fi

if test "$message_proxing" != "0"; then
  
  if test -z "$shared_message_source"; then
    AC_MSG_ERROR([shared message source should be specified])
  fi

  message_proxing="true"
else
  message_proxing="false"
fi

if test "$informer_enabled" != "1"; then
  informer_enabled="0"
fi

if test -n "$shared_message_source"; then

  if test $event_detection -eq 1; then
    pull_event_info="false"
  else
    pull_event_info="true"
  fi

  shared_messages_config="      <bank_manager 
        ref=\"$shared_message_source\"
        new_messages=\"true\"
        shared_messages=\"true\"
        old_messages=\"true\"
        own_events=\"$pull_event_info\"
        shared_events=\"$pull_event_info\"
        message_categorizer=\"true\"
        own_message_filter=\"true\"
        shared_message_filter=\"true\"
        own_message_stat=\"true\"
        shared_message_stat=\"true\"/>"

fi

message_image_thumbnail_sizes=""
message_image_thumbnail="0"

index=0

for ts in $message_image_thumbnails; do

  width=`echo "$ts" | sed -n -e "s/\(.*\)x.*/\1/p"`
  height=`echo "$ts" | sed -n -e "s/.*x\(.*\)/\1/p"`

  crop=`echo "$height" | sed -n -e "s/.*\(C\)/\1/p"`

  if test -n "$crop"; then
    height=`echo "$height" | sed -n -e "s/\(.*\)C/\1/p"`
  fi

  width=`expr 0 + "$width" 2>/dev/null`
  height=`expr 0 + "$height" 2>/dev/null`

  if test -z "$width" -o -z "$height"; then
    AC_MSG_ERROR([invalid thumbnail size '$ts'])
  fi

  if test $width -lt 1 -o $height -lt 1; then
    AC_MSG_ERROR([invalid thumbnail size '$ts'])
  fi
 
  message_image_thumbnail_sizes=\
"$message_image_thumbnail_sizes          <size width=\"$width\" height=\"$height\""

  if test -n "$crop"; then
    message_image_thumbnail_sizes=\
"$message_image_thumbnail_sizes crop=\"true\""
  fi

  message_image_thumbnail_sizes="$message_image_thumbnail_sizes/>
"

  message_image_thumbnail="1"
  index=`expr $index + 1`
done

if test $message_image_referer_check -eq 1; then
  image_referer="http://$service_frontend_search_hostname/"
else
  image_referer=""
fi

if test -n "$server_instance_email"; then
  server_admin_directive="ServerAdmin \"$server_instance_email\""
else
  server_admin_directive=""
fi

if test -n "$word_pair_counter_type" -a "$word_pair_counter_type" != "mem" -a \
   "$word_pair_counter_type" != "hash" -a \
   "$word_pair_counter_type" != "btree" -a \
   "$word_pair_counter_type" != "db"; then
  AC_MSG_ERROR([invalid word pair counter type spec: $word_pair_counter_type])
fi

if test -z "$word_pair_counter_type" -o "$word_pair_counter_type" = "mem"; then
  delete_message_pack="3000"
  saving_feed_state_packet="1000"
  saving_feed_state_delay="15"
else
  delete_message_pack="30"
  saving_feed_state_packet="10"
  saving_feed_state_delay="2"
fi

AC_SUBST([syslog_host])
AC_SUBST([syslog_port])

AC_SUBST([delete_message_pack])
AC_SUBST([saving_feed_state_packet])
AC_SUBST([saving_feed_state_delay])

AC_SUBST([server_instance_name])
AC_SUBST([server_instance_email])
AC_SUBST([copyright_note])
AC_SUBST([server_admin_directive])
AC_SUBST([feed_request_min_period])
AC_SUBST([feed_request_max_period])
AC_SUBST([message_max_age])
AC_SUBST([store_duplicate_messages])
AC_SUBST([category_name_as_last_resort])
OSBE_SUBST_VIA_FILE([shared_messages_config])
AC_SUBST([message_mirroring])
AC_SUBST([message_proxing])
AC_SUBST([message_max_core_words])
AC_SUBST([message_core_words_prc])
AC_SUBST([word_pair_counter_type])
AC_SUBST([word_pair_counter_filename])
AC_SUBST([word_pair_interval_groups])
AC_SUBST([word_pair_counter_bucket_factor])
AC_SUBST([word_pair_counter_cache_size])
AC_SUBST([message_image_max_width])
AC_SUBST([message_image_max_height])
AC_SUBST([message_description_max_chars])
AC_SUBST([message_image_max_count])
AC_SUBST([message_image_thumbnail])
OSBE_SUBST_VIA_FILE([message_image_thumbnail_sizes])
AC_SUBST([message_image_mob_thumb])
AC_SUBST([message_bank_capacity])
AC_SUBST([event_bank_backets])
AC_SUBST([event_max_size])
AC_SUBST([event_max_time_range])
AC_SUBST([event_merge_deny_size_factor])
AC_SUBST([event_merge_deny_max_time])
AC_SUBST([event_min_rift_time])
AC_SUBST([event_merge_max_time_diff])
AC_SUBST([event_merge_strain])
AC_SUBST([event_merge_level_base])
AC_SUBST([event_merge_level_min])
AC_SUBST([event_merge_level_time_based_increment_step])
AC_SUBST([event_merge_level_range_based_increment_step])
AC_SUBST([event_merge_level_strain_based_increment_step])
AC_SUBST([merge_level_size_based_decrement_step])
AC_SUBST([impression_respected_level])
AC_SUBST([stat_uid_cookie])
AC_SUBST([frontend_home_category_columns])
AC_SUBST([frontend_home_subcategory_count])
AC_SUBST([frontend_lang_last_resort])
AC_SUBST([cookie_domain_offset])
AC_SUBST([search_frontend_limited_option])
AC_SUBST([informer_enabled])
AC_SUBST([informer_thumbnails])
AC_SUBST([informer_default_thumbnail])
AC_SUBST([informer_cache_timeout])
AC_SUBST([trace_search_duration])
AC_SUBST([frontend_canonize_event])
AC_SUBST([xsearch_client_ips])
AC_SUBST([fraud_prevention_click_user])
AC_SUBST([fraud_prevention_click_user_msg])
AC_SUBST([fraud_prevention_click_ip])
AC_SUBST([fraud_prevention_click_ip_msg])
AC_SUBST([fraud_prevention_add_search_mail_user])
AC_SUBST([fraud_prevention_add_search_mail_email])
AC_SUBST([fraud_prevention_add_search_mail_ip])
AC_SUBST([fraud_prevention_update_search_mail_user])
AC_SUBST([fraud_prevention_update_search_mail_email])
AC_SUBST([fraud_prevention_update_search_mail_ip])
AC_SUBST([frontend_translator_default])
AC_SUBST([frontend_translator_google_id])
AC_SUBST([frontend_language_filters])
AC_SUBST([content_main_languages])
AC_SUBST([frontend_country_filters])
AC_SUBST([request_user_agent])
AC_SUBST([image_referer])
AC_SUBST([crawler_googlebot_message_max_age])
AC_SUBST([crawler_googlebot_results_per_page])
AC_SUBST([crawler_yandex_message_max_age])
AC_SUBST([crawler_yandex_results_per_page])
AC_SUBST([crawler_other_message_max_age])
AC_SUBST([crawler_other_results_per_page])

segmentation_implementations=""
corbaloc_segmentor_ref=""

for factory in $service_segmentor_factories; do

  factory_lib=`echo "$factory" | sed -n -e "s/.*@\(.*\)/\1/p"`

  if test -z "$factory_lib"; then
    AC_MSG_ERROR([no factory lib path in $factory])
  fi

  factory_func=`echo "$factory" | sed -n -e "s/\(.*\)@.*/\1/p"`

  if test -z "$factory_func"; then
    AC_MSG_ERROR([no factory function in $factory])
  fi

  factory_args=`echo "$factory_func" | sed -n -e "s/.*@\(.*\)/\1/p"`
  factory_func=`echo "$factory_func" | sed -n -e "s/\(.*\)@.*/\1/p"`

  if test -z "$factory_func"; then
    AC_MSG_ERROR([no factory function in $factory])
  fi

  segmentation_implementations="$segmentation_implementations
      <factory create_func_name=\"$factory_func\" create_func_args=\"$factory_args\" filename=\"$factory_lib\" />"
done

if test -n "$segmentation_implementations"; then
  corbaloc_segmentor_ref=`corbaloc_url "$server_host_segmentor" \
                         "$service_segmentor_port"`"/Segmentor"

fi

OSBE_SUBST_VIA_FILE([segmentation_implementations])
AC_SUBST([corbaloc_segmentor_ref])

corbaloc_event_bank_manager_ref=""

if test $event_detection -eq 1; then
  corbaloc_event_bank_manager_ref=`corbaloc_url "$server_host_event_bank_manager" \
                            "$service_event_bank_manager_port"`"/BankManager"
fi

AC_SUBST([corbaloc_event_bank_manager_ref])

OSBE_SUBST_VIA_FILE([extra_installs])

if test $search_frontend_count -lt 1; then
  AC_MSG_ERROR([No SearchFrontend services specified for colocation])
fi

if test $message_bank_count -lt 1; then
  AC_MSG_ERROR([No MessageBank services specified for colocation])
fi

bank_client_session_threads=`expr 10 \* $message_bank_count`

if test $bank_client_session_threads -gt 1000; then
  bank_client_session_threads=1000
fi

# +10 for pullers, message sharing, event banks, ...
message_bank_threads=`expr 10 \* $search_frontend_count + 10`

if test $message_bank_threads -gt 1000; then
  message_bank_threads=1000
fi

AC_SUBST([bank_client_session_threads])
AC_SUBST([message_bank_threads])

if test -n "$server_host_feed_manager" -a \
        -n "$server_host_rss_puller_manager" -a \
        "$server_host_feed_manager" != "$server_host_rss_puller_manager"; then
  AC_MSG_ERROR([FeedManager and RSSPullerManager services should be colocated on the same host])
fi

if test -n "$server_host_ad_server" -a \
        -n "$server_host_ad_manager" -a \
        "$server_host_ad_server" != "$server_host_ad_manager"; then
  AC_MSG_ERROR([AdServer and AdManager services should be colocated on the same host])
fi

if test -n "$server_host_feed_manager"; then
  if test "$server_host_feed_manager" != "$server_host_message_bank_manager"; then
    message_managing_db_endpoint="host=\"$server_host_feed_manager\" port=\"$service_db_port\""
  else
    message_managing_db_endpoint="unix_socket=\"$run_root/MySQL/mysql.socket\""
  fi
fi

OSBE_SUBST_VIA_FILE([message_managing_db_endpoint])

db_passwd=`dd if=/dev/urandom count=128 bs=1 2>&1 | md5sum | cut -b-20`
AC_SUBST([db_passwd])

m4_include([dir.ac])
OSBE_OUTPUT
