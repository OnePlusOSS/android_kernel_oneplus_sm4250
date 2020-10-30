/***************************************************************
** Copyright (C),  2018,  Oneplus Mobile Comm Corp.,  Ltd
**
** File : oneplus_display_private_api.h
** Description : oneplus display private api implement
** Version : 1.0
** Date :
** Author : PSW.MM.Display.Stability
**
** ------------------------------- Revision History: -----------
**  <author>        <data>        <version >        <desc>
**
******************************************************************/
#include "oneplus_display_private_api.h"
/*
 * we will create a sysfs which called /sys/kernel/oneplus_display,
 * In that directory, oneplus display private api can be called
 */
#include <linux/notifier.h>
#include <linux/msm_drm_notify.h>
#include "../oneplus_mm_kevent_fb.h"
#include <soc/oneplus/device_info.h>
#include <linux/oem/project_info.h>

int hbm_mode = 0;
int spr_mode = 0;
int failsafe_mode = 0;
int seed_mode = 0;
int aod_light_mode = 0;
int lcd_closebl_flag = 0;
int lcd_closebl_flag_fp = 0;
int oneplus_request_power_status = ONEPLUS_DISPLAY_POWER_ON;
int oneplus_aod_gamma_check_data = -1;
int cabc_mode = 2;

extern int oneplus_underbrightness_alpha;
extern int msm_drm_notifier_call_chain(unsigned long val, void *v);
int oneplus_dc2_alpha;
int oneplus_dimlayer_bl_enable_v3 = 0;
int oneplus_dimlayer_bl_enable_v2 = 0;
int oneplus_dimlayer_bl_enable = 0;
int oneplus_dimlayer_bl_enabled = 0;
int oneplus_dimlayer_bl_alpha = 260;
int oneplus_dimlayer_bl_alpha_value = 260;
int oneplus_dimlayer_bl_enable_real = 0;
int oneplus_dimlayer_dither_threshold = 0;
int oneplus_dimlayer_dither_bitdepth = 6;
int oneplus_dimlayer_bl_delay = 500;
int oneplus_dimlayer_bl_delay_after = 0;
int oneplus_boe_dc_min_backlight = 175;
int oneplus_boe_dc_max_backlight = 3350;
int oneplus_boe_max_backlight = 2050;
int oneplus_dimlayer_bl_enable_v3_real;
int oneplus_onscreenfp_status = 0;
int oneplus_display_mode = 1;
bool oneplus_dc_v2_on = false;

ktime_t oneplus_onscreenfp_pressed_time;
u32 oneplus_onscreenfp_vblank_count = 0;

#define PANEL_TX_MAX_BUF 256
#define PANEL_CMD_MIN_TX_COUNT 2
#define FFL_FP_LEVEL 150

DEFINE_MUTEX(oneplus_power_status_lock);
DEFINE_MUTEX(oneplus_seed_lock);
DEFINE_MUTEX(oneplus_hbm_lock);
DEFINE_MUTEX(oneplus_aod_light_mode_lock);
DEFINE_MUTEX(oneplus_spr_lock);
DEFINE_MUTEX(oneplus_failsafe_lock);
DEFINE_MUTEX(oneplus_cabc_lock);

static DECLARE_WAIT_QUEUE_HEAD(oneplus_aod_wait);

static inline bool is_oneplus_display_aod_mode(void)
{
	return !oneplus_onscreenfp_status && !oneplus_display_mode;
}

bool is_oneplus_aod_ramless(void)
{
	struct dsi_display *display = get_main_display();
	if (!display || !display->panel)
		return false;
	return display->panel->oneplus_priv.is_aod_ramless;
}

int oneplus_set_display_vendor(struct dsi_display *display)
{
	if (!display || !display->panel ||
	    !display->panel->oneplus_priv.vendor_name ||
	    !display->panel->oneplus_priv.manufacture_name) {
		pr_err("failed to config lcd proc device");
		return -EINVAL;
	}

	register_device_proc("lcd", (char *)display->panel->oneplus_priv.vendor_name,
			     (char *)display->panel->oneplus_priv.manufacture_name);

	push_component_info(LCD, (char *)display->panel->oneplus_priv.vendor_name,
				(char *)display->panel->oneplus_priv.manufacture_name);

	return 0;
}

bool is_dsi_panel(struct drm_crtc *crtc)
{
	struct dsi_display *display = get_main_display();

	if (!display || !display->drm_conn || !display->drm_conn->state) {
		pr_err("failed to find dsi display\n");
		return false;
	}

	if (crtc != display->drm_conn->state->crtc)
		return false;

	return true;
}

int dsi_panel_aod_on(struct dsi_panel *panel) {
	int rc = 0;

	if (!panel) {
		pr_err("Invalid params\n");
		return -EINVAL;
	}

	mutex_lock(&panel->panel_lock);

	if (!dsi_panel_initialized(panel)) {
		rc = -EINVAL;
		goto error;
	}

	rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_AOD_ON);
	if (rc) {
		pr_err("[%s] failed to send DSI_CMD_AOD_ON cmds, rc=%d\n",
		       panel->name, rc);
	}

error:
	mutex_unlock(&panel->panel_lock);
	return rc;
}

int dsi_panel_aod_off(struct dsi_panel *panel) {
	int rc = 0;

	if (!panel) {
		pr_err("Invalid params\n");
		return -EINVAL;
	}

	mutex_lock(&panel->panel_lock);

	if (!dsi_panel_initialized(panel)) {
		rc = -EINVAL;
		goto error;
	}

	rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_AOD_OFF);
	if (rc) {
		pr_err("[%s] failed to send DSI_CMD_AOD_OFF cmds, rc=%d\n",
		       panel->name, rc);
	}

error:
	mutex_unlock(&panel->panel_lock);
	return rc;
}

int dsi_panel_failsafe_on(struct dsi_panel *panel) {
	int rc = 0;

	if (!panel) {
		pr_err("Invalid params\n");
		return -EINVAL;
	}

	mutex_lock(&panel->panel_lock);

	if (!dsi_panel_initialized(panel)) {
		rc = -EINVAL;
		goto error;
	}

	rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_FAILSAFE_ON);
	if (rc) {
		pr_err("[%s] failed to send DSI_CMD_FAILSAFE_ON cmds, rc=%d\n",
		       panel->name, rc);
	}

error:
	mutex_unlock(&panel->panel_lock);
	return rc;
}

int dsi_panel_failsafe_off(struct dsi_panel *panel) {
	int rc = 0;

	if (!panel) {
		pr_err("Invalid params\n");
		return -EINVAL;
	}

	mutex_lock(&panel->panel_lock);

	if (!dsi_panel_initialized(panel)) {
		rc = -EINVAL;
		goto error;
	}

	rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_FAILSAFE_OFF);
	if (rc) {
		pr_err("[%s] failed to send DSI_CMD_FAILSAFE_OFF cmds, rc=%d\n",
		       panel->name, rc);
	}

error:
	mutex_unlock(&panel->panel_lock);
	return rc;
}

int dsi_panel_hbm_on(struct dsi_panel *panel) {
	int rc = 0;

	if (!panel) {
		pr_err("Invalid params\n");
		return -EINVAL;
	}

	mutex_lock(&panel->panel_lock);

	if (!dsi_panel_initialized(panel)) {
		rc = -EINVAL;
		goto error;
	}

	rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_HBM_ON);
	if (rc) {
		pr_err("[%s] failed to send DSI_CMD_HBM_ON cmds, rc=%d\n",
		       panel->name, rc);
	}

error:
	mutex_unlock(&panel->panel_lock);
	return rc;
}

int dsi_panel_normal_hbm_on(struct dsi_panel *panel) {
	int rc = 0;

	if (!panel) {
		pr_err("Invalid params\n");
		return -EINVAL;
	}

	mutex_lock(&panel->panel_lock);

	if (!dsi_panel_initialized(panel)) {
		rc = -EINVAL;
		goto error;
	}

	rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_NORMAL_HBM_ON);
	if (rc) {
		pr_err("[%s] failed to send DSI_CMD_NORMAL_HBM_ON cmds, rc=%d\n",
		       panel->name, rc);
	}

error:
	mutex_unlock(&panel->panel_lock);
	return rc;
}

int dsi_panel_hbm_off(struct dsi_panel *panel) {
	int rc = 0;

	if (!panel) {
		pr_err("Invalid params\n");
		return -EINVAL;
	}

	mutex_lock(&panel->panel_lock);

	if (!dsi_panel_initialized(panel)) {
		rc = -EINVAL;
		goto error;
	}

	/* if hbm already set by onscreenfinger, keep it */
	if (panel->is_hbm_enabled) {
		rc = 0;
		goto error;
	}

	dsi_panel_set_backlight(panel, panel->bl_config.bl_level);

	rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_HBM_OFF);
	if (rc) {
		pr_err("[%s] failed to send DSI_CMD_HBM_OFF cmds, rc=%d\n",
		       panel->name, rc);
	}

error:
	mutex_unlock(&panel->panel_lock);
	return rc;
}

int dsi_panel_cabc_mode(struct dsi_panel *panel, enum dsi_cmd_set_type type) {
	int rc = 0;

	if (!panel) {
		pr_err("Invalid params\n");
		return -EINVAL;
	}

	if (panel->panel_type == DSI_DISPLAY_PANEL_TYPE_OLED) {
		printk("it's not lcd, cabc no need!\n");
		return -EINVAL;
	}

	mutex_lock(&panel->panel_lock);

	if (!dsi_panel_initialized(panel)) {
		rc = -EINVAL;
		goto error;
	}

	rc = dsi_panel_tx_cmd_set(panel, type);
	if (rc) {
		pr_err("[%s] failed to send new cabc cmds, rc=%d, type = %d\n",
		       panel->name, rc, type);
	}

error:
	mutex_unlock(&panel->panel_lock);
	return rc;
}

int dsi_panel_seed_mode_unlock(struct dsi_panel *panel, int mode)
{
	int rc = 0;

	if (!dsi_panel_initialized(panel))
		return -EINVAL;
	if (oneplus_dc_v2_on) {
		rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SEED_ENTER);

		if (rc) {
			pr_err("[%s] failed to send DSI_CMD_SEED_ENTER cmds, rc=%d\n",
				panel->name, rc);
		}
	}
	switch (mode) {
	case 0:
		rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SEED_MODE0);
		if (rc) {
			pr_err("[%s] failed to send DSI_CMD_SEED_MODE0 cmds, rc=%d\n",
					panel->name, rc);
		}
		break;
	case 1:
		rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SEED_MODE1);
		if (rc) {
			pr_err("[%s] failed to send DSI_CMD_SEED_MODE1 cmds, rc=%d\n",
					panel->name, rc);
		}
		break;
	case 2:
		rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SEED_MODE2);
		if (rc) {
			pr_err("[%s] failed to send DSI_CMD_SEED_MODE2 cmds, rc=%d\n",
					panel->name, rc);
		}
		break;
	case 3:
		rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SEED_MODE3);
		if (rc) {
			pr_err("[%s] failed to send DSI_CMD_SEED_MODE3 cmds, rc=%d\n",
					panel->name, rc);
		}
		break;
	case 4:
		rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SEED_MODE4);
		if (rc) {
			pr_err("[%s] failed to send DSI_CMD_SEED_MODE4 cmds, rc=%d\n",
					panel->name, rc);
		}
		break;
	default:
		rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SEED_OFF);
		if (rc) {
			pr_err("[%s] failed to send DSI_CMD_SEED_OFF cmds, rc=%d\n",
					panel->name, rc);
		}
		pr_err("[%s] seed mode Invalid %d\n",
			panel->name, mode);
	}
	if (!oneplus_dc_v2_on) {
		rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SEED_EXIT);
		if (rc) {
			pr_err("[%s] failed to send DSI_CMD_SEED_EXIT cmds, rc=%d\n",
				panel->name, rc);
		}
	}

	return rc;
}

int dsi_display_cabc_mode(struct dsi_display *display, int mode) {
	int rc = 0;
	if (!display || !display->panel) {
		pr_err("Invalid params\n");
		return -EINVAL;
	}

	if (display->panel->panel_type == DSI_DISPLAY_PANEL_TYPE_OLED) {
		printk("it's not lcd, cabc no need!\n");
		return -EINVAL;
	}

	mutex_lock(&display->display_lock);

		/* enable the clk vote for CMD mode panels */
	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		dsi_display_clk_ctrl(display->dsi_clk_handle,
			DSI_CORE_CLK, DSI_CLK_ON);
	}

	switch (mode) {
	case 0:
		rc = dsi_panel_cabc_mode(display->panel, DSI_CMD_CABC_OFF);
		if (rc)
			pr_err("[%s] failed to dsi_panel_cabc_low_mode, rc=%d\n",
			       display->name, rc);
		break;
	case 1:
		rc = dsi_panel_cabc_mode(display->panel, DSI_CMD_CABC_LOW_MODE);
		if (rc)
			pr_err("[%s] failed to dsi_panel_cabc_low_mode, rc=%d\n",
			       display->name, rc);
		break;
	case 2:
		rc = dsi_panel_cabc_mode(display->panel, DSI_CMD_CABC_HIGH_MODE);
		if (rc)
			pr_err("[%s] failed to dsi_panel_cabc_low_mode, rc=%d\n",
			       display->name, rc);
		break;
	default:
		printk(KERN_ERR "%s error_input\n", __func__);
		break;
	}

	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
	rc = dsi_display_clk_ctrl(display->dsi_clk_handle,
				DSI_CORE_CLK, DSI_CLK_OFF);
	}
	mutex_unlock(&display->display_lock);
	return rc;
}

int dsi_panel_seed_mode(struct dsi_panel *panel, int mode) {
	int rc = 0;

	if (!panel) {
		pr_err("Invalid params\n");
		return -EINVAL;
	}

	mutex_lock(&panel->panel_lock);

	rc = dsi_panel_seed_mode_unlock(panel, mode);

	mutex_unlock(&panel->panel_lock);
	return rc;
}

int dsi_panel_spr_mode(struct dsi_panel *panel, int mode) {
	int rc = 0;

	if (!panel) {
		pr_err("Invalid params\n");
		return -EINVAL;
	}

	mutex_lock(&panel->panel_lock);

	if (!dsi_panel_initialized(panel)) {
		rc = -EINVAL;
		goto error;
	}

	switch (mode) {
	case 0:
		rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SPR_MODE0);
		if (rc) {
			pr_err("[%s] failed to send DSI_CMD_SPR_MODE0 cmds, rc=%d\n",
					panel->name, rc);
		}
		break;
	case 1:
		rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SPR_MODE1);
		if (rc) {
			pr_err("[%s] failed to send DSI_CMD_SPR_MODE1 cmds, rc=%d\n",
					panel->name, rc);
		}
		break;
	case 2:
		rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SPR_MODE2);
		if (rc) {
			pr_err("[%s] failed to send DSI_CMD_SEED_MODE2 cmds, rc=%d\n",
					panel->name, rc);
		}
		break;
	default:
		rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SPR_MODE0);  //gaos edit default is spr mode 0
		if (rc) {
			pr_err("[%s] failed to send DSI_CMD_SPR_MODE0 cmds, rc=%d\n",
					panel->name, rc);
		}
		pr_err("[%s] seed mode Invalid %d\n",
			panel->name, mode);
	}

error:
	mutex_unlock(&panel->panel_lock);
	return rc;
}

int dsi_panel_read_panel_reg(struct dsi_display_ctrl *ctrl, struct dsi_panel *panel, u8 cmd, void *rbuf,  size_t len)
{
	int rc = 0;
	struct dsi_cmd_desc cmdsreq;
	u32 flags = 0;

	if (!panel || !ctrl || !ctrl->ctrl) {
		return -EINVAL;
	}

	if (!dsi_ctrl_validate_host_state(ctrl->ctrl)) {
		return 1;
	}

	/* acquire panel_lock to make sure no commands are in progress */
	mutex_lock(&panel->panel_lock);

	if (!dsi_panel_initialized(panel)) {
		rc = -EINVAL;
		goto error;
	}

	memset(&cmdsreq, 0x0, sizeof(cmdsreq));
	cmdsreq.msg.type = 0x06;
	cmdsreq.msg.tx_buf = &cmd;
	cmdsreq.msg.tx_len = 1;
	cmdsreq.msg.rx_buf = rbuf;
	cmdsreq.msg.rx_len = len;
	cmdsreq.msg.flags |= MIPI_DSI_MSG_LASTCOMMAND;
	flags |= (DSI_CTRL_CMD_FETCH_MEMORY | DSI_CTRL_CMD_READ |
		DSI_CTRL_CMD_CUSTOM_DMA_SCHED |
		DSI_CTRL_CMD_LAST_COMMAND);

	rc = dsi_ctrl_cmd_transfer(ctrl->ctrl, &cmdsreq.msg, &flags);
	if (rc <= 0) {
		pr_err("%s, dsi_display_read_panel_reg rx cmd transfer failed rc=%d\n",
			__func__,
			rc);
		goto error;
	}
error:
	/* release panel_lock */
	mutex_unlock(&panel->panel_lock);
	return rc;
}

int dsi_display_aod_on(struct dsi_display *display) {
	int rc = 0;
	if (!display || !display->panel) {
		pr_err("Invalid params\n");
		return -EINVAL;
	}

	mutex_lock(&display->display_lock);

		/* enable the clk vote for CMD mode panels */
	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		dsi_display_clk_ctrl(display->dsi_clk_handle,
			DSI_CORE_CLK, DSI_CLK_ON);
	}

	rc = dsi_panel_aod_on(display->panel);
		if (rc) {
			pr_err("[%s] failed to dsi_panel_aod_on, rc=%d\n",
			       display->name, rc);
	}

	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
	rc = dsi_display_clk_ctrl(display->dsi_clk_handle,
				DSI_CORE_CLK, DSI_CLK_OFF);

	}
	mutex_unlock(&display->display_lock);
	return rc;
}

