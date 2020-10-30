/***************************************************
 * File:tp_devices.h
 * Copyright (c)  2008- 2030  Oneplus Mobile communication Corp.ltd.
 * Description:
 *             tp dev
 * Version:1.0:
 * Date created:2016/09/02
 * Author: Tong.han@Bsp.Driver
 * TAG: BSP.TP.Init
 *
 * -------------- Revision History: -----------------
 *  <author >  <data>  <version>  <desc>
 ***************************************************/
#ifndef ONEPLUS_TP_DEVICES_H
#define ONEPLUS_TP_DEVICES_H
//device list define
typedef enum tp_dev{
    TP_BOE,
	TP_INX,
	TP_BOE_90HZ,
	TP_TXD_90HZ_82n,
    TP_UNKNOWN,
}tp_dev;

struct tp_dev_name {
    tp_dev type;
    char name[32];
};

#endif

