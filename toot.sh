#!/bin/sh

# load config
source /etc/dev_mastodon.conf
#INSTANCE="example.com"
#VIS="public" # or unlisted
#TOKEN="PATSE YOUR ACCESS TOKEN HERE !"

toot_text="$1"
header="Authorization: Bearer $TOKEN"
url="https://$INSTANCE/api/v1/statuses"

curl --header "$header" -sS $url -X POST \
-d "status=$toot_text&visibility="$VIS"" \
-o /dev/null
