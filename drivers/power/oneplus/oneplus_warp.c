/**********************************************************************************
* Copyright (c)  2008-2015  Guangdong ONEPLUS Mobile Comm Corp., Ltd
* Description: Charger IC management module for charger system framework.
*              Manage all charger IC and define abstarct function flow.
* Version    : 1.0
* Date       : 2015-06-22
* Author     : fanhui@PhoneSW.BSP
*            : Fanhong.Kong@ProDrv.CHG
* ------------------------------ Revision History: --------------------------------
* <version>           <date>                <author>                       <desc>
* Revision 1.0       2015-06-22        fanhui@PhoneSW.BSP            Created for new architecture
* Revision 1.0       2015-06-22        Fanhong.Kong@ProDrv.CHG       Created for new architecture
* Revision 2.0       2018-04-14        Fanhong.Kong@ProDrv.CHG       Upgrade for SWARP
***********************************************************************************/
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#include "oneplus_charger.h"
#include "oneplus_warp.h"
#include "oneplus_gauge.h"
#include "oneplus_adapter.h"

#define WARP_NOTIFY_FAST_PRESENT			0x52
#define WARP_NOTIFY_FAST_ABSENT				0x54
#define WARP_NOTIFY_ALLOW_READING_IIC		0x58
#define WARP_NOTIFY_NORMAL_TEMP_FULL		0x5a
#define WARP_NOTIFY_LOW_TEMP_FULL			0x53
#define WARP_NOTIFY_FIRMWARE_UPDATE			0x56
#define WARP_NOTIFY_BAD_CONNECTED			0x59
#define WARP_NOTIFY_TEMP_OVER				0x5c
#define WARP_NOTIFY_ADAPTER_FW_UPDATE		0x5b
#define WARP_NOTIFY_BTB_TEMP_OVER			0x5d
#define WARP_NOTIFY_ADAPTER_MODEL_FACTORY	0x5e

extern int charger_abnormal_log;
extern int enable_charger_log;
#define warp_xlog_printk(num, fmt, ...) \
	do { \
		if (enable_charger_log >= (int)num) { \
			printk(KERN_NOTICE pr_fmt("[ONEPLUS_CHG][%s]"fmt), __func__, ##__VA_ARGS__);\
	} \
} while (0)


static struct oneplus_warp_chip *g_warp_chip = NULL;
bool __attribute__((weak)) oneplus_get_fg_i2c_err_occured(void)
{
	return false;
}

void __attribute__((weak)) oneplus_set_fg_i2c_err_occured(bool i2c_err)
{
	return;
}
int __attribute__((weak)) request_firmware_select(const struct firmware **firmware_p,
		const char *name, struct device *device)
{
	return 1;
}
//int __attribute__((weak)) register_devinfo(char *name, struct manufacture_info *info)//debug for bring up 
//{
//	return 1;
//}

void oneplus_warp_battery_update()
{
	struct oneplus_warp_chip *chip = g_warp_chip;
/*
		if (!chip) {
			chg_err("  g_warp_chip is NULL\n");
			return ;
		}
*/
	if (!chip->batt_psy) {
		chip->batt_psy = power_supply_get_by_name("battery");
	}
	if (chip->batt_psy) {
		power_supply_changed(chip->batt_psy);
	}
}

void oneplus_warp_switch_mode(int mode)
{
	if (!g_warp_chip) {
		chg_err("  g_warp_chip is NULL\n");
	} else {
		g_warp_chip->vops->set_switch_mode(g_warp_chip, mode);
	}
}

static void oneplus_warp_awake_init(struct oneplus_warp_chip *chip)
{
	if (!chip) {
		return;
	}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
	wake_lock_init(&chip->warp_wake_lock, WAKE_LOCK_SUSPEND, "warp_wake_lock");
#else
	chip->warp_ws = wakeup_source_register(NULL, "warp_wake_lock");
#endif
}

static void oneplus_warp_set_awake(struct oneplus_warp_chip *chip, bool awake)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
	if (awake) {
		wake_lock(&chip->warp_wake_lock);
	} else {
		wake_unlock(&chip->warp_wake_lock);
	}
#else
	static bool pm_flag = false;

	if (!chip || !chip->warp_ws) {
		return;
	}
	if (awake && !pm_flag) {
		pm_flag = true;
		__pm_stay_awake(chip->warp_ws);
	} else if (!awake && pm_flag) {
		__pm_relax(chip->warp_ws);
		pm_flag = false;
	}
#endif
}
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 0))
static void oneplus_warp_watchdog(unsigned long data)
#else
static void oneplus_warp_watchdog(struct timer_list *unused)
#endif
{
        struct oneplus_warp_chip *chip = g_warp_chip;

        if (!chip) {
                chg_err(" g_warp_chip is NULL\n");
                return;
        }

	chg_err("watchdog bark: cannot receive mcu data\n");
	chip->allow_reading = true;
	chip->fastchg_started = false;
	chip->fastchg_ing = false;
	chip->fastchg_to_normal = false;
	chip->fastchg_to_warm = false;
	chip->fastchg_low_temp_full = false;
	chip->btb_temp_over = false;
	chip->fast_chg_type = FASTCHG_CHARGER_TYPE_UNKOWN;
	charger_abnormal_log = CRITICAL_LOG_WARP_WATCHDOG;
	schedule_work(&chip->warp_watchdog_work);
}

static void oneplus_warp_init_watchdog_timer(struct oneplus_warp_chip *chip)
{
    if (!chip) {
        chg_err("oneplus_warp_chip is not ready\n");
        return;
    }
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 0))
    init_timer(&chip->watchdog);
    chip->watchdog.data = (unsigned long)chip;
    chip->watchdog.function = oneplus_warp_watchdog;
#else
    timer_setup(&chip->watchdog, oneplus_warp_watchdog, 0);
#endif
}

static void oneplus_warp_del_watchdog_timer(struct oneplus_warp_chip *chip)
{
    if (!chip) {
        chg_err("oneplus_warp_chip is not ready\n");
        return;
    }
    del_timer(&chip->watchdog);
}

static void oneplus_warp_setup_watchdog_timer(struct oneplus_warp_chip *chip, unsigned int ms)
{
    if (!chip) {
        chg_err("oneplus_warp_chip is not ready\n");
        return;
    }
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 0))
    mod_timer(&chip->watchdog, jiffies+msecs_to_jiffies(25000));
#else
    del_timer(&chip->watchdog);
    chip->watchdog.expires  = jiffies + msecs_to_jiffies(ms);
    add_timer(&chip->watchdog);
#endif
}

static void check_charger_out_work_func(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oneplus_warp_chip *chip = container_of(dwork, struct oneplus_warp_chip, check_charger_out_work);
	int chg_vol = 0;

	chg_vol = oneplus_chg_get_charger_voltage();
	if (chg_vol >= 0 && chg_vol < 2000) {
		chip->vops->reset_fastchg_after_usbout(chip);
		oneplus_chg_clear_chargerid_info();
		oneplus_warp_battery_update();
		warp_xlog_printk(CHG_LOG_CRTI, "charger out, chg_vol:%d\n", chg_vol);
	}
}

