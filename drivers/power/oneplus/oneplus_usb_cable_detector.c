// SPDX-License-Identifier: GPL-2.0

/*For OEM project monitor USB cable connection status,
 * and config OTG cc status.
 */

#include <linux/export.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/sys_soc.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/gpio.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <soc/qcom/subsystem_restart.h>
#include <linux/oem/boot_mode.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/kobject.h>
#include <linux/pm_wakeup.h>

#define USB_CABLE_NAME "oneplus_usb_gpio"
#define USB_CABLE "oneplus,usb_cable"

#define PRINT_SKIN_THERM_TEMP_BY_TIMES    2000
extern void onepluschg_set_otg_switch(bool value);
struct delayed_work gpio_work;

struct usb_info {
	int    cable_gpio;
	struct device *dev;
	struct pinctrl *p_ctrl;
	struct pinctrl_state *usb_cable_active;
};

static struct usb_info *g_dev_info;

static int parse_gpio_dts(struct device *dev, struct usb_info *dev_info)
{
	dev_info->p_ctrl = devm_pinctrl_get(dev);

	if (!IS_ERR_OR_NULL(dev_info->p_ctrl))
		dev_info->usb_cable_active = pinctrl_lookup_state(dev_info->p_ctrl, "usb_cable_active");
	return 0;
}

int pre_value = -1;
int cur_value = -1;
int op_usb_cable_detecter(void)
{
	struct device_node *np = g_dev_info->dev->of_node;
	enum oem_boot_mode boot_mode = get_boot_mode();

	if (boot_mode == MSM_BOOT_MODE_RF || boot_mode == MSM_BOOT_MODE_FACTORY)
		return 0;

	g_dev_info->cable_gpio = of_get_named_gpio(np, "usb,cable-gpio", 0);

	cur_value = gpio_get_value(g_dev_info->cable_gpio);
	//pr_debug("oneplus detected gpio,g_dev_info->cable_gpio = %d\n", value);

	if (cur_value != pre_value) {
		if (cur_value == 1) {
			pr_err("oneplus detected gpio,g_dev_info->cable_gpio high = %d\n", cur_value);
			usleep_range(10000, 11000);
			onepluschg_set_otg_switch(false);
		} else if (cur_value == 0) {
			pr_err("oneplus detected gpio,g_dev_info->cable_gpio low= %d\n", cur_value);
			usleep_range(10000, 11000);
			onepluschg_set_otg_switch(true);
		}
	}
	pre_value = cur_value;
	//pr_err("oneplus detected gpio,g_dev_info->cable_gpio = %d\n", cur_value);
	return cur_value;
}
EXPORT_SYMBOL(op_usb_cable_detecter);

static void get_gpio_status_work(struct work_struct *work)
{
	int gpio_value;

	gpio_value = op_usb_cable_detecter();//usb,cable-gpio
	//pr_err("oneplus detected gpio,op_usb_cable_detecter = %d\n", gpio_value);
	schedule_delayed_work(&gpio_work, msecs_to_jiffies(PRINT_SKIN_THERM_TEMP_BY_TIMES));
}

static int op_usb_cable_probe(struct platform_device *pdev)
{
	struct usb_info *dev_info;

	dev_info = kzalloc(sizeof(struct usb_info), GFP_KERNEL);
	if (!dev_info)
		return -ENOMEM;

	g_dev_info = dev_info;
	g_dev_info->dev = &pdev->dev;

	platform_set_drvdata(pdev, dev_info);
	/*parse dts first*/

	parse_gpio_dts(&pdev->dev, dev_info);

	INIT_DELAYED_WORK(&gpio_work, get_gpio_status_work);
	schedule_delayed_work(&gpio_work, msecs_to_jiffies(60000));
	return 0;
}

static const struct of_device_id match_table[] = {
	{ .compatible = USB_CABLE, },
	{}
};

static struct platform_driver op_usb_cable_driver = {
	.driver = {
		.name       = USB_CABLE_NAME,
		.of_match_table = match_table,
	},
	.probe = op_usb_cable_probe,
};

static int __init op_usb_cable_init(void)
{
	int ret;

	ret = platform_driver_register(&op_usb_cable_driver);
	if (ret)
		pr_err("usb_cable_driver register failed: %d\n", ret);
	return ret;
}

device_initcall(op_usb_cable_init);

MODULE_DESCRIPTION("vendor usb info");
MODULE_LICENSE("GPL v2");