int dsi_display_aod_off(struct dsi_display *display) {
	int rc = 0;
	if (!display || !display->panel) {
		pr_err("Invalid params\n");
		return -EINVAL;
	}

	mutex_lock(&display->display_lock);

		/* enable the clk vote for CMD mode panels */
	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		dsi_display_clk_ctrl(display->dsi_clk_handle,
			DSI_CORE_CLK, DSI_CLK_ON);
	}

	rc = dsi_panel_aod_off(display->panel);
		if (rc) {
			pr_err("[%s] failed to dsi_panel_aod_off, rc=%d\n",
			       display->name, rc);
	}

	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
	rc = dsi_display_clk_ctrl(display->dsi_clk_handle,
				DSI_CORE_CLK, DSI_CLK_OFF);

	}
	mutex_unlock(&display->display_lock);
	return rc;
}

int dsi_display_failsafe_on(struct dsi_display *display) {
	int rc = 0;
	if (!display || !display->panel) {
		pr_err("Invalid params\n");
		return -EINVAL;
	}

	mutex_lock(&display->display_lock);

		/* enable the clk vote for CMD mode panels */
	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		dsi_display_clk_ctrl(display->dsi_clk_handle,
			DSI_CORE_CLK, DSI_CLK_ON);
	}

	rc = dsi_panel_failsafe_on(display->panel);
		if (rc) {
			pr_err("[%s] failed to dsi_panel_failsafe_on, rc=%d\n",
			       display->name, rc);
	}

	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
	rc = dsi_display_clk_ctrl(display->dsi_clk_handle,
				DSI_CORE_CLK, DSI_CLK_OFF);

	}
	mutex_unlock(&display->display_lock);
	return rc;
}

int dsi_display_failsafe_off(struct dsi_display *display) {
	int rc = 0;
	if (!display || !display->panel) {
		pr_err("Invalid params\n");
		return -EINVAL;
	}

	mutex_lock(&display->display_lock);

		/* enable the clk vote for CMD mode panels */
	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		dsi_display_clk_ctrl(display->dsi_clk_handle,
			DSI_CORE_CLK, DSI_CLK_ON);
	}

	rc = dsi_panel_failsafe_off(display->panel);
		if (rc) {
			pr_err("[%s] failed to dsi_panel_failsafe_off, rc=%d\n",
			       display->name, rc);
	}

	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
	rc = dsi_display_clk_ctrl(display->dsi_clk_handle,
				DSI_CORE_CLK, DSI_CLK_OFF);

	}
	mutex_unlock(&display->display_lock);
	return rc;
}

int dsi_display_hbm_on(struct dsi_display *display) {
	int rc = 0;
	if (!display || !display->panel) {
		pr_err("Invalid params\n");
		return -EINVAL;
	}

	mutex_lock(&display->display_lock);

		/* enable the clk vote for CMD mode panels */
	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		dsi_display_clk_ctrl(display->dsi_clk_handle,
			DSI_CORE_CLK, DSI_CLK_ON);
	}

	rc = dsi_panel_hbm_on(display->panel);
		if (rc) {
			pr_err("[%s] failed to dsi_panel_hbm_on, rc=%d\n",
			       display->name, rc);
	}

	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
	rc = dsi_display_clk_ctrl(display->dsi_clk_handle,
				DSI_CORE_CLK, DSI_CLK_OFF);

	}
	mutex_unlock(&display->display_lock);
	return rc;
}

int dsi_display_normal_hbm_on(struct dsi_display *display) {
	int rc = 0;
	if (!display || !display->panel) {
		pr_err("Invalid params\n");
		return -EINVAL;
	}

	mutex_lock(&display->display_lock);

		/* enable the clk vote for CMD mode panels */
	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		dsi_display_clk_ctrl(display->dsi_clk_handle,
			DSI_CORE_CLK, DSI_CLK_ON);
	}

	rc = dsi_panel_normal_hbm_on(display->panel);
		if (rc) {
			pr_err("[%s] failed to dsi_panel_normal_hbm_on, rc=%d\n",
			       display->name, rc);
	}

	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
	rc = dsi_display_clk_ctrl(display->dsi_clk_handle,
				DSI_CORE_CLK, DSI_CLK_OFF);

	}
	mutex_unlock(&display->display_lock);
	return rc;
}

int dsi_display_hbm_off(struct dsi_display *display) {
	int rc = 0;
	if (!display || !display->panel) {
		pr_err("Invalid params\n");
		return -EINVAL;
	}

	mutex_lock(&display->display_lock);

	/* enable the clk vote for CMD mode panels */
	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		dsi_display_clk_ctrl(display->dsi_clk_handle,
			DSI_CORE_CLK, DSI_CLK_ON);
	}

	rc = dsi_panel_hbm_off(display->panel);
		if (rc) {
			pr_err("[%s] failed to dsi_panel_hbm_off, rc=%d\n",
			       display->name, rc);
	}

	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
	rc = dsi_display_clk_ctrl(display->dsi_clk_handle,
				DSI_CORE_CLK, DSI_CLK_OFF);

	}
	mutex_unlock(&display->display_lock);
	return rc;
}

int dsi_display_seed_mode(struct dsi_display *display, int mode) {
	int rc = 0;
	if (!display || !display->panel) {
		pr_err("Invalid params\n");
		return -EINVAL;
	}

	mutex_lock(&display->display_lock);

		/* enable the clk vote for CMD mode panels */
	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		dsi_display_clk_ctrl(display->dsi_clk_handle,
			DSI_CORE_CLK, DSI_CLK_ON);
	}

	rc = dsi_panel_seed_mode(display->panel, mode);
		if (rc) {
			pr_err("[%s] failed to dsi_panel_seed_on, rc=%d\n",
			       display->name, rc);
	}

	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
	rc = dsi_display_clk_ctrl(display->dsi_clk_handle,
				DSI_CORE_CLK, DSI_CLK_OFF);

	}
	mutex_unlock(&display->display_lock);
	return rc;
}

int dsi_display_spr_mode(struct dsi_display *display, int mode) {
	int rc = 0;
	if (!display || !display->panel) {
		pr_err("Invalid params\n");
		return -EINVAL;
	}

	mutex_lock(&display->display_lock);

		/* enable the clk vote for CMD mode panels */
	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		dsi_display_clk_ctrl(display->dsi_clk_handle,
			DSI_CORE_CLK, DSI_CLK_ON);
	}

	rc = dsi_panel_spr_mode(display->panel, mode);
		if (rc) {
			pr_err("[%s] failed to dsi_panel_spr_on, rc=%d\n",
			       display->name, rc);
	}

	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
	rc = dsi_display_clk_ctrl(display->dsi_clk_handle,
				DSI_CORE_CLK, DSI_CLK_OFF);

	}
	mutex_unlock(&display->display_lock);
	return rc;
}

int dsi_display_read_panel_reg(struct dsi_display *display,u8 cmd, void *data, size_t len) {
	int rc = 0;
	struct dsi_display_ctrl *m_ctrl;
	if (!display || !display->panel || data == NULL) {
		pr_err("%s, Invalid params\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&display->display_lock);

	m_ctrl = &display->ctrl[display->cmd_master_idx];

	if (display->tx_cmd_buf == NULL) {
		rc = dsi_host_alloc_cmd_tx_buffer(display);
		if (rc) {
			pr_err("%s, failed to allocate cmd tx buffer memory\n", __func__);
			goto done;
		}
	}

	rc = dsi_display_cmd_engine_enable(display);
	if (rc) {
		pr_err("%s, cmd engine enable failed\n", __func__);
		goto done;
	}

	/* enable the clk vote for CMD mode panels */
	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		dsi_display_clk_ctrl(display->dsi_clk_handle,
			DSI_ALL_CLKS, DSI_CLK_ON);
	}

	rc = dsi_panel_read_panel_reg(m_ctrl,display->panel, cmd, data,len);
	if (rc < 0) {
		pr_err("%s, [%s] failed to read panel register, rc=%d,cmd=%d\n",
		       __func__,
		       display->name,
		       rc,
		       cmd);
	}

	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		rc = dsi_display_clk_ctrl(display->dsi_clk_handle,
					  DSI_ALL_CLKS, DSI_CLK_OFF);
	}

	dsi_display_cmd_engine_disable(display);

done:
	mutex_unlock(&display->display_lock);
	pr_err("%s, return: %d\n", __func__, rc);
	return rc;
}

static ssize_t oneplus_display_set_aod(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count) {
	int temp_save = 0;

	sscanf(buf, "%du", &temp_save);
	printk(KERN_INFO "%s oneplus_display_set_aod = %d\n", __func__, temp_save);
	if(get_oneplus_display_power_status() == ONEPLUS_DISPLAY_POWER_ON) {
		if(get_main_display() == NULL) {
			printk(KERN_INFO "oneplus_display_set_aod and main display is null");
			return count;
		}
		if(temp_save == 0) {
			dsi_display_aod_on(get_main_display());
		} else if(temp_save == 1) {
			dsi_display_aod_off(get_main_display());
		}
	} else {
		printk(KERN_ERR	 "%s oneplus_display_set_aod = %d, but now display panel status is not on\n", __func__, temp_save);
	}
	return count;
}

int oneplus_display_get_hbm_mode(void)
{
	return hbm_mode;
}

int __oneplus_display_set_hbm(int mode) {
	mutex_lock(&oneplus_hbm_lock);
	if(mode != hbm_mode) {
		hbm_mode = mode;
	}
	mutex_unlock(&oneplus_hbm_lock);
	return 0;
}

int oneplus_display_get_cabc_mode(void)
{
	return cabc_mode;
}

int __oneplus_display_set_cabc(int mode) {
	mutex_lock(&oneplus_cabc_lock);
	if(mode != cabc_mode) {
		cabc_mode = mode;
	}
	mutex_unlock(&oneplus_cabc_lock);
	return 0;
}

int oneplus_display_get_seed_mode(void)
{
	return seed_mode;
}

int __oneplus_display_set_seed(int mode) {
	mutex_lock(&oneplus_seed_lock);
	if(mode != seed_mode) {
		seed_mode = mode;
	}
	mutex_unlock(&oneplus_seed_lock);
	return 0;
}

int __oneplus_display_set_aod_light_mode(int mode) {
	mutex_lock(&oneplus_aod_light_mode_lock);
	if(mode != aod_light_mode) {
		aod_light_mode = mode;
	}
	mutex_unlock(&oneplus_aod_light_mode_lock);
	return 0;
}

static ssize_t oneplus_display_set_hbm(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count) {
	struct dsi_display *display = get_main_display();
	int temp_save = 0;
	int ret = 0;

	sscanf(buf, "%du", &temp_save);
	printk(KERN_INFO "%s oneplus_display_set_hbm = %d\n", __func__, temp_save);
	if (get_oneplus_display_power_status() != ONEPLUS_DISPLAY_POWER_ON) {
		printk(KERN_ERR	 "%s oneplus_display_set_hbm = %d, but now display panel status is not on\n", __func__, temp_save);
		return -EFAULT;
	}

	if (!display) {
		printk(KERN_INFO "oneplus_display_set_hbm and main display is null");
		return -EINVAL;
	}
	__oneplus_display_set_hbm(temp_save);

	if((hbm_mode > 1) &&(hbm_mode <= 10)) {
		ret = dsi_display_normal_hbm_on(get_main_display());
	} else if(hbm_mode == 1) {
		ret = dsi_display_hbm_on(get_main_display());
	} else if(hbm_mode == 0) {
		ret = dsi_display_hbm_off(get_main_display());
	}

	if (ret) {
		pr_err("failed to set hbm status ret=%d", ret);
		return ret;
	}

	return count;
}

static ssize_t oneplus_display_set_cabc(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count) {
	struct dsi_display *display = get_main_display();
	int temp_save = 0;
	int ret = 0;

	sscanf(buf, "%du", &temp_save);
	printk(KERN_INFO "%s oneplus_display_set_cabc = %d\n", __func__, temp_save);
	if (get_oneplus_display_power_status() != ONEPLUS_DISPLAY_POWER_ON) {
		printk(KERN_ERR	 "%s oneplus_display_set_cabc = %d, but now display panel status is not on\n", __func__, temp_save);
		return -EFAULT;
	}

	if (!display) {
		printk(KERN_INFO "oneplus_display_set_cabc and main display is null");
		return -EINVAL;
	}
	__oneplus_display_set_cabc(temp_save);

	ret = dsi_display_cabc_mode(get_main_display(), cabc_mode);

	if (ret) {
		pr_err("failed to set cabc status ret=%d", ret);
		return ret;
	}

	return count;
}

int __oneplus_display_set_failsafe(int mode) {
	mutex_lock(&oneplus_failsafe_lock);
	if(mode != failsafe_mode) {
		failsafe_mode = mode;
	}
	mutex_unlock(&oneplus_failsafe_lock);
	return 0;
}

static ssize_t oneplus_display_set_failsafe(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count) {
	struct dsi_display *display = get_main_display();
	int temp_save = 0;
	int ret = 0;

	sscanf(buf, "%du", &temp_save);
	printk(KERN_INFO "%s oneplus_display_set_failsafe = %d\n", __func__, temp_save);
	if (get_oneplus_display_power_status() != ONEPLUS_DISPLAY_POWER_ON) {
		printk(KERN_ERR	 "%s oneplus_display_set_failsafe = %d, but now display panel status is not on\n", __func__, temp_save);
		return -EFAULT;
	}

	if (!display) {
		printk(KERN_INFO "oneplus_display_set_failsafe and main display is null");
		return -EINVAL;
	}

	__oneplus_display_set_failsafe(temp_save);

	if(failsafe_mode == 1) {
		ret = dsi_display_failsafe_on(get_main_display());
	} else if(failsafe_mode == 0) {
		ret = dsi_display_failsafe_off(get_main_display());
	}

	if (ret) {
		pr_err("failed to set failsafe status ret=%d", ret);
		return ret;
	}

	return count;
}

int oneplus_panel_update_seed_mode_unlock(struct dsi_panel *panel)
{
	dsi_panel_seed_mode_unlock(panel, seed_mode);

	return 0;
}

int oneplus_dsi_update_seed_mode(void)
{
	struct dsi_display *display = get_main_display();
	int ret = 0;

	if (!display) {
		pr_err("failed for: %s %d\n", __func__, __LINE__);
		return -EINVAL;
	}

	ret = dsi_display_seed_mode(display, seed_mode);

	return ret;
}

static ssize_t oneplus_display_set_seed(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count) {
	int temp_save = 0;

	sscanf(buf, "%du", &temp_save);
	printk(KERN_INFO "%s oneplus_display_set_seed = %d\n", __func__, temp_save);

	__oneplus_display_set_seed(temp_save);
	if(get_oneplus_display_power_status() == ONEPLUS_DISPLAY_POWER_ON) {
		if(get_main_display() == NULL) {
			printk(KERN_INFO "oneplus_display_set_seed and main display is null");
			return count;
		}

		dsi_display_seed_mode(get_main_display(), seed_mode);
	} else {
		printk(KERN_ERR	 "%s oneplus_display_set_seed = %d, but now display panel status is not on\n", __func__, temp_save);
	}
	return count;
}

int oneplus_update_aod_light_mode_unlock(struct dsi_panel *panel)
{
	int rc = 0;
	enum dsi_cmd_set_type type;

	if (aod_light_mode == 1)
		type = DSI_CMD_AOD_LOW_LIGHT_MODE;
	else
		type = DSI_CMD_AOD_HIGH_LIGHT_MODE;

	rc = dsi_panel_tx_cmd_set(panel, type);
	if (rc) {
		pr_err("[%s] failed to send DSI_CMD_AOD_LIGHT_MODE cmds, rc=%d\n",
		       panel->name, rc);
	}

	return rc;
}

static int oneplus_update_aod_light_mode(void)
{
	struct dsi_display *display = get_main_display();
	int ret = 0;

	if (!display || !display->panel) {
		printk(KERN_INFO "oneplus_set_aod_light_mode and main display is null");
		return -EINVAL;
	}

	if (display->panel->is_hbm_enabled) {
		pr_err("%s error panel->is_hbm_enabled\n", __func__);
		return -EINVAL;
	}

	if (get_oneplus_display_scene() != ONEPLUS_DISPLAY_POWER_ON) {
		pr_err("%s error get_oneplus_display_scene = %d, \n", __func__, get_oneplus_display_scene());
		return -EFAULT;
	}
	mutex_lock(&display->display_lock);
	/* enable the clk vote for CMD mode panels */
	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		dsi_display_clk_ctrl(display->dsi_clk_handle,
			DSI_CORE_CLK, DSI_CLK_ON);
	}

	mutex_lock(&display->panel->panel_lock);

	if (display->panel->oneplus_priv.is_aod_ramless &&
	    !is_oneplus_display_aod_mode()) {
		pr_err("not support update aod_light_mode at non-aod mode\n");
		ret = -EINVAL;
		goto error;
	}

	if (!dsi_panel_initialized(display->panel)) {
		pr_err("dsi_panel_aod_low_light_mode is not init\n");
		ret = -EINVAL;
		goto error;
	}

	ret = oneplus_update_aod_light_mode_unlock(display->panel);

	if (ret) {
		pr_err("failed to set aod light status ret=%d", ret);
		goto error;
	}

error:
	mutex_unlock(&display->panel->panel_lock);
	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		dsi_display_clk_ctrl(display->dsi_clk_handle,
			DSI_ALL_CLKS, DSI_CLK_OFF);
	}
	mutex_unlock(&display->display_lock);

	return ret;
}

static ssize_t oneplus_set_aod_light_mode(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count) {
	int temp_save = 0;

	sscanf(buf, "%du", &temp_save);

	__oneplus_display_set_aod_light_mode(temp_save);
	oneplus_update_aod_light_mode();

	return count;
}