static void warp_watchdog_work_func(struct work_struct *work)
{
	struct oneplus_warp_chip *chip = container_of(work,
		struct oneplus_warp_chip, warp_watchdog_work);

	oneplus_chg_set_chargerid_switch_val(0);
	oneplus_chg_clear_chargerid_info();
	chip->vops->set_switch_mode(chip, NORMAL_CHARGER_MODE);
	oneplus_chg_set_charger_type_unknown();
	oneplus_warp_set_awake(chip, false);
}

static void oneplus_warp_check_charger_out(struct oneplus_warp_chip *chip)
{
	warp_xlog_printk(CHG_LOG_CRTI, "  call\n");
	schedule_delayed_work(&chip->check_charger_out_work,
		round_jiffies_relative(msecs_to_jiffies(3000)));
}

static int oneplus_warp_set_current_when_bleow_setting_batt_temp
		(struct oneplus_warp_chip *chip, int vbat_temp_cur)
{
	int ret = 0;

	switch (chip->fastchg_batt_temp_status) {
		case BAT_TEMP_NATURAL:
			if (vbat_temp_cur > chip->warp_strategy1_batt_up_temp3) {
				chip->fastchg_batt_temp_status = BAT_TEMP_HIGH0;
				ret = chip->warp_strategy1_high0_current;
			} else {
				chip->fastchg_batt_temp_status = BAT_TEMP_NATURAL;
				ret = chip->warp_strategy_normal_current;
			}
			break;
		case BAT_TEMP_HIGH0:
			if (vbat_temp_cur > chip->warp_strategy1_batt_up_temp4) {
				chip->fastchg_batt_temp_status = BAT_TEMP_HIGH1;
				ret = chip->warp_strategy1_high1_current;
			} else {
				chip->fastchg_batt_temp_status = BAT_TEMP_HIGH0;
				ret = chip->warp_strategy1_high0_current;
			}
			break;
		case BAT_TEMP_HIGH1:
			if (vbat_temp_cur > chip->warp_strategy1_batt_up_temp5) {
				chip->fastchg_batt_temp_status = BAT_TEMP_HIGH2;
				ret = chip->warp_strategy1_high2_current;
			} else if (vbat_temp_cur < chip->warp_strategy1_batt_up_down_temp1) {
				chip->fastchg_batt_temp_status = BAT_TEMP_HIGH0;
				ret = chip->warp_strategy1_high0_current;
			} else {
				chip->fastchg_batt_temp_status = BAT_TEMP_HIGH1;
				ret = chip->warp_strategy1_high1_current;
			}
			break;
		case BAT_TEMP_HIGH2:
			if (vbat_temp_cur < chip->warp_strategy1_batt_up_down_temp2) {
				chip->fastchg_batt_temp_status = BAT_TEMP_HIGH1;
				ret = chip->warp_strategy1_high1_current;;
			} else {
				chip->fastchg_batt_temp_status = BAT_TEMP_HIGH2;
				ret = chip->warp_strategy1_high2_current;;
			}
			break;
		default:
			break;
	}
	warp_xlog_printk(CHG_LOG_CRTI, "the ret: %d, the temp =%d\r\n", ret, vbat_temp_cur);
	return ret;
}

static int oneplus_warp_set_current_when_up_setting_batt_temp(struct oneplus_warp_chip *chip, int vbat_temp_cur){
	int ret = 0;
	switch (chip->fastchg_batt_temp_status) {
		case BAT_TEMP_NATURAL:
			if (vbat_temp_cur > chip->warp_strategy2_batt_up_temp1) {
				chip->fastchg_batt_temp_status = BAT_TEMP_HIGH0;
				ret = chip->warp_strategy2_high0_current;
			} else {
				chip->fastchg_batt_temp_status = BAT_TEMP_NATURAL;
				ret = chip->warp_strategy_normal_current;
			}
			break;
		case BAT_TEMP_HIGH0:
			if (vbat_temp_cur > chip->warp_strategy2_batt_up_temp3) {
				chip->fastchg_batt_temp_status = BAT_TEMP_HIGH1;
				ret = chip->warp_strategy2_high1_current;
			} else {
				chip->fastchg_batt_temp_status = BAT_TEMP_HIGH0;
				ret = chip->warp_strategy2_high0_current;;
			}
			break;
		case BAT_TEMP_HIGH1:
			if (vbat_temp_cur > chip->warp_strategy2_batt_up_temp5) {
				chip->fastchg_batt_temp_status = BAT_TEMP_HIGH2;
				ret = chip->warp_strategy2_high2_current;
			} else {
				chip->fastchg_batt_temp_status = BAT_TEMP_HIGH1;
				ret = chip->warp_strategy2_high1_current;
			}
			break;
		case BAT_TEMP_HIGH2:
			if (vbat_temp_cur > chip->warp_strategy2_batt_up_temp6) {
				chip->fastchg_batt_temp_status = BAT_TEMP_HIGH3;
				ret = chip->warp_strategy2_high3_current;
			} else if (vbat_temp_cur < chip->warp_strategy2_batt_up_down_temp2){
				chip->fastchg_batt_temp_status = BAT_TEMP_HIGH1;
				ret = chip->warp_strategy2_high1_current;
			} else {
				chip->fastchg_batt_temp_status = BAT_TEMP_HIGH2;
				ret = chip->warp_strategy2_high2_current;
			}
			break;
		case BAT_TEMP_HIGH3:
			if (vbat_temp_cur < chip->warp_strategy2_batt_up_down_temp4) {
				chip->fastchg_batt_temp_status = BAT_TEMP_HIGH2;
				ret = chip->warp_strategy2_high2_current;
			} else {
				chip->fastchg_batt_temp_status = BAT_TEMP_HIGH3;
				ret = chip->warp_strategy2_high3_current;
			}
			break;
		default:
			break;
	}
	warp_xlog_printk(CHG_LOG_CRTI, "the ret: %d, the temp =%d\r\n", ret, vbat_temp_cur);
	return ret;
}


