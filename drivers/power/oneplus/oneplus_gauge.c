/**********************************************************************************
* Description: Charger IC management module for charger system framework.
*              Manage all charger IC and define abstarct function flow.
* Version    : 1.0
* Date       : 2015-06-22
* Author     : fanhui@PhoneSW.BSP
*            : Fanhong.Kong@ProDrv.CHG
* ------------------------------ Revision History: --------------------------------
* <version>           <date>                <author>                          <desc>
* Revision 1.0        2015-06-22       fanhui@PhoneSW.BSP            Created for new architecture
* Revision 1.0        2015-06-22       Fanhong.Kong@ProDrv.CHG       Created for new architecture
***********************************************************************************/

#include "oneplus_gauge.h"
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/debugfs.h>


static struct oneplus_gauge_chip *g_gauge_chip = NULL;

static int gauge_dbg_tbat = 0;
module_param(gauge_dbg_tbat, int, 0644);
MODULE_PARM_DESC(gauge_dbg_tbat, "debug battery temperature");

static int gauge_dbg_vbat = 0;
module_param(gauge_dbg_vbat, int, 0644);
MODULE_PARM_DESC(gauge_dbg_vbat, "debug battery voltage");

static int gauge_dbg_ibat = 0;
module_param(gauge_dbg_ibat, int, 0644);
MODULE_PARM_DESC(gauge_dbg_ibat, "debug battery current");

int oneplus_gauge_get_batt_mvolts(void)
{
	if (!g_gauge_chip) {
		return 3800;
	} else {
		if (gauge_dbg_vbat != 0) {
        		printk(KERN_ERR "[ONEPLUS_CHG]%s:debug enabled,voltage gauge_dbg_vbat[%d] \n",  __func__, gauge_dbg_vbat);
			return gauge_dbg_vbat;
		}
		return g_gauge_chip->gauge_ops->get_battery_mvolts();
	}
}

int oneplus_gauge_get_batt_fc(void)
{
	if (!g_gauge_chip) {
		return 0;
	} else {
		return g_gauge_chip->gauge_ops->get_battery_fc();
	}
}

int oneplus_gauge_get_batt_qm(void)
{
	if (!g_gauge_chip) {
		return 0;
	} else {
		return g_gauge_chip->gauge_ops->get_battery_qm();
	}
}

int oneplus_gauge_get_batt_pd(void)
{
	if (!g_gauge_chip) {
		return 0;
	} else {
		return g_gauge_chip->gauge_ops->get_battery_pd();
	}
}

int oneplus_gauge_get_batt_rcu(void)
{
	if (!g_gauge_chip) {
		return 0;
	} else {
		return g_gauge_chip->gauge_ops->get_battery_rcu();
	}
}

int oneplus_gauge_get_batt_rcf(void)
{
	if (!g_gauge_chip) {
		return 0;
	} else {
		return g_gauge_chip->gauge_ops->get_battery_rcf();
	}
}

int oneplus_gauge_get_batt_fcu(void)
{
	if (!g_gauge_chip) {
		return 0;
	} else {
		return g_gauge_chip->gauge_ops->get_battery_fcu();
	}
}

int oneplus_gauge_get_batt_fcf(void)
{
	if (!g_gauge_chip) {
		return 0;
	} else {
		return g_gauge_chip->gauge_ops->get_battery_fcf();
	}
}

int oneplus_gauge_get_batt_sou(void)
{
	if (!g_gauge_chip) {
		return 0;
	} else {
		return g_gauge_chip->gauge_ops->get_battery_sou();
	}
}

int oneplus_gauge_get_batt_do0(void)
{
	if (!g_gauge_chip) {
		return 0;
	} else {
		return g_gauge_chip->gauge_ops->get_battery_do0();
	}
}

int oneplus_gauge_get_batt_doe(void)
{
	if (!g_gauge_chip) {
		return 0;
	} else {
		return g_gauge_chip->gauge_ops->get_battery_doe();
	}
}

