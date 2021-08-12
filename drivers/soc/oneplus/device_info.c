// SPDX-License-Identifier: GPL-2.0

/**
 * Copyright 2008-2013  Mobile Comm , All rights reserved.
 * FileName:devinfo.c
 * ModuleName:devinfo
 * Author: wangjc
 * Create Date: 2013-10-23
 * Description:add interface to get device information.
 * History:
   <version >  <time>  <author>  <desc>
   1.0		2013-10-23	wangjc	init
   2.0      2015-04-13  hantong modify as platform device  to support different configure in dts
*/

#include <linux/module.h>
#include <linux/proc_fs.h>
#include <soc/oneplus/device_info.h>
#include <linux/slab.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include "../../../../fs/proc/internal.h"
#include <linux/gpio.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/list.h>
#include <linux/iio/consumer.h>
#include <linux/uaccess.h>

#define DEVINFO_NAME "devinfo"
#define DEVI_DTB "oneplus-devinfo"

static struct proc_dir_entry *g_parent;
struct device_info {
	struct device *dev;
	struct pinctrl *p_ctrl;
	struct pinctrl_state *active, *sleep;
	struct list_head dev_list;
};

static struct device_info *g_dev_info;

/* *
 * in fact, it's not right and can't differentiate 20882 and 20883, but our hardware engineer
 * said there is no way to differentiate, so we just get the stage. default all is match. Just
 * create a interface, for someone to modify it sometime.
 */
static char stage[10][10] = { "UNKNOWN", "T0", "EVT1", "EVT2", "DVT1", "DVT2", "PVT", "MP", };
static char *current_stage;
int current_version;

static int __init init_hw_version(char *str)
{
	int version, ret;

	if (!str)
		return 0;

	ret = kstrtoint(str, 10, &version);
	if (ret < 0)
		version = 0;

	current_stage = stage[version];
	current_version = version;

	return version;
}
__setup("androidboot.hw_version=", init_hw_version);

bool check_id_match(const char *label, const char *id_match, int id)
{
	struct o_hw_id *pos;

	list_for_each_entry(pos, &(g_dev_info->dev_list), list) {
		if (sizeof(label) != sizeof(pos->label))
			continue;
		if (!strcasecmp(pos->label, label)) {
			if (id_match) {
				if (!strcasecmp(pos->match, id_match))
					return true;
			} else {
				if (pos->id == id)
					return true;
			}
		}
	}

	return false;
}

static int devinfo_read_func(struct seq_file *s, void *v)
{
	struct manufacture_info *info = (struct manufacture_info *)s->private;

	if (info->version)
		seq_printf(s, "Device version:\t\t%s\n", info->version);

	if (info->manufacture)
		seq_printf(s, "Device manufacture:\t\t%s\n", info->manufacture);

	if (info->fw_path)
		seq_printf(s, "Device fw_path:\t\t%s\n", info->fw_path);

	return 0;
}

static int device_info_open(struct inode *inode, struct file *file)
{
	return single_open(file, devinfo_read_func, PDE_DATA(inode));
}

static const struct file_operations device_node_fops = {
	.owner = THIS_MODULE,
	.open  = device_info_open,
	.read  = seq_read,
	.release = single_release,
};

static int devinfo_read_ufsplus_func(struct seq_file *s, void *v)
{
	struct o_ufsplus_status *ufsplus_status  = (struct o_ufsplus_status *)s->private;

	if (!ufsplus_status)
		return -EINVAL;

	seq_printf(s, "HPB status: %d\n", *(ufsplus_status->hpb_status));
	seq_printf(s, "TW status: %d\n", *(ufsplus_status->tw_status));
	return 0;
}

static int device_info_for_ufsplus_open(struct inode *inode, struct file *file)
{
	return single_open(file, devinfo_read_ufsplus_func, PDE_DATA(inode));
}


static const struct file_operations device_node_for_ufsplus_fops = {
.owner = THIS_MODULE,
.open  = device_info_for_ufsplus_open,
.read  = seq_read,
.release = single_release,
};

static int deviceid_read_func(struct seq_file *s, void *v)
{
	struct o_hw_id *info = (struct o_hw_id *)s->private;

	if (info->match)
		seq_printf(s, "%s", info->match);
	else
		seq_printf(s, "%d", info->id);

	return 0;
}

static int device_id_open(struct inode *inode, struct file *file)
{
	return single_open(file, deviceid_read_func, PDE_DATA(inode));
}

static const struct file_operations device_id_fops = {
	.owner = THIS_MODULE,
	.open  = device_id_open,
	.read  = seq_read,
	.release = single_release,
};

