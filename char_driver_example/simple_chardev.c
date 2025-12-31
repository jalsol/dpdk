/*
 * simple_chardev.c - A simple character device driver
 * 
 * This driver creates a character device that allows reading and writing
 * to a simple kernel buffer.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>

#define DEVICE_NAME "simple_chardev"
#define CLASS_NAME "simple"
#define BUFFER_SIZE 256

MODULE_LICENSE("GPL");
MODULE_AUTHOR("truongnguyen");
MODULE_DESCRIPTION("A simple character device driver");
MODULE_VERSION("1.0");

static int major_number;
static char device_buffer[BUFFER_SIZE];
static size_t buffer_size = 0;
static struct class *chardev_class = NULL;
static struct device *chardev_device = NULL;

/* Function prototypes */
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char __user *, size_t, loff_t *);

/* File operations structure */
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = device_open,
    .read = device_read,
    .write = device_write,
    .release = device_release,
};

/*
 * Called when a process opens the device file
 */
static int device_open(struct inode *inode, struct file *file) {
    printk(KERN_INFO "simple_chardev: Device opened\n");
    return 0;
}

/*
 * Called when a process closes the device file
 */
static int device_release(struct inode *inode, struct file *file) {
    printk(KERN_INFO "simple_chardev: Device closed\n");
    return 0;
}

/*
 * Called when a process reads from the device
 */
static ssize_t device_read(struct file *filp, char __user *buffer,
                          size_t len, loff_t *offset) {
    size_t bytes_to_read;
    
    if (*offset >= buffer_size)
        return 0;  // EOF
    
    bytes_to_read = min(len, buffer_size - (size_t)*offset);
    
    if (copy_to_user(buffer, device_buffer + *offset, bytes_to_read)) {
        return -EFAULT;
    }
    
    *offset += bytes_to_read;
    printk(KERN_INFO "simple_chardev: Read %zu bytes\n", bytes_to_read);
    
    return bytes_to_read;
}

/*
 * Called when a process writes to the device
 */
static ssize_t device_write(struct file *filp, const char __user *buffer,
                           size_t len, loff_t *offset) {
    size_t bytes_to_write;
    
    bytes_to_write = min(len, (size_t)(BUFFER_SIZE - 1));
    
    if (copy_from_user(device_buffer, buffer, bytes_to_write)) {
        return -EFAULT;
    }
    
    device_buffer[bytes_to_write] = '\0';
    buffer_size = bytes_to_write;
    
    printk(KERN_INFO "simple_chardev: Wrote %zu bytes\n", bytes_to_write);
    
    return bytes_to_write;
}

/*
 * Module initialization
 */
static int __init chardev_init(void) {
    printk(KERN_INFO "simple_chardev: Initializing\n");
    
    /* Register character device */
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        printk(KERN_ALERT "simple_chardev: Failed to register major number\n");
        return major_number;
    }
    printk(KERN_INFO "simple_chardev: Registered with major number %d\n", major_number);
    
    /* Create device class */
    chardev_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(chardev_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "simple_chardev: Failed to create class\n");
        return PTR_ERR(chardev_class);
    }
    printk(KERN_INFO "simple_chardev: Device class created\n");
    
    /* Create device */
    chardev_device = device_create(chardev_class, NULL, MKDEV(major_number, 0),
                                   NULL, DEVICE_NAME);
    if (IS_ERR(chardev_device)) {
        class_destroy(chardev_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "simple_chardev: Failed to create device\n");
        return PTR_ERR(chardev_device);
    }
    
    printk(KERN_INFO "simple_chardev: Device created successfully\n");
    printk(KERN_INFO "simple_chardev: Use /dev/simple_chardev to access\n");
    
    return 0;
}

/*
 * Module cleanup
 */
static void __exit chardev_exit(void) {
    device_destroy(chardev_class, MKDEV(major_number, 0));
    class_destroy(chardev_class);
    unregister_chrdev(major_number, DEVICE_NAME);
    printk(KERN_INFO "simple_chardev: Module unloaded\n");
}

module_init(chardev_init);
module_exit(chardev_exit);