int __oneplus_display_set_spr(int mode) {
	mutex_lock(&oneplus_spr_lock);
	if(mode != spr_mode) {
		spr_mode = mode;
	}
	mutex_unlock(&oneplus_spr_lock);
	return 0;
}
int oneplus_dsi_update_spr_mode(void)
{
	struct dsi_display *display = get_main_display();
	int ret = 0;

	if (!display) {
		pr_err("failed for: %s %d\n", __func__, __LINE__);
		return -EINVAL;
	}

	ret = dsi_display_spr_mode(display, spr_mode);

	return ret;
}

static ssize_t oneplus_display_set_spr(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count) {
	int temp_save = 0;

	sscanf(buf, "%du", &temp_save);
	printk(KERN_INFO "%s oneplus_display_set_spr = %d\n", __func__, temp_save);

	__oneplus_display_set_spr(temp_save);
	if(get_oneplus_display_power_status() == ONEPLUS_DISPLAY_POWER_ON) {
		if(get_main_display() == NULL) {
			printk(KERN_INFO "oneplus_display_set_spr and main display is null");
			return count;
		}

		dsi_display_spr_mode(get_main_display(), spr_mode);
	} else {
		printk(KERN_ERR	 "%s oneplus_display_set_spr = %d, but now display panel status is not on\n", __func__, temp_save);
	}
	return count;
}

int oneplus_display_audio_ready = 0;
static ssize_t oneplus_display_set_audio_ready(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count) {

	sscanf(buf, "%du", &oneplus_display_audio_ready);

	return count;
}

static ssize_t oneplus_display_get_hbm(struct device *dev,
struct device_attribute *attr, char *buf) {

	printk(KERN_INFO "oneplus_display_get_hbm = %d\n",hbm_mode);

	return sprintf(buf, "%d\n", hbm_mode);
}

static ssize_t oneplus_display_get_cabc(struct device *dev,
struct device_attribute *attr, char *buf) {

	printk(KERN_INFO "oneplus_display_get_cabc = %d\n",cabc_mode);

	return sprintf(buf, "%d\n", cabc_mode);
}

static ssize_t oneplus_display_get_seed(struct device *dev,
struct device_attribute *attr, char *buf) {

	printk(KERN_INFO "oneplus_display_get_seed = %d\n",seed_mode);

	return sprintf(buf, "%d\n", seed_mode);
}

static ssize_t oneplus_get_aod_light_mode(struct device *dev,
struct device_attribute *attr, char *buf) {

	printk(KERN_INFO "oneplus_get_aod_light_mode = %d\n",aod_light_mode);

	return sprintf(buf, "%d\n", aod_light_mode);
}

static ssize_t oneplus_display_get_spr(struct device *dev,
struct device_attribute *attr, char *buf) {

	printk(KERN_INFO "oneplus_display_get_spr = %d\n",spr_mode);

	return sprintf(buf, "%d\n", spr_mode);
}

static ssize_t oneplus_display_get_mipi_clk_rate_hz(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct dsi_display *display = get_main_display();
	u64 clk_rate_hz = 0;
	
	if (!display) {
		pr_err("failed to get display");
		return -EINVAL;
	}

	mutex_lock(&display->display_lock);
	if (!display->panel || !display->panel->cur_mode) {
		pr_err("failed to get display mode");
		mutex_unlock(&display->display_lock);
		return -EINVAL;
	}
	clk_rate_hz = display->panel->cur_mode->timing.clk_rate_hz;

	mutex_unlock(&display->display_lock);

	return sprintf(buf, "%llu\n", clk_rate_hz);
}

static ssize_t oneplus_display_get_failsafe(struct device *dev,
struct device_attribute *attr, char *buf) {

	printk(KERN_INFO "oneplus_display_get_failsafe = %d\n",failsafe_mode);

	return sprintf(buf, "%d\n", failsafe_mode);
}
static ssize_t oneplus_display_regulator_control(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count) {
	int temp_save = 0;
	struct dsi_display *temp_display;
	sscanf(buf, "%du", &temp_save);
	printk(KERN_INFO "%s oneplus_display_regulator_control = %d\n", __func__, temp_save);
	if(get_main_display() == NULL) {
		printk(KERN_INFO "oneplus_display_regulator_control and main display is null");
		return count;
	}
	temp_display = get_main_display();
	if(temp_save == 0) {
		dsi_pwr_enable_regulator(&temp_display->panel->power_info, false);
	} else if(temp_save == 1) {
		dsi_pwr_enable_regulator(&temp_display->panel->power_info, true);
	}
	return count;
}

static ssize_t oneplus_display_get_panel_serial_number(struct device *dev,
struct device_attribute *attr, char *buf) {
	int ret = 0;
	unsigned char read[30];
	PANEL_SERIAL_INFO panel_serial_info;
	uint64_t serial_number;
	struct dsi_display *display = get_main_display();
	int i;

	if (!display) {
		printk(KERN_INFO "oneplus_display_get_panel_serial_number and main display is null");
		return -1;
	}

	if(get_oneplus_display_power_status() != ONEPLUS_DISPLAY_POWER_ON) {
		printk(KERN_ERR"%s display panel in off status\n", __func__);
		return ret;
	}

	/*
	 * for some unknown reason, the panel_serial_info may read dummy,
	 * retry when found panel_serial_info is abnormal.
	 */
	for (i = 0;i < 10; i++) {
		if(!strcmp(display->panel->name,"samsung amb655uv01 amoled fhd+ panel with DSC")
			|| !strcmp(display->panel->name,"samsung amb655uv01 amoled fhd+ panel with DSC 2nd")) {
			ret = dsi_display_read_panel_reg(get_main_display(), 0xA1, read, 17);
		}else if (!strcmp(display->panel->name,"samsung ams643xf01 amoled fhd+ panel")) {
			ret = dsi_display_read_panel_reg(get_main_display(), 0xA1, read, 18);
		}else if (!strcmp(display->name,"qcom,mdss_dsi_oneplus19101boe_nt37800_1080_2400_cmd")) {
			u32 mtime = 0;

			mutex_lock(&display->display_lock);
			mutex_lock(&display->panel->panel_lock);

			if (display->panel->panel_initialized) {

				if (display->config.panel_mode == DSI_OP_CMD_MODE) {
					dsi_display_clk_ctrl(display->dsi_clk_handle,
							DSI_ALL_CLKS, DSI_CLK_ON);
				}
				{
					char value[] = { 0x55,0xAA,0x52,0x08,0x01 };
					ret = mipi_dsi_dcs_write(&display->panel->mipi_device, 0xF0, value, sizeof(value));
				}
				if (display->config.panel_mode == DSI_OP_CMD_MODE) {
					dsi_display_clk_ctrl(display->dsi_clk_handle,
							DSI_ALL_CLKS, DSI_CLK_OFF);
				}
				mutex_unlock(&display->panel->panel_lock);
				mutex_unlock(&display->display_lock);
			}
			if(ret < 0) {
				ret = scnprintf(buf, PAGE_SIZE,
						"Get panel serial number failed, reason:%d",ret);
				msleep(20);
				continue;
			}

			ret = dsi_display_read_panel_reg(display, 0xA2, read, 4);
			if(ret < 0) {
				ret = scnprintf(buf, PAGE_SIZE,
						"Get panel serial number failed, reason:%d",ret);
				msleep(20);
				continue;
			}

			pr_err("panel model: %2x %2x %2x %2x\n", read[0], read[1], read[2], read[3]);
			ret = dsi_display_read_panel_reg(display, 0xA3, read, 7);
			if(ret < 0) {
				ret = scnprintf(buf, PAGE_SIZE,
						"Get panel serial number failed, reason:%d",ret);
				msleep(20);
				continue;
			}

			panel_serial_info.reg_index = 10;

			panel_serial_info.year		= (read[0] >> 4) + 1;
			panel_serial_info.month		= read[0] & 0x0F;
			panel_serial_info.day		= read[1]	& 0x1F;
			panel_serial_info.hour		= read[2]	& 0x1F;
			panel_serial_info.minute	= read[3]	& 0x3F;
			panel_serial_info.second	= read[4]	& 0x3F;
			mtime = (read[5] >> 4) * 1000 + (read[5] & 0xF) * 100 + (read[6] >> 4) * 10 + (read[6] & 0x0F);
			panel_serial_info.reserved[0] = mtime >> 8;
			panel_serial_info.reserved[1] = mtime & 0xFF;
		} else {
			ret = dsi_display_read_panel_reg(get_main_display(), 0xA1, read, 16);
		}
		if(ret < 0) {
			ret = scnprintf(buf, PAGE_SIZE,
					"Get panel serial number failed, reason:%d",ret);
			msleep(20);
			continue;
		}

		if (read[0] == 0x09 && read[1] == 0x40 && read[2] == 0x10 && read[3] == 0x27) {
			/* 1-4th oneplus code 09401027
			 * 5th vendor code/Month
			 * 6th Year/Day
			 * 7th Hour
			 * 8th Minute
			 * 9th Second
			 * 10-11th Second(ms)
			 */
			panel_serial_info.month		= read[4] & 0x0F;
			panel_serial_info.year		= ((read[5] & 0xE0) >> 5) + 7;
			panel_serial_info.day		= read[5] & 0x1F;
			panel_serial_info.hour		= read[6] & 0x17;
			panel_serial_info.minute	= read[7];
			panel_serial_info.second	= read[8];
			panel_serial_info.reserved[0] = read[9];
			panel_serial_info.reserved[1] = read[10];
		} else if(!strcmp(display->panel->name,"samsung amb655uv01 amoled fhd+ panel with DSC")
			|| !strcmp(display->panel->name,"samsung amb655uv01 amoled fhd+ panel with DSC 2nd")) {
			/*  0xA1               11th        12th    13th    14th    15th
			 *  HEX                0x32        0x0C    0x0B    0x29    0x37
			 *  Bit           [D7:D4][D3:D0] [D5:D0] [D5:D0] [D5:D0] [D5:D0]
			 *  exp              3      2       C       B       29      37
			 *  Yyyy,mm,dd      2014   2m      12d     11h     41min   55sec
			*/
			panel_serial_info.reg_index = 10;

			panel_serial_info.year		= (read[panel_serial_info.reg_index] & 0xF0) >> 0x4;
			panel_serial_info.month		= read[panel_serial_info.reg_index]	& 0x0F;
			panel_serial_info.day		= read[panel_serial_info.reg_index + 1]	& 0x1F;
			panel_serial_info.hour		= read[panel_serial_info.reg_index + 2]	& 0x1F;
			panel_serial_info.minute	= read[panel_serial_info.reg_index + 3]	& 0x3F;
			panel_serial_info.second	= read[panel_serial_info.reg_index + 4]	& 0x3F;
			panel_serial_info.reserved[0] = read[panel_serial_info.reg_index + 5];
			panel_serial_info.reserved[1] = read[panel_serial_info.reg_index + 6];
		}else if(!strcmp(display->panel->name,"samsung ams643xf01 amoled fhd+ panel")) {
			/*  0xA1               12th        13th    14th    15th    16th
			 *  HEX                0x32        0x0C    0x0B    0x29    0x37
			 *  Bit           [D7:D4][D3:D0] [D5:D0] [D5:D0] [D5:D0] [D5:D0]
			 *  exp              3      2       C       B       29      37
			 *  Yyyy,mm,dd      2014   2m      12d     11h     41min   55sec
			*/
			panel_serial_info.reg_index = 11;

			panel_serial_info.year		= (read[panel_serial_info.reg_index] & 0xF0) >> 0x4;
			panel_serial_info.month		= read[panel_serial_info.reg_index]	& 0x0F;
			panel_serial_info.day		= read[panel_serial_info.reg_index + 1]	& 0x1F;
			panel_serial_info.hour		= read[panel_serial_info.reg_index + 2]	& 0x1F;
			panel_serial_info.minute	= read[panel_serial_info.reg_index + 3]	& 0x3F;
			panel_serial_info.second	= read[panel_serial_info.reg_index + 4]	& 0x3F;
			panel_serial_info.reserved[0] = read[panel_serial_info.reg_index + 5];
			panel_serial_info.reserved[1] = read[panel_serial_info.reg_index + 6];
		}

		serial_number = (panel_serial_info.year		<< 56)\
			+ (panel_serial_info.month		<< 48)\
			+ (panel_serial_info.day		<< 40)\
			+ (panel_serial_info.hour		<< 32)\
			+ (panel_serial_info.minute	<< 24)\
			+ (panel_serial_info.second	<< 16)\
			+ (panel_serial_info.reserved[0] << 8)\
			+ (panel_serial_info.reserved[1]);

		if (!panel_serial_info.year) {
			/*
			 * the panel we use always large than 2011, so
			 * force retry when year is 2011
			 */
			msleep(20);
			continue;
		}

		ret = scnprintf(buf, PAGE_SIZE, "Get panel serial number: %llx\n",serial_number);
		break;
	}

	return ret;
}

static char oneplus_rx_reg[PANEL_TX_MAX_BUF] = {0x0};
static char oneplus_rx_len = 0;
static ssize_t oneplus_display_get_panel_reg(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct dsi_display *display = get_main_display();
	int i, cnt = 0;

	if (!display)
		return -EINVAL;
	mutex_lock(&display->display_lock);

	for (i = 0; i < oneplus_rx_len; i++)
		cnt += snprintf(buf + cnt, PANEL_TX_MAX_BUF - cnt,
				"%02x ", oneplus_rx_reg[i]);
	cnt += snprintf(buf + cnt, PANEL_TX_MAX_BUF - cnt, "\n");
	mutex_unlock(&display->display_lock);

	return cnt;
}

static uint64_t production_date_serial_number = 0;
int oneplus_display_get_panel_production_date_serial_number(void)
{
	int ret = 0;
	unsigned char read[30];
	PANEL_SERIAL_INFO panel_serial_info;
	struct dsi_display *display = get_main_display();
	int i;

	if (!display) {
		printk(KERN_INFO "oneplus_display_get_panel_serial_number and main display is null");
		return -1;
	}

	if (get_oneplus_display_power_status() != ONEPLUS_DISPLAY_POWER_ON) {
		printk(KERN_ERR"%s display panel in off status\n", __func__);
		return ret;
	}

	/*
	 * for some unknown reason, the panel_serial_info may read dummy,
	 * retry when found panel_serial_info is abnormal.
	 */
	for (i = 0;i < 10; i++) {
	if(!strcmp(display->panel->name,"samsung amb655uv01 amoled fhd+ panel with DSC")
			|| !strcmp(display->panel->name,"samsung amb655uv01 amoled fhd+ panel with DSC 2nd")) {
			ret = dsi_display_read_panel_reg(get_main_display(), 0xA1, read, 17);
		}else if (!strcmp(display->panel->name,"samsung ams643xf01 amoled fhd+ panel")) {
			ret = dsi_display_read_panel_reg(get_main_display(), 0xA1, read, 18);
		}else if (!strcmp(display->name,"qcom,mdss_dsi_oneplus19101boe_nt37800_1080_2400_cmd")) {
			u32 mtime = 0;

			mutex_lock(&display->display_lock);
			mutex_lock(&display->panel->panel_lock);

			if (display->panel->panel_initialized) {

				if (display->config.panel_mode == DSI_OP_CMD_MODE) {
					dsi_display_clk_ctrl(display->dsi_clk_handle,
							DSI_ALL_CLKS, DSI_CLK_ON);
				}
				{
					char value[] = { 0x55,0xAA,0x52,0x08,0x01 };
					ret = mipi_dsi_dcs_write(&display->panel->mipi_device, 0xF0, value, sizeof(value));
				}
				if (display->config.panel_mode == DSI_OP_CMD_MODE) {
					dsi_display_clk_ctrl(display->dsi_clk_handle,
							DSI_ALL_CLKS, DSI_CLK_OFF);
				}
				mutex_unlock(&display->panel->panel_lock);
				mutex_unlock(&display->display_lock);
			}
			if(ret < 0) {
				printk(KERN_ERR"%s Get panel serial number failed,reason:%d \n",__func__,ret);
				msleep(20);
				continue;
			}

			ret = dsi_display_read_panel_reg(display, 0xA2, read, 4);
			if(ret < 0) {
				printk(KERN_ERR"%s Get panel serial number failed,reason:%d \n",__func__,ret);
				msleep(20);
				continue;
			}

			pr_err("panel model: %2x %2x %2x %2x\n", read[0], read[1], read[2], read[3]);
			ret = dsi_display_read_panel_reg(display, 0xA3, read, 7);
			if(ret < 0) {
				printk(KERN_ERR"%s Get panel serial number failed,reason:%d \n",__func__,ret);
				msleep(20);
				continue;
			}

			panel_serial_info.reg_index = 10;

			panel_serial_info.year		= (read[0] >> 4) + 1;
			panel_serial_info.month		= read[0] & 0x0F;
			panel_serial_info.day		= read[1]	& 0x1F;
			panel_serial_info.hour		= read[2]	& 0x1F;
			panel_serial_info.minute	= read[3]	& 0x3F;
			panel_serial_info.second	= read[4]	& 0x3F;
			mtime = (read[5] >> 4) * 1000 + (read[5] & 0xF) * 100 + (read[6] >> 4) * 10 + (read[6] & 0x0F);
			panel_serial_info.reserved[0] = mtime >> 8;
			panel_serial_info.reserved[1] = mtime & 0xFF;
		} else {
			ret = dsi_display_read_panel_reg(get_main_display(), 0xA1, read, 16);
		}
		if(ret < 0) {
			printk(KERN_ERR"%s Get panel serial number failed,reason:%d \n",__func__,ret);
			msleep(20);
			continue;
		}

		if (read[0] == 0x09 && read[1] == 0x40 && read[2] == 0x10 && read[3] == 0x27) {
			/* 1-4th oneplus code 09401027
			 * 5th vendor code/Month
			 * 6th Year/Day
			 * 7th Hour
			 * 8th Minute
			 * 9th Second
			 * 10-11th Second(ms)
			 */
			panel_serial_info.month		= read[4] & 0x0F;
			panel_serial_info.year		= ((read[5] & 0xE0) >> 5) + 7;
			panel_serial_info.day		= read[5] & 0x1F;
			panel_serial_info.hour		= read[6] & 0x17;
			panel_serial_info.minute	= read[7];
			panel_serial_info.second	= read[8];
			panel_serial_info.reserved[0] = read[9];
			panel_serial_info.reserved[1] = read[10];
		} else if(!strcmp(display->panel->name,"samsung amb655uv01 amoled fhd+ panel with DSC")
			|| !strcmp(display->panel->name,"samsung amb655uv01 amoled fhd+ panel with DSC 2nd")) {
			/*  0xA1               11th        12th    13th    14th    15th
			 *  HEX                0x32        0x0C    0x0B    0x29    0x37
			 *  Bit           [D7:D4][D3:D0] [D5:D0] [D5:D0] [D5:D0] [D5:D0]
			 *  exp              3      2       C       B       29      37
			 *  Yyyy,mm,dd      2014   2m      12d     11h     41min   55sec
			*/
			panel_serial_info.reg_index = 10;

			panel_serial_info.year		= (read[panel_serial_info.reg_index] & 0xF0) >> 0x4;
			panel_serial_info.month		= read[panel_serial_info.reg_index]	& 0x0F;
			panel_serial_info.day		= read[panel_serial_info.reg_index + 1]	& 0x1F;
			panel_serial_info.hour		= read[panel_serial_info.reg_index + 2]	& 0x1F;
			panel_serial_info.minute	= read[panel_serial_info.reg_index + 3]	& 0x3F;
			panel_serial_info.second	= read[panel_serial_info.reg_index + 4]	& 0x3F;
			panel_serial_info.reserved[0] = read[panel_serial_info.reg_index + 5];
			panel_serial_info.reserved[1] = read[panel_serial_info.reg_index + 6];
		}else if(!strcmp(display->panel->name,"samsung ams643xf01 amoled fhd+ panel")) {
			/*  0xA1               12th        13th    14th    15th    16th
			 *  HEX                0x32        0x0C    0x0B    0x29    0x37
			 *  Bit           [D7:D4][D3:D0] [D5:D0] [D5:D0] [D5:D0] [D5:D0]
			 *  exp              3      2       C       B       29      37
			 *  Yyyy,mm,dd      2014   2m      12d     11h     41min   55sec
			*/
			panel_serial_info.reg_index = 11;

			panel_serial_info.year		= (read[panel_serial_info.reg_index] & 0xF0) >> 0x4;
			panel_serial_info.month		= read[panel_serial_info.reg_index]	& 0x0F;
			panel_serial_info.day		= read[panel_serial_info.reg_index + 1]	& 0x1F;
			panel_serial_info.hour		= read[panel_serial_info.reg_index + 2]	& 0x1F;
			panel_serial_info.minute	= read[panel_serial_info.reg_index + 3]	& 0x3F;
			panel_serial_info.second	= read[panel_serial_info.reg_index + 4]	& 0x3F;
			panel_serial_info.reserved[0] = read[panel_serial_info.reg_index + 5];
			panel_serial_info.reserved[1] = read[panel_serial_info.reg_index + 6];
		}

		production_date_serial_number = (panel_serial_info.year		<< 56)\
			+ (panel_serial_info.month		<< 48)\
			+ (panel_serial_info.day		<< 40)\
			+ (panel_serial_info.hour		<< 32)\
			+ (panel_serial_info.minute	<< 24)\
			+ (panel_serial_info.second	<< 16)\
			+ (panel_serial_info.reserved[0] << 8)\
			+ (panel_serial_info.reserved[1]);

		if (!panel_serial_info.year) {
			/*
			 * the panel we use always large than 2011, so
			 * force retry when year is 2011
			 */
			msleep(20);
			continue;
		}
		break;
	}
	printk(KERN_ERR "Get panel production date serial number: %llx\n",production_date_serial_number);
	return ret;
}

