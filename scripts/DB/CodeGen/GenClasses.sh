#!/bin/sh

# product   : NewsGate - news search WEB server
# copyright : Copyright (c) 2005-2016 Karen Arutyunov
# licenses  : CC BY-NC-SA 3.0; see accompanying LICENSE file
#             Commercial; contact karen.arutyunov@gmail.com

echo "Generating classes for NewsGate DB ..."

feed_query="select Feed.id as id, Feed.type as type, Feed.url as url, \
Feed.encoding as encoding, Feed.space as space, Feed.lang as lang, \
Feed.country as country, Feed.status as status, Feed.keywords as keywords, \
Feed.adjustment_script as adjustment_script, \
RSSFeedState.channel_title as channel_title, \
RSSFeedState.channel_description as channel_description, \
RSSFeedState.channel_html_link as channel_html_link, \
RSSFeedState.channel_lang as channel_lang, \
RSSFeedState.channel_country as channel_country, \
RSSFeedState.channel_ttl as channel_ttl, \
RSSFeedState.channel_last_build_date as channel_last_build_date, \
RSSFeedState.last_request_date as last_request_date, \
RSSFeedState.last_modified_hdr as last_modified_hdr, \
RSSFeedState.etag_hdr as etag_hdr, \
RSSFeedState.content_length_hdr as content_length_hdr, \
RSSFeedState.entropy as entropy, \
RSSFeedState.entropy_updated_date as entropy_updated_date, \
RSSFeedState.size as size, \
RSSFeedState.single_chunked as single_chunked, \
RSSFeedState.first_chunk_size as first_chunk_size, \
RSSFeedState.heuristics_counter as heuristics_counter, \
RSSFeedState.cache as cache \
from Feed left join RSSFeedState on Feed.id=RSSFeedState.feed_id \
where update_num<1 limit 1"

echo "Feed classes ..."

ElMySQLClassGen gen \
class="NewsGate::RSS::FeedRecord" ignore-not-null-flag="1" \
query="$feed_query" \
class="NewsGate::RSS::FeedMessageCode" \
query="select msg_id, published from RSSFeedMessageCodes limit 1" \
class="NewsGate::RSS::MaxId" \
query="select max(id) as value from Feed" \
class="NewsGate::RSS::FeedUpdateNum" \
query="select update_num from FeedUpdateNum" \
class="NewsGate::RSS::FeedStatusRecord" \
query="select id, url, status from Feed limit 1" \
class="NewsGate::RSS::FeedFormerUrlRecord" \
query="select FeedFormerUrl.feed_id as feed_id, FeedFormerUrl.url as url, Feed.status as status from FeedFormerUrl left join Feed on feed_id=Feed.id limit 1" \
class="NewsGate::RSS::MessageFilterUpdateNum" \
query="select update_num from MessageFilterUpdateNum" \
unix_socket="$MYSQL_SOCKET" \
user=root db=NewsGate > \
$SERVER_ROOT/Server/Services/Feeds/RSS/PullerManager/FeedRecord.hpp

result=$?

if test $result -eq 0; then
  echo "done"
else
  echo "failed"
  exit $result
fi

echo "Message Bank Manager classes ..."

ElMySQLClassGen gen \
class="NewsGate::Message::MessageFilterUpdateNum" \
query="select update_num from MessageFilterUpdateNum" \
class="NewsGate::Message::MessageFetchFilter" \
query="select expression from MessageFetchFilter limit 1" \
class="NewsGate::Message::Feed" \
query="select id, url from Feed limit 1" \
class="NewsGate::Message::FeedFormerUrl" \
query="select FeedFormerUrl.url as url from FeedFormerUrl left join Feed on \
FeedFormerUrl.url=Feed.url limit 1" \
class="NewsGate::Message::MessageRec" \
query="select id from MessageFilter limit 1" \
class="NewsGate::Message::CategoryUpdateNum" \
query="select update_num from CategoryUpdateNum" \
class="NewsGate::Message::CategoryDesc" \
query="select id, name, searcheable from Category limit 1" \
class="NewsGate::Message::CategoryLocaleDesc" \
query="select lang, country, name, title, short_title, description, keywords \
from CategoryLocale limit 1" \
class="NewsGate::Message::CategoryExpressionDesc" \
query="select expression from CategoryExpression limit 1" \
class="NewsGate::Message::CategoryMessage" \
query="select message_id, updated, relation from CategoryMessage limit 1" \
unix_socket="$MYSQL_SOCKET" \
user=root db=NewsGate > \
$SERVER_ROOT/Server/Services/Message/BankManager/DB_Record.hpp

