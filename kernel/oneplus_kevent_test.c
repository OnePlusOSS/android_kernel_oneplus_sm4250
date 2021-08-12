// SPDX-License-Identifier: GPL-2.0-only
/* oneplus_kevent_test.c - for kevent action upload test
 *  author by wangzhenhua,Plf.Framework
 */

#include <linux/delay.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/kernel.h>

#include <linux/oneplus_kevent.h>
#include <linux/genhd.h>

#ifdef CONFIG_ONEPLUS_KEVENT_TEST
#define MAX_PAYLOAD_LENGTH  2560
static char *str;


static int kevent_proc_show(struct seq_file *m, void *v)
{
	struct kernel_packet_info *user_msg_info;
	char log_tag[32] = "2100001";
	char event_id[32] = "user_location";
	void *buffer = NULL;
	int len, size;

	seq_printf(m, "%s", str);

	len = strlen(str);

	size = sizeof(struct kernel_packet_info) + len + 1;

	buffer = kmalloc(size, GFP_ATOMIC);
	memset(buffer, 0, size);
	user_msg_info = (struct kernel_packet_info *)buffer;
	user_msg_info->type = 2;

	memcpy(user_msg_info->log_tag, log_tag, strlen(log_tag) + 1);
	memcpy(user_msg_info->event_id, event_id, strlen(event_id) + 1);

	user_msg_info->payload_length = len + 1;
	memcpy(user_msg_info->payload, str, len + 1);

	//kevent_send_to_user(user_msg_info);
	msleep(20);
	kfree(buffer);
	return 0;
}



//file_operations->open
static int kevent_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, kevent_proc_show, NULL);
}


static ssize_t kevent_proc_write(struct file *file,
		const char __user *buffer, size_t count, loff_t *f_pos)
{
	char *tmp = kzalloc((count + 1), GFP_KERNEL);
	char testbuf[32];
	int n = 0;
	struct block_device *bdev = NULL;

	if (!tmp)
		return -ENOMEM;

	if (put_user('\0', (char __user *)buffer + count - 1))
		return -EFAULT;

	if (copy_from_user(tmp, buffer, count)) {
		kfree(tmp);
		return -EFAULT;
	}

	n = strlen(tmp);
	memcpy(testbuf, tmp, n);

	pr_info("kevent_send_to_user,before %s call do_mount\n", __func__);

	do_mount("/dev/block/platform/soc/c0c4000.sdhci/by-name/system",
			buffer, "ext4", ~MS_RDONLY, NULL);

	bdev = lookup_bdev("/dev/block/platform/soc/
			c0c4000.sdhci/by-name/vendor");
	bdev = lookup_bdev("/dev/block/mmcblk0p67");
	bdev = lookup_bdev("/dev/block/bootdevice/by-name/vendor");

	pr_info("kevent_send_to_user,after s% call do_mount\n", __func__);

	str = tmp;
	return count;
}

static const struct file_operations kevent_fops = {
	.open = kevent_proc_open,
	.release = single_release,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = kevent_proc_write,
};

static int __init kevent_init(void)
{
	struct proc_dir_entry *file;

	/* create proc and file_operations */
	file = proc_create("kevent", 0664, NULL, &kevent_fops);
	if (!file)
		return -ENOMEM;

	str = kzalloc((MAX_PAYLOAD_LENGTH + 1), GFP_KERNEL);
	memset(str, 0, MAX_PAYLOAD_LENGTH);
	strlcpy(str, "kevent");

	return 0;
}

static void __exit kevent_exit(void)
{
	remove_proc_entry("kevent", NULL);
	kfree(str);
}

module_init(kevent_init);
module_exit(kevent_exit);

MODULE_LICENSE("GPL");
#endif /* CONFIG_ONEPLUS_KEVENT_TEST */