static ssize_t oneplus_display_get_serial_number(struct device *dev,
struct device_attribute *attr, char *buf) {
	if(production_date_serial_number == 0){
		printk(KERN_ERR"dsi_cmd %s Failed,Get panel serial number\n",__func__);
		return sprintf(buf, "%llx\n",production_date_serial_number);
	}
	printk(KERN_INFO "oneplus_display_get_serial_number = %llx\n",production_date_serial_number);

	return scnprintf(buf, PAGE_SIZE, "%llx\n",production_date_serial_number);
}

static ssize_t oneplus_display_set_panel_reg(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count) {
	char reg[PANEL_TX_MAX_BUF] = {0x0};
	char payload[PANEL_TX_MAX_BUF] = {0x0};
	u32 index = 0, value = 0, step =0;
	int ret = 0;
	int len = 0;
	char *bufp = (char *)buf;
	struct dsi_display *display = get_main_display();
	char read;

	if (!display || !display->panel) {
		pr_err("debug for: %s %d\n", __func__, __LINE__);
		return -EFAULT;
	}

	if (sscanf(bufp, "%c%n", &read, &step) && read == 'r') {
		bufp += step;
		sscanf(bufp, "%x %d", &value, &len);
		if (len > PANEL_TX_MAX_BUF) {
			pr_err("failed\n");
			return -EINVAL;
		}
		dsi_display_read_panel_reg(get_main_display(),value, reg, len);

		for (index; index < len; index++) {
			printk("%x ", reg[index]);
		}
		mutex_lock(&display->display_lock);
		memcpy(oneplus_rx_reg, reg, PANEL_TX_MAX_BUF);
		oneplus_rx_len = len;
		mutex_unlock(&display->display_lock);
		return count;
	}

	while (sscanf(bufp, "%x%n", &value, &step) > 0) {
		reg[len++] = value;
		if (len >= PANEL_TX_MAX_BUF) {
			pr_err("wrong input reg len\n");
			return -EFAULT;
		}
		bufp += step;
	}

	for(index; index < len; index ++ ) {
		payload[index] = reg[index + 1];
	}

	/* enable the clk vote for CMD mode panels */
	mutex_lock(&display->display_lock);
	mutex_lock(&display->panel->panel_lock);

	if (display->panel->panel_initialized) {
		if (display->config.panel_mode == DSI_OP_CMD_MODE) {
			dsi_display_clk_ctrl(display->dsi_clk_handle,
					DSI_ALL_CLKS, DSI_CLK_ON);
		}

		ret = mipi_dsi_dcs_write(&display->panel->mipi_device, reg[0],
				payload, len -1);

		if (display->config.panel_mode == DSI_OP_CMD_MODE) {
			dsi_display_clk_ctrl(display->dsi_clk_handle,
					DSI_ALL_CLKS, DSI_CLK_OFF);
		}
	}

	mutex_unlock(&display->panel->panel_lock);
	mutex_unlock(&display->display_lock);

	if (ret < 0) {
		return ret;
	}

	return count;
}

static ssize_t oneplus_display_get_panel_id(struct device *dev,
struct device_attribute *attr, char *buf) {
	struct dsi_display *display = get_main_display();
	int ret = 0;
	unsigned char read[30];
	char DA = 0;
	char DB = 0;
	char DC = 0;

	if(get_oneplus_display_power_status() == ONEPLUS_DISPLAY_POWER_ON) {
		if(display == NULL) {
			printk(KERN_INFO "oneplus_display_get_panel_id and main display is null");
			ret = -1;
			return ret;
		}

		ret = dsi_display_read_panel_reg(display,0xDA, read, 1);
		if(ret < 0) {
			pr_err("failed to read DA ret=%d\n", ret);
			return -EINVAL;
		}
		DA = read[0];

		ret = dsi_display_read_panel_reg(display, 0xDB, read, 1);
		if(ret < 0) {
			pr_err("failed to read DA ret=%d\n", ret);
			return -EINVAL;
		}
		DB = read[0];

		ret = dsi_display_read_panel_reg(display,0xDC, read, 1);
		if(ret < 0) {
			pr_err("failed to read DA ret=%d\n", ret);
			return -EINVAL;
		}
		DC = read[0];
		ret = scnprintf(buf, PAGE_SIZE, "%02x %02x %02x\n", DA, DB, DC);
	} else {
		printk(KERN_ERR	 "%s oneplus_display_get_panel_id, but now display panel status is not on\n", __func__);
	}
	return ret;
}

static ssize_t oneplus_display_get_panel_dsc(struct device *dev,
struct device_attribute *attr, char *buf) {
	int ret = 0;
	unsigned char read[30];

	if(get_oneplus_display_power_status() == ONEPLUS_DISPLAY_POWER_ON) {
		if(get_main_display() == NULL) {
			printk(KERN_INFO "oneplus_display_get_panel_dsc and main display is null");
			ret = -1;
			return ret;
		}

		ret = dsi_display_read_panel_reg(get_main_display(),0x03, read, 1);
		if(ret < 0) {
			ret = scnprintf(buf, PAGE_SIZE, "oneplus_display_get_panel_dsc failed, reason:%d",ret);
		} else {
			ret = scnprintf(buf, PAGE_SIZE, "oneplus_display_get_panel_dsc: 0x%x\n",read[0]);
		}
	} else {
		printk(KERN_ERR	 "%s oneplus_display_get_panel_dsc, but now display panel status is not on\n", __func__);
	}
	return ret;
}

static ssize_t oneplus_display_dump_info(struct device *dev,
struct device_attribute *attr, char *buf) {
	int ret = 0;
	struct dsi_display * temp_display;

	temp_display = get_main_display();

	if(temp_display == NULL ) {
		printk(KERN_INFO "oneplus_display_dump_info and main display is null");
		ret = -1;
		return ret;
	}

	if(temp_display->modes == NULL) {
		printk(KERN_INFO "oneplus_display_dump_info and display modes is null");
		ret = -1;
		return ret;
	}

	ret = scnprintf(buf , PAGE_SIZE, "oneplus_display_dump_info: height =%d,width=%d,frame_rate=%d,clk_rate=%llu\n",
		temp_display->modes->timing.h_active,temp_display->modes->timing.v_active,
		temp_display->modes->timing.refresh_rate,temp_display->modes->timing.clk_rate_hz);

	return ret;
}

int __oneplus_display_set_power_status(int status) {
	mutex_lock(&oneplus_power_status_lock);
	if(status != oneplus_request_power_status) {
		oneplus_request_power_status = status;
	}
	mutex_unlock(&oneplus_power_status_lock);
	return 0;
}
static ssize_t oneplus_display_get_power_status(struct device *dev,
struct device_attribute *attr, char *buf) {

	printk(KERN_INFO "oneplus_display_get_power_status = %d\n",get_oneplus_display_power_status());

	return sprintf(buf, "%d\n", get_oneplus_display_power_status());
}

static ssize_t oneplus_display_set_power_status(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count) {
	int temp_save = 0;

	sscanf(buf, "%du", &temp_save);
	printk(KERN_INFO "%s oneplus_display_set_power_status = %d\n", __func__, temp_save);

	__oneplus_display_set_power_status(temp_save);

	return count;
}

static ssize_t oneplus_display_get_closebl_flag(struct device *dev,
                                struct device_attribute *attr, char *buf)
{
	printk(KERN_INFO "oneplus_display_get_closebl_flag = %d\n",lcd_closebl_flag);
	return sprintf(buf, "%d\n", lcd_closebl_flag);
}

static ssize_t oneplus_display_set_closebl_flag(struct device *dev,
                               struct device_attribute *attr,
                               const char *buf, size_t count)
{
	int closebl = 0;
	sscanf(buf, "%du", &closebl);
	pr_err("lcd_closebl_flag = %d\n",closebl);
	if(1 != closebl)
		lcd_closebl_flag = 0;
	pr_err("oneplus_display_set_closebl_flag = %d\n",lcd_closebl_flag);
	return count;
}

extern const char *cmd_set_prop_map[];
static ssize_t oneplus_display_get_dsi_command(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int i, cnt;

	cnt = snprintf(buf, PAGE_SIZE,
		"read current dsi_cmd:\n"
		"    echo dump > dsi_cmd  - then you can find dsi cmd on kmsg\n"
		"set sence dsi cmd:\n"
		"  example hbm on:\n"
		"    echo qcom,mdss-dsi-hbm-on-command > dsi_cmd\n"
		"    echo [dsi cmd0] > dsi_cmd\n"
		"    echo [dsi cmd1] > dsi_cmd\n"
		"    echo [dsi cmdX] > dsi_cmd\n"
		"    echo flush > dsi_cmd\n"
		"available dsi_cmd sences:\n");
	for (i = 0; i < DSI_CMD_SET_MAX; i++)
		cnt += snprintf(buf + cnt, PAGE_SIZE - cnt,
				"    %s\n", cmd_set_prop_map[i]);

	return cnt;
}
static int oneplus_display_dump_dsi_command(struct dsi_display *display)
{
	struct dsi_display_mode *mode;
	struct dsi_display_mode_priv_info *priv_info;
	struct dsi_panel_cmd_set *cmd_sets;
	enum dsi_cmd_set_state state;
	struct dsi_cmd_desc *cmds;
	const char *cmd_name;
	int i, j, k, cnt = 0;
	const u8 *tx_buf;
	char bufs[SZ_256];

	if (!display || !display->panel || !display->panel->cur_mode) {
		pr_err("failed to get main dsi display\n");
		return -EFAULT;
	}

	mode = display->panel->cur_mode;
	if (!mode || !mode->priv_info) {
		pr_err("failed to get dsi display mode\n");
		return -EFAULT;
	}

	priv_info = mode->priv_info;
	cmd_sets = priv_info->cmd_sets;

	for (i = 0; i < DSI_CMD_SET_MAX; i++) {
		cmd_name = cmd_set_prop_map[i];
		if (!cmd_name)
			continue;
		state = cmd_sets[i].state;
		pr_err("%s: %s", cmd_name, state == DSI_CMD_SET_STATE_LP ?
				"dsi_lp_mode" : "dsi_hs_mode");

		for (j = 0; j < cmd_sets[i].count; j++) {
			cmds = &cmd_sets[i].cmds[j];
			tx_buf = cmds->msg.tx_buf;
			cnt = snprintf(bufs, SZ_256,
				" %02x %02x %02x %02x %02x %02x %02x",
				cmds->msg.type, cmds->last_command,
				cmds->msg.channel,
				cmds->msg.flags == MIPI_DSI_MSG_REQ_ACK,
				cmds->post_wait_ms,
				(int)(cmds->msg.tx_len >> 8),
				(int)(cmds->msg.tx_len & 0xff));
			for (k = 0; k < cmds->msg.tx_len; k++)
				cnt += snprintf(bufs + cnt,
						SZ_256 > cnt ? SZ_256 - cnt : 0,
						" %02x", tx_buf[k]);
			pr_err("%s", bufs);
		}
	}

	return 0;
}

static int oneplus_dsi_panel_get_cmd_pkt_count(const char *data, u32 length, u32 *cnt)
{
	const u32 cmd_set_min_size = 7;
	u32 count = 0;
	u32 packet_length;
	u32 tmp;

	while (length >= cmd_set_min_size) {
		packet_length = cmd_set_min_size;
		tmp = ((data[5] << 8) | (data[6]));
		packet_length += tmp;
		if (packet_length > length) {
			pr_err("format error packet_length[%d] length[%d] count[%d]\n",
				packet_length, length, count);
			return -EINVAL;
		}
		length -= packet_length;
		data += packet_length;
		count++;
	};

	*cnt = count;
	return 0;
}

static int oneplus_dsi_panel_create_cmd_packets(const char *data,
					     u32 length,
					     u32 count,
					     struct dsi_cmd_desc *cmd)
{
	int rc = 0;
	int i, j;
	u8 *payload;

	for (i = 0; i < count; i++) {
		u32 size;

		cmd[i].msg.type = data[0];
		cmd[i].last_command = (data[1] == 1 ? true : false);
		cmd[i].msg.channel = data[2];
		cmd[i].msg.flags |= (data[3] == 1 ? MIPI_DSI_MSG_REQ_ACK : 0);
		cmd[i].msg.ctrl = 0;
		cmd[i].post_wait_ms = data[4];
		cmd[i].msg.tx_len = ((data[5] << 8) | (data[6]));

		size = cmd[i].msg.tx_len * sizeof(u8);

		payload = kzalloc(size, GFP_KERNEL);
		if (!payload) {
			rc = -ENOMEM;
			goto error_free_payloads;
		}

		for (j = 0; j < cmd[i].msg.tx_len; j++)
			payload[j] = data[7 + j];

		cmd[i].msg.tx_buf = payload;
		data += (7 + cmd[i].msg.tx_len);
	}

	return rc;
error_free_payloads:
	for (i = i - 1; i >= 0; i--) {
		cmd--;
		kfree(cmd->msg.tx_buf);
	}

	return rc;
}