result=$?

if test $result -eq 0; then
  echo "done"
else
  echo "failed"
  exit $result
fi

echo "Message Bank classes ..."

ElMySQLClassGen gen \
class="NewsGate::Message::BankSessionInfo" \
query="select bank_ior, session_id, last_ping_time from MessageBankSession limit 1" \
class="NewsGate::Message::MessageBankManagerStateInfo" \
query="select last_check_banks_presence_time from MessageBankManagerState limit 1" \
unix_socket="$MYSQL_SOCKET" \
user=root db=NewsGate > \
$SERVER_ROOT/Server/Services/Commons/Message/MessageBankRecord.hpp

result=$?

if test $result -eq 0; then
  echo "done"
else
  echo "failed"
  exit $result
fi

echo "Message classes ..."

ElMySQLClassGen gen \
class="NewsGate::Message::MessageContentRecord" \
query="select Message.id as id, MessageDict.hash as dict_hash, complements, \
url, source_html_link from Message \
left join MessageDict on Message.id=MessageDict.id \
limit 1" \
class="NewsGate::Message::MessageCount" \
query="select count(1) as count from MessageDict" \
unix_socket="$MYSQL_SOCKET" \
user=root db=NewsGate > \
$SERVER_ROOT/Server/Services/Message/Bank/MessageRecord.hpp

result=$?

if test $result -eq 0; then
  echo "done"
else
  echo "failed"
  exit $result
fi

echo "Moderator Manager classes ..."

privileges_query="select moderator, privilege, granted_by, args \
from ModeratorPrivileges limit 1"

ElMySQLClassGen gen \
class="NewsGate::Moderation::ModeratorRecord" \
query="select id, name, password_digest, email, updated, created, \
creator, superior, status, show_deleted, comment from Moderator \
where status != 'L' limit 1" \
class="NewsGate::Moderation::ModeratorPrivilegeRecord" \
query="$privileges_query" \
unix_socket="$MYSQL_SOCKET" \
user=root db=NewsGateModeration > \
$SERVER_ROOT/Server/Services/Moderator/ModeratorManager/ModeratorRecord.hpp

result=$?

if test $result -eq 0; then
  echo "done"
else
  echo "failed"
  exit $result
fi

echo "Feed Manager classes ..."

feed_query="select Feed.id as id, Feed.type as type, Feed.url as url, \
Feed.encoding as encoding, Feed.space as space, Feed.lang as lang, \
Feed.country as country, Feed.status as status, Feed.creator as creator, \
Feed.creator_type as creator_type, Feed.keywords as keywords, \
Feed.adjustment_script as adjustment_script, Feed.comment as comment, 
Feed.created as created, Feed.updated as updated, \
RSSFeedState.channel_title as channel_title, \
RSSFeedState.channel_description as channel_description, \
RSSFeedState.channel_html_link as channel_html_link, \
RSSFeedState.channel_lang as channel_lang, \
RSSFeedState.channel_country as channel_country, \
RSSFeedState.channel_ttl as channel_ttl, \
RSSFeedState.channel_last_build_date as channel_last_build_date, \
RSSFeedState.last_request_date as last_request_date, \
RSSFeedState.last_modified_hdr as last_modified_hdr, \
RSSFeedState.etag_hdr as etag_hdr, \
RSSFeedState.content_length_hdr as content_length_hdr, \
RSSFeedState.entropy as entropy, \
RSSFeedState.entropy_updated_date as entropy_updated_date, \
RSSFeedState.size as size, \
RSSFeedState.single_chunked as single_chunked, \
RSSFeedState.first_chunk_size as first_chunk_size, \
RSSFeedState.heuristics_counter as heuristics_counter, \
sum(RSSFeedStat.requests) as requests, \
sum(RSSFeedStat.failed) as failed, \
sum(RSSFeedStat.unchanged) as unchanged, \
sum(RSSFeedStat.not_modified) as not_modified, \
sum(RSSFeedStat.presumably_unchanged) as presumably_unchanged, \
sum(RSSFeedStat.has_changes) as has_changes, \
sum(RSSFeedStat.wasted) as wasted, \
sum(RSSFeedStat.outbound) as outbound, \
sum(RSSFeedStat.inbound) as inbound, \
sum(RSSFeedStat.requests_duration) as requests_duration, \
sum(RSSFeedStat.messages) as messages, \
sum(RSSFeedStat.messages_size) as messages_size, \
sum(RSSFeedStat.messages_delay) as messages_delay, \
max(RSSFeedStat.max_message_delay) as max_message_delay, \
sum(StatFeed.msg_impressions) as msg_impressions, \
sum(StatFeed.msg_clicks) as msg_clicks, \
if(msg_impressions > 0, msg_clicks * 10000 / msg_impressions, 0) as msg_ctr \
from Feed left join RSSFeedStat on id=RSSFeedStat.feed_id \
left join RSSFeedState on id=RSSFeedState.feed_id \
left join StatFeed on id=StatFeed.feed_id \
where update_num<1 \
group by id limit 1"

