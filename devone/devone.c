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


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rafael do Nascimento Pereira");
MODULE_DESCRIPTION("devone driver");
MODULE_VERSION("0.1");

#define NR_DEVS 1


static int devone_major = 0;
static int devone_minor = 0;


ssize_t devone_read(struct file *f, char __user *u, size_t size, loff_t *l);

int devone_open(struct inode *inode, struct file *filp);


struct devone_dev {
	struct cdev devone_cdev;
};

static struct file_operations devone_fops = {
	.owner   = THIS_MODULE,
	.read    = devone_read,
	.open    = devone_open
};

struct devone_dev *devone;


ssize_t devone_read(struct file *f, char __user *u, size_t size, loff_t *l)
{

	return 0;
}

void devone_release(void)
{

}


int devone_open(struct inode *inode, struct file *filp)
{

	return 0;
}

static void __exit devone_exit(void)
{
	dev_t dev;
	dev = MKDEV(devone_major, devone_minor);

	cdev_del(&devone->devone_cdev);
	kfree(devone);
	unregister_chrdev_region(dev, NR_DEVS);
}

static int __init devone_init(void)
{
	int ret = 0;
	int err = 0;
	int devno;
	dev_t dev = 0;

	ret = alloc_chrdev_region(&dev, devone_minor, NR_DEVS, "devone");
	devone_major = MAJOR(dev);

	if (ret < 0) {
		printk(KERN_WARNING "devone: can't get major %d\n", devone_major);
		return ret;
	}

	devone = kmalloc(sizeof(struct devone_dev), GFP_KERNEL);

	if (!devone) {
		ret = -ENOMEM;
		goto fail;
	}

	memset(devone, 0, sizeof(struct devone_dev));

	devno = MKDEV(devone_major, devone_minor);
	cdev_init(&devone->devone_cdev, &devone_fops);
	devone->devone_cdev.owner = THIS_MODULE;
	devone->devone_cdev.ops = &devone_fops;
	err = cdev_add(&devone->devone_cdev, devno, NR_DEVS);

	if (err)
		printk(KERN_NOTICE "Error %d adding /dev/one", err);
	return 0;
fail:
	devone_exit();
	return ret;
}


module_init(devone_init);
module_exit(devone_exit);
