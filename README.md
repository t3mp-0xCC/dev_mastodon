# /dev/mastodon
mastodon as device

## Build & Install
0. Depends
* build-essential
* kernel-header
* curl

1. Edit `dev_mastodon.conf`
```
INSTANCE="example.com"
VIS="public"# or unlisted
TOKEN="PATSE YOUR ACCESS TOKEN HERE !"
```

2. Build & Install
```
$ make
$ sudo make load
```

## Uninstall
```
$ sudo make unload
```
