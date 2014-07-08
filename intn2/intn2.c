/*
 * /dev/intn2 driver
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
 * The intn2 device driver extends the previous one, the intn, by allowing
 * the user to pass an integer as an parameter, for the internal counter.
 * Otherwise, this driver implements the same functionality and its
 * predecessor.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>


/* Driver infos */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rafael do Nascimento Pereira <rnp@25ghz.net>");
MODULE_DESCRIPTION("intn2 driver");
MODULE_VERSION("0.1");

#define DEVNAME "intn2"
#define CLASSNAME "dummy2"
#define NR_DEVS 1
#define INIT_VALUE 25
#define INT_LEN  12
#define BASE10  10
#define DEFAULT_INT "25"

static int intn2_major;
static int intn2_minor;
static int counter = 25;
static char cint[INT_LEN];

struct intn2_dev {
	struct cdev intn2_cdev;
	struct class *intn2_class;
	struct device *intn2_device;
	struct mutex intn2_mutex;
};

struct intn2_dev *intn2 = NULL;

module_param(counter, int, 0664);
MODULE_PARM_DESC(counter,"integer that holds the intn2 driver counter");

ssize_t intn2_read(struct file *f, char __user *u, size_t size, loff_t *l)
{
	if (u == NULL)
		return -EFAULT;

	if (snprintf(cint, INT_LEN, "%d", counter) < 0) {
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
		pr_err("Value read: %d\n", counter);
		return strlen(cint);
	}
}

ssize_t intn2_write(struct file *f, const char __user *u, size_t size, loff_t *l)
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
		pr_err("Return %lu bytes from userspace\n",
				strlen(ctmp));
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

	counter = (int)long_tmp;
	pr_err("Value stored: %d\n", counter);

	return 0;
}

int intn2_release(struct inode *inode, struct file *filp)
{
	mutex_unlock(&intn2->intn2_mutex);
	return 0;
}


int intn2_open(struct inode *inode, struct file *filp)
{
	mutex_lock(&intn2->intn2_mutex);
	return 0;
}

/* define the driver file operations. These are function pointers used by
 * kernel when a call from user space is perfomed
 */
static const struct file_operations intn2_fops = {
	.owner   = THIS_MODULE,
	.read    = intn2_read,
	.write   = intn2_write,
	.open    = intn2_open,
	.release = intn2_release,
};

void intn2_cleanup(void)
{
	dev_t dev;

	dev = MKDEV(intn2_major, intn2_minor);

	if (&intn2->intn2_cdev)
		cdev_del(&intn2->intn2_cdev);

	if (intn2->intn2_device)
		device_destroy(intn2->intn2_class, dev);

	if (intn2->intn2_class)
		class_destroy(intn2->intn2_class);

	kfree(intn2);
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
static int __init intn2_init(void)
{ int ret = 0;
	int err = 0;
	int devno = 0;
	dev_t dev = 0;

	intn2 = kzalloc(sizeof(struct intn2_dev), GFP_KERNEL);
	if (!intn2) {
		pr_err("Error allocating memory\n");
		ret = -ENOMEM;
		goto fail;
	}

	/* allocates a major and minor dynamically */
	ret = alloc_chrdev_region(&dev, intn2_minor, NR_DEVS, DEVNAME);
	intn2_major = MAJOR(dev);
	if (ret < 0) {
		pr_err("can't get major %d\n", intn2_major);
		ret = -ENOMEM;
		goto fail;
	}

	pr_info("device: <Major, Minor>: <%d, %d>\n", MAJOR(dev), MINOR(dev));

	/* creates the device class under /sys */
	intn2->intn2_class = class_create(THIS_MODULE, CLASSNAME);
	if (!intn2->intn2_class) {
		pr_err(pr_fmt("Error creating device class %s\n"), DEVNAME);
		ret = -ENOMEM;
		goto fail;
	}

	/* creates the device class under /dev */
	intn2->intn2_device = device_create(intn2->intn2_class,
			NULL, dev, NULL, DEVNAME);
	if (!intn2->intn2_device) {
		pr_err("Error creating device %s\n",
				DEVNAME);
		ret = -ENOMEM;
		goto fail;
	}

	/* Mutex must be initialized  before the device is allocated */
	mutex_init(&intn2->intn2_mutex);

	/* char device registration */
	devno = MKDEV(intn2_major, intn2_minor);
	cdev_init(&intn2->intn2_cdev, &intn2_fops);
	intn2->intn2_cdev.owner = THIS_MODULE;
	intn2->intn2_cdev.ops = &intn2_fops;
	err = cdev_add(&intn2->intn2_cdev, devno, NR_DEVS);

	if (err) {
		pr_err("Error %d adding /dev/intn2\n", err);
		ret = err;
		goto fail;
	}

	memset(cint, 0, INT_LEN);
	return 0;
fail:
	intn2_cleanup();
	return ret;
}


/* dealocate the drivers data and unload it from kernel space memory */
static void __exit intn2_exit(void)
{
	intn2_cleanup();
	pr_info("exiting\n");
}

/* Declare the driver constructor/destructor */
module_init(intn2_init);
module_exit(intn2_exit);
