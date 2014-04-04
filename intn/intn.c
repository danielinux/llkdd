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
 * implements concurrency management. Every time a process reads/writes from/to
 * it, a mutex is set, so a value can be read/written. 
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/string.h>
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
#define INIT_VALUE 25
#define INT_LEN  12
#define BASE10  10
#define DEFAULT_INT "25\0"

static int intn_major = 0;
static int intn_minor = 0;
static int int_value = 0;
static char cint[INT_LEN];

ssize_t intn_read(struct file *f, char __user *u, size_t size, loff_t *l);

int intn_open(struct inode *inode, struct file *filp);

ssize_t intn_write(struct file *f, const char __user *u, size_t size, loff_t *l);

int intn_release(struct inode *inode, struct file *filp);

static int __init intn_init(void);

static void __exit intn_exit(void);

struct intn_dev {
	struct cdev intn_cdev;
	struct class *intn_class;
	struct device *intn_device;
	struct mutex intn_mutex;
};

/* define the driver file operations. These are function pointers used by
 * kernel when a call from user space is perfomed 
 */
static struct file_operations intn_fops = {
	.owner   = THIS_MODULE,
	.read    = intn_read,
	.write   = intn_write,
	.open    = intn_open,
	.release = intn_release,
};

struct intn_dev *intn = NULL;


/* The read() file opetation, it returns only a "1" character to user space */
ssize_t intn_read(struct file *f, char __user *u, size_t size, loff_t *l)
{
	if (u == NULL)
		return -EFAULT;

	if (snprintf(cint, INT_LEN, "%d", int_value) < 0) {
		printk(KERN_ERR "[intn] Conversion not possible, returning default value");
		if (!strncpy(cint, DEFAULT_INT, 3))
			return -EFAULT;
	}

	/* copy the buffer to user space */
	if (copy_to_user(u, cint, strlen(cint)) < 0) {
		printk(KERN_ERR "[intn] Error copying buffer to userspace");
		return -EFAULT;
	} else {
		/* we return the number of written bytes, always 4 */
		printk(KERN_ERR "[intn] Return %lu bytes to userspace", strlen(cint));
		return (strlen(cint));
	}
}

ssize_t intn_write(struct file *f, const char __user *u, size_t size, loff_t *l)
{
	long ltmp;
	int ret;
	int  tmp = int_value;

	if (u == NULL)
		return -EFAULT;

	ret = kstrtol(u, BASE10, &ltmp);

	//if (ret < 0 ) {
	//	if (ret == -EV

	/* copy the buffer to user space */

	return 0;
}

int intn_release(struct inode *inode, struct file *filp)
{
	//mutex_unlock(&intn->intn_mutex);
	return 0;
}


int intn_open(struct inode *inode, struct file *filp)
{
	//mutex_lock(&intn->intn_mutex);

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
static int __init intn_init(void)
{
	int ret = 0;
	int err = 0;
	int devno = 0;
	dev_t dev = 0;

	intn = kmalloc(sizeof(struct intn_dev), GFP_KERNEL);

	if (!intn) {
		printk(KERN_ERR "[intn] Error allocating memory");
		ret = -ENOMEM;
		goto fail;
	}

	memset(intn, 0, sizeof(struct intn_dev));

	/* allocates a major and minor dynamically */
	ret = alloc_chrdev_region(&dev, intn_minor, NR_DEVS, DEVNAME);
	intn_major = MAJOR(dev);

	if (ret < 0) {
		printk(KERN_ERR "[intn] can't get major %d\n", intn_major);
		ret = -ENOMEM;
		goto fail;
	}

	printk(KERN_INFO "[intn] %s device: <Major, Minor>: <%d, %d>\n", DEVNAME, MAJOR(dev), MINOR(dev));

	/* creates the device class under /sys */
	intn->intn_class = class_create(THIS_MODULE, CLASSNAME);

	if (!intn->intn_class) {
		printk(KERN_ERR "[intn] Error creating device class %s", DEVNAME);
		ret = -ENOMEM;
		goto fail;
	}

	/* creates the device class under /dev */
	intn->intn_device = device_create(intn->intn_class, NULL, dev, NULL, DEVNAME);

	if (!intn->intn_device) {
		printk(KERN_ERR "[intn] Error creating device %s", DEVNAME);
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
		printk(KERN_ERR "[intn] Error %d adding /dev/intn", err);
		ret = err;
		goto fail;
	}

	memset(cint, 0, INT_LEN);
	int_value = INIT_VALUE;
	//mutex_init(&intn->intn_mutex);

	return 0;
fail:
	intn_exit();
	return ret;
}


/* dealocate the drivers data and unload it from kernel space memory */
static void __exit intn_exit(void)
{
	dev_t dev;
	dev = MKDEV(intn_major, intn_minor);

	if (intn->intn_device)
		device_destroy(intn->intn_class, dev);

	if (intn->intn_class)
		class_destroy(intn->intn_class);

	cdev_del(&intn->intn_cdev);
	unregister_chrdev_region(dev, NR_DEVS);
	kfree(intn);
	printk(KERN_ERR "[%s] exiting", DEVNAME);
}

/* Declare the driver constructor/destructor */
module_init(intn_init);
module_exit(intn_exit);
