/*
 * /dev/one driver
 *
 * Copyright (C) 2014 Rafael do Nascimento Pereira
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/fs.h> /* everything... */
#include <linux/errno.h> /* error codes */
#include <linux/types.h> /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h> /* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rafael do Nascimento Pereira");
MODULE_DESCRIPTION("dev one driver");
MODULE_VERSION("0.1");

#define NR_DEVS 1


int devone_major = 0;
int devone_minor = 0;


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

	return ret;
}

static void __exit devone_exit(void)
{

}

module_init(devone_init);
module_exit(devone_exit);
