obj-m := dev_mastodon.o

RM := rm
CP := cp
CHMOD := chmod
MKNOD := mknod
INSMOD := insmod
RMMOD := rmmod

default:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules

clean:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean

load:
	$(INSMOD) dev_mastodon.ko
	$(MKNOD) /dev/mastodon c 62 1
	$(CHMOD) 222 /dev/mastodon
	$(CP) dev_mastodon.conf /etc/dev_mastodon.conf
	$(CHMOD) 660 /etc/dev_mastodon.conf
	$(CP) toot.sh /usr/local/bin
	$(CHMOD) +x /usr/local/bin/toot.sh

unload:
	$(RMMOD) dev_mastodon.ko
	$(RM) -f /dev/mastodon
	$(RM) -f /etc/dev_mastodon.conf
	$(RM) -f /usr/local/toot.sh
