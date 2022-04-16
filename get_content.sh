#!/bin/sh

# load config
source /etc/dev_mastodon.conf
#INSTANCE="example.com"

url="https://$INSTANCE/api/v1/timelines/public"

curl $url?local=something| jq '. | map(.content)' \
| tr -d -d "\n" | tr -d -d "<p>" | tr -d -d "</p>" \
| tr -d -d "[" | tr -d -d "]" | tr -d -d ", " | tr -d -d "\"" | tr -d -d "br"
