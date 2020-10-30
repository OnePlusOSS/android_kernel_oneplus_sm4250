// SPDX-License-Identifier: GPL-2.0

/*For OEM project monitor RF cable connection status,
 * and config different RF configuration
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
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/kobject.h>
#include <linux/pm_wakeup.h>

#define RF_CABLE_OUT        0
#define RF_CABLE_IN            1
#define CABLE_0                0
#define CABLE_1                1
#define INVALID_GPIO          -2
#define PAGESIZE 512
#define MAX_CABLE_STS 4
#define RF_CABLE "oneplus,rf_cable"
#define RF_CABLE_NAME "rf_cable"

struct rf_info_type {
	unsigned int        rf_cable_sts;
	unsigned char        rf_cable_gpio_sts[MAX_CABLE_STS];
	unsigned int        reserver[6];
};

struct rf_cable_data {
	int             cable0_irq;
	int             cable1_irq;
	int             cable0_gpio;
	int             cable1_gpio;
	unsigned int        rf_cable_sts;
	unsigned char        rf_cable_gpio_sts[MAX_CABLE_STS];

	struct             device *dev;
	struct             input_dev  *input_dev;
	struct             wakeup_source cable_ws;
	struct             pinctrl *gpio_pinctrl;
	struct             pinctrl_state *rf_cable_active;
};

static struct rf_cable_data *the_rf_data;
static struct rf_info_type *the_rf_format;
static const char *sar_name = "v-sar";
static const char *sar_input_phys = "input/sar";
static bool rf_cable_0_support;
static bool rf_cable_1_support;

//=====================================
static ssize_t cable_read_proc(struct file *file, char __user *buf, size_t count, loff_t *off)
{
	char page[128] = {0};
	int len = 0;

	if (!the_rf_data || !the_rf_format)
		return -EFAULT;

	if (gpio_is_valid(the_rf_data->cable0_gpio))
		the_rf_data->rf_cable_gpio_sts[0] = gpio_get_value(the_rf_data->cable0_gpio);
	else
		the_rf_data->rf_cable_gpio_sts[0] = 0xFF;

	if (gpio_is_valid(the_rf_data->cable1_gpio))
		the_rf_data->rf_cable_gpio_sts[1] = gpio_get_value(the_rf_data->cable1_gpio);
	else
		the_rf_data->rf_cable_gpio_sts[1] = 0xFF;

	the_rf_format->rf_cable_sts = the_rf_data->rf_cable_sts;

	len += snprintf(&page[len], 256, "%d,%d\n",
		the_rf_data->rf_cable_gpio_sts[0], the_rf_data->rf_cable_gpio_sts[1]);

	if (len > *off)
		len -= *off;
	else
		len = 0;

	if (copy_to_user(buf, page, (len < count ? len : count)))
		return -EFAULT;
	*off += len < count ? len : count;

	return (len < count ? len : count);
}

static const struct file_operations cable_proc_fops_cable = {
	.read = cable_read_proc,
};

//=====================================
static irqreturn_t cable_interrupt0(int irq, void *dev_id)
{
	struct rf_cable_data *sdata = (struct rf_cable_data *)dev_id;

	__pm_stay_awake(&sdata->cable_ws);
	usleep_range(10000, 20000);
	if (gpio_is_valid(sdata->cable0_gpio))
		sdata->rf_cable_gpio_sts[0] = gpio_get_value(sdata->cable0_gpio);
	else
		sdata->rf_cable_gpio_sts[0] = 0xFF;

	if (sdata->rf_cable_gpio_sts[0] == 1) {
		input_report_key(sdata->input_dev, KEY_RF_CABLE_IN0, 1);//0x2f0
		input_report_key(sdata->input_dev, KEY_RF_CABLE_IN0, 0);
	} else if (sdata->rf_cable_gpio_sts[0] == 0) {
		input_report_key(sdata->input_dev, KEY_RF_CABLE_OUT0, 1);//0x2f1
		input_report_key(sdata->input_dev, KEY_RF_CABLE_OUT0, 0);
	}

	input_sync(sdata->input_dev);

	__pm_relax(&sdata->cable_ws);

	return IRQ_HANDLED;
}
static irqreturn_t cable_interrupt1(int irq, void *dev_id)
{
	struct rf_cable_data *sdata = (struct rf_cable_data *)dev_id;

	__pm_stay_awake(&sdata->cable_ws);
	usleep_range(10000, 20000);
	if (gpio_is_valid(sdata->cable1_gpio))
		sdata->rf_cable_gpio_sts[1] = gpio_get_value(sdata->cable1_gpio);
	else
		sdata->rf_cable_gpio_sts[1] = 0xFF;

	if (sdata->rf_cable_gpio_sts[1] == 1) {
		input_report_key(sdata->input_dev, KEY_RF_CABLE_IN1, 1);
		input_report_key(sdata->input_dev, KEY_RF_CABLE_IN1, 0);
	} else if (sdata->rf_cable_gpio_sts[1] == 0) {
		input_report_key(sdata->input_dev, KEY_RF_CABLE_OUT1, 1);
		input_report_key(sdata->input_dev, KEY_RF_CABLE_OUT1, 0);
	}
	input_sync(sdata->input_dev);

	__pm_relax(&sdata->cable_ws);

	return IRQ_HANDLED;
}

static int rf_cable_initial_request_irq(struct rf_cable_data *rf_data)
{
	int rc = 0;

	if (gpio_is_valid(rf_data->cable0_gpio)) {
		rc = request_threaded_irq(rf_data->cable0_irq, NULL, cable_interrupt0,
			IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING |
			IRQF_ONESHOT | IRQF_NO_SUSPEND, "rf_cable0_irq", rf_data);
		if (rc) {
			pr_err("%s cable0_gpio, request falling fail\n",
				__func__);
			return rc;
		}
	}

	if (gpio_is_valid(rf_data->cable1_gpio)) {
		rc = request_threaded_irq(rf_data->cable1_irq, NULL, cable_interrupt1,
			IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING |
			IRQF_ONESHOT|IRQF_NO_SUSPEND, "rf_cable1_irq", rf_data);
		if (rc) {
			pr_err("%s cable1_gpio, request falling fail\n",
				__func__);
			return rc;
		}
	}

	return rc;
}


static int rf_cable_gpio_pinctrl_init
(struct platform_device *pdev, struct device_node *np, struct rf_cable_data *rf_data)
{
	int retval = 0;

	if (rf_cable_0_support)
		rf_data->cable0_gpio = of_get_named_gpio(np, "rf,cable0-gpio", 0);
	else
		rf_data->cable0_gpio = INVALID_GPIO;

	if (rf_cable_1_support)
		rf_data->cable1_gpio = of_get_named_gpio(np, "rf,cable1-gpio", 0);
	else
		rf_data->cable1_gpio = INVALID_GPIO;

	rf_data->gpio_pinctrl = devm_pinctrl_get(&(pdev->dev));

	if (IS_ERR_OR_NULL(rf_data->gpio_pinctrl)) {
		retval = PTR_ERR(rf_data->gpio_pinctrl);
		pr_err("%s get gpio_pinctrl fail, retval:%d\n", __func__, retval);
		goto err_pinctrl_get;
	}

	rf_data->rf_cable_active = pinctrl_lookup_state(rf_data->gpio_pinctrl, "rf_cable_active");
	if (!IS_ERR_OR_NULL(rf_data->rf_cable_active)) {
		retval = pinctrl_select_state(rf_data->gpio_pinctrl,
				rf_data->rf_cable_active);
		if (retval < 0) {
			pr_err("%s select pinctrl fail, retval:%d\n", __func__, retval);
		goto err_pinctrl_lookup;
		}
	}

	if (gpio_is_valid(rf_data->cable0_gpio)) {
		gpio_direction_input(rf_data->cable0_gpio);
		rf_data->cable0_irq = gpio_to_irq(rf_data->cable0_gpio);
		if (rf_data->cable0_irq < 0) {
			pr_err("Unable to get irq number for GPIO %d, error %d\n",
				rf_data->cable0_gpio, rf_data->cable0_irq);
			goto err_pinctrl_lookup;
		}
		rf_data->rf_cable_gpio_sts[0] = gpio_get_value(rf_data->cable0_gpio);
	} else
		rf_data->rf_cable_gpio_sts[0] = 0xFF;

	if (gpio_is_valid(rf_data->cable1_gpio)) {
		gpio_direction_input(rf_data->cable1_gpio);
			rf_data->cable1_irq = gpio_to_irq(rf_data->cable1_gpio);
		if (rf_data->cable1_irq < 0) {
			pr_err("Unable to get irq number for GPIO %d, error %d\n",
				rf_data->cable1_gpio, rf_data->cable1_irq);
			goto err_pinctrl_lookup;
		}
		rf_data->rf_cable_gpio_sts[1] = gpio_get_value(rf_data->cable1_gpio);
	} else
		rf_data->rf_cable_gpio_sts[1] = 0xFF;

	if (the_rf_format)
		the_rf_format->rf_cable_sts = rf_data->rf_cable_sts;

	return 0;

err_pinctrl_lookup:
	devm_pinctrl_put(rf_data->gpio_pinctrl);
err_pinctrl_get:
	rf_data->gpio_pinctrl = NULL;

	return -EFAULT;
}

static s8 sar_request_input_dev(struct rf_cable_data *sdata)
{
	s8 ret = -1;

	sdata->input_dev = input_allocate_device();
	if (sdata->input_dev == NULL)
		return -ENOMEM;

	sdata->input_dev->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY);

	__set_bit(EV_REP, sdata->input_dev->evbit);

	sdata->input_dev->name = sar_name;
	sdata->input_dev->phys = sar_input_phys;
	sdata->input_dev->id.bustype = BUS_HOST;
	sdata->input_dev->id.vendor = 0x0001;
	sdata->input_dev->id.product = 0x0001;
	sdata->input_dev->id.version = 0x0001;

	ret = input_register_device(sdata->input_dev);
	if (ret)
		return -ENODEV;

	input_set_capability(sdata->input_dev, EV_KEY, KEY_RF_CABLE_OUT0);
	input_set_capability(sdata->input_dev, EV_KEY, KEY_RF_CABLE_IN0);
	input_set_capability(sdata->input_dev, EV_KEY, KEY_RF_CABLE_OUT1);
	input_set_capability(sdata->input_dev, EV_KEY, KEY_RF_CABLE_IN1);

	return 0;
}

static int op_rf_cable_probe(struct platform_device *pdev)
{
	int rc = 0;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct rf_cable_data *rf_data = NULL;
	struct proc_dir_entry *op_rf = NULL;

	pr_err("%s enter!\n", __func__);

	rf_cable_0_support = of_property_read_bool(np, "rf_cable_0_support");
	rf_cable_1_support = of_property_read_bool(np, "rf_cable_1_support");

	the_rf_format = kzalloc(sizeof(struct rf_info_type), GFP_KERNEL);
	if (IS_ERR(the_rf_format)) {
		pr_err("%s smem_get fail\n", __func__);
		return -EFAULT;
	}

	rf_data = kzalloc(sizeof(struct rf_cable_data), GFP_KERNEL);
	if (!rf_data) {
		rc = -ENOMEM;
		goto exit_nomem;
	}

	rf_data->dev = dev;
	dev_set_drvdata(dev, rf_data);
	wakeup_source_init(&rf_data->cable_ws, "rf_cable_wake_lock");
	rc = rf_cable_gpio_pinctrl_init(pdev, np, rf_data);
	if (rc) {
		pr_err("%s gpio_init fail\n", __func__);
		goto exit;
	}

	the_rf_data = rf_data;
	rc = rf_cable_initial_request_irq(rf_data);
	if (rc) {
		pr_err("could not request cable_irq\n");
		goto exit;
	}

	op_rf = proc_mkdir("op_rf", NULL);
	if (!op_rf) {
		pr_err("can't create op_rf proc\n");
		goto exit;
	}

	if (rf_cable_0_support || rf_cable_1_support)
		proc_create("rf_cable", 0444, op_rf, &cable_proc_fops_cable);

	rc = sar_request_input_dev(rf_data);

	pr_err("%s: probe ok, rf_cable, sts:%d, [0_gpio:%d, 0_val:%d], [1_gpio:%d, 1_val:%d], 0_irq:%d, 1_irq:%d\n",
		__func__, the_rf_format->rf_cable_sts,
		rf_data->cable0_gpio, the_rf_format->rf_cable_gpio_sts[0],
		rf_data->cable1_gpio, the_rf_format->rf_cable_gpio_sts[1],
		rf_data->cable0_irq, rf_data->cable1_irq);

	return 0;

exit:
	wakeup_source_trash(&rf_data->cable_ws);
	kfree(rf_data);
exit_nomem:
	the_rf_data = NULL;
	the_rf_format = NULL;
	pr_err("%s: probe Fail!\n", __func__);

	return rc;
}

static const struct of_device_id rf_of_match[] = {
	{ .compatible = RF_CABLE, },
	{}
};
MODULE_DEVICE_TABLE(of, rf_of_match);

static struct platform_driver op_rf_cable_driver = {
	.driver = {
		.name       = RF_CABLE_NAME,
		.owner      = THIS_MODULE,
		.of_match_table = rf_of_match,
	},
	.probe = op_rf_cable_probe,
};

static int __init op_rf_cable_init(void)
{
	int ret;

	ret = platform_driver_register(&op_rf_cable_driver);
	if (ret)
		pr_err("rf_cable_driver register failed: %d\n", ret);

	return ret;
}

MODULE_LICENSE("GPL v2");
late_initcall(op_rf_cable_init);