int oneplus_gauge_get_batt_trm(void)
{
	if (!g_gauge_chip) {
		return 0;
	} else {
		return g_gauge_chip->gauge_ops->get_battery_trm();
	}
}

int oneplus_gauge_get_batt_pc(void)
{
	if (!g_gauge_chip) {
		return 0;
	} else {
		return g_gauge_chip->gauge_ops->get_battery_pc();
	}
}

int oneplus_gauge_get_batt_qs(void)
{
	if (!g_gauge_chip) {
		return 0;
	} else {
		return g_gauge_chip->gauge_ops->get_battery_qs();
	}
}

int oneplus_gauge_get_batt_mvolts_2cell_max(void)
{
	if(!g_gauge_chip)
		return 3800;
	else
		return g_gauge_chip->gauge_ops->get_battery_mvolts_2cell_max();
}

int oneplus_gauge_get_batt_mvolts_2cell_min(void)
{
	if(!g_gauge_chip)
		return 3800;
	else
		return g_gauge_chip->gauge_ops->get_battery_mvolts_2cell_min();
}

int oneplus_gauge_get_batt_temperature(void)
{
	int batt_temp = 0;
	if (!g_gauge_chip) {
		return 250;
	} else {
		if (gauge_dbg_tbat != 0) {
			printk(KERN_ERR "[ONEPLUS_CHG]debug enabled, gauge_dbg_tbat[%d] \n", gauge_dbg_tbat);
			return gauge_dbg_tbat;
			}
		batt_temp = g_gauge_chip->gauge_ops->get_battery_temperature();
		return batt_temp;
	}
}

int oneplus_gauge_get_batt_soc(void)
{
	if (!g_gauge_chip) {
		return -1;
	} else {
		return g_gauge_chip->gauge_ops->get_battery_soc();
	}
}

int oneplus_gauge_get_batt_current(void)
{
	if (!g_gauge_chip) {
		return 100;
	} else {
		if (gauge_dbg_ibat != 0) {
        		printk(KERN_ERR "[ONEPLUS_CHG]debug enabled,current gauge_dbg_ibat[%d] \n", gauge_dbg_ibat);
			return gauge_dbg_ibat;
		}
		return g_gauge_chip->gauge_ops->get_average_current();
	}
}

int oneplus_gauge_get_remaining_capacity(void)
{
	if (!g_gauge_chip) {
		return 0;
	} else {
		return g_gauge_chip->gauge_ops->get_batt_remaining_capacity();
	}
}

int oneplus_gauge_get_device_type(void)
{
	if (!g_gauge_chip) {
		return 0;
	} else {
		return g_gauge_chip->device_type;
	}
}

int oneplus_gauge_get_device_type_for_warp(void)
{
	if (!g_gauge_chip) {
		return 0;
	} else {
		return g_gauge_chip->device_type_for_warp;
	}
}

int oneplus_gauge_get_batt_fcc(void)
{
	if (!g_gauge_chip) {
		return 0;
	} else {
		return g_gauge_chip->gauge_ops->get_battery_fcc();
	}
}

int oneplus_gauge_get_batt_cc(void)
{
	if (!g_gauge_chip) {
		return 0;
	} else {
		return g_gauge_chip->gauge_ops->get_battery_cc();
	}
}

int oneplus_gauge_get_batt_soh(void)
{
	if (!g_gauge_chip) {
		return 0;
	} else {
		return g_gauge_chip->gauge_ops->get_battery_soh();
	}
}

bool oneplus_gauge_get_batt_authenticate(void)
{
	if (!g_gauge_chip) {
		return false;
	} else {
		return g_gauge_chip->gauge_ops->get_battery_authenticate();
	}
}

void oneplus_gauge_set_batt_full(bool full)
{
	if (g_gauge_chip) {
		g_gauge_chip->gauge_ops->set_battery_full(full);
	}
}

bool oneplus_gauge_check_chip_is_null(void)
{
	if (!g_gauge_chip) {
		return true;
	} else {
		return false;
	}
}

