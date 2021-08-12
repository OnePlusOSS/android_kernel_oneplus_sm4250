# oem device driver
ifneq ($(filter billie%, $(OEM_TARGET_PRODUCT)),)
obj-$(CONFIG_OP_HIMAX) += touchscreen_himax/
else
obj-$(CONFIG_TOUCHPANEL_ONEPLUS) += touchscreen/
obj-$(CONFIG_OP_TOUCHSCREEN) += touchscreen/
endif
obj-$(CONFIG_INPUT_FINGERPRINT) += fingerprint/
