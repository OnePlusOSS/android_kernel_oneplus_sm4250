// SPDX-License-Identifier: GPL-2.0
/*
 *
 * This software program is licensed subject to the GNU General Public License
 * (GPL).Version 2,June 1991, available at http://www.fsf.org/copyleft/gpl.html
 * (C) Copyright 2011 Bosch Sensortec GmbH
 * All Rights Reserved
 *
 * file TPS61280.c
 * brief This file contains all function for the TPS61280 in linux
 */


#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/unistd.h>
#ifdef CONFIG_OF
#include <linux/of_gpio.h>
#endif
//#include <linux/sensors.h>
#define TPS61280_NAME			"tps61280"
#define TPS61280_INPUT_DEV_NAME	"TPS61280"
#define TPS61280_COMPAT	"ti,tps61280"

#define TPS61280_REG_NUM		3

static unsigned int  InitDataAddr[TPS61280_REG_NUM] = { 0x01, 0x02, 0x03 };
static unsigned int InitDataVal[TPS61280_REG_NUM]  = { 0x05, 0x0B, 0x1D };

struct tps61280_data {
	struct i2c_client *tps_client;
	unsigned char mode;
	int chip_en;
	int chip_vsel;
	struct input_dev *input;
};

static ssize_t tps61280_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct tps61280_data *data = dev_get_drvdata(dev);
	int enable;

	enable = gpio_get_value(data->chip_en);

	return snprintf(buf, 64, "%d\n", enable);
}
static ssize_t tps61280_enable_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct tps61280_data *data = dev_get_drvdata(dev);
	unsigned long val;

	if (kstrtoul(buf, 0, &val))
		return -EINVAL;

	if (val == 1)
		gpio_direction_output(data->chip_en, 1);
	else if (val == 0)
		gpio_direction_output(data->chip_en, 0);

	return count;
}
static DEVICE_ATTR_RW(tps61280_enable);

static struct attribute *tps61280_attributes[] = {
	&dev_attr_tps61280_enable.attr,
	//&dev_attr_tps61280_reg_ctrl.attr,
	NULL,
};

static struct attribute_group tps61280_attribute_group = {
	.attrs = tps61280_attributes
};

static int tps61280_init_func(struct i2c_client *client, unsigned char len)
{
	int ret = 0;
	int i;
	int reg_val = 0;

	for (i = 0; i < len ; i++) {
		//msleep(200);
		ret = i2c_smbus_write_byte_data
			(client, InitDataAddr[i], InitDataVal[i]);
		if (ret < 0)
			dev_err(&client->dev,
				"tps61280 i = %d,write 0x%x to addr 0x%x failed, ret = %d\n",
				i, InitDataVal[i], InitDataAddr[i], ret);
			//return ret;
		msleep(20);
		reg_val = i2c_smbus_read_byte_data(client, InitDataAddr[i]);
		dev_err(&client->dev, "tps61280 0x0%d=0x%x\n",
			InitDataAddr[i], reg_val);
	}
	return 0;
}
static int tps61280_parse_dt(struct device *dev,
				struct tps61280_data *pdata)
{
	//struct device_node *np = dev->of_node;
	return 0;
}

#define  TPS_WAIT_INTERVAL     (1000 * HZ)


//static struct of_device_id tps61280_match_table[];

static int tps61280_probe(struct i2c_client *client,
const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct tps61280_data *data = {NULL};
	struct input_dev *dev;
	int ret = 0;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE))
		return -EIO;

	if (client->dev.of_node) {
		data = devm_kzalloc(&client->dev,
			sizeof(struct tps61280_data), GFP_KERNEL);
		if (!data)
			return -ENOMEM;
		ret = tps61280_parse_dt(&client->dev, data);
		if (ret) {
			dev_err(&client->dev,
				"tps61280 Unable parse data ret=%d\n", ret);
			return ret;
		}
	} else {
		dev_err(&client->dev, "tps61280 probe, dev.of_node error\n");
		goto kfree_exit;
	}

	data->tps_client = client;
	i2c_set_clientdata(client, data);

	data->chip_en = of_get_named_gpio(client->dev.of_node,
		"tps61280,chip_enable", 0);
	ret = gpio_request(data->chip_en, "tps61280_en");
	if (ret)
		dev_err(&client->dev, "tps61280_en request fail\n");

	dev_err(&client->dev, "tps61280 enable = %d\n ",
		gpio_get_value(data->chip_en));
	//mdelay(10);
	gpio_direction_output(data->chip_en, 1);
	//gpio_set_value(data->chip_en, 1);
	//mdelay(10);
	dev_err(&client->dev, "tps61280 enable = %d\n ",
		gpio_get_value(data->chip_en));
	i2c_smbus_write_byte_data(client, 0x01, 0x80);

	ret = tps61280_init_func(client, TPS61280_REG_NUM);
	if (ret < 0)
		dev_err(&client->dev, "init TPS61280 device error\n");

	dev = input_allocate_device();
	if (!dev)
		dev_err(&client->dev, "tps61280 input allocate device error\n");

	dev->name = TPS61280_INPUT_DEV_NAME;
	dev->id.bustype = BUS_I2C;

	input_set_drvdata(dev, data);

	ret = input_register_device(dev);
	if (ret < 0)
		input_free_device(dev);
	data->input = dev;

	ret = sysfs_create_group(&data->input->dev.kobj,
		&tps61280_attribute_group);
	if (ret < 0)
		goto error_sysfs;

	return 0;

error_sysfs:
	input_unregister_device(data->input);
kfree_exit:
	kfree(data);
	return ret;
}
static int  tps61280_remove(struct i2c_client *client)
{
	struct tps61280_data *data = i2c_get_clientdata(client);

	kfree(data);
	return 0;
}

static const struct i2c_device_id tps61280_id[] = {
	{ TPS61280_NAME, 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, bma2x2_id);

static const struct of_device_id tps61280_match_table[] = {
	{ .compatible = TPS61280_COMPAT, },
	{ },
};

static struct i2c_driver tps61280_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= TPS61280_NAME,
		.of_match_table = tps61280_match_table,
	},
	.id_table	= tps61280_id,
	.probe		= tps61280_probe,
	.remove		= tps61280_remove,

};
static int __init tps61280_init(void)
{
	return i2c_add_driver(&tps61280_driver);
}

static void __exit tps61280_exit(void)
{
	i2c_del_driver(&tps61280_driver);
}

MODULE_AUTHOR("HongTao guo <guohongtao@yeptelecom.com>");
MODULE_DESCRIPTION("TPS61280 davicom ic driver");
MODULE_LICENSE("GPL");

module_init(tps61280_init);
module_exit(tps61280_exit);

