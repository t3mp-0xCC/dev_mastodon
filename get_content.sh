#!/bin/sh

# load config
source /etc/dev_mastodon.conf
#INSTANCE="example.com"

url="https://$INSTANCE/api/v1/timelines/public?limit=40"
tmp="/tmp/contents.txt"

# get instance timeline & parse with jq
curl $url 2>/dev/null \ | jq '.[] | .content' |\
# remove html content
sed -e "s/\"<p>//g" | sed -e "s/<\/p>\"/\n/g" | sed -e "s/<br \/>/\n/g" | sed -e "s/<br>/\n/g" \
> $tmp
