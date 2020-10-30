ifeq ($(CONFIG_ONEPLUS_CHARGER_MTK),y)
subdir-ccflags-y += -Werror
subdir-ccflags-y += -Wno-unused-variable
endif

obj-y      += charger_ic/
obj-y      += gauge_ic/
obj-y      += warp_ic/
obj-y      += adapter_ic/

obj-y	+= oneplus_charger.o
obj-y	+= oneplus_gauge.o
obj-y	+= oneplus_warp.o
obj-y	+= oneplus_adapter.o
obj-y	+= oneplus_short.o
obj-y	+= oneplus_usb_cable_detector.o