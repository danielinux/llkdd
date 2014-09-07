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
 * Implements sysfs attributes to communicate with userspace and the functions
 * already implemented by the intn2 driver
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <asm/uaccess.h>
#include <linux/platform_device.h>
#include <linux/sysfs.h>
#include <linux/device.h>
#include <linux/ctype.h>



#define INIT_VALUE 25
#define DEVNAME    "intn_sysfs"

static int int_value;
static struct platform_device *intn_sysfs_dev;
struct mutex intn_sysfs_mutex;

ssize_t intn_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret;

	mutex_lock(&intn_sysfs_mutex);
	ret = sprintf(buf, "%d\n", int_value);
	mutex_unlock(&intn_sysfs_mutex);
	return ret;
}

ssize_t intn_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
//	int ret, tmp;

	if (!buf)
		return -ENOMEM;

//	mutex_lock(&intn_sysfs_mutex);
//	ret = sscanf(buf, "%d", &tmp);
//	pr_err("ret: %d\n", ret);
//	pr_err("tmp: %d\n", tmp);
//
//	if (ret <= 0)
//		goto err_sscanf;
//	else if (ret == 1) {
//		if (tmp >= INT_MAX) {
//			ret = -ERANGE;
//			pr_err("INT_MAX\n");
//			goto err_sscanf;
//		} else if (tmp <= INT_MIN) {
//			ret = -ERANGE;
//			pr_err("INT_MIN\n");
//			goto err_sscanf;
//		} else if (isdigit(tmp) == 0) {
//			ret = -EINVAL;
//			pr_err("not a digit\n");
//			goto err_sscanf;
//		}
//		int_value = tmp;
//	}

//	mutex_unlock(&intn_sysfs_mutex);
	return 0;

//err_sscanf:
//	mutex_unlock(&intn_sysfs_mutex);
//	return ret;
}

DEVICE_ATTR(intn, 0666, intn_show, intn_store);

static struct attribute *intn_attrs[] = {
	&dev_attr_intn.attr,
	NULL
};

static struct attribute_group intn_attr_group = {
	.attrs = intn_attrs,
};


static int intn_sysfs_init(void)
{
	int ret;

	pr_alert("loading %s\n", DEVNAME);
	intn_sysfs_dev = platform_device_register_simple("intn_sysfs", -1, NULL, 0);
	ret = PTR_ERR_OR_ZERO(intn_sysfs_dev);
	if (ret)
		goto err;

	sysfs_create_group(&intn_sysfs_dev->dev.kobj, &intn_attr_group);
	int_value = INIT_VALUE;
	mutex_init(&intn_sysfs_mutex);
	return 0;

err:
	pr_err("Error registering platform device: %d\n",ret);
	return ret;
}

static void intn_sysfs_exit(void)
{
	pr_alert("unloading %s\n", DEVNAME);
	sysfs_remove_group(&intn_sysfs_dev->dev.kobj, &intn_attr_group);
	platform_device_unregister(intn_sysfs_dev);
}

module_init(intn_sysfs_init);
module_exit(intn_sysfs_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rafael do Nascimento Pereira");
MODULE_DESCRIPTION("Implements sysfs attributes to communicate with userspace");

