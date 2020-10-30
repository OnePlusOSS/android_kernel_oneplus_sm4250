// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2016 OnePlus. All rights reserved.
 *
 * Description:
 * op_temp_cali_info.c (sw23)
 *
 * Version: 1.0
 * Date created: 13/06/2020
 * Author: Zejun.Xu@PSW.BSP.Sensor
 * --------------------------- Revision History: -----------
 *  <author>      <data>            <desc>
 *  xuzejun      13/06/2020     create the file
 */

#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/sysfs.h>
#include <linux/syscalls.h>
#include <linux/unistd.h>


struct op_press_cali_data {
	int offset;
	struct proc_dir_entry	   *proc_op_press;
};
static struct op_press_cali_data *gdata;

static ssize_t press_offset_read_proc(struct file *file, char __user *buf,
			size_t count, loff_t *off)
{
	char page[256] = {0};
	int len = 0;

	if (!gdata)
		return -ENOMEM;

	len = snprintf(page, sizeof(page), "%d", gdata->offset);

	if (len > *off)
		len -= *off;
	else
		len = 0;

	if (copy_to_user(buf, page, (len < count ? len : count)))
		return -EFAULT;

	*off += len < count ? len : count;
	return (len < count ? len : count);
}
static ssize_t press_offset_write_proc(struct file *file, const char __user *buf,
			size_t count, loff_t *off)

{
	char page[256] = {0};
	int input = 0;

	if (!gdata)
		return -ENOMEM;

	if (count > 256)
		count = 256;
	if (count > *off)
		count -= *off;
	else
		count = 0;

	if (copy_from_user(page, buf, count))
		return -EFAULT;
	*off += count;

	if (sscanf(page, "%d", &input) != 1) {
		count = -EINVAL;
		return count;
	}

	if (input != gdata->offset)
		gdata->offset = input;

	return count;
}
static const struct file_operations press_offset_fops = {
	.read = press_offset_read_proc,
	.write = press_offset_write_proc,
};

static int __init op_press_cali_data_init(void)
{
	int rc = 0;
	struct proc_dir_entry *pentry;
	struct op_press_cali_data *data = NULL;

	if (gdata) {
		pr_err("%s:just can be call one time\n", __func__);
		return 0;
	}
	data = kzalloc(sizeof(struct op_press_cali_data), GFP_KERNEL);
	if (data == NULL) {
		rc = -ENOMEM;
		pr_err("%s:kzalloc fail %d\n", __func__, rc);
		return rc;
	}
	gdata = data;
	gdata->offset = 0;

	if (gdata->proc_op_press) {
		pr_err("proc_op_press has alread inited\n");
		return 0;
	}

	gdata->proc_op_press =  proc_mkdir("OnePlusPressCali", NULL);
	if (!gdata->proc_op_press) {
		pr_err("can't create proc_op_press proc\n");
		rc = -EFAULT;
		return rc;
	}

	pentry = proc_create("offset", 0666, gdata->proc_op_press,
		&press_offset_fops);
	if (!pentry) {
		pr_err("create offset proc failed.\n");
		rc = -EFAULT;
		return rc;
	}

	return 0;
}

void op_press_cali_data_clean(void)
{
	kfree(gdata);
}
module_init(op_press_cali_data_init);
module_exit(op_press_cali_data_clean);
MODULE_DESCRIPTION("OENPLUS custom version");
MODULE_AUTHOR("xuzejun <xuzejun@huaqin.com>");
MODULE_LICENSE("GPL v2");