static ssize_t oneplus_display_set_dsi_command(struct device *dev,
                               struct device_attribute *attr,
                               const char *buf, size_t count)
{
	struct dsi_display *display = get_main_display();
	struct dsi_display_mode *mode;
	struct dsi_display_mode_priv_info *priv_info;
	struct dsi_panel_cmd_set *cmd_sets;
	char *bufp = (char *)buf;
	struct dsi_cmd_desc *cmds;
	struct dsi_panel_cmd_set *cmd;
	static char *cmd_bufs;
	static int cmd_counts;
	static u32 oneplus_dsi_command = DSI_CMD_SET_MAX;
	static int oneplus_dsi_state = DSI_CMD_SET_STATE_HS;
	u32 old_dsi_command = oneplus_dsi_command;
	u32 packet_count = 0, size;
	int rc = count, i;
	char data[SZ_256];
	bool flush = false;

	if (!cmd_bufs) {
		cmd_bufs = kmalloc(SZ_4K, GFP_KERNEL);
		if (!cmd_bufs)
			return -ENOMEM;
	}
	sscanf(buf, "%s", data);
	if (!strcmp("dump", data)) {
		rc = oneplus_display_dump_dsi_command(display);
		if (rc < 0)
			return rc;
		return count;
	} else if (!strcmp("flush", data)) {
		flush = true;
	} else if (!strcmp("dsi_hs_mode", data)) {
		oneplus_dsi_state = DSI_CMD_SET_STATE_HS;
	} else if (!strcmp("dsi_lp_mode", data)) {
		oneplus_dsi_state = DSI_CMD_SET_STATE_LP;
	} else {
		for (i = 0; i < DSI_CMD_SET_MAX; i++) {
			if (!strcmp(cmd_set_prop_map[i], data)) {
				oneplus_dsi_command = i;
				flush = true;
				break;
			}
		}
	}

	if (!flush) {
		u32 value = 0, step = 0;

		while (sscanf(bufp, "%x%n", &value, &step) > 0) {
			if (value > 0xff) {
				pr_err("input reg don't large than 0xff\n");
				return -EINVAL;
			}
			cmd_bufs[cmd_counts++] = value;
			if (cmd_counts >= SZ_4K) {
				pr_err("wrong input reg len\n");
				cmd_counts = 0;
				return -EFAULT;
			}
			bufp += step;
		}
		return count;
	}

	if (!cmd_counts)
		return rc;
	if (old_dsi_command >= DSI_CMD_SET_MAX) {
		pr_err("UnSupport dsi command set\n");
		goto error;
	}

	if (!display || !display->panel || !display->panel->cur_mode) {
		pr_err("failed to get main dsi display\n");
		rc = -EFAULT;
		goto error;
	}

	mode = display->panel->cur_mode;
	if (!mode || !mode->priv_info) {
		pr_err("failed to get dsi display mode\n");
		rc = -EFAULT;
		goto error;
	}

	priv_info = mode->priv_info;
	cmd_sets = priv_info->cmd_sets;

	cmd = &cmd_sets[old_dsi_command];

	rc = oneplus_dsi_panel_get_cmd_pkt_count(cmd_bufs, cmd_counts,
			&packet_count);
	if (rc) {
		pr_err("commands failed, rc=%d\n", rc);
		goto error;
	}

	size = packet_count * sizeof(*cmd->cmds);

	cmds = kzalloc(size, GFP_KERNEL);
	if (!cmds) {
		rc = -ENOMEM;
		goto error;
	}

	rc = oneplus_dsi_panel_create_cmd_packets(cmd_bufs, cmd_counts,
			packet_count, cmds);
	if (rc) {
		pr_err("failed to create cmd packets, rc=%d\n", rc);
		goto error_free_cmds;
	}

	mutex_lock(&display->panel->panel_lock);

	kfree(cmd->cmds);
	cmd->cmds = cmds;
	cmd->count = packet_count;
	if (oneplus_dsi_state == DSI_CMD_SET_STATE_LP)
		cmd->state = DSI_CMD_SET_STATE_LP;
	else if (oneplus_dsi_state == DSI_CMD_SET_STATE_LP)
		cmd->state = DSI_CMD_SET_STATE_HS;

	mutex_unlock(&display->panel->panel_lock);

	cmd_counts = 0;
	oneplus_dsi_state = DSI_CMD_SET_STATE_HS;

	return count;

error_free_cmds:
	kfree(cmds);
error:
	cmd_counts = 0;
	oneplus_dsi_state = DSI_CMD_SET_STATE_HS;

	return rc;
}

int oneplus_panel_alpha = 0;
struct oneplus_brightness_alpha *oneplus_brightness_alpha_lut = NULL;
struct oneplus_brightness_alpha brightness_alpha_lut[] = {
	{0, 0xff},
	{1, 0xee},
	{2, 0xe8},
	{3, 0xe6},
	{4, 0xe5},
	{6, 0xe4},
	{10, 0xe0},
	{20, 0xd5},
	{30, 0xce},
	{45, 0xc6},
	{70, 0xb7},
	{100, 0xad},
	{150, 0xa0},
	{227, 0x8a},
	{300, 0x80},
	{400, 0x6e},
	{500, 0x5b},
	{600, 0x50},
	{800, 0x38},
	{1023, 0x18},
};

struct oneplus_brightness_alpha brightness_seed_alpha_lut_dc[] = {
	{0, 0xff},
	{1, 0xfc},
	{2, 0xfb},
	{3, 0xfa},
	{4, 0xf9},
	{5, 0xf8},
	{6, 0xf7},
	{8, 0xf6},
	{10, 0xf4},
	{15, 0xf0},
	{20, 0xea},
	{30, 0xe0},
	{45, 0xd0},
	{70, 0xbc},
	{100, 0x98},
	{120, 0x80},
	{140, 0x70},
	{160, 0x58},
	{180, 0x48},
	{200, 0x30},
	{220, 0x20},
	{240, 0x10},
	{260, 0x00},
};

struct oneplus_brightness_alpha brightness_alpha_lut_dc[] = {
	{0, 0xff},
	{1, 0xE0},
	{2, 0xd6},
	{3, 0xd5},
	{4, 0xcf},
	{5, 0xcb},
	{6, 0xc9},
	{8, 0xc5},
	{10, 0xc1},
	{15, 0xb6},
	{20, 0xac},
	{30, 0x9d},
	{45, 0x82},
	{70, 0x6c},
	{100, 0x56},
	{120, 0x47},
	{140, 0x3a},
	{160, 0x2f},
	{180, 0x22},
	{200, 0x16},
	{220, 0xe},
	{240, 0x6},
	{260, 0x00},
};

static int interpolate(int x, int xa, int xb, int ya, int yb)
{
	int bf, factor, plus;
	int sub = 0;

	bf = 2 * (yb - ya) * (x - xa) / (xb - xa);
	factor = bf / 2;
	plus = bf % 2;
	if ((xa - xb) && (yb - ya))
		sub = 2 * (x - xa) * (x - xb) / (yb - ya) / (xa - xb);

	return ya + factor + plus + sub;
}

int oneplus_seed_bright_to_alpha(int brightness)
{
	int level = ARRAY_SIZE(brightness_seed_alpha_lut_dc);
	int i = 0;
	int alpha;

	if (oneplus_panel_alpha)
		return oneplus_panel_alpha;
	for (i = 0; i < ARRAY_SIZE(brightness_seed_alpha_lut_dc); i++){
		if (brightness_seed_alpha_lut_dc[i].brightness >= brightness)
			break;
	}

	if (i == 0)
		alpha = brightness_seed_alpha_lut_dc[0].alpha;
	else if (i == level)
		alpha = brightness_seed_alpha_lut_dc[level - 1].alpha;
	else
		alpha = interpolate(brightness,
			brightness_seed_alpha_lut_dc[i-1].brightness,
			brightness_seed_alpha_lut_dc[i].brightness,
			brightness_seed_alpha_lut_dc[i-1].alpha,
			brightness_seed_alpha_lut_dc[i].alpha);

	return alpha;
}

int bl_to_alpha(int brightness)
{
	struct dsi_display *display = get_main_display();
	struct oneplus_brightness_alpha *lut = NULL;
	int count = 0;
	int i = 0;
	int alpha;

	if (!display)
		return 0;

	if (display->panel->ba_seq && display->panel->ba_count) {
		count = display->panel->ba_count;
		lut = display->panel->ba_seq;
	} else {
		count = ARRAY_SIZE(brightness_alpha_lut);
		lut = brightness_alpha_lut;
	}

	for (i = 0; i < count; i++){
		if (lut[i].brightness >= brightness)
			break;
	}

	if (i == 0)
		alpha = lut[0].alpha;
	else if (i == count)
		alpha = lut[count - 1].alpha;
	else
		alpha = interpolate(brightness, lut[i-1].brightness,
				    lut[i].brightness, lut[i-1].alpha,
				    lut[i].alpha);

	return alpha;
}

int bl_to_alpha_dc(int brightness)
{
	int level = ARRAY_SIZE(brightness_alpha_lut_dc);
	int i = 0;
	int alpha;

	for (i = 0; i < ARRAY_SIZE(brightness_alpha_lut_dc); i++){
		if (brightness_alpha_lut_dc[i].brightness >= brightness)
			break;
	}

	if (i == 0)
		alpha = brightness_alpha_lut_dc[0].alpha;
	else if (i == level)
		alpha = brightness_alpha_lut_dc[level - 1].alpha;
	else
		alpha = interpolate(brightness,
			brightness_alpha_lut_dc[i-1].brightness,
			brightness_alpha_lut_dc[i].brightness,
			brightness_alpha_lut_dc[i-1].alpha,
			brightness_alpha_lut_dc[i].alpha);
	return alpha;
}

int oneplus_dimlayer_hbm = 0;
extern int oneplus_last_backlight;
int brightness_to_alpha(int brightness)
{
	int alpha;

	if (brightness == 0 || brightness == 1)
		brightness = oneplus_last_backlight;

	if (oneplus_dimlayer_hbm)
		alpha = bl_to_alpha(brightness);
	else
		alpha = bl_to_alpha_dc(brightness);

	return alpha;
}

int oneplus_get_panel_brightness(void)
{
	struct dsi_display *display = get_main_display();

	if (!display)
		return 0;

	return display->panel->bl_config.bl_level;
}

extern bool oneplus_ffl_trigger_finish;
int oneplus_get_panel_brightness_to_alpha(void)
{
	struct dsi_display *display = get_main_display();

	if (!display)
		return 0;
	if (oneplus_panel_alpha)
		return oneplus_panel_alpha;

	if (hbm_mode)
		return 0;

	if (!oneplus_ffl_trigger_finish)
		return brightness_to_alpha(FFL_FP_LEVEL);

	return brightness_to_alpha(display->panel->bl_config.bl_level);
}

static ssize_t oneplus_display_get_dim_alpha(struct device *dev,
                                struct device_attribute *attr, char *buf)
{
	struct dsi_display *display = get_main_display();

	if (!display->panel->is_hbm_enabled ||
	    (get_oneplus_display_power_status() != ONEPLUS_DISPLAY_POWER_ON))
		return sprintf(buf, "%d\n", 0);

	return sprintf(buf, "%d\n", oneplus_underbrightness_alpha);
}

static ssize_t oneplus_display_set_dim_alpha(struct device *dev,
                               struct device_attribute *attr,
                               const char *buf, size_t count)
{
	sscanf(buf, "%x", &oneplus_panel_alpha);

	return count;
}

static ssize_t oneplus_display_get_dc_dim_alpha(struct device *dev,
                                struct device_attribute *attr, char *buf)
{
	int ret = 0;
	struct dsi_display *display = get_main_display();

	if (display->panel->is_hbm_enabled ||
	    get_oneplus_display_power_status() != ONEPLUS_DISPLAY_POWER_ON)
		ret = 0;
	if(oneplus_dc2_alpha != 0) {
		ret = oneplus_dc2_alpha;
	} else if(oneplus_underbrightness_alpha != 0){
		ret = oneplus_underbrightness_alpha;
	} else if (oneplus_dimlayer_bl_enable_v3_real) {
		ret = 1;
	}

	return sprintf(buf, "%d\n", ret);
}

static ssize_t oneplus_display_get_dimlayer_backlight(struct device *dev,
                                struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d %d %d %d %d %d\n", oneplus_dimlayer_bl_alpha,
			oneplus_dimlayer_bl_alpha_value, oneplus_dimlayer_dither_threshold,
			oneplus_dimlayer_dither_bitdepth, oneplus_dimlayer_bl_delay, oneplus_dimlayer_bl_delay_after);
}

static ssize_t oneplus_display_set_dimlayer_backlight(struct device *dev,
                               struct device_attribute *attr,
                               const char *buf, size_t count)
{
	sscanf(buf, "%d %d %d %d %d %d", &oneplus_dimlayer_bl_alpha,
		&oneplus_dimlayer_bl_alpha_value, &oneplus_dimlayer_dither_threshold,
		&oneplus_dimlayer_dither_bitdepth, &oneplus_dimlayer_bl_delay,
		&oneplus_dimlayer_bl_delay_after);

	return count;
}

int oneplus_fod_on_vblank = -1;
int oneplus_fod_off_vblank = -1;
static int oneplus_datadimming_v3_debug_value = -1;
static int oneplus_datadimming_v3_debug_delay = 16000;
static ssize_t oneplus_display_get_debug(struct device *dev,
                                struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d %d %d %d\n", oneplus_fod_on_vblank, oneplus_fod_off_vblank, oneplus_datadimming_v3_debug_value, oneplus_datadimming_v3_debug_delay);
}

static ssize_t oneplus_display_set_debug(struct device *dev,
                               struct device_attribute *attr,
                               const char *buf, size_t count)
{
	sscanf(buf, "%d %d %d %d", &oneplus_fod_on_vblank, &oneplus_fod_off_vblank, &oneplus_datadimming_v3_debug_value, &oneplus_datadimming_v3_debug_delay);

	return count;
}


static ssize_t oneplus_display_get_dimlayer_enable(struct device *dev,
                                struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", oneplus_dimlayer_bl_enable_v2);
}


int oneplus_datadimming_vblank_count = 0;
atomic_t oneplus_datadimming_vblank_ref = ATOMIC_INIT(0);
static int oneplus_boe_data_dimming_process_unlock(int brightness, int enable)
{
	struct dsi_display *display = get_main_display();
	struct drm_connector *dsi_connector = display->drm_conn;
	struct dsi_panel *panel = display->panel;
	struct mipi_dsi_device *mipi_device;
	int rc = 0;

	if (!panel) {
		pr_err("failed to find display panel\n");
		return -EINVAL;
	}

	if (!dsi_panel_initialized(panel)) {
		pr_err("dsi_panel_aod_low_light_mode is not init\n");
		return -EINVAL;
	}

	mipi_device = &panel->mipi_device;

	if (!panel->is_hbm_enabled && oneplus_datadimming_vblank_count != 0) {
		drm_crtc_wait_one_vblank(dsi_connector->state->crtc);
		rc = mipi_dsi_dcs_set_display_brightness(mipi_device, brightness);
		drm_crtc_wait_one_vblank(dsi_connector->state->crtc);
	}

	/* enable the clk vote for CMD mode panels */
	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		dsi_display_clk_ctrl(display->dsi_clk_handle,
				DSI_ALL_CLKS, DSI_CLK_ON);
	}

	if (enable) {
		rc = dsi_panel_tx_cmd_set(display->panel, DSI_CMD_DATA_DIMMING_ON);
	} else {
		rc = dsi_panel_tx_cmd_set(display->panel, DSI_CMD_DATA_DIMMING_OFF);
	}

	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		dsi_display_clk_ctrl(display->dsi_clk_handle,
				DSI_ALL_CLKS, DSI_CLK_OFF);
	}

	return rc;
}

struct oneplus_brightness_alpha brightness_remapping[] = {
	{0, 0},
	{1, 1},
	{2, 155},
	{3, 160},
	{5, 165},
	{6, 170},
	{8, 175},
	{10, 180},
	{20, 190},
	{40, 230},
	{80, 270},
	{100, 300},
	{150, 400},
	{200, 600},
	{300, 800},
	{400, 1000},
	{600, 1250},
	{800, 1400},
	{1000, 1500},
	{1200, 1650},
	{1400, 1800},
	{1600, 1900},
	{1780, 2000},
	{2047, 2047},
};

int oneplus_backlight_remapping(int brightness)
{
	struct oneplus_brightness_alpha *lut = brightness_remapping;
	int count = ARRAY_SIZE(brightness_remapping);
	int i = 0;
	int bl_lvl = brightness;

	if (oneplus_datadimming_v3_debug_value >=0)
		return oneplus_datadimming_v3_debug_value;

	for (i = 0; i < count; i++){
		if (lut[i].brightness >= brightness)
			break;
	}

	if (i == 0)
		bl_lvl = lut[0].alpha;
	else if (i == count)
		bl_lvl = lut[count - 1].alpha;
	else
		bl_lvl = interpolate(brightness, lut[i-1].brightness,
				    lut[i].brightness, lut[i-1].alpha,
				    lut[i].alpha);

	return bl_lvl;
}

static bool dimming_enable = false;

int oneplus_panel_process_dimming_v3_post(struct dsi_panel *panel, int brightness)
{
	bool enable = oneplus_dimlayer_bl_enable_v3_real;
	int ret = 0;

	if (brightness <= 1)
		enable = false;
	if (enable != dimming_enable) {
		if (enable) {
			ret = oneplus_boe_data_dimming_process_unlock(brightness, true);
			if (ret) {
				pr_err("failed to enable data dimming\n");
				goto error;
			}
			dimming_enable = true;
			pr_err("Enter DC backlight v3\n");
		} else {
			ret = oneplus_boe_data_dimming_process_unlock(brightness, false);
			if (ret) {
				pr_err("failed to enable data dimming\n");
				goto error;
			}
			dimming_enable = false;
			pr_err("Exit DC backlight v3\n");
		}
	}

error:
	return 0;
}

extern int oneplus_seed_backlight;
extern int oneplus_dimlayer_bl_enable_v2_real;
extern bool oneplus_skip_datadimming_sync;
static bool oneplus_datadimming_v2_need_flush = false;
static bool oneplus_datadimming_v2_need_sync = false;
void oneplus_panel_process_dimming_v2_post(struct dsi_panel *panel, bool force_disable)
{
	struct dsi_display *display = get_main_display();
	struct drm_connector *dsi_connector = display->drm_conn;

	if (oneplus_datadimming_v2_need_flush) {
		if (oneplus_datadimming_v2_need_sync &&
		    (!strcmp(panel->oneplus_priv.vendor_name,"AMB655UV01") ||
			 !strcmp(panel->oneplus_priv.vendor_name,"AMS643XF01")) &&
		    dsi_connector && dsi_connector->state && dsi_connector->state->crtc) {
			struct drm_crtc *crtc = dsi_connector->state->crtc;
			int frame_time_us, ret = 0;
			u32 current_vblank;

			frame_time_us = mult_frac(1000, 1000, panel->cur_mode->timing.refresh_rate);

			current_vblank = drm_crtc_vblank_count(crtc);
			ret = wait_event_timeout(*drm_crtc_vblank_waitqueue(crtc),
					current_vblank != drm_crtc_vblank_count(crtc),
					usecs_to_jiffies(frame_time_us + 1000));
			if (!ret)
				pr_err("%s: crtc wait_event_timeout \n", __func__);

		}

		if (display->config.panel_mode == DSI_OP_VIDEO_MODE)
			panel->oneplus_priv.skip_mipi_last_cmd = true;
		dsi_panel_seed_mode_unlock(panel, seed_mode);
		if (display->config.panel_mode == DSI_OP_VIDEO_MODE)
			panel->oneplus_priv.skip_mipi_last_cmd = false;
		oneplus_datadimming_v2_need_flush = false;
	}
}

