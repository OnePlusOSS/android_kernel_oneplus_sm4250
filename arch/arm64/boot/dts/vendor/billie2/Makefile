ifeq ($(CONFIG_BUILD_ARM64_DT_OVERLAY),y)
        dtbo-$(CONFIG_ARCH_BENGAL) += \
		billie2-qrd-overlay.dtbo

billie2-qrd-overlay.dtbo-base := bengal.dtb
endif


always          := $(dtb-y)
subdir-y        := $(dts-dirs)
clean-files     := *.dtb *.dtbo