ElMySQLClassGen gen \
class="NewsGate::Moderation::FeedTypeRecord" \
query="select type from Feed limit 1" \
class="NewsGate::Moderation::FeedUpdateNum" \
query="select update_num from FeedUpdateNum" \
class="NewsGate::Moderation::FeedRecord" ignore-not-null-flag="1" \
ignore-binary-flag="1" \
query="$feed_query" \
class="NewsGate::Moderation::FeedIdAndUrl" \
query="select id, url from Feed limit 1" \
class="NewsGate::Moderation::MessageFilterUpdateNum" \
query="select update_num from MessageFilterUpdateNum" \
class="NewsGate::Moderation::MessageFetchFilter" \
query="select expression, description from MessageFetchFilter limit 1" \
unix_socket="$MYSQL_SOCKET" \
user=root db=NewsGate > \
$SERVER_ROOT/Server/Services/Moderator/FeedManager/FeedRecord.hpp

result=$?

if test $result -eq 0; then
  echo "done"
else
  echo "failed"
  exit $result
fi

echo "Event classes ..."

ElMySQLClassGen gen \
class="NewsGate::Event::EventRecord" \
query="select event_id, data from Event limit 1" \
class="NewsGate::Event::EventMessageRecord" \
query="select id, data from EventMessage limit 1" \
class="NewsGate::Event::EventLangRecord" \
query="select lang, count(*) as events from Event group by lang limit 1" \
unix_socket="$MYSQL_SOCKET" \
user=root db=NewsGate > \
$SERVER_ROOT/Server/Services/Event/Bank/EventRecord.hpp

result=$?

if test $result -eq 0; then
  echo "done"
else
  echo "failed"
  exit $result
fi

echo "Category classes ..."

ElMySQLClassGen gen \
class="NewsGate::Moderation::Category::CategoryUpdateNum" \
query="select update_num from CategoryUpdateNum" \
class="NewsGate::Moderation::Category::CategoryDesc" \
query="select id, name, status, searcheable, updated, created, creator, version, description from Category limit 1" \
class="NewsGate::Moderation::Category::CategoryVer" \
query="select version from Category limit 1" \
class="NewsGate::Moderation::Category::CategoryExpressionDesc" \
query="select expression, description from CategoryExpression limit 1" \
class="NewsGate::Moderation::Category::CategoryWordListDesc" \
query="select name, words, version, description from CategoryWordList limit 1" \
class="NewsGate::Moderation::Category::CategoryWordListFindingDesc" \
query="select category_id, name, words from CategoryWordList limit 1" \
class="NewsGate::Moderation::Category::CategoryWordListVer" \
query="select name, version from CategoryWordList limit 1" \
class="NewsGate::Moderation::Category::CategoryLocaleDesc" \
query="select lang, country, name, title, short_title, description, keywords \
from CategoryLocale limit 1" \
class="NewsGate::Moderation::Category::CategoryChild" \
query="select CategoryChild.child_id as id, Category.name as name from CategoryChild left join Category on CategoryChild.child_id = Category.id limit 1" \
class="NewsGate::Moderation::Category::CategoryMessage" \
query="select message_id, relation from CategoryMessage limit 1" \
unix_socket="$MYSQL_SOCKET" \
user=root db=NewsGate > \
$SERVER_ROOT/Server/Services/Moderator/CategoryManager/DB_Record.hpp

