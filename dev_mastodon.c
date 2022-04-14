#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("MIT");
MODULE_DESCRIPTION("toot from /dev/mastodon");

#define DRIVER_NAME "DEV_MASTODON"
#define MAX_TOOT_LENGTH 500

static int dev_mastodon_init(void) {
  printk(KERN_INFO "Starting dev_mastodon !\n");
  return 0;
}

static void dev_mastodon_exit(void) {
  printk(KERN_INFO "Starting dev_mastodon !\n");
}

module_init(dev_mastodon_init);
module_exit(dev_mastodon_exit);
