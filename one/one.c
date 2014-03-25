/*
 * /dev/one driver
 *
 * Copyright (C) 2014 Rafael do Nascimento Pereira
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


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rafael do Nascimento Pereira");
MODULE_DESCRIPTION("one driver");
MODULE_VERSION("0.1");

#define DEVNAME "one"
#define CLASSNAME "dummy"
#define NR_DEVS 1


static int one_major = 0;
static int one_minor = 0;

struct class *one_class = NULL;

struct device *one_device = NULL;


ssize_t one_read(struct file *f, char __user *u, size_t size, loff_t *l);

int one_open(struct inode *inode, struct file *filp);

int one_release(struct inode *inode, struct file *filp);

struct one_dev {
	struct cdev one_cdev;
};

static struct file_operations one_fops = {
	.owner   = THIS_MODULE,
	.read    = one_read,
	.open    = one_open,
	.release = one_release,
};

struct one_dev *one = NULL;


ssize_t one_read(struct file *f, char __user *u, size_t size, loff_t *l)
{

	return 0;
}


int one_release(struct inode *inode, struct file *filp)
{
	return 0;
}


int one_open(struct inode *inode, struct file *filp)
{

	return 0;
}

static void __exit one_exit(void)
{
	dev_t dev;
	dev = MKDEV(one_major, one_minor);

	if (one_device)
		device_destroy(one_class, dev);

	if (one_class)
		class_destroy(one_class);

	cdev_del(&one->one_cdev);
	kfree(one);
	unregister_chrdev_region(dev, NR_DEVS);
}

/*
 * struct device *device_create(struct class *cls, struct device *parent,
			     dev_t devt, void *drvdata,
			     const char *fmt, ...);
 */
static int __init one_init(void)
{
	int ret = 0;
	int err = 0;
	int devno;
	dev_t dev = 0;

	ret = alloc_chrdev_region(&dev, one_minor, NR_DEVS, DEVNAME);
	one_major = MAJOR(dev);

	if (ret < 0) {
		printk(KERN_WARNING "one: can't get major %d\n", one_major);
		return ret;
	}

	printk(KERN_INFO "<Major, Minor>: <%d, %d>\n", MAJOR(dev), MINOR(dev));
	one = kmalloc(sizeof(struct one_dev), GFP_KERNEL);

	if (!one) {
		ret = -ENOMEM;
		goto fail;
	}

	memset(one, 0, sizeof(struct one_dev));

	one_class = class_create(THIS_MODULE, CLASSNAME);

	if (!one_class) {
		printk(KERN_ERR " Error creating device class %s", DEVNAME);
		ret = -ENOMEM;
		goto fail;
	}

	one_device = device_create(one_class, NULL, dev, NULL, DEVNAME);

	if (!one_device) {
		printk(KERN_ERR " Error creating device %s", DEVNAME);
		ret = -ENOMEM;
		goto fail;
	}

	devno = MKDEV(one_major, one_minor);
	cdev_init(&one->one_cdev, &one_fops);
	one->one_cdev.owner = THIS_MODULE;
	one->one_cdev.ops = &one_fops;
	err = cdev_add(&one->one_cdev, devno, NR_DEVS);

	if (err) {
		printk(KERN_NOTICE "Error %d adding /dev/one", err);
		goto fail;
	}


	return 0;
fail:
	one_exit();
	return ret;
}


module_init(one_init);
module_exit(one_exit);