result=$?

if test $result -eq 0; then
  echo "done"
else
  echo "failed"
  exit $result
fi

echo "Customer classes ..."

ElMySQLClassGen gen \
class="NewsGate::Moderation::Customer::CustomerBalance" \
query="select balance from Customer limit 1" \
class="NewsGate::Moderation::Customer::Customer" \
query="select id, status, balance from Customer limit 1" \
unix_socket="$MYSQL_SOCKET" \
user=root db=NewsGate > \
$SERVER_ROOT/Server/Services/Moderator/CustomerManager/DB_Record.hpp

result=$?

if test $result -eq 0; then
  echo "done"
else
  echo "failed"
  exit $result
fi

echo "Ad Server classes ..."

ElMySQLClassGen gen \
class="NewsGate::Ad::DB::AdSelector" \
query="select status, update_num, pcws_weight_zones, pcws_reduction_rate \
from AdSelector" \
class="NewsGate::Ad::DB::Page" \
query="select id, max_ad_num from AdPage limit 1" \
class="NewsGate::Ad::DB::PageAdvRestriction" \
query="select advertiser, max_ad_num from AdPageAdvRestriction limit 1" \
class="NewsGate::Ad::DB::AdPageAdvAdvRestriction" \
query="select advertiser, advertiser2, max_ad_num from \
AdPageAdvAdvRestriction limit 1" \
class="NewsGate::Ad::DB::Placement" \
query="select AdPlacement.id as id, AdGroup.id as group_id, \
AdGroup.cap_min_time as group_cap_min_time, \
AdPlacement.slot as slot, AdSize.width as width, AdSize.height as height, \
AdPlacement.cpm * AdGroup.auction_factor as weight, inject, \
Ad.advertiser as advertiser, Ad.text as text from AdPlacement \
left join AdSlot on AdPlacement.slot=AdSlot.id \
left join Ad on AdPlacement.ad=Ad.id \
left join AdSize on Ad.size=AdSize.id \
left join AdGroup on AdPlacement.group_id=AdGroup.id \
left join AdCampaign on AdGroup.campaign=AdCampaign.id \
left join Advertiser on Ad.advertiser=Advertiser.id \
where AdPlacement.status='E' and AdSlot.status='E' and \
Ad.status='E' and AdSize.status='E' and AdGroup.status='E' and \
AdCampaign.status='E' and Advertiser.status='E' limit 1" \
class="NewsGate::Ad::DB::CounterPlacement" \
query="select AdCounterPlacement.id as id, AdGroup.id as group_id, \
AdGroup.cap_min_time as group_cap_min_time, \
AdCounterPlacement.advertiser as advertiser, AdCounter.text as text \
from AdCounterPlacement \
left join AdPage on AdCounterPlacement.page=AdPage.id \
left join AdCounter on AdCounterPlacement.counter=AdCounter.id \
left join AdGroup on AdCounterPlacement.group_id=AdGroup.id \
left join AdCampaign on AdGroup.campaign=AdCampaign.id \
left join Advertiser on AdCounter.advertiser=Advertiser.id \
where AdCounterPlacement.status='E' and AdPage.status='E' and \
AdCounter.status='E' and AdGroup.status='E' and \
AdCampaign.status='E' and Advertiser.status='E' limit 1" \
class="NewsGate::Ad::DB::Condition" \
query="select AdCondition.id as id, rnd_mod, rnd_mod_from, rnd_mod_to, \
group_freq_cap, group_count_cap, query_types, query_type_exclusions, \
page_sources, page_source_exclusions, \
message_sources, message_source_exclusions, \
page_categories, page_category_exclusions, \
message_categories, message_category_exclusions, \
search_engines, search_engine_exclusions, \
crawlers, crawler_exclusions, languages, language_exclusions, \
countries, country_exclusions, ip_masks, ip_mask_exclusions, \
tags, tag_exclusions, referers, referer_exclusions, \
content_languages, content_language_exclusions \
from AdPlacement \
left join AdGroupCondition on AdPlacement.group_id=AdGroupCondition.group_id \
left join AdCondition on AdGroupCondition.condition_id=AdCondition.id \
where AdCondition.status='E' limit 1" \
unix_socket="$MYSQL_SOCKET" \
user=root db=NewsGate > \
$SERVER_ROOT/Server/Services/Ad/AdServer/DB_Record.hpp