int oneplus_panel_process_dimming_v2(struct dsi_panel *panel, int bl_lvl, bool force_disable)
{
	struct dsi_display *display = get_main_display();
	struct drm_connector *dsi_connector = display->drm_conn;

	oneplus_datadimming_v2_need_flush = false;
	oneplus_datadimming_v2_need_sync = false;
	if (!force_disable && oneplus_dimlayer_bl_enable_v2_real &&
	    bl_lvl > 1 && bl_lvl < 260) {
		if (!oneplus_seed_backlight) {
			pr_err("Enter DC backlight v2\n");
			oneplus_dc_v2_on = true;
			if (!oneplus_skip_datadimming_sync &&
			    oneplus_last_backlight != 0 &&
			    oneplus_last_backlight != 1)
				oneplus_datadimming_v2_need_sync = true;
		}
		oneplus_seed_backlight = bl_lvl;
		bl_lvl = oneplus_dimlayer_bl_alpha;
		oneplus_datadimming_v2_need_flush = true;
	} else if (oneplus_seed_backlight) {
		pr_err("Exit DC backlight v2\n");
		oneplus_dc_v2_on = false;
		oneplus_seed_backlight = 0;
		oneplus_dc2_alpha = 0;
		oneplus_datadimming_v2_need_flush = true;
		oneplus_datadimming_v2_need_sync = true;
	}

	if (oneplus_datadimming_v2_need_flush) {
		if (oneplus_datadimming_v2_need_sync &&
		    (!strcmp(panel->oneplus_priv.vendor_name,"AMB655UV01") ||
			 !strcmp(panel->oneplus_priv.vendor_name,"AMS643XF01")) &&
		    dsi_connector && dsi_connector->state && dsi_connector->state->crtc) {
			struct drm_crtc *crtc = dsi_connector->state->crtc;
			int frame_time_us, ret = 0;
			u32 current_vblank;

			frame_time_us = mult_frac(1000, 1000, panel->cur_mode->timing.refresh_rate);

			current_vblank = drm_crtc_vblank_count(crtc);
			ret = wait_event_timeout(*drm_crtc_vblank_waitqueue(crtc),
					current_vblank != drm_crtc_vblank_count(crtc),
					usecs_to_jiffies(frame_time_us + 1000));
			if (!ret)
				pr_err("%s: crtc wait_event_timeout \n", __func__);

		}
	}
	if (oneplus_datadimming_v2_need_flush && strcmp(panel->oneplus_priv.vendor_name,"AMB655UV01"))
		oneplus_panel_process_dimming_v2_post(panel, force_disable);

	return bl_lvl;
}

int oneplus_panel_process_dimming_v3(struct dsi_panel *panel, int brightness)
{
	bool enable = oneplus_dimlayer_bl_enable_v3_real;
	int bl_lvl = brightness;
	if (enable)
		bl_lvl = oneplus_backlight_remapping(brightness);

	oneplus_panel_process_dimming_v3_post(panel, bl_lvl);
	return bl_lvl;
}

int oneplus_panel_update_backlight_unlock(struct dsi_panel *panel)
{
	return dsi_panel_set_backlight(panel, panel->bl_config.bl_level);
}

static ssize_t oneplus_display_set_dimlayer_enable(struct device *dev,
                               struct device_attribute *attr,
                               const char *buf, size_t count)
{
	struct dsi_display *display = get_main_display();
	struct drm_connector *dsi_connector = display->drm_conn;

	if (display && display->name) {
		int enable = 0;
		int err = 0;

		sscanf(buf, "%d", &enable);
		mutex_lock(&display->display_lock);
		if (!dsi_connector || !dsi_connector->state || !dsi_connector->state->crtc) {
			pr_err("[%s]: display not ready\n", __func__);
		}else {
			err = drm_crtc_vblank_get(dsi_connector->state->crtc);
			if (err) {
				pr_err("failed to get crtc vblank, error=%d\n", err);
			} else {
				/* do vblank put after 7 frames */
				oneplus_datadimming_vblank_count= 7;
				atomic_inc(&oneplus_datadimming_vblank_ref);
			}
		}

		usleep_range(17000, 17100);
		if (!strcmp(display->name,"qcom,mdss_dsi_oneplus19101boe_nt37800_1080_2400_cmd"))
			oneplus_dimlayer_bl_enable_v3 = enable;
		else
			oneplus_dimlayer_bl_enable_v2 = enable;

		mutex_unlock(&display->display_lock);
	}

	return count;
}

static ssize_t oneplus_display_get_dimlayer_hbm(struct device *dev,
                                struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", oneplus_dimlayer_hbm);
}

int oneplus_dimlayer_hbm_vblank_count = 0;
atomic_t oneplus_dimlayer_hbm_vblank_ref = ATOMIC_INIT(0);
static ssize_t oneplus_display_set_dimlayer_hbm(struct device *dev,
                               struct device_attribute *attr,
                               const char *buf, size_t count)
{
	struct dsi_display *display = get_main_display();
	struct drm_connector *dsi_connector = display->drm_conn;
	int err = 0;
	int value = 0;

	sscanf(buf, "%d", &value);
	value = !!value;
	if (oneplus_dimlayer_hbm == value)
		return count;
	if (!dsi_connector || !dsi_connector->state || !dsi_connector->state->crtc) {
		pr_err("[%s]: display not ready\n", __func__);
	} else {
		err = drm_crtc_vblank_get(dsi_connector->state->crtc);
		if (err) {
			pr_err("failed to get crtc vblank, error=%d\n", err);
		} else {
			/* do vblank put after 5 frames */
			oneplus_dimlayer_hbm_vblank_count = 5;
			atomic_inc(&oneplus_dimlayer_hbm_vblank_ref);
		}
	}
	oneplus_dimlayer_hbm = value;

	pr_err("debug for oneplus_display_set_dimlayer_hbm set oneplus_dimlayer_hbm = %d\n",oneplus_dimlayer_hbm);

	return count;
}

int oneplus_force_screenfp = 0;
static ssize_t oneplus_display_get_forcescreenfp(struct device *dev,
                                struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", oneplus_force_screenfp);
}

static ssize_t oneplus_display_set_forcescreenfp(struct device *dev,
                               struct device_attribute *attr,
                               const char *buf, size_t count)
{
	sscanf(buf, "%x", &oneplus_force_screenfp);

	return count;
}

static ssize_t oneplus_display_get_esd_status(struct device *dev,
                                struct device_attribute *attr, char *buf)
{
	struct dsi_display *display = get_main_display();
	int rc = 0;

	return rc;

	if (!display)
		return -ENODEV;
	mutex_lock(&display->display_lock);

	if (!display->panel) {
		rc = -EINVAL;
		goto error;
	}

	rc = sprintf(buf, "%d\n", display->panel->esd_config.esd_enabled);

error:
	mutex_unlock(&display->display_lock);
	return rc;
}

static ssize_t oneplus_display_set_esd_status(struct device *dev,
                               struct device_attribute *attr,
                               const char *buf, size_t count)
{
	struct dsi_display *display = get_main_display();
	int enable = 0;

	return count;

	sscanf(buf, "%du", &enable);

	pr_err("debug for oneplus_display_set_esd_status, the enable value = %d\n", enable);

	if (!display)
		return -ENODEV;

	if (!display->panel || !display->drm_conn) {
		return -EINVAL;
	}

	if (!enable) {
		if (display->panel->esd_config.esd_enabled)
		{
			sde_connector_schedule_status_work(display->drm_conn, false);
			display->panel->esd_config.esd_enabled = false;
			pr_err("disable esd work");
		}
	}

	return count;
}

static ssize_t oneplus_display_notify_panel_blank(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count) {

	struct msm_drm_notifier notifier_data;
	int blank;
	int temp_save = 0;

	sscanf(buf, "%du", &temp_save);
	printk(KERN_INFO "%s oneplus_display_notify_panel_blank = %d\n", __func__, temp_save);

	if(temp_save == 1) {
		blank = MSM_DRM_BLANK_UNBLANK;
		notifier_data.data = &blank;
		notifier_data.id = 0;
		msm_drm_notifier_call_chain(MSM_DRM_EARLY_EVENT_BLANK,
						   &notifier_data);
		msm_drm_notifier_call_chain(MSM_DRM_EVENT_BLANK,
						   &notifier_data);
	} else if (temp_save == 0) {
		blank = MSM_DRM_BLANK_POWERDOWN;
		notifier_data.data = &blank;
		notifier_data.id = 0;
		msm_drm_notifier_call_chain(MSM_DRM_EARLY_EVENT_BLANK,
						   &notifier_data);
	}
	return count;
}

#define FFL_LEVEL_START 2
#define FFL_LEVEL_END  236
#define FFLUPRARE  1
#define BACKUPRATE 6
#define FFL_PENDING_END 600
#define FFL_EXIT_CONTROL 0
#define FFL_TRIGGLE_CONTROL 1
#define FFL_EXIT_FULLY_CONTROL 2

bool ffl_work_running = false;
bool oneplus_ffl_trigger_finish = true;
static int is_ffl_enable = FFL_EXIT_CONTROL;
struct task_struct *oneplus_ffl_thread;
struct kthread_worker oneplus_ffl_worker;
struct kthread_work oneplus_ffl_work;
static DEFINE_MUTEX(oneplus_ffl_lock);

static ssize_t oneplus_get_ffl_setting(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", is_ffl_enable);
}

static ssize_t oneplus_set_ffl_setting(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int enable = 0;
	unsigned char payload[150] = "";

	sscanf(buf, "%du", &enable);
	printk(KERN_INFO "%s oneplus_set_ffl_setting = %d\n", __func__, enable);

	mutex_lock(&oneplus_ffl_lock);

	if(enable != is_ffl_enable) {
		pr_debug("set_ffl_setting need change is_ffl_enable\n");
		is_ffl_enable = enable;
		if ((is_ffl_enable ==FFL_TRIGGLE_CONTROL) && ffl_work_running){
			oneplus_ffl_trigger_finish = false;
			kthread_queue_work(&oneplus_ffl_worker, &oneplus_ffl_work);
		}
	}

	mutex_unlock(&oneplus_ffl_lock);

	if ((is_ffl_enable ==FFL_TRIGGLE_CONTROL) && ffl_work_running) {
		scnprintf(payload, sizeof(payload), "NULL$$EventID@@%d$$fflset@@%d",ONEPLUS_MM_DIRVER_FB_EVENT_ID_FFLSET,enable);
		upload_mm_kevent_fb_data(ONEPLUS_MM_DIRVER_FB_EVENT_MODULE_DISPLAY,payload);
	}

	return count;
}

static void oneplus_ffl_setting_thread(struct kthread_work *work)
{
	struct dsi_display *display = get_main_display();
	int index =0;
	int pending =0;
	int system_backlight_target;
	int rc;

	if (get_oneplus_display_power_status() == ONEPLUS_DISPLAY_POWER_OFF)
		return;

	if (is_ffl_enable != FFL_TRIGGLE_CONTROL)
		return;

	if (!display || !display->panel) {
		pr_err("failed to find display panel \n");
		return;
	}

	if (!ffl_work_running)
		return;

	rc = dsi_display_clk_ctrl(display->dsi_clk_handle,
			DSI_CORE_CLK, DSI_CLK_ON);
	if (rc) {
		pr_err("[%s] failed to enable DSI core clocks, rc=%d\n",
				display->name, rc);
		return;
	}

	for(index = FFL_LEVEL_START;index < FFL_LEVEL_END;index = index + FFLUPRARE ) {
		if((is_ffl_enable ==FFL_EXIT_CONTROL) ||
		   (is_ffl_enable ==FFL_EXIT_FULLY_CONTROL) ||
		   !ffl_work_running)
			break;
		/*
		 * On Onscreenfingerprint mode, max backlight level should be FFL_FP_LEVEL
		 */
		if (display->panel->is_hbm_enabled && index > FFL_FP_LEVEL)
			break;

		mutex_lock(&display->panel->panel_lock);
		dsi_panel_set_backlight(display->panel, index);
		mutex_unlock(&display->panel->panel_lock);
		usleep_range(1000, 1100);
	}

	for(pending =0; pending <= FFL_PENDING_END; pending++)
	{
		if((is_ffl_enable ==FFL_EXIT_CONTROL) ||
		   (is_ffl_enable ==FFL_EXIT_FULLY_CONTROL) ||
		   !ffl_work_running)
			break;
		usleep_range(8000, 8100);
	}

	system_backlight_target = display->panel->bl_config.bl_level;

	if(index < system_backlight_target) {
		for(index; index < system_backlight_target; index =index + BACKUPRATE) {
			if((is_ffl_enable ==FFL_EXIT_FULLY_CONTROL) ||
			   !ffl_work_running)
				break;
			mutex_lock(&display->panel->panel_lock);
			dsi_panel_set_backlight(display->panel, index);
			mutex_unlock(&display->panel->panel_lock);
			usleep_range(6000, 6100);
		}
	} else if (index > system_backlight_target) {
		for(index; index > system_backlight_target; index =index - BACKUPRATE) {
			if((is_ffl_enable ==FFL_EXIT_FULLY_CONTROL) ||
			   !ffl_work_running)
				break;
			mutex_lock(&display->panel->panel_lock);
			dsi_panel_set_backlight(display->panel, index);
			mutex_unlock(&display->panel->panel_lock);
			usleep_range(6000, 6100);
		}
	}

	mutex_lock(&display->panel->panel_lock);
	system_backlight_target = display->panel->bl_config.bl_level;
	dsi_panel_set_backlight(display->panel, system_backlight_target);
	oneplus_ffl_trigger_finish = true;
	mutex_unlock(&display->panel->panel_lock);

	rc = dsi_display_clk_ctrl(display->dsi_clk_handle,
			DSI_CORE_CLK, DSI_CLK_OFF);
	if (rc) {
		pr_err("[%s] failed to disable DSI core clocks, rc=%d\n",
				display->name, rc);
	}
}

int oneplus_start_ffl_thread(void)
{
	mutex_lock(&oneplus_ffl_lock);

	ffl_work_running = true;
	if (is_ffl_enable == FFL_TRIGGLE_CONTROL) {
		oneplus_ffl_trigger_finish = false;
		kthread_queue_work(&oneplus_ffl_worker, &oneplus_ffl_work);
	}

	mutex_unlock(&oneplus_ffl_lock);
	return 0;
}

void oneplus_stop_ffl_thread(void)
{
	mutex_lock(&oneplus_ffl_lock);

	oneplus_ffl_trigger_finish = true;
	ffl_work_running = false;
	kthread_flush_worker(&oneplus_ffl_worker);

	mutex_unlock(&oneplus_ffl_lock);
}

int oneplus_display_atomic_check(struct drm_crtc *crtc, struct drm_crtc_state *state)
{
	struct dsi_display *display = get_main_display();

	if (!is_dsi_panel(crtc))
		return 0;

	if (display && display->panel &&
	    display->panel->oneplus_priv.is_aod_ramless &&
	    is_oneplus_display_aod_mode() &&
	    (crtc->state->mode.flags | DRM_MODE_FLAG_CMD_MODE_PANEL)) {
		wait_event_timeout(oneplus_aod_wait, !is_oneplus_display_aod_mode(),
				   msecs_to_jiffies(100));
	}

	return 0;
}

