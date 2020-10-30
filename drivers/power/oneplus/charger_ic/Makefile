ifeq ($(CONFIG_ONEPLUS_CHARGER_MTK),y)

	ifeq ($(CONFIG_MTK_HAFG_20),y)
		obj-y	+= oneplus_battery_mtk_fg_20.o
	else ifeq ($(CONFIG_ONEPLUS_CHARGER_MTK6763),y)
		obj-y	+= oneplus_battery_mtk6763.o
	else ifeq ($(CONFIG_ONEPLUS_CHARGER_MTK6771),y)
		obj-y	+= oneplus_bq24190.o
		obj-y	+= oneplus_bq25890h.o
		obj-y   += oneplus_bq25601d.o
		obj-y   += smb1351-charger.o
	else ifeq ($(CONFIG_ONEPLUS_CHARGER_MTK6779),y)
		MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
		subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include
		subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/pmic/include
		subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/typec/tcpc/inc
		subdir-ccflags-y += -I$(srctree)/drivers/power/supply/mediatek/misc/
		obj-y	+= oneplus_battery_mtk6779.o
	else
		obj-y	+= oneplus_battery_mtk.o
	endif
	//obj-y	+= oneplus_bq24196.o

else ifeq ($(CONFIG_ONEPLUS_MSM8953N_CHARGER),y)
	obj-y	+= oneplus_battery_msm8953_N.o
else ifeq ($(CONFIG_ONEPLUS_MSM8953_CHARGER),y)
	obj-y	+= oneplus_battery_msm8953.o
else ifeq ($(CONFIG_ONEPLUS_MSM8998_CHARGER),y)
	obj-y	+= oneplus_battery_msm8998.o
else ifeq ($(CONFIG_ONEPLUS_MSM8998O_CHARGER),y)
	obj-y	+= oneplus_battery_msm8998_O.o
else ifeq ($(CONFIG_ONEPLUS_SDM845_CHARGER),y)
	obj-y	+= oneplus_battery_sdm845.o
	obj-y	+= oneplus_bq25882.o oneplus_da9313.o
else ifeq ($(CONFIG_ONEPLUS_SDM670_CHARGER),y)
	obj-y	+= oneplus_battery_sdm670.o
	obj-y	+= oneplus_bq25882.o oneplus_da9313.o
else ifeq ($(CONFIG_ONEPLUS_SDM670P_CHARGER),y)
	obj-y	+= oneplus_battery_sdm670P.o
	obj-y	+= oneplus_bq25882.o oneplus_da9313.o
else ifeq ($(CONFIG_ONEPLUS_SDM670Q_CHARGER),y)
	obj-y	+= oneplus_battery_sdm670Q.o
	obj-y	+= oneplus_bq25882.o oneplus_da9313.o
else ifeq ($(CONFIG_ONEPLUS_SM8150_CHARGER),y)
	obj-y	+= oneplus_bq25882.o
	obj-y	+= oneplus_da9313.o
	obj-y	+= oneplus_battery_msm8150.o
else ifeq ($(CONFIG_ONEPLUS_SM8150_PRO_CHARGER),y)
	obj-y	+= oneplus_mp2650.o
	obj-y	+= oneplus_da9313.o
	obj-y	+= oneplus_battery_msm8150_pro.o
else ifeq ($(CONFIG_ONEPLUS_SM6125_CHARGER),y)
	obj-y   += oneplus_battery_sm6125.o
else ifeq ($(CONFIG_ONEPLUS_PMI632_CHARGER),y)
	obj-y   += oneplus_battery_pmi632.o
else ifeq ($(CONFIG_ONEPLUS_SM7150_CHARGER),y)
	obj-y   += oneplus_battery_sm7150_P.o
else
	obj-y	+= oneplus_battery_msm8976.o
endif 

	obj-y   += oneplus_short_ic.o
