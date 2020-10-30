/************************************************************************************
** Copyright (C), 2008-2012, ONEPLUS Mobile Comm Corp., Ltd
** 
** Description:  
**     change define of boot_mode here for other place to use it
** Version: 1.0 
** --------------------------- Revision History: --------------------------------
** 	           <author>	           <data>			    <desc>
** tong.han@BasicDrv.TP&LCD      11/01/2014          add this file
************************************************************************************/
#ifndef _ONEPLUS_BOOT_H
#define _ONEPLUS_BOOT_H
enum{
        MSM_BOOT_MODE__NORMAL,
        MSM_BOOT_MODE__FASTBOOT,
        MSM_BOOT_MODE__RECOVERY,
        MSM_BOOT_MODE__FACTORY,
        MSM_BOOT_MODE__RF,
        MSM_BOOT_MODE__WLAN,
        MSM_BOOT_MODE__MOS,
        MSM_BOOT_MODE__CHARGE,
        MSM_BOOT_MODE__SILENCE,
        MSM_BOOT_MODE__SAU,
        //xiaofan.yang@PSW.TECH.AgingTest, 2019/01/07,Add for factory agingtest
        MSM_BOOT_MODE__AGING = 998,
        MSM_BOOT_MODE__SAFE = 999,
};

extern int get_boot_mode(void);
extern bool qpnp_is_power_off_charging(void);
extern bool qpnp_is_charger_reboot(void);

#ifdef PHOENIX_PROJECT
extern bool op_is_monitorable_boot(void);
#endif
#endif  /*_ONEPLUS_BOOT_H*/
