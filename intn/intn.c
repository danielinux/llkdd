/*
 * /dev/intn driver
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
 * Dummy character device driver an extended version of /dev/one. It
 * implements concurrency management. Every time a process reads from is, the
 * value return is decremented (til it reaches INT_MIN). The same way it is
 * incremented every time a process writes to it (til it reachs INT_MAX)
 * found on /dev/zero, except the read output is ones. At every operation the
 * current value at /dev/intn is returned.
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
MODULE_DESCRIPTION("intn driver");
MODULE_VERSION("0.1");

#define DEVNAME "intn"
#define CLASSNAME "dummy"
#define NR_DEVS 1


static int intn_major = 0;
static int intn_minor = 0;

struct class *intn_class = NULL;
struct device *intn_device = NULL;

char c = 0;

ssize_t intn_read(struct file *f, char __user *u, size_t size, loff_t *l);

int intn_open(struct inode *inode, struct file *filp);

int intn_release(struct inode *inode, struct file *filp);

struct intn_dev {
	struct cdev intn_cdev;
};

/* define the driver file operations. These are function pointers used by
 * kernel when a call from user space is perfomed 
 */
static struct file_operations intn_fops = {
	.owner   = THIS_MODULE,
	.read    = intn_read,
	.open    = intn_open,
	.release = intn_release,
};

struct intn_dev *intn = NULL;


/* The read() file opetation, it returns only a "1" character to user space */
ssize_t intn_read(struct file *f, char __user *u, size_t size, loff_t *l)
{
	c = 1;

	/* copy the buffer to user space */
	if (copy_to_user(u, &c, 1) != 0)
		return -EFAULT;
	else
		return 1;
}


int intn_release(struct inode *inode, struct file *filp)
{
	return 0;
}


int intn_open(struct inode *inode, struct file *filp)
{
	return 0;
}

/* dealocate the drivers data and unload it from kernel space memory */
static void __exit intn_exit(void)
{
	dev_t dev;
	dev = MKDEV(intn_major, intn_minor);

	if (intn_device)
		device_destroy(intn_class, dev);

	if (intn_class)
		class_destroy(intn_class);

	cdev_del(&intn->intn_cdev);
	kfree(intn);
	unregister_chrdev_region(dev, NR_DEVS);
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
static int __init intn_init(void)
{
	int ret = 0;
	int err = 0;
	int devno;
	dev_t dev = 0;

	/* allocates a major and minor dynamically */
	ret = alloc_chrdev_region(&dev, intn_minor, NR_DEVS, DEVNAME);
	intn_major = MAJOR(dev);

	if (ret < 0) {
		printk(KERN_WARNING "intn: can't get major %d\n", intn_major);
		return ret;
	}

	printk(KERN_INFO "%s<Major, Minor>: <%d, %d>\n", DEVNAME, MAJOR(dev), MINOR(dev));
	intn = kmalloc(sizeof(struct intn_dev), GFP_KERNEL);

	if (!intn) {
		ret = -ENOMEM;
		goto fail;
	}

	memset(intn, 0, sizeof(struct intn_dev));

	/* creates the device class under /sys */
	intn_class = class_create(THIS_MODULE, CLASSNAME);

	if (!intn_class) {
		printk(KERN_ERR " Error creating device class %s", DEVNAME);
		ret = -ENOMEM;
		goto fail;
	}

	/* creates the device class under /dev */
	intn_device = device_create(intn_class, NULL, dev, NULL, DEVNAME);

	if (!intn_device) {
		printk(KERN_ERR " Error creating device %s", DEVNAME);
		ret = -ENOMEM;
		goto fail;
	}

	/* char device registration */
	devno = MKDEV(intn_major, intn_minor);
	cdev_init(&intn->intn_cdev, &intn_fops);
	intn->intn_cdev.owner = THIS_MODULE;
	intn->intn_cdev.ops = &intn_fops;
	err = cdev_add(&intn->intn_cdev, devno, NR_DEVS);

	if (err) {
		printk(KERN_NOTICE "Error %d adding /dev/intn", err);
		ret = err;
		goto fail;
	}

	return 0;
fail:
	intn_exit();
	return ret;
}

/* Declare the driver constructor/destructor */
module_init(intn_init);
module_exit(intn_exit);
