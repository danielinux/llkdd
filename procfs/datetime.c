#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/jiffies.h>
#include <linux/time.h>
#include <linux/rtc.h>


static int datetime_proc_show(struct seq_file *m, void *v)
{
	struct timeval time;
	unsigned long local_time;
	struct rtc_time tm;

	do_gettimeofday(&time);
	local_time = (u32)(time.tv_sec - (sys_tz.tz_minuteswest * 60));
	rtc_time_to_tm(local_time, &tm);

	seq_printf(m, "%04d.%02d.%02d %02d:%02d:%02d\n",
		tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec);

	return 0;
}

static int datetime_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, datetime_proc_show, NULL);
}

static const struct file_operations datetime_proc_fops = {
	.owner      = THIS_MODULE,
	.open       = datetime_proc_open,
	.read       = seq_read,
	.llseek     = seq_lseek,
	.release    = single_release,
};

static int __init datetime_proc_init(void)
{
	proc_create("datetime", 0, NULL, &datetime_proc_fops);
	return 0;
}

static void __exit datetime_proc_exit(void)
{
	remove_proc_entry("datetime", NULL);
}

module_init(datetime_proc_init);
module_exit(datetime_proc_exit);

MODULE_AUTHOR("Rafael do Nascimento Pereira <rnp@25ghz.net>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("store the current date and time in a file under /proc.");
