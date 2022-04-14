#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <asm/current.h>
#include <asm/uaccess.h>
#include <linux/kmod.h>

MODULE_LICENSE("MIT");
MODULE_DESCRIPTION("toot from /dev/mastodon");

#define DRIVER_NAME "DEV_MASTODON"
#define MAX_TOOT_LENGTH 500
#define TOOT_BUFFER_SIZE MAX_TOOT_LENGTH * 6
#define SCRIPT_PATH "/usr/local/bin/toot.sh"

struct toot_buffer {
  // buffer for the toot text
  char buffer[TOOT_BUFFER_SIZE + 1/* for NULL */];
  // pseudo-pointer to char in buffer
  unsigned int pointer;
};

static int toot(char *text) {
  // exec toot script when fops.release
  char *argv[] = {SCRIPT_PATH, text, NULL};
  char *envp[] = {"HOME=/", "TERM=linux",
                  "PATH=/sbin:/usr/sbin:/bin:/usr/bin:/usr/local/bin", NULL};
  if (call_usermodehelper(argv[0], argv, envp, UMH_WAIT_PROC)) {
    printk(KERN_WARNING "Cannot toot text\n");
    return -1;
  }
  return 0;
}

static int open(struct inode *inode, struct file *file) {
                struct toot_buffer *toot_buf;

  toot_buf = kmalloc(sizeof(struct toot_buffer), GFP_KERNEL);
  if (toot_buf == NULL) {
    printk(KERN_WARNING "Cannot alloc memory\n");
    return -ENOMEM;
  }
  toot_buf->pointer = 0;
  file->private_data = toot_buf;
  return 0;
}

static int release(struct inode *inode, struct file *file) {
  struct toot_buffer *toot_buf;

  toot_buf = file->private_data;
  if(toot_buf) {
    if (toot_buf->pointer > 0) {
      toot_buf->buffer[toot_buf->pointer] = '\0';
      toot(toot_buf->buffer);
    }
    kfree(toot_buf);
    file->private_data = NULL;
  }
  return 0;
}

static ssize_t dummy_read(struct file *file, char __user *buf, size_t count,
                          loff_t *f_pos) {
  // /dev/mastodon is not readable device...
  return 0;
}

static ssize_t write(struct file *file, const char __user *buf, size_t count,
                     loff_t *f_pos) {
  struct toot_buffer *toot_buf;
  unsigned int read_size;
  unsigned int text_pointer, text_counter;
  char char_buf;
  unsigned int processed_count;

  toot_buf = file->private_data;
  if (toot_buf) {
    processed_count = 0;
    while (count > processed_count) {
      if (count > TOOT_BUFFER_SIZE - toot_buf->pointer) {
        read_size = TOOT_BUFFER_SIZE - toot_buf->pointer;
      } else {
        read_size = count;
      }
      // read data from user process
      if (raw_copy_from_user(toot_buf->buffer + toot_buf->pointer,
                             buf + processed_count, read_size) != 0) {
      printk(KERN_WARNING "Cannot copy data from buffer\n");
      return -EFAULT;
      }
      toot_buf->buffer[toot_buf->pointer + read_size] = '\0';
      for (text_counter = 0, text_pointer = 0;
           text_pointer < toot_buf->pointer + read_size;
           text_pointer++) {
        if(!((unsigned char)toot_buf->buffer[text_pointer] >= 0x80 &&
           0xBF >= (unsigned)toot_buf->buffer[text_pointer])) {
          text_counter++;
          if(text_counter > MAX_TOOT_LENGTH) {
            char_buf = toot_buf->buffer[text_pointer];
            toot_buf->buffer[text_pointer] = '\0';
            if(toot(toot_buf->buffer)) {
              return -EFAULT;
            }
            toot_buf->buffer[text_pointer] = char_buf;
            break;
          }
        }
      }
      if (text_counter == MAX_TOOT_LENGTH) {
        if (toot_buf->buffer) {
          return -EFAULT;
        }
        toot_buf->pointer = 0;
      } else if (text_counter > MAX_TOOT_LENGTH) {
        memcpy(toot_buf->buffer, toot_buf->buffer + text_pointer,
               toot_buf->pointer + read_size - text_pointer);
        toot_buf->pointer =
          toot_buf->pointer + read_size - text_pointer;
      } else {
        toot_buf->pointer += read_size;
      }
      processed_count += read_size;
    }
  } else {
    printk(KERN_WARNING "Cannot get toot buffer\n");
    return -EFAULT;
  }
  return count;
}

struct file_operations fops = {
  // define operations on /dev/mastodon
  .open = open,
  .release = release,
  .read = dummy_read,
  .write = write,
};

static int dev_mastodon_init(void) {
  // init
  printk(KERN_INFO "Starting dev_mastodon !\n");
  return 0;
}

static void dev_mastodon_exit(void) {
  // exit
  printk(KERN_INFO "Removing dev_mastodon\n");
}

module_init(dev_mastodon_init);
module_exit(dev_mastodon_exit);
