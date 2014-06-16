/*
 * /dev/one driver
 *
 * Copyright (C) 2014 Rafael do Nascimento Pereira <rnp@25ghz.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Dummy character device driver that performs the same functionality
 * found on /dev/zero, except the read output is ones. This is not a
 * practical driver, it is just written for learning purposes.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/fcntl.h>
#include <asm/uaccess.h>


/* Driver infos */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rafael do Nascimento Pereira <rnp@25ghz.net>");
MODULE_DESCRIPTION("one driver");
MODULE_VERSION("0.1");

#define DEVNAME "one"
#define CLASSNAME "dummy"
#define NR_DEVS 1


static int one_major = 0;
static int one_minor = 0;

char c = 0;

ssize_t one_read(struct file *f, char __user *u, size_t size, loff_t *l);

int one_open(struct inode *inode, struct file *filp);

int one_release(struct inode *inode, struct file *filp);

struct one_dev {
	struct cdev one_cdev;
	struct device *one_device;
	struct class *one_class;
};

/* define the driver file operations. These are function pointers used by
 * kernel when a call from user space is perfomed
 */
static struct file_operations one_fops = {
	.owner   = THIS_MODULE,
	.read    = one_read,
	.open    = one_open,
	.release = one_release,
};

struct one_dev *one = NULL;


/* The read() file opetation, it returns only a "1" character to user space */
ssize_t one_read(struct file *f, char __user *u, size_t size, loff_t *l)
{
	c = 1;

	/* copy the buffer to user space */
	if (copy_to_user(u, &c, 1) != 0)
		return -EFAULT;
	else
		return 1;
}


int one_release(struct inode *inode, struct file *filp)
{
	return 0;
}


int one_open(struct inode *inode, struct file *filp)
{
	return 0;
}

/*
 * Initializes the driver and create the /dev/ and /sys/ files. For further
 * information see:
 *
 * include/cdev.h
 * include/device.h
 * include/module.h
 * include/fs.h
 */
static int __init one_init(void)
{
	int ret = 0;
	int err = 0;
	int devno;
	dev_t dev = 0;

	/* allocates a major and minor dynamically */
	ret = alloc_chrdev_region(&dev, one_minor, NR_DEVS, DEVNAME);
	one_major = MAJOR(dev);

	if (ret < 0) {
		printk(KERN_WARNING "one: can't get major %d\n", one_major);
		return ret;
	}

	printk(KERN_INFO "%s<Major, Minor>: <%d, %d>\n", DEVNAME, MAJOR(dev), MINOR(dev));
	one = kmalloc(sizeof(struct one_dev), GFP_KERNEL);

	if (!one) {
		ret = -ENOMEM;
		goto fail;
	}

	memset(one, 0, sizeof(struct one_dev));

	/* creates the device class under /sys */
	one->one_class = class_create(THIS_MODULE, CLASSNAME);

	if (!one->one_class) {
		printk(KERN_ERR " Error creating device class %s", DEVNAME);
		ret = -ENOMEM;
		goto fail;
	}

	/* creates the device class under /dev */
	one->one_device = device_create(one->one_class, NULL, dev, NULL, DEVNAME);

	if (!one->one_device) {
		printk(KERN_ERR " Error creating device %s", DEVNAME);
		ret = -ENOMEM;
		goto fail;
	}

	/* char device registration */
	devno = MKDEV(one_major, one_minor);
	cdev_init(&one->one_cdev, &one_fops);
	one->one_cdev.owner = THIS_MODULE;
	one->one_cdev.ops = &one_fops;
	err = cdev_add(&one->one_cdev, devno, NR_DEVS);

	if (err) {
		printk(KERN_NOTICE "Error %d adding /dev/one", err);
		ret = err;
		goto fail;
	}

	return 0;
fail:
	if (one->one_device)
		device_destroy(one->one_class, dev);

	if (one->one_class)
		class_destroy(one->one_class);

	cdev_del(&one->one_cdev);
	kfree(one);
	unregister_chrdev_region(dev, NR_DEVS);
	return ret;
}


/* dealocate the drivers data and unload it from kernel space memory */
static void __exit one_exit(void)
{
	dev_t dev;
	dev = MKDEV(one_major, one_minor);

	if (one->one_device)
		device_destroy(one->one_class, dev);

	if (one->one_class)
		class_destroy(one->one_class);

	cdev_del(&one->one_cdev);
	kfree(one);
	unregister_chrdev_region(dev, NR_DEVS);
	printk(KERN_INFO "Removing %s driver", DEVNAME);
}


/* Declare the driver constructor/destructor */
module_init(one_init);
module_exit(one_exit);