result=$?

if test $result -eq 0; then
  echo "done"
else
  echo "failed"
  exit $result
fi

echo "Ad Manager classes ..."

ElMySQLClassGen gen \
class="NewsGate::Moderation::Ad::DB::Global" \
query="select AdSelector.status as selector_status, \
Advertiser.max_ads_per_page as adv_max_ads_per_page, \
update_num, pcws_weight_zones, pcws_reduction_rate from AdSelector join \
Advertiser limit 1" \
class="NewsGate::Moderation::Ad::DB::Page" \
query="select id, name, status, AdPage.max_ad_num as max_ad_num, \
AdPageAdvRestriction.max_ad_num as advertiser_max_ad_num \
from AdPage left join AdPageAdvRestriction on \
AdPage.id=AdPageAdvRestriction.page limit 1" \
class="NewsGate::Moderation::Ad::DB::Slot" \
query="select id, name, status, min_width, max_width, min_height, \
max_height from AdSlot limit 1" \
class="NewsGate::Moderation::Ad::DB::Size" \
query="select id, name, status, width, height from AdSize limit 1" \
class="NewsGate::Moderation::Ad::DB::Ad" \
query="select Ad.id as id, Ad.name as name, Ad.status as status, \
Ad.size as size, AdSize.name as size_name, width, height, text from Ad \
join AdSize on Ad.size=AdSize.id limit 1" \
class="NewsGate::Moderation::Ad::DB::Counter" \
query="select id, name, status, text from AdCounter limit 1" \
class="NewsGate::Moderation::Ad::DB::Condition" \
query="select id, name, status, rnd_mod, rnd_mod_from, rnd_mod_to, \
group_freq_cap, group_count_cap, query_types, query_type_exclusions, \
page_sources, page_source_exclusions, \
message_sources, message_source_exclusions, \
page_categories, page_category_exclusions, \
message_categories, message_category_exclusions, \
search_engines, search_engine_exclusions, crawlers, crawler_exclusions, \
languages, language_exclusions, countries, country_exclusions, \
ip_masks, ip_mask_exclusions, \
tags, tag_exclusions, referers, referer_exclusions, \
content_languages, content_language_exclusions  \
from AdCondition limit 1" \
class="NewsGate::Moderation::Ad::DB::Campaign" \
query="select id, name, status from AdCampaign limit 1" \
class="NewsGate::Moderation::Ad::DB::Group" \
query="select AdGroup.id as id, campaign, AdCampaign.name as campaign_name, \
AdGroup.name as name, AdGroup.status as status, auction_factor from \
AdGroup join AdCampaign on AdGroup.campaign=AdCampaign.id limit 1" \
class="NewsGate::Moderation::Ad::DB::GroupCondition" \
query="select AdGroupCondition.condition_id as id, \
AdCondition.name as name, AdCondition.status as status from AdGroupCondition \
join AdCondition on AdGroupCondition.condition_id=AdCondition.id limit 1" \
class="NewsGate::Moderation::Ad::DB::Placement" \
query="select AdPlacement.id as id, group_id, \
AdGroup.name as group_name, AdGroup.campaign as campaign, \
AdCampaign.name as campaign_name, AdPlacement.name as name, \
AdPlacement.status as status, slot, AdSlot.name as slot_name, \
AdSlot.min_width as slot_min_width, \
AdSlot.max_width as slot_max_width, \
AdSlot.min_height as slot_min_height, \
AdSlot.max_height as slot_max_height, \
AdSlot.status as slot_status, ad, Ad.name as ad_name, \
Ad.status as ad_status, Ad.size as ad_size, \
AdSize.name as ad_size_name, AdSize.width as ad_width, \
AdSize.height as ad_height, cpm, inject, auction_factor, \
AdPage.max_ad_num as page_max_ad_num, AdPage.id as page_id, \
AdPage.name as page_name, \
AdPageAdvRestriction.max_ad_num as adv_rst_max_ad_num, \
Advertiser.max_ads_per_page as adv_max_ads_per_page, \
Advertiser.status as adv_status, Advertiser.name as adv_name, \
AdSelector.status as selector_status, AdGroup.status as group_status, \
AdSize.status as size_status, AdPage.status as page_status, \
AdCampaign.status as campaign_status \
from AdPlacement join AdGroup on AdPlacement.group_id=AdGroup.id \
join AdCampaign on AdGroup.campaign=AdCampaign.id \
join Ad on AdPlacement.ad=Ad.id \
join AdSize on Ad.size=AdSize.id \
join AdSlot on slot=AdSlot.id \
join AdPage on AdPage.id=AdSlot.page \
join AdPageAdvRestriction on AdPage.id=AdPageAdvRestriction.page and \
AdPlacement.advertiser=AdPageAdvRestriction.advertiser \
join Advertiser on Advertiser.id=AdPlacement.advertiser \
join AdSelector limit 1" \
class="NewsGate::Moderation::Ad::DB::CounterPlacement" \
query="select AdCounterPlacement.id as id, group_id, \
AdGroup.name as group_name, AdGroup.campaign as campaign, \
AdCampaign.name as campaign_name, AdCounterPlacement.name as name, \
AdCounterPlacement.status as status, page, \
AdPage.name as page_name, AdPage.status as page_status, \
counter, AdCounter.name as counter_name, AdCounter.status as counter_status, \
Advertiser.status as adv_status, Advertiser.name as adv_name, \
AdSelector.status as selector_status, AdGroup.status as group_status, \
AdCampaign.status as campaign_status from AdCounterPlacement join AdGroup \
on AdCounterPlacement.group_id=AdGroup.id \
join AdCampaign on AdGroup.campaign=AdCampaign.id \
join AdPage on page=AdPage.id \
join AdCounter on AdCounterPlacement.counter=AdCounter.id \
join Advertiser on Advertiser.id=AdCounterPlacement.advertiser \
join AdSelector limit 1" \
class="NewsGate::Moderation::Ad::DB::PageAdvAdvRestriction" \
query="select advertiser2, Advertiser.name as advertiser2_name, max_ad_num \
from AdPageAdvAdvRestriction join Advertiser on \
AdPageAdvAdvRestriction.advertiser2=Advertiser.id limit 1" \
unix_socket="$MYSQL_SOCKET" \
user=root db=NewsGate > \
$SERVER_ROOT/Server/Services/Moderator/AdManager/DB_Record.hpp

