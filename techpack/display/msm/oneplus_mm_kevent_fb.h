/***************************************************************
** Copyright (C), 2018, Oneplus Mobile Comm Corp.,  Ltd
**
** File : oneplus_mm_kevent_fb.h
** Description : MM kevent fb data
** Version : 1.0
** Date :
** Author : PSW.MM.Display.Stability
**
** ------------------------------- Revision History: -----------
**  <author>        <data>        <version >        <desc>
**
**
******************************************************************/
#ifndef _ONEPLUS_MM_KEVENT_FB_
#define _ONEPLUS_MM_KEVENT_FB_

enum ONEPLUS_MM_DIRVER_FB_EVENT_ID {
	ONEPLUS_MM_DIRVER_FB_EVENT_ID_ESD = 401,
	ONEPLUS_MM_DIRVER_FB_EVENT_ID_VSYNC,
	ONEPLUS_MM_DIRVER_FB_EVENT_ID_HBM,
	ONEPLUS_MM_DIRVER_FB_EVENT_ID_FFLSET,
	ONEPLUS_MM_DIRVER_FB_EVENT_ID_MTK_CMDQ,
	ONEPLUS_MM_DIRVER_FB_EVENT_ID_MTK_UNDERFLOW,
	ONEPLUS_MM_DIRVER_FB_EVENT_ID_MTK_FENCE,
	ONEPLUS_MM_DIRVER_FB_EVENT_ID_MTK_JS,
	ONEPLUS_MM_DIRVER_FB_EVENT_ID_SMMU,
	ONEPLUS_MM_DIRVER_FB_EVENT_ID_GPU_FAULT,
	ONEPLUS_MM_DIRVER_FB_EVENT_ID_PANEL_MATCH_FAULT,
	ONEPLUS_MM_DIRVER_FB_EVENT_ID_AUDIO = 801,
};

enum ONEPLUS_MM_DIRVER_FB_EVENT_MODULE {
	ONEPLUS_MM_DIRVER_FB_EVENT_MODULE_DISPLAY = 0,
	ONEPLUS_MM_DIRVER_FB_EVENT_MODULE_AUDIO
};

int upload_mm_kevent_fb_data(enum ONEPLUS_MM_DIRVER_FB_EVENT_MODULE module, unsigned char *payload);

#endif /* _ONEPLUS_MM_KEVENT_FB_ */

