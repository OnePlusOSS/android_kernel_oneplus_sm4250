/**********************************************************************************
* Copyright (c)  2008-2015  Guangdong ONEPLUS Mobile Comm Corp., Ltd
* Description: Charger IC management module for charger system framework.
*             Manage all charger IC and define abstarct function flow.
* Version   : 1.0
* Date      : 2015-05-22
* Author    : fanhui@PhoneSW.BSP
*           : Fanhong.Kong@ProDrv.CHG
* ------------------------------ Revision History: --------------------------------
* <version>           <date>                <author>                               <desc>
* Revision 1.0        2015-05-22        fanhui@PhoneSW.BSP                 Created for new architecture
* Revision 1.0        2015-05-22        Fanhong.Kong@ProDrv.CHG            Created for new architecture
* Revision 2.0        2018-04-14        Fanhong.Kong@ProDrv.CHG            Upgrade for SWARP
***********************************************************************************/

#ifndef _ONEPLUS_WARP_H_
#define _ONEPLUS_WARP_H_

#include <linux/workqueue.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
#include <linux/wakelock.h>
#endif
#include <linux/timer.h>
#include <linux/slab.h>
//#include <soc/oneplus/device_info.h>//debug for bring up 
//#include <soc/oneplus/oneplus_project.h>
#include <linux/firmware.h>

#define ONEPLUS_WARP_MCU_HWID_STM8S	0
#define ONEPLUS_WARP_MCU_HWID_N76E		1
#define ONEPLUS_WARP_ASIC_HWID_RK826	2
#define ONEPLUS_WARP_ASIC_HWID_OP10	3

enum {
	WARP_CHARGER_MODE,
	HEADPHONE_MODE,
	NORMAL_CHARGER_MODE,
};

enum {
	WARP_MAX_CURRENT_NO_LIMIT,
	WARP_MAX_CURRENT_LIMIT_2A,
	WARP_MAX_CURRENT_LIMIT_OTHER,
};
enum {
	FASTCHG_CHARGER_TYPE_UNKOWN,
	PORTABLE_PIKAQIU_1 = 0x31,
	PORTABLE_PIKAQIU_2 = 0x32,
	PORTABLE_50W = 0x33,
	PORTABLE_20W_1 = 0X34,
	PORTABLE_20W_2 = 0x35,
	PORTABLE_20W_3 = 0x36,
};

enum {
	BAT_TEMP_NATURAL = 0,
	BAT_TEMP_HIGH0,
	BAT_TEMP_HIGH1,
	BAT_TEMP_HIGH2,
	BAT_TEMP_HIGH3,
	BAT_TEMP_HIGH4,
	BAT_TEMP_HIGH5,
};


struct warp_gpio_control {
	int switch1_gpio;
	int switch1_ctr1_gpio;
	int switch2_gpio;
	int switch3_gpio;
	int reset_gpio;
	int clock_gpio;
	int data_gpio;
	int warp_mcu_id_gpio;
	int data_irq;
	struct pinctrl *pinctrl;

	struct pinctrl_state *gpio_switch1_act_switch2_act;
	struct pinctrl_state *gpio_switch1_sleep_switch2_sleep;
	struct pinctrl_state *gpio_switch1_act_switch2_sleep;
	struct pinctrl_state *gpio_switch1_sleep_switch2_act;
	struct pinctrl_state *gpio_switch1_ctr1_act;
	struct pinctrl_state *gpio_switch1_ctr1_sleep;

	struct pinctrl_state *gpio_clock_active;
	struct pinctrl_state *gpio_clock_sleep;
	struct pinctrl_state *gpio_data_active;
	struct pinctrl_state *gpio_data_sleep;
	struct pinctrl_state *gpio_reset_active;
	struct pinctrl_state *gpio_reset_sleep;
	struct pinctrl_state *gpio_warp_mcu_id_default;
};

struct oneplus_warp_chip {
	struct i2c_client *client;
	struct device *dev;
	struct oneplus_warp_operations *vops;
	struct warp_gpio_control warp_gpio;
	struct delayed_work fw_update_work;
	struct delayed_work fastchg_work;
	struct delayed_work delay_reset_mcu_work;
	struct delayed_work check_charger_out_work;
	struct work_struct warp_watchdog_work;
	struct timer_list watchdog;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
	struct wake_lock warp_wake_lock;
#else
	struct wakeup_source *warp_ws;
#endif

	struct power_supply *batt_psy;
	int pcb_version;
	bool allow_reading;
	bool fastchg_started;
	bool fastchg_ing;
	bool fastchg_allow;
	bool fastchg_to_normal;
	bool fastchg_to_warm;
	bool fastchg_low_temp_full;
	bool btb_temp_over;
	bool fastchg_dummy_started;
	bool need_to_up;
	bool have_updated;
	bool mcu_update_ing;
	bool mcu_boot_by_gpio;
	const unsigned char *firmware_data;
	unsigned int fw_data_count;
	int fw_mcu_version;
	int fw_data_version;
	int adapter_update_real;
	int adapter_update_report;
	int dpdm_switch_mode;
	bool support_warp_by_normal_charger_path;
	bool batt_type_4400mv;
	bool warp_fw_check;
	int warp_fw_type;
	int fw_update_flag;
	//struct manufacture_info manufacture_info;//debug for bring up 
	bool warp_fw_update_newmethod;
	char *fw_path;
	struct mutex pinctrl_mutex;
	int warp_low_temp;
	int warp_high_temp;
	int warp_low_soc;
	int warp_high_soc;
	int fast_chg_type;
	bool disable_adapter_output;// 0--warp adapter output normal,  1--disable warp adapter output
	int set_warp_current_limit;///0--no limit;  1--max current limit 2A
	bool warp_multistep_adjust_current_support;
	int warp_multistep_initial_batt_temp;
	int warp_strategy_normal_current;
	int warp_strategy1_batt_up_down_temp1;
	int warp_strategy1_batt_up_down_temp2;
	int warp_strategy1_batt_up_temp3;
	int warp_strategy1_batt_up_temp4;
	int warp_strategy1_batt_up_temp5;
	int warp_strategy1_high0_current;
	int warp_strategy1_high1_current;
	int warp_strategy1_high2_current;
	int warp_strategy2_batt_up_temp1;
	int warp_strategy2_batt_up_down_temp2;
	int warp_strategy2_batt_up_temp3;
	int warp_strategy2_batt_up_down_temp4;
	int warp_strategy2_batt_up_temp5;
	int warp_strategy2_batt_up_temp6;
	int warp_strategy2_high0_current;
	int warp_strategy2_high1_current;
	int warp_strategy2_high2_current;
	int warp_strategy2_high3_current;
	int fastchg_batt_temp_status;
};

