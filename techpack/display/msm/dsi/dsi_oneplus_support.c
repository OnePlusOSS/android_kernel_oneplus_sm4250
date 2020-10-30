/***************************************************************
** Copyright (C),  2018,  Oneplus Mobile Comm Corp.,  Ltd
**
** File : dsi_oneplus_support.c
** Description : display driver private management
** Version : 1.0
** Date :
** Author : PSW.MM.Display.Stability
**
** ------------------------------- Revision History: -----------
**  <author>        <data>        <version >        <desc>
** 
******************************************************************/
#include "dsi_oneplus_support.h"
/*
#include <soc/oneplus/boot_mode.h>
#include <soc/oneplus/oneplus_project.h>
#include <soc/oneplus/device_info.h>
#include <soc/oneplus/boot_mode.h>
*/
#include <linux/notifier.h>
#include <linux/module.h>

static enum oneplus_display_support_list  oneplus_display_vendor = ONEPLUS_DISPLAY_UNKNOW;
static enum oneplus_display_power_status oneplus_display_status = ONEPLUS_DISPLAY_POWER_OFF;
static enum oneplus_display_scene oneplus_siaplay_save_scene = ONEPLUS_DISPLAY_NORMAL_SCENE;

static BLOCKING_NOTIFIER_HEAD(oneplus_display_notifier_list);

int oneplus_display_register_client(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&oneplus_display_notifier_list,
						nb);
}
EXPORT_SYMBOL(oneplus_display_register_client);


int oneplus_display_unregister_client(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&oneplus_display_notifier_list,
						  nb);
}
EXPORT_SYMBOL(oneplus_display_unregister_client);

static int oneplus_display_notifier_call_chain(unsigned long val, void *v)
{
	return blocking_notifier_call_chain(&oneplus_display_notifier_list, val,
					    v);
}

bool is_oneplus_correct_display(enum oneplus_display_support_list lcd_name) {
	return (oneplus_display_vendor == lcd_name ? true:false);
}

bool is_silence_reboot(void) {
/*
	if((MSM_BOOT_MODE__SILENCE == get_boot_mode()) || (MSM_BOOT_MODE__SAU == get_boot_mode())) {
		return true;
	} else {
		return false;
	}
*/
		return false;
}

int set_oneplus_display_vendor(const char * display_name) {
	if (display_name == NULL)
		return -1;
	/*
	if (!strcmp(display_name,"dsi_samsung_fhd_plus_dsc_cmd_display")) {
		oneplus_display_vendor = ONEPLUS_SAMSUNG_S6E3FC2X01_DISPLAY_FHD_DSC_CMD_PANEL;
		register_device_proc("lcd", "S6E3FC2X01", "samsung1024");
	} else if (!strcmp(display_name,"dsi_samsung_fhd_plus_dsc_cmd_61fps_display")) {
		oneplus_display_vendor = ONEPLUS_SAMSUNG_S6E3FC2X01_DISPLAY_FHD_DSC_CMD_PANEL;
		register_device_proc("lcd", "S6E3FC2X01", "samsung1024");
	} else if (!strcmp(display_name,"dsi_samsung_fhd_plus_dsc_cmd_90fps_display")) {
		oneplus_display_vendor = ONEPLUS_SAMSUNG_SOFEF03F_M_DISPLAY_FHD_DSC_CMD_PANEL;
		register_device_proc("lcd", "SOFEF03F_M", "samsung1024");
	}else {
		oneplus_display_vendor = ONEPLUS_DISPLAY_UNKNOW;
		pr_err("%s panel vendor info set failed, use vendor dummy!", __func__);
		register_device_proc("lcd", "PanelVendorDummy", "samsung1024");
	}
	*/
	return 0;
}

