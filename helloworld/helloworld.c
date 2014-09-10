#include <linux/init.h>
#include <linux/module.h>

MODULE_LICENSE("GPL");

/*
 * This "device driver" is quite simple, it just sends a message using
 * the printk function (see include/linux/kernel.h) when it is load and
 * unloaded in the kernel space memory. You can see its output by typing
 * as the root user:
 *
 * syslog systems:  tail -f /var/log/syslog
 *
 * systemd systems: journalctl -f
 */
static int hello_init(void)
{
	pr_alert("Hello, world\n");
	return 0;
}

static void hello_exit(void)
{
	pr_alert("Goodbye, cruel world\n");
}

/*
 * register kernel entry points for the device driver (aka constructor and
 * destructor)
 */
module_init(hello_init);
module_exit(hello_exit);
