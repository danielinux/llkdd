/*
 * intn_sysfs driver
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
 * Implements sysfs attributes to communicate with userspace
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/sysfs.h>


#define DEVNAME    "intn_sysfs"

static struct platform_device *intn_sysfs_dev;

static int intn_sysfs_init(void)
{
	int ret;

	pr_alert("loading %s\n", DEVNAME);
	intn_sysfs_dev = platform_device_register_simple("intn_sysfs", -1, NULL, 0);
	ret =PTR_ERR_OR_ZERO(intn_sysfs_dev);
	if (ret)
		goto err;

	return 0;

err:
	pr_err("Error registering platform device: %d\n",ret);
	return ret;
}

static void intn_sysfs_exit(void)
{
	pr_alert("unloading %s\n", DEVNAME);
	platform_device_unregister(intn_sysfs_dev);
}

module_init(intn_sysfs_init);
module_exit(intn_sysfs_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rafael do Nascimento Pereira");
MODULE_DESCRIPTION("Implements sysfs attributes to communicate with userspace");

