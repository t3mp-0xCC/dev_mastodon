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
MODULE_DESCRIPTION("mastodon as device");

#define DRIVER_NAME "DEV_MASTODON"
#define DRIVER_MAJOR 62
#define MAX_TOOT_LENGTH 500
#define TOOT_BUFFER_SIZE MAX_TOOT_LENGTH * 6
#define MAX_CONTENT 40
#define TOOT_SCRIPT_PATH "/usr/local/bin/toot.sh"
#define CONTENT_SCRIPT_PATH "/usr/local/bin/get_content.sh"

struct content_buffer {
  // buffer for the toot text
  char buffer[TOOT_BUFFER_SIZE * MAX_CONTENT + 1];
  // pseudo-pointer to char in buffer
  unsigned int pointer;
};

static int toot(char *text) {
  // exec toot script when fops.release
  char *argv[] = {TOOT_SCRIPT_PATH, text, NULL};
  char *envp[] = {"HOME=/", "TERM=linux",
                  "PATH=/sbin:/usr/sbin:/bin:/usr/bin:/usr/local/bin", NULL};
  if (call_usermodehelper(argv[0], argv, envp, UMH_WAIT_PROC)) {
    printk(KERN_WARNING "%s: Failed to toot text\n", DRIVER_NAME);
    return -1;
  }
  return 0;
}

static int open(struct inode *inode, struct file *file) {
  struct content_buffer *content_buf;

  //printk(KERN_DEBUG "%s: open is called\n", DRIVER_NAME);
  content_buf = kmalloc(sizeof(struct content_buffer), GFP_KERNEL);
  if (content_buf == NULL) {
    printk(KERN_WARNING "%s: Failed to alloc memory\n", DRIVER_NAME);
    return -ENOMEM;
  }
  content_buf->pointer = 0;
  file->private_data = content_buf;
  return 0;
}

static int release(struct inode *inode, struct file *file) {
  struct content_buffer *content_buf;

  //printk(KERN_DEBUG "%s: release is called\n", DRIVER_NAME);
  content_buf = file->private_data;
  if(content_buf) {
    if (content_buf->pointer > 0) {
      content_buf->buffer[content_buf->pointer] = '\0';
      toot(content_buf->buffer);
    }
    kfree(content_buf);
    file->private_data = NULL;
  }
  return 0;
}

static ssize_t read(struct file *file, char __user *buf, size_t count,
                          loff_t *f_pos) {
  struct content_buffer *content_buf;
  struct file *fp;
  ssize_t read_size;
  loff_t offset = 0;
  content_buf = file->private_data;

  //printk(KERN_DEBUG "%s: read is called\n", DRIVER_NAME);
  char *argv[] = {CONTENT_SCRIPT_PATH};
  char *envp[] = {"HOME=/", "TERM=linux",
                  "PATH=/sbin:/usr/sbin:/bin:/usr/bin:/usr/local/bin", NULL};
  if (call_usermodehelper(argv[0], argv, envp, UMH_WAIT_PROC)) {
    printk(KERN_WARNING "%s: Failed to get contents\n", DRIVER_NAME);
    return -1;
  }

  fp = filp_open("/tmp/contents.txt", O_RDONLY, 0);
  if (IS_ERR(fp)) {
    printk(KERN_WARNING "%s: Failed to open contents file\n", DRIVER_NAME);
    return -1;
  }
  read_size = kernel_read(fp, content_buf, sizeof(struct content_buffer), &offset);
  if (read_size > count) {
    read_size = count;
  }
  if (copy_to_user(buf, content_buf, read_size) != 0) {
      printk(KERN_WARNING "%s: Failed to copy data from buffer\n", DRIVER_NAME);
      read_size = 0;
  }

  filp_close(fp, NULL);
  return read_size;
}

static ssize_t write(struct file *file, const char __user *buf, size_t count,
                     loff_t *f_pos) {
  struct content_buffer *toot_buf;
  unsigned int read_size;
  unsigned int text_pointer, text_counter;
  char char_buf;
  unsigned int processed_count;

  //printk(KERN_DEBUG "%s: write is called\n", DRIVER_NAME);
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
      printk(KERN_WARNING "%s: Failed to copy data from buffer\n", DRIVER_NAME);
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
    printk(KERN_WARNING "%s: Failed to get toot buffer\n", DRIVER_NAME);
    return -EFAULT;
  }
  return count;
}

struct file_operations fops = {
  // define operations on /dev/mastodon
  .open = open,
  .release = release,
  .read = read,
  .write = write,
};

static int dev_mastodon_init(void) {
  // init
  printk(KERN_INFO "%s: Starting dev_mastodon !\n", DRIVER_NAME);
  register_chrdev(DRIVER_MAJOR, DRIVER_NAME, &fops);
  return 0;
}

static void dev_mastodon_exit(void) {
  // exit
  printk(KERN_INFO "%s: Removing dev_mastodon\n", DRIVER_NAME);
  unregister_chrdev(DRIVER_MAJOR, DRIVER_NAME);
}

module_init(dev_mastodon_init);
module_exit(dev_mastodon_exit);