int register_device_id(struct device_info *dev_info, const char *label, const char *id_match, int id)
{
	struct o_hw_id *hw_id = NULL;
	struct proc_dir_entry *d_entry;

	hw_id = kzalloc(sizeof(*hw_id), GFP_KERNEL);
	if (!hw_id)
		return -ENOMEM;

	hw_id->label = label;
	hw_id->match = id_match;
	hw_id->id = id;

	list_add(&(hw_id->list), &(dev_info->dev_list));

	d_entry = proc_create_data(label, 0444, g_parent, &device_id_fops, hw_id);

	return 0;
}

int register_devinfo(char *name, struct manufacture_info *info)
{
	struct proc_dir_entry *d_entry;

	if (!info)
		return -EINVAL;

	memcpy(info->name, name, strlen(name) > INFO_LEN-1 ? INFO_LEN-1:strlen(name));

	d_entry = proc_create_data(name, 0444, g_parent, &device_node_fops, info);
	if (!d_entry)
		return -EINVAL;

	return 0;
}

int register_device_proc_for_ufsplus(char *name, int *hpb_status, int *tw_status)
{
	struct proc_dir_entry *d_entry;
	struct o_ufsplus_status *ufsplus_status;

	ufsplus_status = kzalloc(sizeof(*ufsplus_status), GFP_KERNEL);

	if (!ufsplus_status)
		return -ENOMEM;

	ufsplus_status->hpb_status = hpb_status;
	ufsplus_status->tw_status = tw_status;

	d_entry = proc_create_data(name, 0444, g_parent, &device_node_for_ufsplus_fops, ufsplus_status);
	if (!d_entry) {
		kfree(ufsplus_status);
		return -EINVAL;
	}

	return 0;
}

int register_device_proc(char *name, char *version, char *vendor)
{
	struct manufacture_info *info;

	if (!g_parent)
		return -ENOMEM;

	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	if (version) {
		info->version = kzalloc(32, GFP_KERNEL);
		if (!info->version) {
			kfree(info);
			return -ENOMEM;
		}
		memcpy(info->version, version, strlen(version) > 31?31:strlen(version));

	}
	if (vendor) {
		info->manufacture = kzalloc(32, GFP_KERNEL);
		if (!info->manufacture) {
			kfree(info->version);
			kfree(info);
			return -ENOMEM;
		}
		memcpy(info->manufacture, vendor, strlen(vendor) > 31?31:strlen(vendor));
	}

	return register_devinfo(name, info);
}

#define BOARD_GPIO_SUPPORT 4
#define MAIN_BOARD_SUPPORT 256

static int parse_gpio_dts(struct device *dev, struct device_info *dev_info)
{
	dev_info->p_ctrl = devm_pinctrl_get(dev);

	if (!IS_ERR_OR_NULL(dev_info->p_ctrl)) {
		dev_info->active = pinctrl_lookup_state(dev_info->p_ctrl, "active");
		dev_info->sleep = pinctrl_lookup_state(dev_info->p_ctrl, "sleep");
	}

	return 0;
}

static void set_gpios_active(struct device_info *dev_info)
{
	if (!IS_ERR_OR_NULL(dev_info->p_ctrl) && !IS_ERR_OR_NULL(dev_info->active))
		pinctrl_select_state(dev_info->p_ctrl, dev_info->active);
}

static void set_gpios_sleep(struct device_info *dev_info)
{
	if (!IS_ERR_OR_NULL(dev_info->p_ctrl) && !IS_ERR_OR_NULL(dev_info->sleep))
		pinctrl_select_state(dev_info->p_ctrl, dev_info->sleep);
}