static ssize_t oneplus_display_notify_fp_press(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct dsi_display *display = get_main_display();
	struct drm_device *drm_dev = display->drm_dev;
	struct drm_connector *dsi_connector = display->drm_conn;
	struct drm_mode_config *mode_config = &drm_dev->mode_config;
	struct msm_drm_private *priv = drm_dev->dev_private;
	struct drm_display_mode *cmd_mode = NULL;
	struct drm_display_mode *vid_mode = NULL;
	struct drm_display_mode *cur_mode = NULL;
	struct drm_display_mode *mode = NULL;
	struct drm_atomic_state *state;
	struct drm_crtc_state *crtc_state;
	struct drm_crtc *crtc;
	static ktime_t on_time;
	bool mode_changed = false;
	int onscreenfp_status = 0;
	int vblank_get = -EINVAL;
	int err = 0;
	int i;

	if (!dsi_connector || !dsi_connector->state || !dsi_connector->state->crtc) {
		pr_err("[%s]: display not ready\n", __func__);
		return count;
	}

	sscanf(buf, "%du", &onscreenfp_status);
	onscreenfp_status = !!onscreenfp_status;
	if (onscreenfp_status == oneplus_onscreenfp_status)
		return count;

	pr_err("notify fingerpress %s\n", onscreenfp_status ? "on" : "off");
	if (ONEPLUS_DISPLAY_AOD_SCENE == get_oneplus_display_scene()) {
		if (onscreenfp_status) {
			on_time = ktime_get();
		} else {
			ktime_t now = ktime_get();
			ktime_t delta = ktime_sub(now, on_time);

			if (ktime_to_ns(delta) < 300000000)
				msleep(300 - (ktime_to_ns(delta) / 1000000));
		}
	}

	vblank_get = drm_crtc_vblank_get(dsi_connector->state->crtc);
	if (vblank_get) {
		pr_err("failed to get crtc vblank\n", vblank_get);
	}
	oneplus_onscreenfp_status = onscreenfp_status;

	if (onscreenfp_status &&
	    ONEPLUS_DISPLAY_AOD_SCENE == get_oneplus_display_scene() && !display->panel->oneplus_priv.is_aod_ramless) {
		/* enable the clk vote for CMD mode panels */
		if (display->config.panel_mode == DSI_OP_CMD_MODE) {
			dsi_display_clk_ctrl(display->dsi_clk_handle,
					DSI_ALL_CLKS, DSI_CLK_ON);
		}

		mutex_lock(&display->panel->panel_lock);

		if (display->panel->panel_initialized)
			err = dsi_panel_tx_cmd_set(display->panel, DSI_CMD_AOD_HBM_ON);

		mutex_unlock(&display->panel->panel_lock);
		if (err)
			pr_err("failed to setting aod hbm on mode %d\n", err);

		if (display->config.panel_mode == DSI_OP_CMD_MODE) {
			dsi_display_clk_ctrl(display->dsi_clk_handle,
					DSI_ALL_CLKS, DSI_CLK_OFF);
		}
	}

	if (!display->panel->oneplus_priv.is_aod_ramless) {
		oneplus_onscreenfp_vblank_count = drm_crtc_vblank_count(dsi_connector->state->crtc);
		oneplus_onscreenfp_pressed_time = ktime_get();
	}

	drm_modeset_lock_all(drm_dev);

	state = drm_atomic_state_alloc(drm_dev);
	if (!state)
		goto error;
	state->acquire_ctx = mode_config->acquire_ctx;
	crtc = dsi_connector->state->crtc;
	crtc_state = drm_atomic_get_crtc_state(state, crtc);
	cur_mode = &crtc->state->mode;
	for (i = 0; i < priv->num_crtcs; i++) {
		if (priv->disp_thread[i].crtc_id == crtc->base.id) {
			if (priv->disp_thread[i].thread)
				kthread_flush_worker(&priv->disp_thread[i].worker);
		}
	}

	if (display->panel->oneplus_priv.is_aod_ramless) {
		struct drm_display_mode *set_mode = NULL;

		if (oneplus_display_mode == 2)
			goto error;

		list_for_each_entry(mode, &dsi_connector->modes, head) {
			if (drm_mode_vrefresh(mode) == 0)
				continue;
			if (mode->flags & DRM_MODE_FLAG_VID_MODE_PANEL)
				vid_mode = mode;
			if (mode->flags & DRM_MODE_FLAG_CMD_MODE_PANEL)
				cmd_mode = mode;
		}

		set_mode = oneplus_display_mode ? vid_mode : cmd_mode;
		set_mode = onscreenfp_status ? vid_mode : set_mode;
		if (!crtc_state->active || !crtc_state->enable)
			goto error;

		if (set_mode && drm_mode_vrefresh(set_mode) != drm_mode_vrefresh(&crtc_state->mode)) {
			mode_changed = true;
		} else {
			mode_changed = false;
		}

		if (mode_changed) {
			display->panel->dyn_clk_caps.dyn_clk_support = false;
			drm_atomic_set_mode_for_crtc(crtc_state, set_mode);
		}

		wake_up(&oneplus_aod_wait);
	}

	err = drm_atomic_commit(state);
	drm_atomic_state_put(state);

	if (display->panel->oneplus_priv.is_aod_ramless && mode_changed) {
		for (i = 0; i < priv->num_crtcs; i++) {
			if (priv->disp_thread[i].crtc_id == crtc->base.id) {
				if (priv->disp_thread[i].thread)
					kthread_flush_worker(&priv->disp_thread[i].worker);
			}
		}

		if (oneplus_display_mode == 1)
			display->panel->dyn_clk_caps.dyn_clk_support = true;
	}

error:
	drm_modeset_unlock_all(drm_dev);
	if (!vblank_get)
		drm_crtc_vblank_put(dsi_connector->state->crtc);
	return count;
}

static ssize_t oneplus_display_get_roundcorner(struct device *dev,
                                struct device_attribute *attr, char *buf)
{
	struct dsi_display *display = get_main_display();
	bool roundcorner = true;

	if (display && display->name &&
	    !strcmp(display->name,"qcom,mdss_dsi_oneplus19101boe_nt37800_1080_2400_cmd"))
		roundcorner = false;

	return sprintf(buf, "%d\n", roundcorner);
}

static int dynamic_osc_clock = 90250;
static ssize_t oneplus_display_get_dynamic_osc_clock(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", dynamic_osc_clock);
}

int oneplus_dsi_update_dynamic_osc_clock(void){
	struct dsi_display *display = get_main_display();
	int rc = 0;
	int osc_clock = dynamic_osc_clock;

	if (!display) {
		pr_err("failed for: %s %d\n", __func__, __LINE__);
		return -EINVAL;
	}
	mutex_lock(&display->display_lock);
	mutex_lock(&display->panel->panel_lock);

	if (!dsi_panel_initialized(display->panel)) {
		rc = -EINVAL;
		goto unlock;
	}

	/* enable the clk vote for CMD mode panels */
	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		dsi_display_clk_ctrl(display->dsi_clk_handle,
				DSI_CORE_CLK, DSI_CLK_ON);
	}

	if(osc_clock == 90250){
		rc = dsi_panel_tx_cmd_set(display->panel, DSI_CMD_OSC_CLK_MODEO0);
	} else if(osc_clock == 88900) {
		rc = dsi_panel_tx_cmd_set(display->panel, DSI_CMD_OSC_CLK_MODEO1);
	}

	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		rc = dsi_display_clk_ctrl(display->dsi_clk_handle,
				DSI_CORE_CLK, DSI_CLK_OFF);
	}

unlock:
	mutex_unlock(&display->panel->panel_lock);
	mutex_unlock(&display->display_lock);

	return rc;
}

static ssize_t oneplus_display_set_dynamic_osc_clock(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct dsi_display *display = get_main_display();
	int osc_clk = 0;

	if (!display) {
		pr_err("failed for: %s %d\n", __func__, __LINE__);
		return -EINVAL;
	}
	sscanf(buf, "%du", &osc_clk);
	dynamic_osc_clock = osc_clk;

	if (display->panel->panel_mode != DSI_OP_CMD_MODE) {
		pr_err("only supported for command mode\n");
		return -EFAULT;
	}

	pr_info("%s: osc clk param value: '%d'\n", __func__, osc_clk);

	oneplus_dsi_update_dynamic_osc_clock();
	return count;
}

int oneplus_debug_max_brightness = 0;
static ssize_t oneplus_display_get_max_brightness(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", oneplus_debug_max_brightness);
}

static ssize_t oneplus_display_set_max_brightness(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	sscanf(buf, "%du", &oneplus_debug_max_brightness);

	return count;
}

static ssize_t oneplus_display_get_ccd_check(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct dsi_display *display = get_main_display();
	struct mipi_dsi_device *mipi_device;
	int rc = 0;
	int ccd_check = 0;

	if (!display || !display->panel) {
		pr_err("failed for: %s %d\n", __func__, __LINE__);
		return -EINVAL;
	}

	if(get_oneplus_display_power_status() != ONEPLUS_DISPLAY_POWER_ON) {
		printk(KERN_ERR"%s display panel in off status\n", __func__);
		return -EFAULT;
	}

	if (display->panel->panel_mode != DSI_OP_CMD_MODE) {
		pr_err("only supported for command mode\n");
		return -EFAULT;
	}

	if (!(display && display->panel->oneplus_priv.vendor_name &&
	      !strcmp(display->panel->oneplus_priv.vendor_name,"AMB655UV01"))) {
		ccd_check = 0;
		goto end;
	}

	mipi_device = &display->panel->mipi_device;

	mutex_lock(&display->display_lock);
	mutex_lock(&display->panel->panel_lock);

	if (!dsi_panel_initialized(display->panel)) {
		rc = -EINVAL;
		goto unlock;
	}

	rc = dsi_display_cmd_engine_enable(display);
	if (rc) {
		pr_err("%s, cmd engine enable failed\n", __func__);
		goto unlock;
	}

	/* enable the clk vote for CMD mode panels */
	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		dsi_display_clk_ctrl(display->dsi_clk_handle,
				DSI_CORE_CLK, DSI_CLK_ON);
	}

	{
		char value[] = { 0x5A, 0x5A };
		rc = mipi_dsi_dcs_write(mipi_device, 0xF0, value, sizeof(value));
	}
	{
		char value[] = { 0x44, 0x50 };
		rc = mipi_dsi_dcs_write(mipi_device, 0xE7, value, sizeof(value));
	}
	usleep_range(1000, 1100);
	{
		char value[] = { 0x03 };
		rc = mipi_dsi_dcs_write(mipi_device, 0xB0, value, sizeof(value));
	}

	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		rc = dsi_display_clk_ctrl(display->dsi_clk_handle,
				DSI_CORE_CLK, DSI_CLK_OFF);
	}
	dsi_display_cmd_engine_disable(display);

	mutex_unlock(&display->panel->panel_lock);
	mutex_unlock(&display->display_lock);
	{
		unsigned char read[10];

		rc = dsi_display_read_panel_reg(display, 0xE1, read, 1);

		pr_err("read ccd_check value = 0x%x rc=%d\n", read[0], rc);
		ccd_check = read[0];
	}

	mutex_lock(&display->display_lock);
	mutex_lock(&display->panel->panel_lock);

	if (!dsi_panel_initialized(display->panel)) {
		rc = -EINVAL;
		goto unlock;
	}

	rc = dsi_display_cmd_engine_enable(display);
	if (rc) {
		pr_err("%s, cmd engine enable failed\n", __func__);
		goto unlock;
	}

	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		dsi_display_clk_ctrl(display->dsi_clk_handle,
				DSI_CORE_CLK, DSI_CLK_ON);
	}

	{
		char value[] = { 0xA5, 0xA5 };
		rc = mipi_dsi_dcs_write(mipi_device, 0xF0, value, sizeof(value));
	}

	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		rc = dsi_display_clk_ctrl(display->dsi_clk_handle,
				DSI_CORE_CLK, DSI_CLK_OFF);
	}

	dsi_display_cmd_engine_disable(display);
unlock:

	mutex_unlock(&display->panel->panel_lock);
	mutex_unlock(&display->display_lock);
end:
	pr_err("[%s] ccd_check = %d\n",  display->panel->oneplus_priv.vendor_name, ccd_check);
	return sprintf(buf, "%d\n", ccd_check);
}

struct aod_area {
	bool enable;
	int x;
	int y;
	int w;
	int h;
	int color;
	int bitdepth;
	int mono;
	int gray;
};

#define RAMLESS_AOD_AREA_NUM		6
#define RAMLESS_AOD_PAYLOAD_SIZE	100
static struct aod_area oneplus_aod_area[RAMLESS_AOD_AREA_NUM];

int oneplus_display_update_aod_area_unlock(void)
{
	struct dsi_display *display = get_main_display();
	struct mipi_dsi_device *mipi_device;
	char payload[RAMLESS_AOD_PAYLOAD_SIZE];
	int rc = 0;
	int i;

	if (!display || !display->panel || !display->panel->oneplus_priv.is_aod_ramless)
		return 0;

	if (!dsi_panel_initialized(display->panel))
		return -EINVAL;

	mipi_device = &display->panel->mipi_device;

	/* enable the clk vote for CMD mode panels */
	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		dsi_display_clk_ctrl(display->dsi_clk_handle,
				DSI_CORE_CLK, DSI_CLK_ON);
	}

	memset(payload, 0, RAMLESS_AOD_PAYLOAD_SIZE);

	for (i = 0; i < RAMLESS_AOD_AREA_NUM; i++) {
		struct aod_area *area = &oneplus_aod_area[i];

		payload[0] |= (!!area->enable) << (RAMLESS_AOD_AREA_NUM - i - 1);
		if (area->enable) {
			int h_start = area->x;
			int h_block = area->w / 100;
			int v_start = area->y;
			int v_end = area->y + area->h;
			int off = i * 5;

			/* Rect Setting */
			payload[1 + off] = h_start >> 4;
			payload[2 + off] = ((h_start & 0xf) << 4) | (h_block & 0xf);
			payload[3 + off] = v_start >> 4;
			payload[4 + off] = ((v_start & 0xf) << 4) | ((v_end >> 8) & 0xf);
			payload[5 + off] = v_end & 0xff;

			/* Mono Setting */
			#define SET_MONO_SEL(index, shift) \
				if (i == index) \
					payload[31] |= area->mono << shift;

			SET_MONO_SEL(0, 6);
			SET_MONO_SEL(1, 5);
			SET_MONO_SEL(2, 4);
			SET_MONO_SEL(3, 2);
			SET_MONO_SEL(4, 1);
			SET_MONO_SEL(5, 0);
			#undef SET_MONO_SEL

			/* Depth Setting */
			if (i < 4)
				payload[32] |= (area->bitdepth & 0x3) << ((3 - i) * 2);
			else if (i == 4)
				payload[33] |= (area->bitdepth & 0x3) << 6;
			else if (i == 5)
				payload[33] |= (area->bitdepth & 0x3) << 4;
			/* Color Setting */
			#define SET_COLOR_SEL(index, reg, shift) \
				if (i == index) \
					payload[reg] |= (area->color & 0x7) << shift;
			SET_COLOR_SEL(0, 34, 4);
			SET_COLOR_SEL(1, 34, 0);
			SET_COLOR_SEL(2, 35, 4);
			SET_COLOR_SEL(3, 35, 0);
			SET_COLOR_SEL(4, 36, 4);
			SET_COLOR_SEL(5, 36, 0);
			#undef SET_COLOR_SEL
			/* Area Gray Setting */
			payload[37 + i] = area->gray & 0xff;
		}
	}
	payload[43] = 0x00;

	rc = mipi_dsi_dcs_write(mipi_device, 0x81, payload, 43);
	pr_err("dsi_cmd aod_area[%x] updated \n", payload[0]);

	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		rc = dsi_display_clk_ctrl(display->dsi_clk_handle,
				DSI_CORE_CLK, DSI_CLK_OFF);
	}

	return 0;
}

static ssize_t oneplus_display_get_aod_area(struct device *dev,
                                struct device_attribute *attr, char *buf)
{
	struct dsi_display *display = get_main_display();
	int i, cnt = 0;

	if (!display || !display->panel || !display->panel->oneplus_priv.is_aod_ramless)
		return -EINVAL;

	mutex_lock(&display->display_lock);
	mutex_lock(&display->panel->panel_lock);

	cnt = snprintf(buf, PAGE_SIZE, "aod_area info:\n");
	for (i = 0; i < RAMLESS_AOD_AREA_NUM; i++) {
		struct aod_area *area = &oneplus_aod_area[i];

		if (area->enable) {
			cnt += snprintf(buf + cnt, PAGE_SIZE,
				"    area[%d]: [%dx%d]-[%dx%d]-%d-%d-%d-%d\n",
				cnt, area->x, area->y, area->w, area->h,
				area->color, area->bitdepth, area->mono, area->gray);
		}
	}

	cnt += snprintf(buf + cnt, PAGE_SIZE, "aod_area raw:\n");
	for (i = 0; i < RAMLESS_AOD_AREA_NUM; i++) {
		struct aod_area *area = &oneplus_aod_area[i];

		if (area->enable) {
			cnt += snprintf(buf + cnt, PAGE_SIZE,
				"%d %d %d %d %d %d %d %d",
				area->x, area->y, area->w, area->h,
				area->color, area->bitdepth, area->mono, area->gray);
		}
		cnt += snprintf(buf + cnt, PAGE_SIZE, ":");
	}
	cnt += snprintf(buf + cnt, PAGE_SIZE, "aod_area end\n");

	mutex_unlock(&display->panel->panel_lock);
	mutex_unlock(&display->display_lock);

	return cnt;
}

static ssize_t oneplus_display_set_aod_area(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count) {
	struct dsi_display *display = get_main_display();
	char *bufp = (char *)buf;
	char *token;
	int cnt = 0;

	if (!display || !display->panel) {
		pr_err("failed to find dsi display\n");
		return false;
	}

	mutex_lock(&display->display_lock);
	mutex_lock(&display->panel->panel_lock);

	memset(oneplus_aod_area, 0, sizeof(struct aod_area) * RAMLESS_AOD_AREA_NUM);

	while ((token = strsep(&bufp, ":")) != NULL) {
		struct aod_area *area = &oneplus_aod_area[cnt];
		if (!*token)
			continue;

		sscanf(token, "%d %d %d %d %d %d %d %d",
			&area->x, &area->y, &area->w, &area->h,
			&area->color, &area->bitdepth, &area->mono, &area->gray);
		pr_err("aod_area[%d]: [%dx%d]-[%dx%d]-%d-%d-%d-%d\n",
			cnt, area->x, area->y, area->w, area->h,
			area->color, area->bitdepth, area->mono, area->gray);
		area->enable = true;
		cnt++;
	}
	oneplus_display_update_aod_area_unlock();
	mutex_unlock(&display->panel->panel_lock);
	mutex_unlock(&display->display_lock);

	return count;
}

static ssize_t oneplus_display_get_video(struct device *dev,
                                struct device_attribute *attr, char *buf)
{
	struct dsi_display *display = get_main_display();
	bool is_aod_ramless = false;

	if (display && display->panel && display->panel->oneplus_priv.is_aod_ramless)
		is_aod_ramless = true;

	return sprintf(buf, "%d\n", is_aod_ramless);
}

