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
MODULE_DESCRIPTION("dev one driver");
MODULE_VERSION("0.1");

#define NR_DEVS 1


static int devone_major = 0;
static int devone_minor = 0;

void devone_open(void)
{

}

void devone_release(void)
{

}


void devone_read(void)
{

}

static int __init devone_init(void)
{
	int ret = 0;
	dev_t dev = 0;

	ret = alloc_chrdev_region(&dev, devone_minor, NR_DEVS, "devone");
	devone_major = MAJOR(dev);

	if (ret < 0) {
		printk(KERN_WARNING "devone: can't get major %d\n", devone_major);
		return ret;
	}

	return ret;
}

static void __exit devone_exit(void)
{
	dev_t dev;
	dev = MKDEV(devone_major, devone_minor);

	unregister_chrdev_region(dev, NR_DEVS);
}

module_init(devone_init);
module_exit(devone_exit);
