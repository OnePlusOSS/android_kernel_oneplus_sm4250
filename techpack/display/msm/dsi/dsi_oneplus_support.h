/***************************************************************
** Copyright (C),  2018,  Oneplus Mobile Comm Corp.,  Ltd
**
** File : dsi_oneplus_support.h
** Description : display driver private management
** Version : 1.0
** Date :
** Author : PSW.MM.Display.Stability
**
** ------------------------------- Revision History: -----------
**  <author>        <data>        <version >        <desc>
**
******************************************************************/
#ifndef _DSI_ONEPLUS_SUPPORT_H_
#define _DSI_ONEPLUS_SUPPORT_H_

#include <linux/err.h>
#include <linux/string.h>
#include <linux/notifier.h>

/* A hardware display blank change occurred */
#define ONEPLUS_DISPLAY_EVENT_BLANK			0x01

/* A hardware display blank early change occurred */
#define ONEPLUS_DISPLAY_EARLY_EVENT_BLANK		0x02


enum oneplus_display_support_list {
	ONEPLUS_SAMSUNG_S6E3FC2X01_DISPLAY_FHD_DSC_CMD_PANEL = 0,
	ONEPLUS_SAMSUNG_SOFEF03F_M_DISPLAY_FHD_DSC_CMD_PANEL,
	ONEPLUS_DISPLAY_UNKNOW,
};

enum oneplus_display_power_status {
	ONEPLUS_DISPLAY_POWER_OFF = 0,
	ONEPLUS_DISPLAY_POWER_DOZE,
	ONEPLUS_DISPLAY_POWER_ON,
	ONEPLUS_DISPLAY_POWER_DOZE_SUSPEND,
	ONEPLUS_DISPLAY_POWER_ON_UNKNOW,
};

enum oneplus_display_scene {
	ONEPLUS_DISPLAY_NORMAL_SCENE = 0,
	ONEPLUS_DISPLAY_NORMAL_HBM_SCENE,
	ONEPLUS_DISPLAY_AOD_SCENE,
	ONEPLUS_DISPLAY_AOD_HBM_SCENE,
	ONEPLUS_DISPLAY_UNKNOW_SCENE,
};

enum oneplus_display_feature {
	ONEPLUS_DISPLAY_HDR = 0,
	ONEPLUS_DISPLAY_SEED,
	ONEPLUS_DISPLAY_HBM,
	ONEPLUS_DISPLAY_LBR,
	ONEPLUS_DISPLAY_AOD,
	ONEPLUS_DISPLAY_ULPS,
	ONEPLUS_DISPLAY_ESD_CHECK,
	ONEPLUS_DISPLAY_DYNAMIC_MIPI,
	ONEPLUS_DISPLAY_PARTIAL_UPDATE,
	ONEPLUS_DISPLAY_FEATURE_MAX,
};

typedef struct panel_serial_info
{
	int reg_index;
	uint64_t year;
	uint64_t month;
	uint64_t day;
	uint64_t hour;
	uint64_t minute;
	uint64_t second;
	uint64_t reserved[2];
} PANEL_SERIAL_INFO;


typedef struct oneplus_display_notifier_event {
	enum oneplus_display_power_status status;
	void *data;
}ONEPLUS_DISPLAY_NOTIFIER_EVENT;

int oneplus_display_register_client(struct notifier_block *nb);

int oneplus_display_unregister_client(struct notifier_block *nb);

void notifier_oneplus_display_early_status(enum oneplus_display_power_status power_status);

void notifier_oneplus_display_status(enum oneplus_display_power_status power_status);

bool is_oneplus_correct_display(enum oneplus_display_support_list lcd_name);

bool is_silence_reboot(void);

int set_oneplus_display_vendor(const char * display_name);

void set_oneplus_display_power_status(enum oneplus_display_power_status power_status);

enum oneplus_display_power_status get_oneplus_display_power_status(void);

void set_oneplus_display_scene(enum oneplus_display_scene display_scene);

enum oneplus_display_scene get_oneplus_display_scene(void);

bool is_oneplus_display_support_feature(enum oneplus_display_feature feature_name);

int oneplus_display_get_resolution(unsigned int *xres, unsigned int *yres);

#endif /* _DSI_ONEPLUS_SUPPORT_H_ */

