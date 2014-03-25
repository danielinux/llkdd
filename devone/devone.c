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
MODULE_DESCRIPTION("devone driver");
MODULE_VERSION("0.1");

#define DEVNAME "one"
#define NR_DEVS 1


static int devone_major = 0;
static int devone_minor = 0;

struct class *devone_class = NULL;

struct device *devone_device = NULL;


ssize_t devone_read(struct file *f, char __user *u, size_t size, loff_t *l);

int devone_open(struct inode *inode, struct file *filp);

int devone_release(struct inode *inode, struct file *filp);

struct devone_dev {
	struct cdev devone_cdev;
};

static struct file_operations devone_fops = {
	.owner   = THIS_MODULE,
	.read    = devone_read,
	.open    = devone_open,
	.release = devone_release,
};

struct devone_dev *devone = NULL;


ssize_t devone_read(struct file *f, char __user *u, size_t size, loff_t *l)
{

	return 0;
}


int devone_release(struct inode *inode, struct file *filp)
{
	return 0;
}


int devone_open(struct inode *inode, struct file *filp)
{

	return 0;
}

static void __exit devone_exit(void)
{
	dev_t dev;
	dev = MKDEV(devone_major, devone_minor);

	if (devone_device)
		device_destroy(devone_class, dev);

	if (devone_class)
		class_destroy(devone_class);

	cdev_del(&devone->devone_cdev);
	kfree(devone);
	unregister_chrdev_region(dev, NR_DEVS);
}

/*
 * struct device *device_create(struct class *cls, struct device *parent,
			     dev_t devt, void *drvdata,
			     const char *fmt, ...);
 */
static int __init devone_init(void)
{
	int ret = 0;
	int err = 0;
	int devno;
	dev_t dev = 0;

	ret = alloc_chrdev_region(&dev, devone_minor, NR_DEVS, "one");
	devone_major = MAJOR(dev);

	if (ret < 0) {
		printk(KERN_WARNING "devone: can't get major %d\n", devone_major);
		return ret;
	}

	printk(KERN_INFO "<Major, Minor>: <%d, %d>\n", MAJOR(dev), MINOR(dev));
	devone = kmalloc(sizeof(struct devone_dev), GFP_KERNEL);

	if (!devone) {
		ret = -ENOMEM;
		goto fail;
	}

	memset(devone, 0, sizeof(struct devone_dev));

	devone_class = class_create(THIS_MODULE, DEVNAME);

	if (!devone_class) {
		printk(KERN_ERR " Error creating device class %s", DEVNAME);
		ret = -ENOMEM;
		goto fail;
	}

	devone_device = device_create(devone_class, NULL, dev, NULL, DEVNAME);

	if (!devone_device) {
		printk(KERN_ERR " Error creating device %s", DEVNAME);
		ret = -ENOMEM;
		goto fail;
	}

	devno = MKDEV(devone_major, devone_minor);
	cdev_init(&devone->devone_cdev, &devone_fops);
	devone->devone_cdev.owner = THIS_MODULE;
	devone->devone_cdev.ops = &devone_fops;
	err = cdev_add(&devone->devone_cdev, devno, NR_DEVS);

	if (err) {
		printk(KERN_NOTICE "Error %d adding /dev/one", err);
		goto fail;
	}


	return 0;
fail:
	devone_exit();
	return ret;
}


module_init(devone_init);
module_exit(devone_exit);