#define MAX_FW_NAME_LENGTH	60
#define MAX_DEVICE_VERSION_LENGTH 16
#define MAX_DEVICE_MANU_LENGTH    60
struct oneplus_warp_operations {
	int (*fw_update)(struct oneplus_warp_chip *chip);
	void (*fw_check_then_recover)(struct oneplus_warp_chip *chip);
	void (*eint_regist)(struct oneplus_warp_chip *chip);
	void (*eint_unregist)(struct oneplus_warp_chip *chip);
	void (*set_data_active)(struct oneplus_warp_chip *chip);
	void (*set_data_sleep)(struct oneplus_warp_chip *chip);
	void (*set_clock_active)(struct oneplus_warp_chip *chip);
	void (*set_clock_sleep)(struct oneplus_warp_chip *chip);
	void (*set_switch_mode)(struct oneplus_warp_chip *chip, int mode);
	int (*get_gpio_ap_data)(struct oneplus_warp_chip *chip);
	int (*read_ap_data)(struct oneplus_warp_chip *chip);
	void (*reply_mcu_data)(struct oneplus_warp_chip *chip, int ret_info, int device_type);
	void (*reply_mcu_data_4bits)(struct oneplus_warp_chip *chip,
		int ret_info, int device_type);
	void (*reset_fastchg_after_usbout)(struct oneplus_warp_chip *chip);
	void (*switch_fast_chg)(struct oneplus_warp_chip *chip);
	void (*reset_mcu)(struct oneplus_warp_chip *chip);
	void (*set_warp_chargerid_switch_val)(struct oneplus_warp_chip *chip, int value);
	bool (*is_power_off_charging)(struct oneplus_warp_chip *chip);
	int (*get_reset_gpio_val)(struct oneplus_warp_chip *chip);
	int (*get_switch_gpio_val)(struct oneplus_warp_chip *chip);
	int (*get_ap_clk_gpio_val)(struct oneplus_warp_chip *chip);
	int (*get_fw_version)(struct oneplus_warp_chip *chip);
	int (*get_clk_gpio_num)(struct oneplus_warp_chip *chip);
	int (*get_data_gpio_num)(struct oneplus_warp_chip *chip);
};

void oneplus_warp_init(struct oneplus_warp_chip *chip);
void oneplus_warp_shedule_fastchg_work(void);
void oneplus_warp_read_fw_version_init(struct oneplus_warp_chip *chip);
void oneplus_warp_fw_update_work_init(struct oneplus_warp_chip *chip);
bool oneplus_warp_wake_fastchg_work(struct oneplus_warp_chip *chip);
void oneplus_warp_print_log(void);
void oneplus_warp_switch_mode(int mode);
bool oneplus_warp_get_allow_reading(void);
bool oneplus_warp_get_fastchg_started(void);
bool oneplus_warp_get_fastchg_ing(void);
bool oneplus_warp_get_fastchg_allow(void);
void oneplus_warp_set_fastchg_allow(int enable);
bool oneplus_warp_get_fastchg_to_normal(void);
void oneplus_warp_set_fastchg_to_normal_false(void);
bool oneplus_warp_get_fastchg_to_warm(void);
void oneplus_warp_set_fastchg_to_warm_false(void);
void oneplus_warp_set_fastchg_type_unknow(void);
bool oneplus_warp_get_fastchg_low_temp_full(void);
void oneplus_warp_set_fastchg_low_temp_full_false(void);
bool oneplus_warp_get_fastchg_dummy_started(void);
void oneplus_warp_set_fastchg_dummy_started_false(void);
int oneplus_warp_get_adapter_update_status(void);
int oneplus_warp_get_adapter_update_real_status(void);
bool oneplus_warp_get_btb_temp_over(void);
void oneplus_warp_reset_fastchg_after_usbout(void);
void oneplus_warp_switch_fast_chg(void);
void oneplus_warp_reset_mcu(void);
void oneplus_warp_set_warp_chargerid_switch_val(int value);
void oneplus_warp_set_ap_clk_high(void);
int oneplus_warp_get_warp_switch_val(void);
bool oneplus_warp_check_chip_is_null(void);
void oneplus_warp_battery_update(void);

int oneplus_warp_get_uart_tx(void);
int oneplus_warp_get_uart_rx(void);
void oneplus_warp_uart_init(void);
void oneplus_warp_uart_reset(void);
void oneplus_warp_set_adapter_update_real_status(int real);
void oneplus_warp_set_adapter_update_report_status(int report);
int oneplus_warp_get_fast_chg_type(void);
void oneplus_warp_set_disable_adapter_output(bool disable);
void oneplus_warp_set_warp_max_current_limit(int current_level);

extern int get_warp_mcu_type(struct oneplus_warp_chip *chip);
#endif /* _ONEPLUS_WARP_H */
