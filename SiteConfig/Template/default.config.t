#ifdef HELP
#undef HELP

Macro-Arguments:

  HELP  - outputs this help information.

  HOST               - host name.

  SSH_KEY            - path to private (passwordless) SSH key.

  ELEMENTS_SRC       - path to Elements sources directory.

  ELEMENTS_BUILD     - path to Element build directory.

  NEWSGATE_SRC       - path to NewsGate sources directory.

  NEWSGATE_BUILD     - path to NewsGate build directory.

  NEWSGATE_WORKSPACE - path to NewsGate (not yet created) workspace directory.

Usage Examples:

  cpp -DHOST=$HOSTNAME -DSSH_KEY=~/.ssh/ngkey -DELEMENTS_SRC=~/projects/Elements -DELEMENTS_BUILD=~/projects/Elements/build -DNEWSGATE_SRC=~/projects/NewsGate/Server -DNEWSGATE_BUILD=~/projects/NewsGate/Server/build -DNEWSGATE_WORKSPACE=~/projects/NewsGate/Workspace default.config.t > ./build/default.config

  cpp -DHELP default.config.t

#else

subsystem.feed_moderating = 1
subsystem.client_moderating = 0
subsystem.customer_moderating = 0
subsystem.feed_pulling = 1
subsystem.event_detection = 1
subsystem.stat_processing = 1
subsystem.fraud_prevention = 1
subsystem.ad_serving = 1
subsystem.ad_management = 1
subsystem.limited_frontend = 0
subsystem.search_mailing = 1
subsystem.tests = 0

elements.paths.root = ~/projects/Elements
elements.paths.bin = ELEMENTS_BUILD/bin
elements.paths.lib = ELEMENTS_BUILD/lib
elements.paths.scripts = ELEMENTS_SRC/scripts
elements.paths.xsd = ELEMENTS_SRC/xsd
elements.paths.loc = ELEMENTS_SRC/loc
elements.paths.dict = ELEMENTS_SRC/dict

server.paths.root = ~/projects/NewsGate
server.paths.bin = NEWSGATE_BUILD/bin
server.paths.lib = NEWSGATE_BUILD/lib
server.paths.scripts = NEWSGATE_SRC/scripts
server.paths.xsd = NEWSGATE_SRC/xsd
server.paths.site_config = NEWSGATE_WORKSPACE/etc/NewsGate
server.paths.data = NEWSGATE_WORKSPACE/var/lib
server.paths.www = NEWSGATE_SRC/www
server.paths.run = NEWSGATE_WORKSPACE/var/run
server.paths.cache = NEWSGATE_WORKSPACE/var/cache
server.paths.log = NEWSGATE_WORKSPACE/var/log
server.paths.tmp = NEWSGATE_WORKSPACE/var/tmp
server.paths.sshkey = SSH_KEY

server.ports.base = 7100

service.moderator_manager.log.level = 10
service.moderator_manager.log.aspects = *

service.feed_manager.log.level = 10
service.feed_manager.log.aspects = *

service.category_manager.log.level = 10
service.category_manager.log.aspects = *

service.rss_puller_manager.log.level = 10
service.rss_puller_manager.log.aspects = *

service.rss_puller.log.level = 10
service.rss_puller.log.aspects = Application, State, PullingFeeds

service.stat_processor.log.level = 10
service.stat_processor.log.aspects = *
service.stat_processor.raw_stat_keep_days = 3

service.search_mailer.log.level = 10
service.search_mailer.log.aspects = *
service.search_mailer.email = mailbox@mycompany.com
service.search_mailer.debug_email = 
service.search_mailer.trust_timeout = 3600
service.search_mailer.recaptcha.client_key=
service.search_mailer.recaptcha.server_key=
service.search_mailer.recipient_blacklist =

service.message_bank_manager.log.level = 10
service.message_bank_manager.log.aspects = * 

service.message_bank.log.level = 10
service.message_bank.log.aspects = *

service.word_manager.log.level = 10
service.word_manager.log.aspects = *

service.segmentor.log.level = 10
service.segmentor.log.aspects = *

service.event_bank_manager.log.level = 10
service.event_bank_manager.log.aspects = *