void oneplus_gauge_init(struct oneplus_gauge_chip *chip)
{
	g_gauge_chip = chip;
}

int oneplus_gauge_get_prev_batt_mvolts(void)
{
	if (!g_gauge_chip)
		return 3800;
	else{
		if (gauge_dbg_vbat != 0) {
			printk(KERN_ERR "[ONEPLUS_CHG]%s:debug enabled,voltage gauge_dbg_vbat[%d] \n",  __func__, gauge_dbg_vbat);
			return gauge_dbg_vbat;
		}
		return g_gauge_chip->gauge_ops->get_prev_battery_mvolts();
	}
}

int oneplus_gauge_get_prev_batt_mvolts_2cell_max(void)
{
	if(!g_gauge_chip)
		return 3800;
	else{
		if (gauge_dbg_vbat != 0) {
		    printk(KERN_ERR "[ONEPLUS_CHG]%s: debug enabled,voltage gauge_dbg_vbat[%d] \n", __func__, gauge_dbg_vbat);
		    return gauge_dbg_vbat;
		}
		return g_gauge_chip->gauge_ops->get_prev_battery_mvolts_2cell_max();
	}
}

int oneplus_gauge_get_prev_batt_mvolts_2cell_min(void)
{
	if(!g_gauge_chip)
		return 3800;
	else{
		if (gauge_dbg_vbat != 0) {
			printk(KERN_ERR "[ONEPLUS_CHG]%s:debug enabled,voltage gauge_dbg_vbat[%d] \n",  __func__, gauge_dbg_vbat);
			return gauge_dbg_vbat;
			}
		return g_gauge_chip->gauge_ops->get_prev_battery_mvolts_2cell_min();
	}
}

int oneplus_gauge_get_prev_batt_temperature(void)
{
	int batt_temp = 0;
	if (!g_gauge_chip)
		return 250;
	else {
		if (gauge_dbg_tbat != 0) {
			printk(KERN_ERR "[ONEPLUS_CHG]%s: debug enabled, gauge_dbg_tbat[%d] \n", __func__, gauge_dbg_tbat);
			return gauge_dbg_tbat;
		}
		batt_temp = g_gauge_chip->gauge_ops->get_prev_battery_temperature();
#ifdef CONFIG_HIGH_TEMP_VERSION
	printk(KERN_ERR "[ONEPLUS_CHG]CONFIG_HIGH_TEMP_VERSION enable here,\
			disable high tbat shutdown \n");
	if (batt_temp > 520)
		batt_temp = 520;
#endif
		return batt_temp;
	}
}

int oneplus_gauge_get_prev_batt_soc(void)
{
	if (!g_gauge_chip)
		return 50;
	else
		return g_gauge_chip->gauge_ops->get_prev_battery_soc();
}

int oneplus_gauge_get_prev_batt_current(void)
{
	if (!g_gauge_chip)
		return 100;
	else{
		if (gauge_dbg_ibat != 0) {
			printk(KERN_ERR "[ONEPLUS_CHG]%s:debug enabled,current gauge_dbg_ibat[%d] \n", __func__, gauge_dbg_ibat);
			return gauge_dbg_ibat;
		}
		return g_gauge_chip->gauge_ops->get_prev_average_current();
	}
}

int oneplus_gauge_get_prev_remaining_capacity(void)
{
	if (!g_gauge_chip) {
		return 0;
	} else {
		return g_gauge_chip->gauge_ops->get_prev_batt_remaining_capacity();
	}
}


int oneplus_gauge_update_battery_dod0(void)
{
	if (!g_gauge_chip)
		return 0;
	else
		return g_gauge_chip->gauge_ops->update_battery_dod0();
}


int oneplus_gauge_update_soc_smooth_parameter(void)
{
	if (!g_gauge_chip)
		return 0;
	else
		return g_gauge_chip->gauge_ops->update_soc_smooth_parameter();
}
