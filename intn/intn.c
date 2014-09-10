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
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define DEVNAME "intn"
#define CLASSNAME "dummy2"
#define NR_DEVS 1
#define INIT_VALUE 25
#define INT_LEN  12
#define BASE10  10
#define DEFAULT_INT "25"

static int intn_major;
static int intn_minor;
static int int_value;
static char cint[INT_LEN];

struct intn_dev {
	struct cdev intn_cdev;
	struct class *intn_class;
	struct device *intn_device;
	struct mutex intn_mutex;
};

struct intn_dev *intn;

/* The read() file opetation, it returns only a "1" character to user space */
ssize_t intn_read(struct file *f, char __user *u, size_t size, loff_t *l)
{
	if (u == NULL)
		return -EFAULT;

	if (snprintf(cint, INT_LEN, "%d", int_value) < 0) {
		pr_err("Error converting, returning default value\n");
		if (!strncpy(cint, DEFAULT_INT, 3))
			return -EFAULT;
	}

	/* copy the buffer to user space */
	if (copy_to_user(u, cint, strlen(cint)) < 0) {
		pr_err("Error copying buffer to userspace\n");
		return -EFAULT;
	} else {
		/* we return the number of written bytes, always 4 */
		pr_err("Return %lu bytes to userspace\n", strlen(cint));
		pr_err("Value read: %d\n", int_value);
		return strlen(cint);
	}
}

ssize_t intn_write(struct file *f, const char __user *u, size_t size, loff_t *l)
{
	int ret;
	long long_tmp;
	char ctmp[INT_LEN];

	if (u == NULL)
		return -EFAULT;

	memset(ctmp, 0, INT_LEN);

	/* copy the buffer from user space */
	if (copy_from_user(&ctmp, u, INT_LEN) < 0) {
		pr_err("Error copying buffer to userspace\n");
		return -EFAULT;
	} else {
		/* we return the number of written bytes, always 4 */
		pr_err("Return %lu bytes from userspace\n", strlen(ctmp));
	}

	ret = kstrtol((const char *)&ctmp, BASE10, &long_tmp);

	if (ret < 0) {
		if (ret == LONG_MAX) {
			pr_err("Overflow !\n");
			return -ERANGE;
		} else if (ret == LONG_MIN) {
			pr_err("Underflow !\n");
			return -ERANGE;
		} else {
			pr_err("Parsing error (invalid input)\n");
			return -EINVAL;
		}
	}

	int_value = (int)long_tmp;
	pr_err("Value stored: %d\n", int_value);

	return 0;
}

int intn_release(struct inode *inode, struct file *filp)
{
	mutex_unlock(&intn->intn_mutex);
	return 0;
}


int intn_open(struct inode *inode, struct file *filp)
{
	mutex_lock(&intn->intn_mutex);
	return 0;
}

/* define the driver file operations. These are function pointers used by
 * kernel when a call from user space is perfomed
 */
static const struct file_operations intn_fops = {
	.owner   = THIS_MODULE,
	.read    = intn_read,
	.write   = intn_write,
	.open    = intn_open,
	.release = intn_release,
};

void intn_cleanup(void)
{
	dev_t dev;

	dev = MKDEV(intn_major, intn_minor);

	if (&intn->intn_cdev)
		cdev_del(&intn->intn_cdev);

	if (intn->intn_device)
		device_destroy(intn->intn_class, dev);

	if (intn->intn_class)
		class_destroy(intn->intn_class);

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
	int ret;
	int err;
	int devno;
	dev_t dev;

	intn = kzalloc(sizeof(struct intn_dev), GFP_KERNEL);

	if (!intn) {
		pr_err("Error allocating memory\n");
		ret = -ENOMEM;
		goto fail;
	}

	/* allocates a major and minor dynamically */
	ret = alloc_chrdev_region(&dev, intn_minor, NR_DEVS, DEVNAME);
	intn_major = MAJOR(dev);

	if (ret < 0) {
		pr_err("can't get major %d\n", intn_major);
		ret = -ENOMEM;
		goto fail;
	}

	pr_info("device: <Major, Minor>: <%d, %d>\n",
			MAJOR(dev), MINOR(dev));

	/* creates the device class under /sys */
	intn->intn_class = class_create(THIS_MODULE, CLASSNAME);

	if (!intn->intn_class) {
		pr_err("Error creating device class %s\n", DEVNAME);
		ret = -ENOMEM;
		goto fail;
	}

	/* creates the device class under /dev */
	intn->intn_device = device_create(intn->intn_class,
				NULL, dev, NULL, DEVNAME);

	if (!intn->intn_device) {
		pr_err("Error creating device %s\n", DEVNAME);
		ret = -ENOMEM;
		goto fail;
	}

	/* Mutex must be initialized  before the device is allocated */
	mutex_init(&intn->intn_mutex);

	/* char device registration */
	devno = MKDEV(intn_major, intn_minor);
	cdev_init(&intn->intn_cdev, &intn_fops);
	intn->intn_cdev.owner = THIS_MODULE;
	intn->intn_cdev.ops = &intn_fops;
	err = cdev_add(&intn->intn_cdev, devno, NR_DEVS);

	if (err) {
		pr_err("Error %d adding /dev/intn\n", err);
		ret = err;
		goto fail;
	}

	memset(cint, 0, INT_LEN);
	int_value = INIT_VALUE;

	return 0;
fail:
	intn_cleanup();
	return ret;
}

/* dealocate the drivers data and unload it from kernel space memory */
static void __exit intn_exit(void)
{
	intn_cleanup();
	pr_err("exiting\n");
}

module_init(intn_init);
module_exit(intn_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rafael do Nascimento Pereira <rnp@25ghz.net>");
MODULE_DESCRIPTION("intn driver");
MODULE_VERSION("0.1");