result=$?

if test $result -eq 0; then
  echo "done"
else
  echo "failed"
  exit $result
fi

echo "SearchMailing classes ..."

ElMySQLClassGen gen \
class="NewsGate::SearchMailing::DB::Subscription" \
query="select id, status, reg_time, reg_time_usec, update_time, \
update_time_usec, search_time, email, format, length, time_offset, title, \
query, modifier, filter, res_query, res_filter_lang, res_filter_country, \
res_filter_feed, res_filter_category, res_filter_event, locale_lang, \
locale_country, lang, user_id, user_ip, user_agent, user_session, times \
from SearchMailSubscription limit 1" \
class="NewsGate::SearchMailing::DB::SubscriptionUpdate" \
query="select token, state, type, conf_email, conf_time, id, status, req_time, 
req_time_usec, email, format, length, time_offset, title, query, modifier, \
filter, res_query, res_filter_lang, res_filter_country, \
res_filter_feed, res_filter_category, res_filter_event, locale_lang, \
locale_country, lang, user_id, user_ip, user_agent, user_session, times \
from SearchMailSubscriptionUpdate limit 1" \
class="NewsGate::SearchMailing::DB::MailState" \
query="select last_dispatch_time, mailer_id from SearchMailState limit 1" \
unix_socket="$MYSQL_SOCKET" \
user=root db=NewsGate > \
$SERVER_ROOT/Server/Services/Mailing/SearchMailer/DB_Record.hpp

result=$?

if test $result -eq 0; then
  echo "done"
else
  echo "failed"
  exit $result
fi
