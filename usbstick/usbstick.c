/*
 * usbstick driver
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
 * implement a generic usbstick driver
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/usb.h>

#define DEVNAME    "usbstick"

/*
 * change both the IDVENDOR and IDPRODUCT to the matching ones from your
 * hardware, otherwise this driver will not work
 */
#define IDVENDOR   0x1234
#define IDPRODUCT  0x1234

static int usbstick_probe(struct usb_interface *iface, const struct usb_device_id *id)
{
	pr_info("usbstick ID %04X:%04X detected\n", id->idVendor, id->idProduct);
	return 0;
}

static void usbstick_disconnect(struct usb_interface *iface)
{
	pr_info("usbstick removed\n");
}

static struct usb_device_id usbstick_table[] =
{
	{ USB_DEVICE(IDVENDOR, IDPRODUCT) },
	{} /* Terminating entry */
};
MODULE_DEVICE_TABLE (usb, usbstick_table);

static struct usb_driver usbstick_driver = {
	.name =       DENAME,
	.id_table =   usbstick_table,
	.probe =      usbstick_probe,
	.disconnect = usbstick_disconnect,
};


static int __init usbstick_init(void)
{
	return usb_register(&usbstick_driver);
}

static void __exit usbstick_exit(void)
{
	usb_deregister(&usbstick_driver);
}

module_init(usbstick_init);
module_exit(usbstick_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rafael do Nascimento Pereira <rnp@25ghz.net>");
MODULE_DESCRIPTION("usbstick driver");