void notifier_oneplus_display_early_status(enum oneplus_display_power_status power_status) {
	int blank;
	ONEPLUS_DISPLAY_NOTIFIER_EVENT oneplus_notifier_data;
	switch (power_status) {
		case ONEPLUS_DISPLAY_POWER_ON:
			blank = ONEPLUS_DISPLAY_POWER_ON;
			oneplus_notifier_data.data = &blank;
			oneplus_notifier_data.status = ONEPLUS_DISPLAY_POWER_ON;
			oneplus_display_notifier_call_chain(ONEPLUS_DISPLAY_EARLY_EVENT_BLANK,
						     &oneplus_notifier_data);
			break;
		case ONEPLUS_DISPLAY_POWER_DOZE:
			blank = ONEPLUS_DISPLAY_POWER_DOZE;
			oneplus_notifier_data.data = &blank;
			oneplus_notifier_data.status = ONEPLUS_DISPLAY_POWER_DOZE;
			oneplus_display_notifier_call_chain(ONEPLUS_DISPLAY_EARLY_EVENT_BLANK,
						     &oneplus_notifier_data);
			break;
		case ONEPLUS_DISPLAY_POWER_DOZE_SUSPEND:
			blank = ONEPLUS_DISPLAY_POWER_DOZE_SUSPEND;
			oneplus_notifier_data.data = &blank;
			oneplus_notifier_data.status = ONEPLUS_DISPLAY_POWER_DOZE_SUSPEND;
			oneplus_display_notifier_call_chain(ONEPLUS_DISPLAY_EARLY_EVENT_BLANK,
						     &oneplus_notifier_data);
			break;
		case ONEPLUS_DISPLAY_POWER_OFF:
			blank = ONEPLUS_DISPLAY_POWER_OFF;
			oneplus_notifier_data.data = &blank;
			oneplus_notifier_data.status = ONEPLUS_DISPLAY_POWER_OFF;
			oneplus_display_notifier_call_chain(ONEPLUS_DISPLAY_EARLY_EVENT_BLANK,
						     &oneplus_notifier_data);
			break;
		default:
			break;
		}
}

void notifier_oneplus_display_status(enum oneplus_display_power_status power_status) {
	int blank;
	ONEPLUS_DISPLAY_NOTIFIER_EVENT oneplus_notifier_data;
	switch (power_status) {
		case ONEPLUS_DISPLAY_POWER_ON:
			blank = ONEPLUS_DISPLAY_POWER_ON;
			oneplus_notifier_data.data = &blank;
			oneplus_notifier_data.status = ONEPLUS_DISPLAY_POWER_ON;
			oneplus_display_notifier_call_chain(ONEPLUS_DISPLAY_EVENT_BLANK,
						     &oneplus_notifier_data);
			break;
		case ONEPLUS_DISPLAY_POWER_DOZE:
			blank = ONEPLUS_DISPLAY_POWER_DOZE;
			oneplus_notifier_data.data = &blank;
			oneplus_notifier_data.status = ONEPLUS_DISPLAY_POWER_DOZE;
			oneplus_display_notifier_call_chain(ONEPLUS_DISPLAY_EVENT_BLANK,
						     &oneplus_notifier_data);
			break;
		case ONEPLUS_DISPLAY_POWER_DOZE_SUSPEND:
			blank = ONEPLUS_DISPLAY_POWER_DOZE_SUSPEND;
			oneplus_notifier_data.data = &blank;
			oneplus_notifier_data.status = ONEPLUS_DISPLAY_POWER_DOZE_SUSPEND;
			oneplus_display_notifier_call_chain(ONEPLUS_DISPLAY_EVENT_BLANK,
						     &oneplus_notifier_data);
			break;
		case ONEPLUS_DISPLAY_POWER_OFF:
			blank = ONEPLUS_DISPLAY_POWER_OFF;
			oneplus_notifier_data.data = &blank;
			oneplus_notifier_data.status = ONEPLUS_DISPLAY_POWER_OFF;
			oneplus_display_notifier_call_chain(ONEPLUS_DISPLAY_EVENT_BLANK,
						     &oneplus_notifier_data);
			break;
		default:
			break;
		}
}

void set_oneplus_display_power_status(enum oneplus_display_power_status power_status) {
	oneplus_display_status = power_status;
}

enum oneplus_display_power_status get_oneplus_display_power_status(void) {
	return oneplus_display_status;
}
EXPORT_SYMBOL(get_oneplus_display_power_status);

void set_oneplus_display_scene(enum oneplus_display_scene display_scene) {
	oneplus_siaplay_save_scene = display_scene;
}

enum oneplus_display_scene get_oneplus_display_scene(void) {
	return oneplus_siaplay_save_scene;
}

EXPORT_SYMBOL(get_oneplus_display_scene);

bool is_oneplus_display_support_feature(enum oneplus_display_feature feature_name) {
	bool ret = false;
	switch (feature_name) {
		case ONEPLUS_DISPLAY_HDR:
			ret = false;
			break;
		case ONEPLUS_DISPLAY_SEED:
			ret = true;
			break;
		case ONEPLUS_DISPLAY_HBM:
			ret = true;
			break;
		case ONEPLUS_DISPLAY_LBR:
			ret = true;
			break;
		case ONEPLUS_DISPLAY_AOD:
			ret = true;
			break;
		case ONEPLUS_DISPLAY_ULPS:
			ret = false;
			break;
		case ONEPLUS_DISPLAY_ESD_CHECK:
			ret = true;
			break;
		case ONEPLUS_DISPLAY_DYNAMIC_MIPI:
			ret = true;
			break;
		case ONEPLUS_DISPLAY_PARTIAL_UPDATE:
			ret = false;
			break;
		default:
			break;
	}
	return ret;
}


