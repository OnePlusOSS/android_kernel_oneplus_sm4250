/**********************************************************************************
* Copyright (c)  2008-2015  Guangdong ONEPLUS Mobile Comm Corp., Ltd
* Description: Charger IC management module for charger system framework.
*                          Manage all charger IC and define abstarct function flow.
**
** Version: 1.0
** Date created: 21:03:46, 05/04/2012
** Author: Fuchun.Liao@BSP.CHG.Basic
**
** --------------------------- Revision History: ------------------------------------------------------------
* <version>           <date>                <author>                            <desc>
* Revision 1.0     2015-06-22        Fuchun.Liao@BSP.CHG.Basic         Created for new architecture from R9
* Revision 1.1     2018-04-12        Fanhong.Kong@BSP.CHG.Basic        Divided for swarp from oneplus_warp.c 
************************************************************************************************************/

#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#include "oneplus_charger.h"
#include "oneplus_warp.h"
#include "oneplus_gauge.h"
#include "oneplus_adapter.h"


extern int enable_charger_log;
#define adapter_xlog_printk(num, fmt, ...) \
	do { \
		if (enable_charger_log >= (int)num) { \
			printk(KERN_NOTICE pr_fmt("[ONEPLUS_CHG][%s]"fmt), __func__, ##__VA_ARGS__);\
	} \
} while (0)

static struct oneplus_adapter_chip *g_adapter_chip = NULL;

static void oneplus_adpater_awake_init(struct oneplus_adapter_chip *chip)
{
	if (!chip){
		return;
	}
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
	wake_lock_init(&chip->adapter_wake_lock, WAKE_LOCK_SUSPEND, "adpater_wake_lock");
#else
	chip->adapter_ws = wakeup_source_register(NULL, "adpater_wake_lock");
#endif
}

static void oneplus_adapter_set_awake(struct oneplus_adapter_chip *chip, bool awake)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
	if (awake){
		wake_lock(&chip->adapter_wake_lock);
	} else {
		wake_unlock(&chip->adapter_wake_lock);
	}
#else
	static bool pm_flag = false;

	if (!chip || !chip->adapter_ws){
		return;
	}
	if (awake && !pm_flag) {
		pm_flag = true;
		__pm_stay_awake(chip->adapter_ws);
	} else if (!awake && pm_flag) {
		__pm_relax(chip->adapter_ws);
		pm_flag = false;
	}
#endif
}


static void adapter_update_work_func(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oneplus_adapter_chip *chip
		= container_of(dwork, struct oneplus_adapter_chip, adapter_update_work);
	bool update_result = false;
	long tx_gpio = 0;
	long rx_gpio = 0;
	int i = 0;

	if (!chip) {
		chg_err("oneplus_adapter_chip NULL\n");
		return;
	}
	oneplus_adapter_set_awake(chip, true);
	tx_gpio = oneplus_warp_get_uart_tx();
	rx_gpio = oneplus_warp_get_uart_rx();
	adapter_xlog_printk(CHG_LOG_CRTI, " begin\n");
	oneplus_warp_uart_init();
	for (i = 0;i < 2;i++) {
		update_result = chip->vops->adapter_update(tx_gpio, rx_gpio);
		if (update_result == true) {
			break;
		}
		if (i < 1) {
			msleep(1650);
		}
	}
	if (update_result) {
		oneplus_warp_set_adapter_update_real_status(ADAPTER_FW_UPDATE_SUCCESS);
	} else {
		oneplus_warp_set_adapter_update_real_status(ADAPTER_FW_UPDATE_FAIL);
		oneplus_warp_set_adapter_update_report_status(ADAPTER_FW_UPDATE_FAIL);
	}
	msleep(20);
	oneplus_warp_uart_reset();
	if (update_result) {
		msleep(2000);
		oneplus_warp_set_adapter_update_report_status(ADAPTER_FW_UPDATE_SUCCESS);
	}
	oneplus_warp_battery_update();
	adapter_xlog_printk(CHG_LOG_CRTI, "  end update_result:%d\n", update_result);
	oneplus_adapter_set_awake(chip, false);
}

#define ADAPTER_UPDATE_DELAY		1400
void oneplus_adapter_fw_update(void)
{
	struct oneplus_adapter_chip *chip = g_adapter_chip ;
	adapter_xlog_printk(CHG_LOG_CRTI, " call \n");
	/*schedule_delayed_work_on(7, &chip->adapter_update_work, */
	/*		        round_jiffies_relative(msecs_to_jiffies(ADAPTER_UPDATE_DELAY)));*/
	schedule_delayed_work(&chip->adapter_update_work,
	round_jiffies_relative(msecs_to_jiffies(ADAPTER_UPDATE_DELAY)));
}


void oneplus_adapter_init(struct oneplus_adapter_chip *chip)
{
	g_adapter_chip = chip;
	oneplus_adpater_awake_init(chip);
	INIT_DELAYED_WORK(&chip->adapter_update_work, adapter_update_work_func);
}

bool oneplus_adapter_check_chip_is_null(void)
{
	if (!g_adapter_chip) {
		return true;
	} else {
		return false;
	}
}