service.event_bank.log.level = 10
service.event_bank.log.aspects = *


service.frontend.error_in_response = 1
service.frontend.debug_info = 1
service.frontend.development_mode = 1

service.frontend.search.log.level = 10
service.frontend.search.log.aspects = *
service.frontend.search.ensure_canonical = 0

service.frontend.limited.log.level = 10
service.frontend.limited.log.aspects = *

service.frontend.moderator.log.level = 10
service.frontend.moderator.log.aspects = *

cluster.host.0 = HOST : DB Configurator RSSPullerManager RSSPuller MessageBankManager MessageBank WordManager Segmentor EventBankManager EventBank SearchFrontend ModeratorManager FeedManager CategoryManager ModeratorFrontend StatProcessor LimitedFrontend CustomerManager LimitChecker AdManager AdServer SearchMailer
cluster.host.count = 1

server.behaviour.server_instance_name = MyCompany
server.behaviour.server_instance_email = mailbox@mycompany.com
server.behaviour.copyright_note = Copyright &copy; 2017 MyCompany.
server.behaviour.feed.request.min_period = 300
server.behaviour.feed.request.max_period = 21600
server.behaviour.message.maxage = 946080000
server.behaviour.message.store_duplicate = 0
server.behaviour.message.shared_message_source =
server.behaviour.message.mirroring = 0
server.behaviour.message.proxing = 0
server.behaviour.message.word_pair_counter.type = mem

server.behaviour.message.max_core_words = 20
server.behaviour.message.core_words_prc = 75
server.behaviour.message.image.max_width = 400
server.behaviour.message.image.max_height = 500
server.behaviour.message.image.max_count = 21
server.behaviour.message.image.thumbnails = 300x300 300x300C 150x150 150x150C 60x60 60x60C
server.behaviour.message.image.mob_thumbnail = 60x60
server.behaviour.message.image.referer.check = 0
server.behaviour.message.stat.impression_respected_level = 100
server.behaviour.message.description.max_chars = 1024

server.behaviour.message.bank.capacity = 100000000
server.behaviour.message.stat.uid_cookie = 1

server.behaviour.frontend.cookie.domain.offset = 1
server.behaviour.frontend.informer.thumbnails = 20x20 20x20C 30x30 30x30C 40x40 40x40C 50x50 50x50C 60x60 60x60C 70x70 70x70C 80x80 80x80C 90x90 90x90C 100x100 100x100C 125x125 125x125C 150x150 150x150C 175x175 175x175C 200x200 200x200C 250x250 250x250C 300x300 300x300C
server.behaviour.frontend.informer.default_thumbnail = 60x60C
server.behaviour.frontend.informer.cache.timeout = 180

server.behaviour.frontend.language.translator.default = google

server.behaviour.frontend.language.filters = abk:rus ady:rus arm:rus aze:rus bel:rus bua:rus cau:rus che:rus chv:rus crh:rus geo:rus kaz:rus kbd:rus kir:rus kom:rus lez:rus lit:rus mol:rus oss:rus rus:rus tat:rus tuk:rus tyv:rus udm:rus ukr:rus uzb:rus xal:rus *:eng
server.behaviour.frontend.country.filters = eng:USA,GBR,IND,AUS,NZL rus:RUS,UKR,BLR
server.behaviour.frontend.canonize_event = 0

server.behaviour.frontend.fraud_prevention.click.user = 100/86400
server.behaviour.frontend.fraud_prevention.click.user_msg = 1/864000
server.behaviour.frontend.fraud_prevention.click.ip = 300/86400
server.behaviour.frontend.fraud_prevention.click.ip_msg = 3/864000

server.behaviour.frontend.fraud_prevention.add_mail.user = 50/86400 10/60
server.behaviour.frontend.fraud_prevention.add_mail.email = 50/86400 10/60
server.behaviour.frontend.fraud_prevention.add_mail.ip = 150/86400 10/60
server.behaviour.frontend.fraud_prevention.update_mail.user = 200/86400 50/300
server.behaviour.frontend.fraud_prevention.update_mail.email = 200/86400 50/300
server.behaviour.frontend.fraud_prevention.update_mail.ip = 600/86400 50/300

#endif // HELP
