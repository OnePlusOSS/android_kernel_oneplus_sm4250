/***************************************************
 * File:touch.c
 * Copyright (c)  2008- 2030  Oneplus Mobile communication Corp.ltd.
 * Description:
 *             tp dev
 * Version:1.0:
 * Date created:2016/09/02
 * Author: hao.wang@Bsp.Driver
 * TAG: BSP.TP.Init
*/


#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/serio.h>
#include "oneplus_touchscreen/tp_devices.h"
#include "oneplus_touchscreen/touchpanel_common.h"

#include "touch.h"

#define MAX_LIMIT_DATA_LENGTH         100

struct tp_dev_name tp_dev_names[] = {
    {TP_BOE,"BOE"},
    {TP_INX, "INX"},
	{TP_BOE_90HZ, "90HZ_BOE"},
	{TP_TXD_90HZ_82n, "TXD"},
    {TP_UNKNOWN, "UNKNOWN"},
};

#define GET_TP_DEV_NAME(tp_type) ((tp_dev_names[tp_type].type == (tp_type))?tp_dev_names[tp_type].name:"UNMATCH")

int g_tp_dev_vendor = TP_UNKNOWN;
int g_chip_name = UNKNOWN_IC;
typedef enum {
    TP_INDEX_NULL,
    ili9881_boe,
	ili9882n_90hz_txd,
	ili9881_90hz_boe,
    nt36525b_inx,
} TP_USED_INDEX;
TP_USED_INDEX tp_used_index  = TP_INDEX_NULL;
extern char* saved_command_line;

/*
* this function is used to judge whether the ic driver should be loaded
* For incell module, tp is defined by lcd module, so if we judge the tp ic
* by the boot command line of containing lcd string, we can also get tp type.
*/

TP_USED_IC __init tp_judge_ic_match(void)
{
    pr_err("[TP] saved_command_line = %s \n", saved_command_line);

    if (strstr(saved_command_line, "mdss_dsi_ili9882n_90hz_video")) {
        g_tp_dev_vendor = TP_TXD_90HZ_82n;
        tp_used_index = ili9882n_90hz_txd;
        g_chip_name =  ILI9882N;
        printk("[TP] g_tp_dev_vendor: %d, tp_used_index: %d\n", g_tp_dev_vendor, tp_used_index);
        return g_chip_name;
    }
    if (strstr(saved_command_line, "mdss_dsi_ili9881h_boe_video")) {
        g_tp_dev_vendor = TP_BOE;
        tp_used_index = ili9881_boe;
        g_chip_name = ILI9881H;
        printk("g_tp_dev_vendor: %d, tp_used_index: %d\n", g_tp_dev_vendor, tp_used_index);
        return g_chip_name;
    }
	if (strnstr(saved_command_line, "mdss_dsi_ili9881h_90hz_boe_video", strlen(saved_command_line))) {
		g_tp_dev_vendor = TP_BOE_90HZ;
		tp_used_index = ili9881_90hz_boe;
		g_chip_name = ILI9881H;
		return g_chip_name;
	}
	if (strstr(saved_command_line, "mdss_dsi_nt36525b_inx_video")) {
		g_tp_dev_vendor = TP_INX;
		tp_used_index = nt36525b_inx;
        g_chip_name =  NT36525B;
        printk("[TP]touchpanel: g_tp_dev_vendor: %d, tp_used_index: %d\n", g_tp_dev_vendor, tp_used_index);
		return g_chip_name;
	}
	pr_err("Lcd module not found\n");
    return UNKNOWN_IC;
}

int tp_util_get_vendor(struct hw_resource *hw_res, struct panel_info *panel_data)
{

    char *vendor = NULL;
    panel_data->test_limit_name = kzalloc(MAX_LIMIT_DATA_LENGTH, GFP_KERNEL);
    if (panel_data->test_limit_name == NULL) {
        pr_err("[TP]panel_data.test_limit_name kzalloc error\n");
    }
    panel_data->tp_type = g_tp_dev_vendor;
    vendor = GET_TP_DEV_NAME(panel_data->tp_type);
    strncpy(panel_data->manufacture_info.manufacture, vendor, MAX_DEVICE_MANU_LENGTH);

    if (panel_data->tp_type == TP_UNKNOWN) {
        pr_err("[TP]%s type is unknown\n", __func__);
        return 0;
    }

    panel_data->vid_len = 7;
    if (panel_data->fw_name) {

        snprintf(panel_data->fw_name, MAX_FW_NAME_LENGTH,
		 "tp/20221/FW_%s_%s.bin",
		panel_data->chip_name, vendor);
    }

	if (panel_data->test_limit_name) {
		snprintf(panel_data->test_limit_name, MAX_LIMIT_DATA_LENGTH,
		"/tp/20221/LIMIT_%s_%s.img", panel_data->chip_name, vendor);
		pr_info("panel_data->test_limit_name = %s\n", panel_data->test_limit_name);
	}

	if (tp_used_index == ili9881_boe) {
		snprintf(panel_data->manufacture_info.version, 14, "0x01");
		snprintf(panel_data->manufacture_info.manufacture, 20, "ili9881h_boe");
		panel_data->firmware_headfile.firmware_data = FW_20221_ILI9881H_BOE;
		panel_data->firmware_headfile.firmware_size = sizeof(FW_20221_ILI9881H_BOE);
		push_component_info(TP, panel_data->manufacture_info.version, panel_data->manufacture_info.manufacture);
	} else if (tp_used_index == ili9881_90hz_boe) {
		snprintf(panel_data->manufacture_info.version, 14, "0xf");
		snprintf(panel_data->manufacture_info.manufacture, 20, "ili9881h_boe");
		panel_data->firmware_headfile.firmware_data = FW_20221_ILI9881H_90HZ_BOE;
		panel_data->firmware_headfile.firmware_size = sizeof(FW_20221_ILI9881H_90HZ_BOE);
		push_component_info(TP, panel_data->manufacture_info.version, panel_data->manufacture_info.manufacture);
    } else if (tp_used_index == ili9882n_90hz_txd) {
	snprintf(panel_data->manufacture_info.version, 14, "0x0C");
	snprintf(panel_data->manufacture_info.manufacture, 20, "ili9882n_txd");
        panel_data->firmware_headfile.firmware_data = FW_20221_ILI9882N_TXD;
        panel_data->firmware_headfile.firmware_size = sizeof(FW_20221_ILI9882N_TXD);
	push_component_info(TP, panel_data->manufacture_info.version, panel_data->manufacture_info.manufacture);
    } else if (tp_used_index == nt36525b_inx) {
        memcpy(panel_data->manufacture_info.version, "FA218IN", 7);
        panel_data->firmware_headfile.firmware_data = FW_20221_NT36525B_INX;
        panel_data->firmware_headfile.firmware_size = sizeof(FW_20221_NT36525B_INX);
    }  else {
        panel_data->firmware_headfile.firmware_data = NULL;
        panel_data->firmware_headfile.firmware_size = 0;
    }


	panel_data->manufacture_info.fw_path = panel_data->fw_name;

	pr_info("fw_path: %s\n", panel_data->manufacture_info.fw_path);
	pr_info("[TP]vendor:%s fw:%s limit:%s\n",
		vendor, panel_data->fw_name,
		panel_data->test_limit_name == NULL ? "NO Limit" : panel_data->test_limit_name);

    return 0;

}
