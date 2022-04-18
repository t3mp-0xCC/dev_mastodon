#!/bin/sh

# load config
source /etc/dev_mastodon.conf
#INSTANCE="example.com"

url="https://$INSTANCE/api/v1/timelines/public?limit=40"
tmp="/tmp/contents.txt"

curl $url 2>/dev/null | jq '. | map(.content)' \
| tr -d -d "\n" | tr -d -d "<p>" | tr -d -d "</p>" \
| tr -d -d "[" | tr -d -d "]" | tr -d -d ", " | tr -d -d "\"" | tr -d -d "br" \
> $tmp
