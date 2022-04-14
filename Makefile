obj-m := dev_mastodon.o

INSMOD := insmod
RMMOD := rmmod

default:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules

clean:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean

load:
	$(INSMOD) dev_mastodon.ko

unload:
	$(RMMOD) dev_mastodon.ko
