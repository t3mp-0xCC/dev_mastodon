#!/bin/sh

# load config
source /etc/dev_mastodon.conf
#INSTANCE="example.com"

url="https://$INSTANCE/api/v1/timelines/public?limit=40"
tmp="/tmp/contents.txt"

# get instance timeline & parse it
curl $url 2>/dev/null | jq '.[] | .content' \
#| tr -d -d "\"<p>" | tr -d -d "</p>\"," \
> $tmp