static void oneplus_warp_fastchg_func(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oneplus_warp_chip *chip = container_of(dwork, struct oneplus_warp_chip, fastchg_work);
	int i = 0;
	int bit = 0;
	int data = 0;
	int ret_info = 0;
	int ret_info_temp = 0;
	static int pre_ret_info = 0;
	static int select_func_flag = 0;
	static bool first_detect_batt_temp = false;
	static bool isnot_power_on = true;
	static bool fw_ver_info = false;
	static bool adapter_fw_ver_info = false;
	static bool data_err = false;
	static bool adapter_model_factory = false;
	int volt = 0;
	int temp = 0;
	int soc = 0;
	int current_now = 0;
	int chg_vol = 0;
	int remain_cap = 0;
	static bool phone_mcu_updated = false;
	static bool normalchg_disabled = false;
/*
	if (!g_adapter_chip) {
		chg_err(" g_adapter_chip NULL\n");
		return;
	}
*/
	usleep_range(2000, 2000);
	if (chip->vops->get_gpio_ap_data(chip) != 1) {
		/*warp_xlog_printk(CHG_LOG_CRTI, "  Shield fastchg irq, return\r\n");*/
		return;
	}

	chip->vops->eint_unregist(chip);
	for (i = 0; i < 7; i++) {
		bit = chip->vops->read_ap_data(chip);
		data |= bit << (6-i);
		if ((i == 2) && (data != 0x50) && (!fw_ver_info)
				&& (!adapter_fw_ver_info) && (!adapter_model_factory)) {	/*data recvd not start from "101"*/
			warp_xlog_printk(CHG_LOG_CRTI, "  data err:0x%x\n", data);
			if (chip->fastchg_started == true) {
				chip->allow_reading = true;
				chip->fastchg_started = false;
				chip->fastchg_to_normal = false;
				chip->fastchg_to_warm = false;
				chip->fastchg_ing = false;
				adapter_fw_ver_info = false;
				/*chip->adapter_update_real = ADAPTER_FW_UPDATE_NONE;*/
				/*chip->adapter_update_report = chip->adapter_update_real;*/
				chip->btb_temp_over = false;
				oneplus_set_fg_i2c_err_occured(false);
				oneplus_chg_set_chargerid_switch_val(0);
				chip->vops->set_switch_mode(chip, NORMAL_CHARGER_MODE);
				data_err = true;
				if (chip->fastchg_dummy_started) {
					chg_vol = oneplus_chg_get_charger_voltage();
					if (chg_vol >= 0 && chg_vol < 2000) {
						chip->fastchg_dummy_started = false;
						oneplus_chg_clear_chargerid_info();
						warp_xlog_printk(CHG_LOG_CRTI,
							"chg_vol:%d dummy_started:false\n", chg_vol);
					}
				} else {
					oneplus_chg_clear_chargerid_info();
				}
				///del_timer(&chip->watchdog);
				oneplus_warp_del_watchdog_timer(chip);
			}
			oneplus_warp_set_awake(chip, false);
			goto out;
		}
	}
	warp_xlog_printk(CHG_LOG_CRTI, " recv data:0x%x, ap:0x%x, mcu:0x%x\n",
		data, chip->fw_data_version, chip->fw_mcu_version);

	if (data == WARP_NOTIFY_FAST_PRESENT) {
		oneplus_warp_set_awake(chip, true);
		oneplus_set_fg_i2c_err_occured(false);
		chip->need_to_up = 0;
		fw_ver_info = false;
		pre_ret_info = 0x06;
		adapter_fw_ver_info = false;
		adapter_model_factory = false;
		data_err = false;
		phone_mcu_updated = false;
		normalchg_disabled = false;
		first_detect_batt_temp = true;
		chip->fastchg_batt_temp_status = BAT_TEMP_NATURAL;
		if (chip->adapter_update_real == ADAPTER_FW_UPDATE_FAIL) {
			chip->adapter_update_real = ADAPTER_FW_UPDATE_NONE;
			chip->adapter_update_report = chip->adapter_update_real;
		}
		if (oneplus_warp_get_fastchg_allow() == true) {
			oneplus_chg_set_input_current_without_aicl(1200);
			chip->allow_reading = false;
			chip->fastchg_started = true;
			chip->fastchg_ing = false;
			chip->fastchg_dummy_started = false;
			chip->fastchg_to_warm = false;
			chip->btb_temp_over = false;
		} else {
			chip->allow_reading = true;
			chip->fastchg_dummy_started = true;
			chip->fastchg_started = false;
			chip->fastchg_to_normal = false;
			chip->fastchg_to_warm = false;
			chip->fastchg_ing = false;
			chip->btb_temp_over = false;
			oneplus_chg_set_chargerid_switch_val(0);
			chip->vops->set_switch_mode(chip, NORMAL_CHARGER_MODE);
		}
		//mod_timer(&chip->watchdog, jiffies+msecs_to_jiffies(25000));
		oneplus_warp_setup_watchdog_timer(chip, 25000);
		if (!isnot_power_on) {
			isnot_power_on = true;
			ret_info = 0x1;
		} else {
			ret_info = 0x2;
		}
	} else if (data == WARP_NOTIFY_FAST_ABSENT) {
		chip->allow_reading = true;
		chip->fastchg_started = false;
		chip->fastchg_to_normal = false;
		chip->fastchg_to_warm = false;
		chip->fastchg_ing = false;
		chip->btb_temp_over = false;
		adapter_fw_ver_info = false;
		adapter_model_factory = false;
		oneplus_set_fg_i2c_err_occured(false);
		if (chip->fastchg_dummy_started) {
			chg_vol = oneplus_chg_get_charger_voltage();
			if (chg_vol >= 0 && chg_vol < 2000) {
				chip->fastchg_dummy_started = false;
				chip->fast_chg_type = FASTCHG_CHARGER_TYPE_UNKOWN;
				oneplus_chg_clear_chargerid_info();
				warp_xlog_printk(CHG_LOG_CRTI,
					"chg_vol:%d dummy_started:false\n", chg_vol);
			}
		} else {
			chip->fast_chg_type = FASTCHG_CHARGER_TYPE_UNKOWN;
			oneplus_chg_clear_chargerid_info();
		}
		warp_xlog_printk(CHG_LOG_CRTI,
			"fastchg stop unexpectly, switch off fastchg\n");
		oneplus_chg_set_chargerid_switch_val(0);
		chip->vops->set_switch_mode(chip, NORMAL_CHARGER_MODE);
		//del_timer(&chip->watchdog);
		oneplus_warp_del_watchdog_timer(chip);
		ret_info = 0x2;
	} else if (data == WARP_NOTIFY_ADAPTER_MODEL_FACTORY) {
		warp_xlog_printk(CHG_LOG_CRTI, " WARP_NOTIFY_ADAPTER_MODEL_FACTORY!\n");
		/*ready to get adapter_model_factory*/
		adapter_model_factory = 1;
		ret_info = 0x2;
	} else if (adapter_model_factory) {
		warp_xlog_printk(CHG_LOG_CRTI, "WARP_NOTIFY_ADAPTER_MODEL_FACTORY:0x%x, \n", data);
		//chip->fast_chg_type = data;
		if (data == 0) {
			chip->fast_chg_type = CHARGER_SUBTYPE_FASTCHG_WARP;
		} else {
			chip->fast_chg_type = data;
		}
		adapter_model_factory = 0;
		if (chip->fast_chg_type == 0x0F
				|| chip->fast_chg_type == 0x1F
				|| chip->fast_chg_type == 0x3F
				|| chip->fast_chg_type == 0x7F) {
			chip->allow_reading = true;
			chip->fastchg_started = false;
			chip->fastchg_to_normal = false;
			chip->fastchg_to_warm = false;
			chip->fastchg_ing = false;
			chip->btb_temp_over = false;
			adapter_fw_ver_info = false;
			oneplus_set_fg_i2c_err_occured(false);
			oneplus_chg_set_chargerid_switch_val(0);
			chip->vops->set_switch_mode(chip, NORMAL_CHARGER_MODE);
			data_err = true;
		}
	ret_info = 0x2;
	} else if (data == WARP_NOTIFY_ALLOW_READING_IIC) {
		chip->fastchg_ing = true;
		chip->allow_reading = true;
		adapter_fw_ver_info = false;
		adapter_model_factory = false;
		soc = oneplus_gauge_get_batt_soc();
		if (oneplus_get_fg_i2c_err_occured() == false) {
			volt = oneplus_gauge_get_batt_mvolts();
		}
		if (oneplus_get_fg_i2c_err_occured() == false) {
			temp = oneplus_gauge_get_batt_temperature();
		}
		if (oneplus_get_fg_i2c_err_occured() == false) {
			current_now = oneplus_gauge_get_batt_current();
		}
		if (oneplus_get_fg_i2c_err_occured() == false) {
			remain_cap = oneplus_gauge_get_remaining_capacity();
		}
		oneplus_chg_kick_wdt();
		if (chip->support_warp_by_normal_charger_path) {//65w
			if(!normalchg_disabled && chip->fast_chg_type != FASTCHG_CHARGER_TYPE_UNKOWN
				&& chip->fast_chg_type != CHARGER_SUBTYPE_FASTCHG_WARP) {
				oneplus_chg_disable_charge();
				normalchg_disabled = true;
			}
		} else {
			if(!normalchg_disabled) {
				oneplus_chg_disable_charge();
				normalchg_disabled = true;
			}
		}
		//don't read
		chip->allow_reading = false;
		warp_xlog_printk(CHG_LOG_CRTI, " volt:%d,temp:%d,soc:%d,current_now:%d,rm:%d, i2c_err:%d\n",
			volt, temp, soc, current_now, remain_cap, oneplus_get_fg_i2c_err_occured());
		//mod_timer(&chip->watchdog, jiffies+msecs_to_jiffies(25000));
		oneplus_warp_setup_watchdog_timer(chip, 25000);
		if (chip->disable_adapter_output == true) {
			ret_info = (chip->warp_multistep_adjust_current_support
				&& (!(chip->support_warp_by_normal_charger_path
				&& chip->fast_chg_type == CHARGER_SUBTYPE_FASTCHG_WARP)))
				? 0x07 : 0x03;
		} else if (chip->set_warp_current_limit == WARP_MAX_CURRENT_LIMIT_2A
				|| (!(chip->support_warp_by_normal_charger_path
				&& chip->fast_chg_type == CHARGER_SUBTYPE_FASTCHG_WARP)
				&& oneplus_chg_get_cool_down_status() == 1)) {
			ret_info = (chip->warp_multistep_adjust_current_support
				&& (!(chip->support_warp_by_normal_charger_path
				&& chip->fast_chg_type == CHARGER_SUBTYPE_FASTCHG_WARP)))
				? 0x03 : 0x01;
		} else {
			ret_info = (chip->warp_multistep_adjust_current_support
				&& (!(chip->support_warp_by_normal_charger_path
				&&  chip->fast_chg_type == CHARGER_SUBTYPE_FASTCHG_WARP)))
				? 0x06 : 0x02;
		}

		if (chip->warp_multistep_adjust_current_support
				&& chip->disable_adapter_output == false
				&& (!(chip->support_warp_by_normal_charger_path
				&&  chip->fast_chg_type == CHARGER_SUBTYPE_FASTCHG_WARP))) {
			if (first_detect_batt_temp) {
				if (temp < chip->warp_multistep_initial_batt_temp) {
					select_func_flag = 1;
				} else {
					select_func_flag = 2;
				}
				first_detect_batt_temp = false;
			}
			if (select_func_flag == 1) {
				ret_info_temp = oneplus_warp_set_current_when_bleow_setting_batt_temp(chip, temp);
				ret_info = ret_info <= ret_info_temp ? ret_info : ret_info_temp;
			} else {
				ret_info_temp = oneplus_warp_set_current_when_up_setting_batt_temp(chip, temp);
				ret_info = ret_info <= ret_info_temp ? ret_info : ret_info_temp;
			}
		}

		if ((chip->warp_multistep_adjust_current_support == true) && (soc > 75)) {
			ret_info = ret_info <= pre_ret_info ? ret_info : pre_ret_info;
		}
		pre_ret_info = ret_info;

		warp_xlog_printk(CHG_LOG_CRTI, " volt:%d,temp:%d,soc:%d,current_now:%d,rm:%d, i2c_err:%d, ret_info:%d\n",
			volt, temp, soc, current_now, remain_cap, oneplus_get_fg_i2c_err_occured(), ret_info);
	} else if (data == WARP_NOTIFY_NORMAL_TEMP_FULL) {
		warp_xlog_printk(CHG_LOG_CRTI, "WARP_NOTIFY_NORMAL_TEMP_FULL\r\n");
		oneplus_chg_set_chargerid_switch_val(0);
		chip->vops->set_switch_mode(chip, NORMAL_CHARGER_MODE);
		//del_timer(&chip->watchdog);
		oneplus_warp_del_watchdog_timer(chip);
		ret_info = 0x2;
	} else if (data == WARP_NOTIFY_LOW_TEMP_FULL) {
		warp_xlog_printk(CHG_LOG_CRTI,
			" fastchg low temp full, switch NORMAL_CHARGER_MODE\n");
		oneplus_chg_set_chargerid_switch_val(0);
		chip->vops->set_switch_mode(chip, NORMAL_CHARGER_MODE);
		//del_timer(&chip->watchdog);
		oneplus_warp_del_watchdog_timer(chip);
		ret_info = 0x2;
	} else if (data == WARP_NOTIFY_BAD_CONNECTED) {
		warp_xlog_printk(CHG_LOG_CRTI,
			" fastchg bad connected, switch NORMAL_CHARGER_MODE\n");
		/*usb bad connected, stop fastchg*/
		chip->btb_temp_over = false;	/*to switch to normal mode*/
		oneplus_chg_set_chargerid_switch_val(0);
		chip->vops->set_switch_mode(chip, NORMAL_CHARGER_MODE);
		//del_timer(&chip->watchdog);
		oneplus_warp_del_watchdog_timer(chip);
		ret_info = 0x2;
		charger_abnormal_log = CRITICAL_LOG_WARP_BAD_CONNECTED;
	} else if (data == WARP_NOTIFY_TEMP_OVER) {
		/*fastchg temp over 45 or under 20*/
		warp_xlog_printk(CHG_LOG_CRTI,
			" fastchg temp > 45 or < 20, switch NORMAL_CHARGER_MODE\n");
		oneplus_chg_set_chargerid_switch_val(0);
		chip->vops->set_switch_mode(chip, NORMAL_CHARGER_MODE);
		//del_timer(&chip->watchdog);
		oneplus_warp_del_watchdog_timer(chip);
		ret_info = 0x2;
	} else if (data == WARP_NOTIFY_BTB_TEMP_OVER) {
		warp_xlog_printk(CHG_LOG_CRTI, "  btb_temp_over\n");
		chip->fastchg_ing = false;
		chip->btb_temp_over = true;
		chip->fastchg_dummy_started = false;
		chip->fastchg_started = false;
		chip->fastchg_to_normal = false;
		chip->fastchg_to_warm = false;
		adapter_fw_ver_info = false;
		adapter_model_factory = false;
		//mod_timer(&chip->watchdog, jiffies + msecs_to_jiffies(25000));
		oneplus_warp_setup_watchdog_timer(chip, 25000);
		ret_info = 0x2;
		charger_abnormal_log = CRITICAL_LOG_WARP_BTB;
	} else if (data == WARP_NOTIFY_FIRMWARE_UPDATE) {
		warp_xlog_printk(CHG_LOG_CRTI, " firmware update, get fw_ver ready!\n");
		/*ready to get fw_ver*/
		fw_ver_info = 1;
		ret_info = 0x2;
	} else if (fw_ver_info && chip->firmware_data != NULL) {
		/*get fw_ver*/
		/*fw in local is large than mcu1503_fw_ver*/
		if (!chip->have_updated
				&& chip->firmware_data[chip->fw_data_count - 4] != data) {
			ret_info = 0x2;
			chip->need_to_up = 1;	/*need to update fw*/
			isnot_power_on = false;
		} else {
			ret_info = 0x1;
			chip->need_to_up = 0;	/*fw is already new, needn't to up*/
			adapter_fw_ver_info = true;
		}
		warp_xlog_printk(CHG_LOG_CRTI, "local_fw:0x%x, need_to_up_fw:%d\n",
			chip->firmware_data[chip->fw_data_count - 4], chip->need_to_up);
		fw_ver_info = 0;
	} else if (adapter_fw_ver_info) {
#if 0
		if (g_adapter_chip->adapter_firmware_data[g_adapter_chip->adapter_fw_data_count - 4] > data
			&& (oneplus_gauge_get_batt_soc() > 2) && (chip->vops->is_power_off_charging(chip) == false)
			&& (chip->adapter_update_real != ADAPTER_FW_UPDATE_SUCCESS)) {
#else
		if (0) {
#endif
			ret_info = 0x02;
			chip->adapter_update_real = ADAPTER_FW_NEED_UPDATE;
			chip->adapter_update_report = chip->adapter_update_real;
		} else {
			ret_info = 0x01;
			chip->adapter_update_real = ADAPTER_FW_UPDATE_NONE;
			chip->adapter_update_report = chip->adapter_update_real;
		}
		adapter_fw_ver_info = false;
		//mod_timer(&chip->watchdog, jiffies + msecs_to_jiffies(25000));
		oneplus_warp_setup_watchdog_timer(chip, 25000);
	} else if (data == WARP_NOTIFY_ADAPTER_FW_UPDATE) {
		oneplus_warp_set_awake(chip, true);
		ret_info = 0x02;
		chip->adapter_update_real = ADAPTER_FW_NEED_UPDATE;
		chip->adapter_update_report = chip->adapter_update_real;
		//mod_timer(&chip->watchdog,  jiffies + msecs_to_jiffies(25000));
		oneplus_warp_setup_watchdog_timer(chip, 25000);
	} else {
		oneplus_chg_set_chargerid_switch_val(0);
		oneplus_chg_clear_chargerid_info();
		chip->vops->set_switch_mode(chip, NORMAL_CHARGER_MODE);
		chip->vops->reset_mcu(chip);
		msleep(100);	/*avoid i2c conflict*/
		chip->allow_reading = true;
		chip->fastchg_started = false;
		chip->fastchg_to_normal = false;
		chip->fastchg_to_warm = false;
		chip->fastchg_ing = false;
		chip->btb_temp_over = false;
		adapter_fw_ver_info = false;
		adapter_model_factory = false;
		data_err = true;
		warp_xlog_printk(CHG_LOG_CRTI,
			" data err, set 0x101, data=0x%x switch off fastchg\n", data);
		goto out;
	}
	msleep(2);
	chip->vops->set_data_sleep(chip);
	if (chip->warp_multistep_adjust_current_support == false) {
		chip->vops->reply_mcu_data(chip, ret_info,
			oneplus_gauge_get_device_type_for_warp());
	} else {
		chip->vops->reply_mcu_data_4bits(chip, ret_info,
			oneplus_gauge_get_device_type_for_warp());
	}

out:
	chip->vops->set_data_active(chip);
	chip->vops->set_clock_active(chip);
	usleep_range(10000, 10000);
	chip->vops->set_clock_sleep(chip);
	usleep_range(25000, 25000);
	if (data == WARP_NOTIFY_NORMAL_TEMP_FULL || data == WARP_NOTIFY_BAD_CONNECTED) {
		usleep_range(350000, 350000);
		chip->allow_reading = true;
		chip->fastchg_ing = false;
		chip->fastchg_to_normal = true;
		chip->fastchg_started = false;
		chip->fastchg_to_warm = false;
		if (data == WARP_NOTIFY_BAD_CONNECTED)
			charger_abnormal_log = CRITICAL_LOG_WARP_BAD_CONNECTED;
	} else if (data == WARP_NOTIFY_LOW_TEMP_FULL) {
		usleep_range(350000, 350000);
		chip->allow_reading = true;
		chip->fastchg_ing = false;
		chip->fastchg_low_temp_full = true;
		chip->fastchg_to_normal = false;
		chip->fastchg_started = false;
		chip->fastchg_to_warm = false;
	} else if (data == WARP_NOTIFY_TEMP_OVER) {
		usleep_range(350000, 350000);
		chip->fastchg_ing = false;
		chip->fastchg_to_warm = true;
		chip->allow_reading = true;
		chip->fastchg_low_temp_full = false;
		chip->fastchg_to_normal = false;
		chip->fastchg_started = false;
	}
	if (chip->need_to_up) {
		msleep(500);
		//del_timer(&chip->watchdog);
		chip->vops->fw_update(chip);
		chip->need_to_up = 0;
		phone_mcu_updated = true;
		//mod_timer(&chip->watchdog, jiffies + msecs_to_jiffies(25000));
		oneplus_warp_setup_watchdog_timer(chip, 25000);
	}
	if ((data == WARP_NOTIFY_FAST_ABSENT || (data_err && !phone_mcu_updated)
			|| data == WARP_NOTIFY_BTB_TEMP_OVER)
			&& (chip->fastchg_dummy_started == false)) {
		oneplus_chg_set_charger_type_unknown();
		oneplus_chg_wake_update_work();
	} else if (data == WARP_NOTIFY_NORMAL_TEMP_FULL
			|| data == WARP_NOTIFY_TEMP_OVER
			|| data == WARP_NOTIFY_BAD_CONNECTED
			|| data == WARP_NOTIFY_LOW_TEMP_FULL) {
		oneplus_chg_set_charger_type_unknown();
		oneplus_warp_check_charger_out(chip);
	} else if (data == WARP_NOTIFY_BTB_TEMP_OVER) {
		oneplus_chg_set_charger_type_unknown();
	}

	if (chip->adapter_update_real != ADAPTER_FW_NEED_UPDATE) {
		chip->vops->eint_regist(chip);
	}

	if (chip->adapter_update_real == ADAPTER_FW_NEED_UPDATE) {
		chip->allow_reading = true;
		chip->fastchg_started = false;
		chip->fastchg_to_normal = false;
		chip->fastchg_low_temp_full = false;
		chip->fastchg_to_warm = false;
		chip->fastchg_ing = false;
		//del_timer(&chip->watchdog);
		oneplus_warp_del_watchdog_timer(chip);
		oneplus_warp_battery_update();
		oneplus_adapter_fw_update();
		oneplus_warp_set_awake(chip, false);
	} else if ((data == WARP_NOTIFY_FAST_PRESENT)
			|| (data == WARP_NOTIFY_ALLOW_READING_IIC)
			|| (data == WARP_NOTIFY_BTB_TEMP_OVER)) {
		oneplus_warp_battery_update();
	} else if ((data == WARP_NOTIFY_LOW_TEMP_FULL)
		|| (data == WARP_NOTIFY_FAST_ABSENT)
		|| (data == WARP_NOTIFY_NORMAL_TEMP_FULL)
		|| (data == WARP_NOTIFY_BAD_CONNECTED)
		|| (data == WARP_NOTIFY_TEMP_OVER)) {
		oneplus_warp_battery_update();
#ifdef CHARGE_PLUG_IN_TP_AVOID_DISTURB
		charge_plug_tp_avoid_distrub(1, is_oneplus_fast_charger);
#endif
		oneplus_warp_set_awake(chip, false);
	} else if (data_err) {
		data_err = false;
		oneplus_warp_battery_update();
#ifdef CHARGE_PLUG_IN_TP_AVOID_DISTURB
		charge_plug_tp_avoid_distrub(1, is_oneplus_fast_charger);
#endif
		oneplus_warp_set_awake(chip, false);
	}
	if (chip->fastchg_started == false
			&& chip->fastchg_dummy_started == false
			&& chip->fastchg_to_normal == false
			&& chip->fastchg_to_warm == false){
		chip->fast_chg_type = FASTCHG_CHARGER_TYPE_UNKOWN;
	}

}

void fw_update_thread(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oneplus_warp_chip *chip = container_of(dwork,
		struct oneplus_warp_chip, fw_update_work);
	const struct firmware *fw = NULL;
	int ret = 1;
	int retry = 5;
	char version[10];

	if(chip->warp_fw_update_newmethod) {
		if(oneplus_is_rf_ftm_mode()) {
			chip->vops->fw_check_then_recover(chip);
			return;
		}
		 do {
			ret = request_firmware_select(&fw, chip->fw_path, chip->dev);
			if (!ret) {
				break;
			}
		} while((ret < 0) && (--retry > 0));
		chg_debug(" retry times %d, chip->fw_path[%s]\n", 5 - retry, chip->fw_path);
		if(!ret) {
			chip->firmware_data =  fw->data;
			chip->fw_data_count =  fw->size;
			chip->fw_data_version = chip->firmware_data[chip->fw_data_count - 4];
			chg_debug("count:0x%x, version:0x%x\n",
				chip->fw_data_count,chip->fw_data_version);
			if(chip->vops->fw_check_then_recover) {
				chip->vops->fw_check_then_recover(chip);
				sprintf(version,"%d", chip->fw_data_version);
				//sprintf(chip->manufacture_info.version,"%s", version);//debug for bring up 
				if (chip->vops->is_power_off_charging && chip->vops->is_power_off_charging(chip) == false) {
					chg_debug("update finish, then clean fastchg_dummy and watch_dog\n");
					chip->fastchg_dummy_started = false;
					del_timer(&chip->watchdog);
				}
			}
			release_firmware(fw);
		} else {
			chg_debug("%s: fw_name request failed, %d\n", __func__, ret);
		}
	}else {
		chip->vops->fw_check_then_recover(chip);
		if (chip->vops->is_power_off_charging && chip->vops->is_power_off_charging(chip) == false) {
			chg_debug("update finish, then clean fastchg_dummy and watch_dog\n");
			chip->fastchg_dummy_started = false;
			del_timer(&chip->watchdog);
		}
	}
}

#define FASTCHG_FW_INTERVAL_INIT	   1000	/*  1S     */
void oneplus_warp_fw_update_work_init(struct oneplus_warp_chip *chip)
{
	INIT_DELAYED_WORK(&chip->fw_update_work, fw_update_thread);
	schedule_delayed_work(&chip->fw_update_work, round_jiffies_relative(msecs_to_jiffies(FASTCHG_FW_INTERVAL_INIT)));
}

void oneplus_warp_shedule_fastchg_work(void)
{
	if (!g_warp_chip) {
		chg_err(" g_warp_chip is NULL\n");
	} else {
		schedule_delayed_work(&g_warp_chip->fastchg_work, 0);
	}
}
static ssize_t proc_fastchg_fw_update_write(struct file *file, const char __user *buff,
		size_t len, loff_t *data)
{
	struct oneplus_warp_chip *chip = PDE_DATA(file_inode(file));
	char write_data[32] = {0};

	if (copy_from_user(&write_data, buff, len)) {
		chg_err("fastchg_fw_update error.\n");
		return -EFAULT;
	}

	if (write_data[0] == '1') {
		chg_err("fastchg_fw_update\n");
		chip->fw_update_flag = 1;
		schedule_delayed_work(&chip->fw_update_work, 0);
	} else {
		chip->fw_update_flag = 0;
		chg_err("Disable fastchg_fw_update\n");
	}

	return len;
}

static ssize_t proc_fastchg_fw_update_read(struct file *file, char __user *buff,
		size_t count, loff_t *off)
{
	struct oneplus_warp_chip *chip = PDE_DATA(file_inode(file));
	char page[256] = {0};
	char read_data[32] = {0};
	int len = 0;

	if(chip->fw_update_flag == 1) {
		read_data[0] = '1';
	} else {
		read_data[0] = '0';
	}
	len = sprintf(page,"%s",read_data);
	if(len > *off) {
		len -= *off;
	} else {
		len = 0;
	}
	if (copy_to_user(buff,page,(len < count ? len : count))) {
		return -EFAULT;
	}
	*off += len < count ? len : count;
	return (len < count ? len : count);
}


static const struct file_operations fastchg_fw_update_proc_fops = {
	.write = proc_fastchg_fw_update_write,
	.read  = proc_fastchg_fw_update_read,
};

static int init_proc_fastchg_fw_update(struct oneplus_warp_chip *chip)
{
	struct proc_dir_entry *p = NULL;

	p = proc_create_data("fastchg_fw_update", 0664, NULL, &fastchg_fw_update_proc_fops,chip);
	if (!p) {
		pr_err("proc_create fastchg_fw_update_proc_fops fail!\n");
	}
	return 0;
}

static int init_warp_proc(struct oneplus_warp_chip *chip)
{
#if 0//debug for bring up 
	strcpy(chip->manufacture_info.version, "0");
	if (get_warp_mcu_type(chip) == ONEPLUS_WARP_MCU_HWID_STM8S) {
		snprintf(chip->fw_path, MAX_FW_NAME_LENGTH, "fastchg/%d/oneplus_warp_fw.bin", get_project());
	} else if (get_warp_mcu_type(chip) == ONEPLUS_WARP_MCU_HWID_N76E) {
		snprintf(chip->fw_path, MAX_FW_NAME_LENGTH, "fastchg/%d/oneplus_warp_fw_n76e.bin", get_project());
	} else if (get_warp_mcu_type(chip) == ONEPLUS_WARP_ASIC_HWID_RK826) {
		snprintf(chip->fw_path, MAX_FW_NAME_LENGTH, "fastchg/%d/oneplus_warp_fw_rk826.bin", get_project());
	} else {
		snprintf(chip->fw_path, MAX_FW_NAME_LENGTH, "fastchg/%d/oneplus_warp_fw_op10.bin", get_project());
	}
	memcpy(chip->manufacture_info.manufacture, chip->fw_path, MAX_FW_NAME_LENGTH);
	register_devinfo("fastchg", &chip->manufacture_info);
#endif
	init_proc_fastchg_fw_update(chip);
	//chg_debug(" version:%s, fw_path:%s\n", chip->manufacture_info.version, chip->fw_path);//debug for bring up 
	return 0;
}
void oneplus_warp_init(struct oneplus_warp_chip *chip)
{
	int ret = 0;

	chip->allow_reading = true;
	chip->fastchg_started = false;
	chip->fastchg_dummy_started = false;
	chip->fastchg_ing = false;
	chip->fastchg_to_normal = false;
	chip->fastchg_to_warm = false;
	chip->fastchg_allow = false;
	chip->fastchg_low_temp_full = false;
	chip->have_updated = false;
	chip->need_to_up = false;
	chip->btb_temp_over = false;
	chip->adapter_update_real = ADAPTER_FW_UPDATE_NONE;
	chip->adapter_update_report = chip->adapter_update_real;
	chip->mcu_update_ing = false;
	chip->mcu_boot_by_gpio = false;
	chip->dpdm_switch_mode = NORMAL_CHARGER_MODE;
	chip->fast_chg_type = FASTCHG_CHARGER_TYPE_UNKOWN;
	/*chip->batt_psy = power_supply_get_by_name("battery");*/
	chip->disable_adapter_output = false;
	chip->set_warp_current_limit = WARP_MAX_CURRENT_NO_LIMIT;
	oneplus_warp_init_watchdog_timer(chip);
	oneplus_warp_awake_init(chip);
	INIT_DELAYED_WORK(&chip->fastchg_work, oneplus_warp_fastchg_func);
	INIT_DELAYED_WORK(&chip->check_charger_out_work, check_charger_out_work_func);
	INIT_WORK(&chip->warp_watchdog_work, warp_watchdog_work_func);
	g_warp_chip = chip;
	chip->vops->eint_regist(chip);
	if(chip->warp_fw_update_newmethod) {
		if(oneplus_is_rf_ftm_mode()) {
			return;
		}
		INIT_DELAYED_WORK(&chip->fw_update_work, fw_update_thread);
		//Alloc fw_name/devinfo memory space

		chip->fw_path = kzalloc(MAX_FW_NAME_LENGTH, GFP_KERNEL);
		if (chip->fw_path == NULL) {
			ret = -ENOMEM;
			chg_err("panel_data.fw_name kzalloc error\n");
			goto manu_fwpath_alloc_err;
		}
		//chip->manufacture_info.version = kzalloc(MAX_DEVICE_VERSION_LENGTH, GFP_KERNEL);//debug for bring up 
		//if (chip->manufacture_info.version == NULL) {
			//ret = -ENOMEM;
			//chg_err("manufacture_info.version kzalloc error\n");
			//goto manu_version_alloc_err;
		//}
		//chip->manufacture_info.manufacture = kzalloc(MAX_DEVICE_MANU_LENGTH, GFP_KERNEL);
		//if (chip->manufacture_info.manufacture == NULL) {
		//	ret = -ENOMEM;
		//	chg_err("panel_data.manufacture kzalloc error\n");
		//	goto manu_info_alloc_err;
		//}
		init_warp_proc(chip);
		return;

manu_fwpath_alloc_err:
		kfree(chip->fw_path);

//manu_info_alloc_err://debug for bring up 
		//kfree(chip->manufacture_info.manufacture);

//manu_version_alloc_err:
		//kfree(chip->manufacture_info.version);
	}
	return ;
}

bool oneplus_warp_wake_fastchg_work(struct oneplus_warp_chip *chip)
{
	return schedule_delayed_work(&chip->fastchg_work, 0);
}

void oneplus_warp_print_log(void)
{
	if (!g_warp_chip) {
		return;
	}
	warp_xlog_printk(CHG_LOG_CRTI, "WARP[ %d / %d / %d / %d / %d / %d]\n",
		g_warp_chip->fastchg_allow, g_warp_chip->fastchg_started, g_warp_chip->fastchg_dummy_started,
		g_warp_chip->fastchg_to_normal, g_warp_chip->fastchg_to_warm, g_warp_chip->btb_temp_over);
}

bool oneplus_warp_get_allow_reading(void)
{
	if (!g_warp_chip) {
		return true;
	} else {
		if (g_warp_chip->support_warp_by_normal_charger_path
				&& g_warp_chip->fast_chg_type == CHARGER_SUBTYPE_FASTCHG_WARP) {
			return true;
		} else {
			return g_warp_chip->allow_reading;
		}
	}
}

bool oneplus_warp_get_fastchg_started(void)
{
	if (!g_warp_chip) {
		return false;
	} else {
		return g_warp_chip->fastchg_started;
	}
}

bool oneplus_warp_get_fastchg_ing(void)
{
	if (!g_warp_chip) {
		return false;
	} else {
		return g_warp_chip->fastchg_ing;
	}
}

bool oneplus_warp_get_fastchg_allow(void)
{
	if (!g_warp_chip) {
		return false;
	} else {
		return g_warp_chip->fastchg_allow;
	}
}

void oneplus_warp_set_fastchg_allow(int enable)
{
	if (!g_warp_chip) {
		return;
	} else {
		g_warp_chip->fastchg_allow = enable;
	}
}

bool oneplus_warp_get_fastchg_to_normal(void)
{
	if (!g_warp_chip) {
		return false;
	} else {
		return g_warp_chip->fastchg_to_normal;
	}
}

void oneplus_warp_set_fastchg_to_normal_false(void)
{
	if (!g_warp_chip) {
		return;
	} else {
		g_warp_chip->fastchg_to_normal = false;
	}
}

void oneplus_warp_set_fastchg_type_unknow(void)
{
	if (!g_warp_chip) {
		return;
	} else {
		g_warp_chip->fast_chg_type = FASTCHG_CHARGER_TYPE_UNKOWN;
	}
}

bool oneplus_warp_get_fastchg_to_warm(void)
{
	if (!g_warp_chip) {
		return false;
	} else {
		return g_warp_chip->fastchg_to_warm;
	}
}

void oneplus_warp_set_fastchg_to_warm_false(void)
{
	if (!g_warp_chip) {
		return;
	} else {
		g_warp_chip->fastchg_to_warm = false;
	}
}

bool oneplus_warp_get_fastchg_low_temp_full()
{
	if (!g_warp_chip) {
		return false;
	} else {
		return g_warp_chip->fastchg_low_temp_full;
	}
}

void oneplus_warp_set_fastchg_low_temp_full_false(void)
{
	if (!g_warp_chip) {
		return;
	} else {
		g_warp_chip->fastchg_low_temp_full = false;
	}
}

bool oneplus_warp_get_fastchg_dummy_started(void)
{
	if (!g_warp_chip) {
		return false;
	} else {
		return g_warp_chip->fastchg_dummy_started;
	}
}

void oneplus_warp_set_fastchg_dummy_started_false(void)
{
	if (!g_warp_chip) {
		return;
	} else {
		g_warp_chip->fastchg_dummy_started = false;
	}
}

int oneplus_warp_get_adapter_update_status(void)
{
	if (!g_warp_chip) {
		return ADAPTER_FW_UPDATE_NONE;
	} else {
		return g_warp_chip->adapter_update_report;
	}
}

int oneplus_warp_get_adapter_update_real_status(void)
{
	if (!g_warp_chip) {
		return ADAPTER_FW_UPDATE_NONE;
	} else {
		return g_warp_chip->adapter_update_real;
	}
}

bool oneplus_warp_get_btb_temp_over(void)
{
	if (!g_warp_chip) {
		return false;
	} else {
		return g_warp_chip->btb_temp_over;
	}
}

void oneplus_warp_reset_fastchg_after_usbout(void)
{
	if (!g_warp_chip) {
		return;
	} else {
		g_warp_chip->vops->reset_fastchg_after_usbout(g_warp_chip);
	}
}

void oneplus_warp_switch_fast_chg(void)
{
	if (!g_warp_chip) {
		return;
	} else {
		g_warp_chip->vops->switch_fast_chg(g_warp_chip);
	}
}

void oneplus_warp_set_ap_clk_high(void)
{
	if (!g_warp_chip) {
		return;
	} else {
		g_warp_chip->vops->set_clock_sleep(g_warp_chip);
	}
}

void oneplus_warp_reset_mcu(void)
{
	if (!g_warp_chip) {
		return;
	} else {
		g_warp_chip->vops->reset_mcu(g_warp_chip);
	}
}

bool oneplus_warp_check_chip_is_null(void)
{
	if (!g_warp_chip) {
		return true;
	} else {
		return false;
	}
}

int oneplus_warp_get_warp_switch_val(void)
{
	if (!g_warp_chip) {
		return 0;
	} else {
		return g_warp_chip->vops->get_switch_gpio_val(g_warp_chip);
	}
}

void oneplus_warp_uart_init(void)
{
	struct oneplus_warp_chip *chip = g_warp_chip;
	if (!chip) {
		return ;
	} else {
		chip->vops->set_data_active(chip);
		chip->vops->set_clock_sleep(chip);
	}
}

int oneplus_warp_get_uart_tx(void)
{
	struct oneplus_warp_chip *chip = g_warp_chip;
	if (!chip) {
		return -1;
	} else {
		return chip->vops->get_clk_gpio_num(chip);
	}
}

int oneplus_warp_get_uart_rx(void)
{
	struct oneplus_warp_chip *chip = g_warp_chip;
	if (!chip) {
		return -1;
	} else {
		return chip->vops->get_data_gpio_num(chip);
	}
}


void oneplus_warp_uart_reset(void)
{
	struct oneplus_warp_chip *chip = g_warp_chip;
	if (!chip) {
		return ;
	} else {
		chip->vops->eint_regist(chip);
		oneplus_chg_set_chargerid_switch_val(0);
		chip->vops->set_switch_mode(chip, NORMAL_CHARGER_MODE);
		chip->vops->reset_mcu(chip);
	}
}

void oneplus_warp_set_adapter_update_real_status(int real)
{
	struct oneplus_warp_chip *chip = g_warp_chip;
	if (!chip) {
		return ;
	} else {
		chip->adapter_update_real = real;
	}
}

void oneplus_warp_set_adapter_update_report_status(int report)
{
	struct oneplus_warp_chip *chip = g_warp_chip;
	if (!chip) {
		return ;
	} else {
		chip->adapter_update_report = report;
	}
}

int oneplus_warp_get_fast_chg_type(void)
{
	struct oneplus_warp_chip *chip = g_warp_chip;
	if (!chip) {
		return FASTCHG_CHARGER_TYPE_UNKOWN;
	} else {
		return chip->fast_chg_type;
	}
}

void oneplus_warp_set_disable_adapter_output(bool disable)
{
	struct oneplus_warp_chip *chip = g_warp_chip;
	if (!chip) {
		return ;
	} else {
		chip->disable_adapter_output = disable;
	}
	chg_err(" chip->disable_adapter_output:%d\n", chip->disable_adapter_output);
}

void oneplus_warp_set_warp_max_current_limit(int current_level)
{
	struct oneplus_warp_chip *chip = g_warp_chip;
	if (!chip) {
		return ;
	} else {
		chip->set_warp_current_limit = current_level;
	}
}

void oneplus_warp_set_warp_chargerid_switch_val(int value)
{
	struct oneplus_warp_chip *chip = g_warp_chip;

	if (!chip) {
		return;
	} else if (chip->vops->set_warp_chargerid_switch_val) {
		chip->vops->set_warp_chargerid_switch_val(chip, value);
	}
}
