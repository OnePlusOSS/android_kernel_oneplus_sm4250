// SPDX-License-Identifier: GPL-2.0-only

/*
 * Copyright (c) 2012-2020, The Linux Foundation. All rights reserved.
 */

#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/of.h>
#include <linux/uaccess.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>

#define RPM_MASTERS_BUF_LEN 1024

#define SNPRINTF(buf, size, format, ...) \
	do { \
		if (size > 0) { \
			int ret; \
			ret = snprintf(buf, size, format, ## __VA_ARGS__); \
			if (ret > size) { \
				buf += size; \
				size = 0; \
			} else { \
				buf += ret; \
				size -= ret; \
			} \
		} \
	} while (0)

#define GET_MASTER_NAME(a, prvdata) \
	((a >= prvdata->num_masters) ? "Invalid Master Name" : \
	 prvdata->master_names[a])

#define GET_FIELD(a) ((strnstr(#a, ".", 80) + 1))

struct msm_rpm_master_stats_platform_data *rpm_pdata;
struct rpm_master_stats_prv_data {
	struct kobj_attribute ka;
	struct kobject *kobj;
};

struct msm_rpm_master_stats_platform_data {
	phys_addr_t phys_addr_base;
	u32 phys_size;
	char **masters;
	/*
	 * RPM maintains PC stats for each master in MSG RAM,
	 * it allocates 256 bytes for this use.
	 * No of masters differs for different targets.
	 * Based on the number of masters, linux rpm stat
	 * driver reads (32 * num_masters) bytes to display
	 * master stats.
	 */
	s32 num_masters;
	u32 master_offset;
	u32 version;
};

static DEFINE_MUTEX(msm_rpm_master_stats_mutex);

struct msm_rpm_master_stats {
	uint32_t active_cores;
	uint32_t numshutdowns;
	uint64_t shutdown_req;
	uint64_t wakeup_ind;
	uint64_t bringup_req;
	uint64_t bringup_ack;
	uint32_t wakeup_reason; /* 0 = rude wakeup, 1 = scheduled wakeup */
	uint32_t last_sleep_transition_duration;
	uint32_t last_wake_transition_duration;
	uint32_t xo_count;
	uint64_t xo_last_entered_at;
	uint64_t xo_last_exited_at;
	uint64_t xo_accumulated_duration;
};

struct msm_rpm_master_stats_private_data {
	void __iomem *reg_base;
	u32 len;
	char **master_names;
	u32 num_masters;
	char buf[RPM_MASTERS_BUF_LEN];
	struct msm_rpm_master_stats_platform_data *platform_data;
};

static int msm_rpm_master_stats_file_close(struct inode *inode,
		struct file *file)
{
	struct msm_rpm_master_stats_private_data *private = file->private_data;

	mutex_lock(&msm_rpm_master_stats_mutex);
	if (private->reg_base)
		iounmap(private->reg_base);
	kfree(file->private_data);
	mutex_unlock(&msm_rpm_master_stats_mutex);

	return 0;
}

static int msm_rpm_master_copy_stats(
		struct msm_rpm_master_stats_private_data *prvdata)
{
	struct msm_rpm_master_stats record;
	struct msm_rpm_master_stats_platform_data *pdata;
	static int master_cnt;
	int count, j = 0;
	char *buf;
	unsigned long active_cores;

	/* Iterate possible number of masters */
	if (master_cnt > prvdata->num_masters - 1) {
		master_cnt = 0;
		return 0;
	}

	pdata = prvdata->platform_data;
	count = RPM_MASTERS_BUF_LEN;
	buf = prvdata->buf;

	if (prvdata->platform_data->version == 2) {
		SNPRINTF(buf, count, "%s\n",
				GET_MASTER_NAME(master_cnt, prvdata));

		record.shutdown_req = readq_relaxed(prvdata->reg_base +
			(master_cnt * pdata->master_offset +
			offsetof(struct msm_rpm_master_stats, shutdown_req)));

		SNPRINTF(buf, count, "\t%s:0x%llX\n",
			GET_FIELD(record.shutdown_req),
			record.shutdown_req);

		record.wakeup_ind = readq_relaxed(prvdata->reg_base +
			(master_cnt * pdata->master_offset +
			offsetof(struct msm_rpm_master_stats, wakeup_ind)));

		SNPRINTF(buf, count, "\t%s:0x%llX\n",
			GET_FIELD(record.wakeup_ind),
			record.wakeup_ind);

		record.bringup_req = readq_relaxed(prvdata->reg_base +
			(master_cnt * pdata->master_offset +
			offsetof(struct msm_rpm_master_stats, bringup_req)));

		SNPRINTF(buf, count, "\t%s:0x%llX\n",
			GET_FIELD(record.bringup_req),
			record.bringup_req);

		record.bringup_ack = readq_relaxed(prvdata->reg_base +
			(master_cnt * pdata->master_offset +
			offsetof(struct msm_rpm_master_stats, bringup_ack)));

		SNPRINTF(buf, count, "\t%s:0x%llX\n",
			GET_FIELD(record.bringup_ack),
			record.bringup_ack);

		record.xo_last_entered_at = readq_relaxed(prvdata->reg_base +
			(master_cnt * pdata->master_offset +
			offsetof(struct msm_rpm_master_stats,
			xo_last_entered_at)));

		SNPRINTF(buf, count, "\t%s:0x%llX\n",
			GET_FIELD(record.xo_last_entered_at),
			record.xo_last_entered_at);

		record.xo_last_exited_at = readq_relaxed(prvdata->reg_base +
			(master_cnt * pdata->master_offset +
			offsetof(struct msm_rpm_master_stats,
			xo_last_exited_at)));

		SNPRINTF(buf, count, "\t%s:0x%llX\n",
			GET_FIELD(record.xo_last_exited_at),
			record.xo_last_exited_at);

		record.xo_accumulated_duration =
				readq_relaxed(prvdata->reg_base +
				(master_cnt * pdata->master_offset +
				offsetof(struct msm_rpm_master_stats,
				xo_accumulated_duration)));

		SNPRINTF(buf, count, "\t%s:0x%llX\n",
			GET_FIELD(record.xo_accumulated_duration),
			record.xo_accumulated_duration);

		record.last_sleep_transition_duration =
				readl_relaxed(prvdata->reg_base +
				(master_cnt * pdata->master_offset +
				offsetof(struct msm_rpm_master_stats,
				last_sleep_transition_duration)));

		SNPRINTF(buf, count, "\t%s:0x%x\n",
			GET_FIELD(record.last_sleep_transition_duration),
			record.last_sleep_transition_duration);

		record.last_wake_transition_duration =
				readl_relaxed(prvdata->reg_base +
				(master_cnt * pdata->master_offset +
				offsetof(struct msm_rpm_master_stats,
				last_wake_transition_duration)));

		SNPRINTF(buf, count, "\t%s:0x%x\n",
			GET_FIELD(record.last_wake_transition_duration),
			record.last_wake_transition_duration);

		record.xo_count =
				readl_relaxed(prvdata->reg_base +
				(master_cnt * pdata->master_offset +
				offsetof(struct msm_rpm_master_stats,
				xo_count)));

		SNPRINTF(buf, count, "\t%s:0x%x\n",
			GET_FIELD(record.xo_count),
			record.xo_count);

		record.wakeup_reason = readl_relaxed(prvdata->reg_base +
					(master_cnt * pdata->master_offset +
					offsetof(struct msm_rpm_master_stats,
					wakeup_reason)));

		SNPRINTF(buf, count, "\t%s:0x%x\n",
			GET_FIELD(record.wakeup_reason),
			record.wakeup_reason);

		record.numshutdowns = readl_relaxed(prvdata->reg_base +
			(master_cnt * pdata->master_offset +
			 offsetof(struct msm_rpm_master_stats, numshutdowns)));

		SNPRINTF(buf, count, "\t%s:0x%x\n",
			GET_FIELD(record.numshutdowns),
			record.numshutdowns);

		record.active_cores = readl_relaxed(prvdata->reg_base +
			(master_cnt * pdata->master_offset) +
			offsetof(struct msm_rpm_master_stats, active_cores));

		SNPRINTF(buf, count, "\t%s:0x%x\n",
			GET_FIELD(record.active_cores),
			record.active_cores);
	} else {
		SNPRINTF(buf, count, "%s\n",
				GET_MASTER_NAME(master_cnt, prvdata));

		record.numshutdowns = readl_relaxed(prvdata->reg_base +
				(master_cnt * pdata->master_offset) + 0x0);

		SNPRINTF(buf, count, "\t%s:0x%0x\n",
			GET_FIELD(record.numshutdowns),
			record.numshutdowns);

		record.active_cores = readl_relaxed(prvdata->reg_base +
				(master_cnt * pdata->master_offset) + 0x4);

		SNPRINTF(buf, count, "\t%s:0x%0x\n",
			GET_FIELD(record.active_cores),
			record.active_cores);
	}

	active_cores = record.active_cores;
	j = find_first_bit(&active_cores, BITS_PER_LONG);
	while (j < (BITS_PER_LONG - 1)) {
		SNPRINTF(buf, count, "\t\tcore%d\n", j);
		j = find_next_bit((const unsigned long *)&active_cores,
							BITS_PER_LONG, j + 1);
	}

	if (j == (BITS_PER_LONG - 1))
		SNPRINTF(buf, count, "\t\tcore%d\n", j);

	master_cnt++;
	return RPM_MASTERS_BUF_LEN - count;
}

static int msm_rpm_master_copy_complete_stats(
		struct msm_rpm_master_stats_private_data *prvdata)
{
	struct msm_rpm_master_stats record;
	struct msm_rpm_master_stats_platform_data *pdata;
	int master_cnt = 0;
	int count, j = 0;
	char *buf;

	/* Iterate possible number of masters */
	if (master_cnt > prvdata->num_masters - 1) {
		master_cnt = 0;
		return 0;
	}

	pdata = prvdata->platform_data;
	count = RPM_MASTERS_BUF_LEN;
	buf = prvdata->buf;

	for (master_cnt = 0; master_cnt < prvdata->num_masters; master_cnt++) {
		if (prvdata->platform_data->version == 2) {
			SNPRINTF(buf, count, "%s\n",
					GET_MASTER_NAME(master_cnt, prvdata));

			record.xo_last_entered_at =
				readq_relaxed(prvdata->reg_base +
				(master_cnt * pdata->master_offset +
				offsetof(struct msm_rpm_master_stats,
				xo_last_entered_at)));

			SNPRINTF(buf, count, "\t%s:0x%llX\n",
				GET_FIELD(record.xo_last_entered_at),
				record.xo_last_entered_at);

			record.xo_last_exited_at =
				readq_relaxed(prvdata->reg_base +
				(master_cnt * pdata->master_offset +
				offsetof(struct msm_rpm_master_stats,
				xo_last_exited_at)));

			SNPRINTF(buf, count, "\t%s:0x%llX\n",
				GET_FIELD(record.xo_last_exited_at),
				record.xo_last_exited_at);

			record.xo_accumulated_duration =
					readq_relaxed(prvdata->reg_base +
					(master_cnt * pdata->master_offset +
					offsetof(struct msm_rpm_master_stats,
					xo_accumulated_duration)));

			SNPRINTF(buf, count, "\t%s:0x%llX\n",
				GET_FIELD(record.xo_accumulated_duration),
				record.xo_accumulated_duration);

			record.last_sleep_transition_duration =
					readl_relaxed(prvdata->reg_base +
					(master_cnt * pdata->master_offset +
					offsetof(struct msm_rpm_master_stats,
					last_sleep_transition_duration)));

			SNPRINTF(buf, count, "\t%s:0x%x\n", GET_FIELD
				(record.last_sleep_transition_duration),
				record.last_sleep_transition_duration);

			record.last_wake_transition_duration =
					readl_relaxed(prvdata->reg_base +
					(master_cnt * pdata->master_offset +
					offsetof(struct msm_rpm_master_stats,
					last_wake_transition_duration)));

			SNPRINTF(buf, count, "\t%s:0x%x\n",
				GET_FIELD(record.last_wake_transition_duration),
				record.last_wake_transition_duration);

			record.xo_count =
					readl_relaxed(prvdata->reg_base +
					(master_cnt * pdata->master_offset +
					offsetof(struct msm_rpm_master_stats,
					xo_count)));

			SNPRINTF(buf, count, "\t%s:0x%x\n",
				GET_FIELD(record.xo_count),
				record.xo_count);
		} else {
			SNPRINTF(buf, count, "%s\n",
					GET_MASTER_NAME(master_cnt, prvdata));

			record.numshutdowns = readl_relaxed(prvdata->reg_base +
					(master_cnt * pdata->master_offset)
					+ 0x0);

			SNPRINTF(buf, count, "\t%s:0x%0x\n",
				GET_FIELD(record.numshutdowns),
				record.numshutdowns);
		}

		if (j == (BITS_PER_LONG - 1))
			SNPRINTF(buf, count, "\t\tcore%d\n", j);
	}

	return RPM_MASTERS_BUF_LEN - count;
}

static ssize_t msm_rpm_master_stats_file_read(struct file *file,
				char __user *bufu, size_t count, loff_t *ppos)
{
	struct msm_rpm_master_stats_private_data *prvdata;
	struct msm_rpm_master_stats_platform_data *pdata;
	ssize_t ret;

	mutex_lock(&msm_rpm_master_stats_mutex);
	prvdata = file->private_data;
	if (!prvdata) {
		ret = -EINVAL;
		goto exit;
	}

	pdata = prvdata->platform_data;
	if (!pdata) {
		ret = -EINVAL;
		goto exit;
	}

	if (!bufu || count == 0) {
		ret = -EINVAL;
		goto exit;
	}

	if (*ppos <= pdata->phys_size) {
		prvdata->len = msm_rpm_master_copy_stats(prvdata);
		*ppos = 0;
	}

	ret = simple_read_from_buffer(bufu, count, ppos,
			prvdata->buf, prvdata->len);
exit:
	mutex_unlock(&msm_rpm_master_stats_mutex);
	return ret;
}

static int msm_rpm_master_stats_file_open(struct inode *inode,
		struct file *file)
{
	struct msm_rpm_master_stats_private_data *prvdata;
	struct msm_rpm_master_stats_platform_data *pdata;
	int ret = 0;

	mutex_lock(&msm_rpm_master_stats_mutex);
	pdata = inode->i_private;

	file->private_data =
		kzalloc(sizeof(struct msm_rpm_master_stats_private_data),
			GFP_KERNEL);

	if (!file->private_data) {
		ret = -ENOMEM;
		goto exit;
	}

	prvdata = file->private_data;

	prvdata->reg_base = ioremap(pdata->phys_addr_base,
						pdata->phys_size);
	if (!prvdata->reg_base) {
		kfree(file->private_data);
		prvdata = NULL;
		pr_err("%s: ERROR could not ioremap start=%pa, len=%u\n",
			__func__, &pdata->phys_addr_base,
			pdata->phys_size);
		ret = -EBUSY;
		goto exit;
	}

	prvdata->len = 0;
	prvdata->num_masters = pdata->num_masters;
	prvdata->master_names = pdata->masters;
	prvdata->platform_data = pdata;
exit:
	mutex_unlock(&msm_rpm_master_stats_mutex);
	return ret;
}

static const struct file_operations msm_rpm_master_stats_fops = {
	.owner	  = THIS_MODULE,
	.open	  = msm_rpm_master_stats_file_open,
	.read	  = msm_rpm_master_stats_file_read,
	.release  = msm_rpm_master_stats_file_close,
	.llseek   = no_llseek,
};

static struct msm_rpm_master_stats_platform_data
			*msm_rpm_master_populate_pdata(struct device *dev)
{
	struct msm_rpm_master_stats_platform_data *pdata;
	struct device_node *node = dev->of_node;
	int rc = 0, i;

	pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		goto err;

	rc = of_property_read_u32(node, "qcom,master-stats-version",
							&pdata->version);
	if (rc) {
		dev_err(dev, "master-stats-version missing rc=%d\n", rc);
		goto err;
	}

	rc = of_property_read_u32(node, "qcom,master-offset",
							&pdata->master_offset);
	if (rc) {
		dev_err(dev, "master-offset missing rc=%d\n", rc);
		goto err;
	}

	pdata->num_masters = of_property_count_strings(node, "qcom,masters");
	if (pdata->num_masters < 0) {
		dev_err(dev, "Failed to get number of masters =%d\n",
						pdata->num_masters);
		goto err;
	}

	pdata->masters = devm_kzalloc(dev, sizeof(char *) * pdata->num_masters,
								GFP_KERNEL);
	if (!pdata->masters)
		goto err;

	/*
	 * Read master names from DT
	 */
	for (i = 0; i < pdata->num_masters; i++) {
		const char *master_name;
		size_t master_name_len;

		of_property_read_string_index(node, "qcom,masters",
							i, &master_name);
		master_name_len = strlen(master_name);
		pdata->masters[i] = devm_kzalloc(dev, sizeof(char) *
				master_name_len + 1, GFP_KERNEL);
		if (!pdata->masters[i])
			goto err;
		strlcpy(pdata->masters[i], master_name,
					master_name_len + 1);
	}
	return pdata;
err:
	return NULL;
}

ssize_t msm_rpm_master_stats_show(struct kobject *kobj,
					struct kobj_attribute *attr, char *buf)
{
	struct msm_rpm_master_stats_private_data *prvdata;
	int ret = 0;

	mutex_lock(&msm_rpm_master_stats_mutex);

	prvdata =
		kzalloc(sizeof(struct msm_rpm_master_stats_private_data),
			GFP_KERNEL);

	if (!prvdata) {
		ret = -EINVAL;
		goto exit;
	}

	if (!rpm_pdata) {
		ret = -EINVAL;
		goto exit;
	}

	prvdata->reg_base = ioremap(rpm_pdata->phys_addr_base,
						rpm_pdata->phys_size);
	if (!prvdata->reg_base) {
		kfree(prvdata);
		prvdata = NULL;
		pr_err("%s: ERROR could not ioremap start=%pa, len=%u\n",
			__func__, &rpm_pdata->phys_addr_base,
			rpm_pdata->phys_size);
		ret = -ENOMEM;
		goto exit;
	}

	prvdata->len = 0;
	prvdata->num_masters = rpm_pdata->num_masters;
	prvdata->master_names = rpm_pdata->masters;
	prvdata->platform_data = rpm_pdata;
	prvdata->len = msm_rpm_master_copy_complete_stats(prvdata);

	mutex_unlock(&msm_rpm_master_stats_mutex);
	return snprintf(buf, sizeof(prvdata->buf), "%s", prvdata->buf);
exit:
	mutex_unlock(&msm_rpm_master_stats_mutex);
	pr_err("[%s]:rpm master stats show fail.", __func__);
	return ret;
}

static  int msm_rpm_master_stats_probe(struct platform_device *pdev)
{
	struct dentry *dent;
	struct rpm_master_stats_prv_data *prvdata = NULL;
	struct kobject *rpm_master_stats_kobj = NULL;
	struct resource *res = NULL;
	int ret = -ENOMEM;

	if (!pdev)
		return -EINVAL;

	if (pdev->dev.of_node)
		rpm_pdata = msm_rpm_master_populate_pdata(&pdev->dev);
	else
		rpm_pdata = pdev->dev.platform_data;

	if (!rpm_pdata) {
		dev_err(&pdev->dev, "%s: Unable to get pdata\n", __func__);
		return -ENOMEM;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	if (!res) {
		dev_err(&pdev->dev,
			"%s: Failed to get IO resource from platform device\n",
			__func__);
		return -ENXIO;
	}

	rpm_pdata->phys_addr_base = res->start;
	rpm_pdata->phys_size = resource_size(res);

	if (IS_ENABLED(CONFIG_DEBUG_FS)) {
		dent = debugfs_create_file("rpm_master_stats",
			0444, NULL,
			rpm_pdata, &msm_rpm_master_stats_fops);

		if (!dent) {
			dev_err(&pdev->dev, "%s: ERROR debugfs_create_file failed\n",
					__func__);
			return -ENOMEM;
		}

		platform_set_drvdata(pdev, dent);
	}

	prvdata = devm_kzalloc(&pdev->dev, sizeof(*prvdata), GFP_KERNEL);
	if (!prvdata)
		return ret;
	if (rpm_master_stats_kobj == NULL) {
		rpm_master_stats_kobj = kobject_create_and_add("rpm",
								power_kobj);
		if (!rpm_master_stats_kobj)
			pr_err("[%s]:Init kobj failed.", __func__);
	}
	prvdata->kobj = rpm_master_stats_kobj;
	sysfs_attr_init(&prvdata->ka.attr);
	prvdata->ka.attr.mode = 0444;
	prvdata->ka.attr.name = "master_stats";
	prvdata->ka.show = msm_rpm_master_stats_show;
	prvdata->ka.store = NULL;
	ret = sysfs_create_file(prvdata->kobj, &prvdata->ka.attr);
	if (ret) {
		pr_err("sysfs_create_file failed\n");
		goto fail_sysfs;
	}

	return 0;

fail_sysfs:
	kobject_put(prvdata->kobj);
	return ret;
}

static int msm_rpm_master_stats_remove(struct platform_device *pdev)
{
	struct dentry *dent;
	struct rpm_master_stats_prv_data *prvdata = NULL;
	struct kobject *rpm_master_stats_kobj = NULL;

	prvdata->kobj = rpm_master_stats_kobj;
	sysfs_attr_init(&prvdata->ka.attr);
	prvdata->ka.attr.mode = 0444;
	prvdata->ka.attr.name = "master_stats";
	prvdata->ka.show = msm_rpm_master_stats_show;
	prvdata->ka.store = NULL;
	sysfs_remove_file(prvdata->kobj, &prvdata->ka.attr);
	kobject_put(prvdata->kobj);
	if (IS_ENABLED(CONFIG_DEBUG_FS)) {
		dent = platform_get_drvdata(pdev);
		debugfs_remove(dent);
		platform_set_drvdata(pdev, NULL);
	}
	return 0;
}

static const struct of_device_id rpm_master_table[] = {
	{.compatible = "qcom,rpm-master-stats"},
	{},
};

static struct platform_driver msm_rpm_master_stats_driver = {
	.probe	= msm_rpm_master_stats_probe,
	.remove = msm_rpm_master_stats_remove,
	.driver = {
		.name = "msm_rpm_master_stats",
		.of_match_table = rpm_master_table,
	},
};

static int __init msm_rpm_master_stats_init(void)
{
	return platform_driver_register(&msm_rpm_master_stats_driver);
}

static void __exit msm_rpm_master_stats_exit(void)
{
	platform_driver_unregister(&msm_rpm_master_stats_driver);
}

module_init(msm_rpm_master_stats_init);
module_exit(msm_rpm_master_stats_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("MSM RPM Master Statistics driver");
MODULE_ALIAS("platform:msm_master_stat_log");