static int init_other_hw_ids(struct platform_device *pdev)
{
	struct device_node *np;
	struct device_info *dev_info = platform_get_drvdata(pdev);
	const char *label = NULL, *name = NULL;
	int ret = 0, i = 0, size = 0;
	int gpio = 0, id = 0;
	uint8_t hw_mask = 0;
	char tmp[24];
	bool fail = false;
	uint32_t hw_combs[16];

	for_each_compatible_node(np, NULL, "hw, devices") {
		ret = of_property_read_string(np, "label", &label);
		if (ret < 0 || !label)
			continue;

		fail = false;
		hw_mask = 0;
		/*get hw mask*/
		for (i = 0; i < BOARD_GPIO_SUPPORT; i++) {
			snprintf(tmp, 24, "hw-id%d", i);
			gpio = of_get_named_gpio(np, tmp, 0);
			if (gpio < 0)
				continue;
			ret = gpio_request(gpio, tmp);
			if (ret < 0) {
				fail = true;
				break;
			}

			id = gpio_get_value(gpio);
			hw_mask |= (((uint8_t)id & 0x01) << i);
		}

		if (fail)
			continue;

		/*get hw mask name*/
		size = of_property_count_elems_of_size(np, "hw-combs", sizeof(uint32_t));
		if (size < 0)
			continue;
		of_property_read_u32_array(np, "hw-combs", hw_combs, size);
		for (i = 0; i < size; i++) {
			if (hw_combs[i] == hw_mask)
				break;
		}
		if (i == size)
			continue;

		/*get hw names*/
		size = of_property_count_strings(np, "hw-names");
		if (size >= i) {
			ret = of_property_read_string_index(np, "hw-names", i, &name);
			if (ret < 0)
				continue;
		}

		/*register hw id*/
		register_device_id(dev_info, label, name, hw_mask);
	}

	return 0;
}

static int init_aboard_info(struct device_info *dev_info)
{
	struct manufacture_info *info = NULL;

	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	if (strlen(current_stage) && strcasecmp(current_stage, stage[0])) {
		info->manufacture = "rf-match";
		info->version = current_stage;
	} else {
		info->manufacture = "rf-notmatch";
		info->version = "Qcom";
	}

	return register_devinfo("audio_mainboard", info);
}

static int sim_tray_status_read_func(struct seq_file *s, void *v)
{
	int value = -1;
	int gpio = -1;
	struct device *dev = NULL;
	struct device_node *np = NULL;

	dev = g_dev_info->dev;
	np = dev->of_node;
	/* sim_tray_detect = <&tlmm 88 GPIO_ACTIVE_HIGH>; */
	gpio = of_get_named_gpio(np, "sim_tray_detect", 0);
	if (gpio < 0) {
		pr_err("sdcard_detect gpio%d is not specified\n", gpio);
		return -ENODEV;
	}

	value = gpio_get_value(gpio);
	if (value < 0) {
		pr_err("sim_tray_detect gpio%d get value failed\n", gpio);
		return -ENODEV;
	}

	seq_printf(s, "%d\n", value);
	return 0;
}

static int sim_tray_status_open(struct inode *inode, struct file *file)
{
	return single_open(file, sim_tray_status_read_func, PDE_DATA(inode));
}

static const struct file_operations proc_sim_tray_status_op = {
	.owner = THIS_MODULE,
	.open  = sim_tray_status_open,
	.read  = seq_read,
	.release = single_release,
};

static void register_sim_tray_status(void)
{
	static struct proc_dir_entry *status;

	status = proc_create("sim_tray_status", 0644, NULL, &proc_sim_tray_status_op);
	if (!status) {
		pr_err("create sim_tray_status node fail!");
	}
}

static int devinfo_probe(struct platform_device *pdev)
{
	struct device_info *dev_info;

	dev_info = kzalloc(sizeof(struct device_info), GFP_KERNEL);
	if (!dev_info)
		return -ENOMEM;

	INIT_LIST_HEAD(&dev_info->dev_list);

	g_dev_info = dev_info;
	g_dev_info->dev = &pdev->dev;

	platform_set_drvdata(pdev, dev_info);

	/*parse dts first*/
	parse_gpio_dts(&pdev->dev, dev_info);

	init_aboard_info(g_dev_info);

	/*init other hw id*/
	set_gpios_active(dev_info);
	init_other_hw_ids(pdev);
	set_gpios_sleep(dev_info);

	/*register special node*/
	register_sim_tray_status();

	return 0;
}

static int devinfo_remove(struct platform_device *dev)
{
	if (g_parent)
		remove_proc_entry("ufsplus_status", g_parent);
	remove_proc_entry(DEVINFO_NAME, NULL);
	return 0;
}

static const struct of_device_id devinfo_id[] = {
	{	.compatible = DEVI_DTB,},
	{},
};

static struct platform_driver devinfo_platform_driver = {
	.probe = devinfo_probe,
	.remove = devinfo_remove,
	.driver = {
		.name = DEVINFO_NAME,
		.of_match_table = devinfo_id,
	},
};

static int __init device_info_init(void)
{
	g_parent = proc_mkdir("devinfo", NULL);

	if (!g_parent)
		return -ENODEV;

	return platform_driver_register(&devinfo_platform_driver);
}

device_initcall(device_info_init);

MODULE_DESCRIPTION("vendor device info");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Klus <Klus@vendora.com>");