static ssize_t oneplus_display_set_video(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count) {
	struct dsi_display *display = get_main_display();
	struct drm_device *drm_dev = display->drm_dev;
	struct drm_connector *dsi_connector = display->drm_conn;
	struct drm_mode_config *mode_config = &drm_dev->mode_config;
	struct msm_drm_private *priv = drm_dev->dev_private;
	struct drm_display_mode *mode;
	struct drm_atomic_state *state;
	struct drm_crtc_state *crtc_state;
	struct drm_crtc *crtc;
	bool mode_changed = false;
	int mode_id = 0;
	int vblank_get = -EINVAL;
	int err = 0;
	int i;

	if (!display || !display->panel) {
		pr_err("failed to find dsi display\n");
		return -EINVAL;
	}

	if (!display->panel->oneplus_priv.is_aod_ramless)
		return count;

	if (!dsi_connector || !dsi_connector->state || !dsi_connector->state->crtc) {
		pr_err("[%s]: display not ready\n", __func__);
		return count;
	}

	sscanf(buf, "%du", &mode_id);
	pr_err("setting display mode %d\n", mode_id);

	vblank_get = drm_crtc_vblank_get(dsi_connector->state->crtc);
	if (vblank_get) {
		pr_err("failed to get crtc vblank\n", vblank_get);
	}

	drm_modeset_lock_all(drm_dev);

	if (oneplus_display_mode != 1)
		display->panel->dyn_clk_caps.dyn_clk_support = false;

	state = drm_atomic_state_alloc(drm_dev);
	if (!state)
		goto error;

	oneplus_display_mode = mode_id;
	state->acquire_ctx = mode_config->acquire_ctx;
	crtc = dsi_connector->state->crtc;
	crtc_state = drm_atomic_get_crtc_state(state, crtc);

	{
		struct drm_display_mode *set_mode = NULL;
		struct drm_display_mode *cmd_mode = NULL;
		struct drm_display_mode *vid_mode = NULL;

		list_for_each_entry(mode, &dsi_connector->modes, head) {
			if (drm_mode_vrefresh(mode) == 0)
				continue;
			if (mode->flags & DRM_MODE_FLAG_VID_MODE_PANEL)
				vid_mode = mode;
			if (mode->flags & DRM_MODE_FLAG_CMD_MODE_PANEL)
				cmd_mode = mode;
		}

		set_mode = oneplus_display_mode ? vid_mode : cmd_mode;
		set_mode = oneplus_onscreenfp_status ? vid_mode : set_mode;

		if (set_mode && drm_mode_vrefresh(set_mode) != drm_mode_vrefresh(&crtc_state->mode)) {
			mode_changed = true;
		} else {
			mode_changed = false;
		}

		if (mode_changed) {
			for (i = 0; i < priv->num_crtcs; i++) {
				if (priv->disp_thread[i].crtc_id == crtc->base.id) {
					if (priv->disp_thread[i].thread)
						kthread_flush_worker(&priv->disp_thread[i].worker);
				}
			}

			display->panel->dyn_clk_caps.dyn_clk_support = false;
			drm_atomic_set_mode_for_crtc(crtc_state, set_mode);
		}
		wake_up(&oneplus_aod_wait);
	}
	err = drm_atomic_commit(state);
	drm_atomic_state_put(state);

	if (mode_changed) {
		for (i = 0; i < priv->num_crtcs; i++) {
			if (priv->disp_thread[i].crtc_id == crtc->base.id) {
				if (priv->disp_thread[i].thread)
					kthread_flush_worker(&priv->disp_thread[i].worker);
			}
		}
	}

	if (oneplus_display_mode == 1)
		display->panel->dyn_clk_caps.dyn_clk_support = true;

error:
	drm_modeset_unlock_all(drm_dev);
	if (!vblank_get)
		drm_crtc_vblank_put(dsi_connector->state->crtc);

	return count;
}

int dsi_display_read_panel_reg_unlock(struct dsi_display *display,u8 cmd, void *data, size_t len)
{
	struct dsi_display_ctrl *ctrl;
	struct dsi_cmd_desc cmdsreq;
	int rc = 0;
	u32 flags = 0;

	if (!display || !display->panel || data == NULL) {
		pr_err("%s, Invalid params\n", __func__);
		return -EINVAL;
	}

	ctrl = &display->ctrl[display->cmd_master_idx];

	if (display->tx_cmd_buf == NULL) {
		rc = dsi_host_alloc_cmd_tx_buffer(display);
		if (rc) {
			pr_err("%s, failed to allocate cmd tx buffer memory\n", __func__);
			goto done;
		}
	}

	rc = dsi_display_cmd_engine_enable(display);
	if (rc) {
		pr_err("%s, cmd engine enable failed\n", __func__);
		goto done;
	}

	/* enable the clk vote for CMD mode panels */
	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		dsi_display_clk_ctrl(display->dsi_clk_handle,
			DSI_ALL_CLKS, DSI_CLK_ON);
	}

	memset(&cmdsreq, 0x0, sizeof(cmdsreq));
	cmdsreq.msg.type = 0x06;
	cmdsreq.msg.tx_buf = &cmd;
	cmdsreq.msg.tx_len = 1;
	cmdsreq.msg.rx_buf = data;
	cmdsreq.msg.rx_len = len;
	cmdsreq.msg.flags |= MIPI_DSI_MSG_LASTCOMMAND;
	flags |= (DSI_CTRL_CMD_FETCH_MEMORY | DSI_CTRL_CMD_READ |
		DSI_CTRL_CMD_CUSTOM_DMA_SCHED |
		DSI_CTRL_CMD_LAST_COMMAND);

	rc = dsi_ctrl_cmd_transfer(ctrl->ctrl, &cmdsreq.msg, &flags);
	if (rc <= 0) {
		pr_err("%s, dsi_display_read_panel_reg rx cmd transfer failed rc=%d\n",
			__func__,
			rc);
	}

	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		rc = dsi_display_clk_ctrl(display->dsi_clk_handle,
					  DSI_ALL_CLKS, DSI_CLK_OFF);
	}

	dsi_display_cmd_engine_disable(display);
done:
	return rc;
}

int oneplus_check_aod_gamma_data_status(struct dsi_panel *panel)
{
	struct dsi_display *display = get_main_display();
	unsigned char read[30];
	int ret = -EINVAL;

	if (oneplus_aod_gamma_check_data >= 0)
		return oneplus_aod_gamma_check_data;

	{
		char value[] = { 0x5A, 0x5A };
		ret = mipi_dsi_dcs_write(&display->panel->mipi_device, 0xF0, value, sizeof(value));
	}
	{
		char value[] = { 0x69 };
		ret = mipi_dsi_dcs_write(&display->panel->mipi_device, 0xB0, value, sizeof(value));
	}
	ret = dsi_display_read_panel_reg_unlock(display, 0xBA, read, 1);
	if (ret < 0)
		pr_err("failed to read panel_reg ret=%d\n", ret);
	else
		oneplus_aod_gamma_check_data = read[0];

	pr_err("%s: read aod gamma check data: %02x\n", __func__, oneplus_aod_gamma_check_data);
	{
		char value[] = { 0xA5, 0xA5 };
		ret = mipi_dsi_dcs_write(&display->panel->mipi_device, 0xF0, value, sizeof(value));
	}

	return oneplus_aod_gamma_check_data;
}

int oneplus_set_aod_gamma_data_status(struct dsi_panel *panel)
{
	struct dsi_display *display = get_main_display();
	int check_data = 0;
	int ret = 0;

	if (strcmp(panel->oneplus_priv.vendor_name,"AMB655UV01"))
		return 0;

	check_data = oneplus_check_aod_gamma_data_status(panel);
	if (check_data < 0 || check_data >= 10)
		return 0;

	{
		char value[] = { 0x5A, 0x5A };
		ret = mipi_dsi_dcs_write(&display->panel->mipi_device, 0xF0, value, sizeof(value));
	}
	{
		char value[] = { 0x4D };
		ret = mipi_dsi_dcs_write(&display->panel->mipi_device, 0xB0, value, sizeof(value));
	}
	{
		char value[] = { 0x07, 0x71, 0xf8, 0xA3, 0x09, 0xA2, 0x70,
			0xBC, 0x0C, 0xD3, 0x04, 0xF3, 0x0E, 0xD3, 0x59, 0x17,
			0x11, 0x13, 0xC9, 0x41, 0x14, 0x24, 0x89, 0x79, 0x18,
			0x05, 0x65, 0xBE, 0x1A, 0x05, 0xE1, 0xE2};
		ret = mipi_dsi_dcs_write(&display->panel->mipi_device, 0xBA, value, sizeof(value));
	}
	{
		char value[] = { 0x03 };
		ret = mipi_dsi_dcs_write(&display->panel->mipi_device, 0xF7, value, sizeof(value));
	}
	{
		char value[] = { 0xA5, 0xA5 };
		ret = mipi_dsi_dcs_write(&display->panel->mipi_device, 0xF0, value, sizeof(value));
	}
	pr_err("resetting aod gamma table\n");

	return 0;
}

static struct kobject *oneplus_display_kobj;

static DEVICE_ATTR(aod, S_IRUGO|S_IWUSR, NULL, oneplus_display_set_aod);
static DEVICE_ATTR(hbm, S_IRUGO|S_IWUSR, oneplus_display_get_hbm, oneplus_display_set_hbm);
static DEVICE_ATTR(audio_ready, S_IRUGO|S_IWUSR, NULL, oneplus_display_set_audio_ready);
static DEVICE_ATTR(seed, S_IRUGO|S_IWUSR, oneplus_display_get_seed, oneplus_display_set_seed);
static DEVICE_ATTR(panel_serial_number, S_IRUGO|S_IWUSR, oneplus_display_get_panel_serial_number, NULL);
static DEVICE_ATTR(dump_info, S_IRUGO|S_IWUSR, oneplus_display_dump_info, NULL);
static DEVICE_ATTR(panel_dsc, S_IRUGO|S_IWUSR, oneplus_display_get_panel_dsc, NULL);
static DEVICE_ATTR(power_status, S_IRUGO|S_IWUSR, oneplus_display_get_power_status, oneplus_display_set_power_status);
static DEVICE_ATTR(display_regulator_control, S_IRUGO|S_IWUSR, NULL, oneplus_display_regulator_control);
static DEVICE_ATTR(panel_id, S_IRUGO|S_IWUSR, oneplus_display_get_panel_id, NULL);
static DEVICE_ATTR(sau_closebl_node, S_IRUGO|S_IWUSR, oneplus_display_get_closebl_flag, oneplus_display_set_closebl_flag);
static DEVICE_ATTR(write_panel_reg, S_IRUGO|S_IWUSR, oneplus_display_get_panel_reg, oneplus_display_set_panel_reg);
static DEVICE_ATTR(dsi_cmd, S_IRUGO|S_IWUSR, oneplus_display_get_dsi_command, oneplus_display_set_dsi_command);
static DEVICE_ATTR(dim_alpha, S_IRUGO|S_IWUSR, oneplus_display_get_dim_alpha, oneplus_display_set_dim_alpha);
static DEVICE_ATTR(dim_dc_alpha, S_IRUGO|S_IWUSR, oneplus_display_get_dc_dim_alpha, oneplus_display_set_dim_alpha);
static DEVICE_ATTR(dimlayer_hbm, S_IRUGO|S_IWUSR, oneplus_display_get_dimlayer_hbm, oneplus_display_set_dimlayer_hbm);
static DEVICE_ATTR(dimlayer_bl_en, S_IRUGO|S_IWUSR, oneplus_display_get_dimlayer_enable, oneplus_display_set_dimlayer_enable);
static DEVICE_ATTR(dimlayer_set_bl, S_IRUGO|S_IWUSR, oneplus_display_get_dimlayer_backlight, oneplus_display_set_dimlayer_backlight);
static DEVICE_ATTR(debug, S_IRUGO|S_IWUSR, oneplus_display_get_debug, oneplus_display_set_debug);
static DEVICE_ATTR(force_screenfp, S_IRUGO|S_IWUSR, oneplus_display_get_forcescreenfp, oneplus_display_set_forcescreenfp);
static DEVICE_ATTR(esd_status, S_IRUGO|S_IWUSR, oneplus_display_get_esd_status, oneplus_display_set_esd_status);
static DEVICE_ATTR(notify_panel_blank, S_IRUGO|S_IWUSR, NULL, oneplus_display_notify_panel_blank);
static DEVICE_ATTR(ffl_set, S_IRUGO|S_IWUSR, oneplus_get_ffl_setting, oneplus_set_ffl_setting);
static DEVICE_ATTR(notify_fppress, S_IRUGO|S_IWUSR, NULL, oneplus_display_notify_fp_press);
static DEVICE_ATTR(aod_light_mode_set, S_IRUGO|S_IWUSR, oneplus_get_aod_light_mode, oneplus_set_aod_light_mode);
static DEVICE_ATTR(serial_number, S_IRUGO|S_IWUSR, oneplus_display_get_serial_number, NULL);
static DEVICE_ATTR(spr, S_IRUGO|S_IWUSR, oneplus_display_get_spr, oneplus_display_set_spr);
static DEVICE_ATTR(roundcorner, S_IRUGO|S_IRUSR, oneplus_display_get_roundcorner, NULL);
static DEVICE_ATTR(dynamic_osc_clock, S_IRUGO|S_IWUSR, oneplus_display_get_dynamic_osc_clock, oneplus_display_set_dynamic_osc_clock);
static DEVICE_ATTR(max_brightness, S_IRUGO|S_IWUSR, oneplus_display_get_max_brightness, oneplus_display_set_max_brightness);
static DEVICE_ATTR(ccd_check, S_IRUGO|S_IRUSR, oneplus_display_get_ccd_check, NULL);
static DEVICE_ATTR(aod_area, S_IRUGO|S_IWUSR, oneplus_display_get_aod_area, oneplus_display_set_aod_area);
static DEVICE_ATTR(video, S_IRUGO|S_IWUSR, oneplus_display_get_video, oneplus_display_set_video);
static DEVICE_ATTR(failsafe, S_IRUGO|S_IWUSR, oneplus_display_get_failsafe, oneplus_display_set_failsafe);
static DEVICE_ATTR(mipi_clk_rate_hz, S_IRUGO|S_IWUSR, oneplus_display_get_mipi_clk_rate_hz, NULL);

DEVICE_ATTR(cabc, S_IRUGO|S_IWUSR, oneplus_display_get_cabc, oneplus_display_set_cabc);

/*
 * Create a group of attributes so that we can create and destroy them all
 * at once.
 */
static struct attribute *oneplus_display_attrs[] = {
	&dev_attr_aod.attr,
	&dev_attr_hbm.attr,
	&dev_attr_audio_ready.attr,
	&dev_attr_seed.attr,
	&dev_attr_panel_serial_number.attr,
	&dev_attr_dump_info.attr,
	&dev_attr_panel_dsc.attr,
	&dev_attr_power_status.attr,
	&dev_attr_display_regulator_control.attr,
	&dev_attr_panel_id.attr,
	&dev_attr_sau_closebl_node.attr,
	&dev_attr_write_panel_reg.attr,
	&dev_attr_dsi_cmd.attr,
	&dev_attr_dim_alpha.attr,
	&dev_attr_dim_dc_alpha.attr,
	&dev_attr_dimlayer_hbm.attr,
	&dev_attr_dimlayer_set_bl.attr,
	&dev_attr_dimlayer_bl_en.attr,
	&dev_attr_debug.attr,
	&dev_attr_force_screenfp.attr,
	&dev_attr_esd_status.attr,
	&dev_attr_notify_panel_blank.attr,
	&dev_attr_ffl_set.attr,
	&dev_attr_notify_fppress.attr,
	&dev_attr_aod_light_mode_set.attr,
	&dev_attr_serial_number.attr,
	&dev_attr_spr.attr,
	&dev_attr_roundcorner.attr,
	&dev_attr_dynamic_osc_clock.attr,
	&dev_attr_max_brightness.attr,
	&dev_attr_ccd_check.attr,
	&dev_attr_aod_area.attr,
	&dev_attr_video.attr,
	&dev_attr_failsafe.attr,
	&dev_attr_mipi_clk_rate_hz.attr,
	NULL,	/* need to NULL terminate the list of attributes */
};

static struct attribute_group oneplus_display_attr_group = {
	.attrs = oneplus_display_attrs,
};

/*
 * Create a new API to get display resolution
 */
int oneplus_display_get_resolution(unsigned int *xres, unsigned int *yres)
{
	*xres = *yres = 0;
	if (get_main_display() && get_main_display()->modes) {
		*xres = get_main_display()->modes->timing.v_active;
		*yres = get_main_display()->modes->timing.h_active;
	}
	return 0;
}
EXPORT_SYMBOL(oneplus_display_get_resolution);

static int __init oneplus_display_private_api_init(void)
{
	struct dsi_display *display = get_main_display();
	int retval;

	if (!display)
		return -EPROBE_DEFER;

	oneplus_display_kobj = kobject_create_and_add("oneplus_display", kernel_kobj);
	if (!oneplus_display_kobj)
		return -ENOMEM;

	/* Create the files associated with this kobject */
	retval = sysfs_create_group(oneplus_display_kobj, &oneplus_display_attr_group);
	if (retval)
		goto error_remove_kobj;

	retval = sysfs_create_link(oneplus_display_kobj,
				   &display->pdev->dev.kobj, "panel");
	if (retval)
		goto error_remove_sysfs_group;

	kthread_init_worker(&oneplus_ffl_worker);
	kthread_init_work(&oneplus_ffl_work, &oneplus_ffl_setting_thread);
	oneplus_ffl_thread = kthread_run(kthread_worker_fn,
				      &oneplus_ffl_worker, "oneplus_ffl");

	return 0;

error_remove_sysfs_group:
	sysfs_remove_group(oneplus_display_kobj, &oneplus_display_attr_group);
error_remove_kobj:
	kobject_put(oneplus_display_kobj);
	oneplus_display_kobj = NULL;
	return retval;
}

static void __exit oneplus_display_private_api_exit(void)
{
	if (oneplus_ffl_thread) {
		is_ffl_enable = FFL_EXIT_FULLY_CONTROL;
		kthread_flush_worker(&oneplus_ffl_worker);
		kthread_stop(oneplus_ffl_thread);
		oneplus_ffl_thread = NULL;
	}

	sysfs_remove_link(oneplus_display_kobj, "panel");
	sysfs_remove_group(oneplus_display_kobj, &oneplus_display_attr_group);
	kobject_put(oneplus_display_kobj);
}

module_init(oneplus_display_private_api_init);
module_exit(oneplus_display_private_api_exit);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Hujie");
