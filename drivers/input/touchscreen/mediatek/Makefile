subdir-ccflags-y += -Werror
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek
# In case the platform does NOT support this type of sensors

ifneq ($(CONFIG_TOUCHSCREEN_MTK),)
obj-y   +=  mtk_tpd.o tpd_button.o tpd_calibrate.o tpd_debug.o \
		tpd_default.o tpd_init.o tpd_misc.o tpd_setting.o
endif

obj-$(CONFIG_TOUCHSCREEN_MTK)   += tpd_common_probe.o

obj-$(CONFIG_TOUCHSCREEN_MTK_GT1151)	+=  GT1151/
obj-$(CONFIG_TOUCHSCREEN_MTK_GT1151TB)	+=  GT1151TB/
obj-$(CONFIG_TOUCHSCREEN_MTK_GT910)	+=  GT910/
obj-$(CONFIG_TOUCHSCREEN_MTK_GT9XXTB_HOTKNOT)	+=  GT9XXTB_hotknot/
obj-$(CONFIG_TOUCHSCREEN_MTK_GT9XX_HOTKNOT)	+=  GT9XX_hotknot/
obj-$(CONFIG_TOUCHSCREEN_MTK_SYNAPTICS_I2C_RMI4)	+=  synaptics_i2c_rmi4/
obj-$(CONFIG_TOUCHSCREEN_MTK_GT928)	+=  GT928/
obj-$(CONFIG_TOUCHSCREEN_MTK_GT9XX_HOTKNOT_SCP)	+=  GT9XX_hotknot_scp/
obj-$(CONFIG_TOUCHSCREEN_MTK_GT911)	+=  GT911/
obj-$(CONFIG_TOUCHSCREEN_MTK_FT5X0X)	+=  ft5x0x/
obj-$(CONFIG_TOUCHSCREEN_MTK_SYNAPTICS_3320_50) +=  synaptics_3320_50/
obj-$(CONFIG_TOUCHSCREEN_MTK_GT9271TB_HOTKNOT) +=  GT9271TB_hotknot/
obj-$(CONFIG_TOUCHSCREEN_SYNAPTICS_3402)	+=  synaptics_3402/
obj-$(CONFIG_TOUCHSCREEN_MTK_FOCALTECH)	+=  focaltech/
obj-$(CONFIG_TOUCHSCREEN_MTK_GT9XX_V26)	+=  GT9XX_V26/
obj-$(CONFIG_TOUCHSCREEN_MTK_LU2X3X) += 	lu2x3x/
obj-$(CONFIG_TOUCHSCREEN_MTK_GT1X_V14)		+=  GT1X_V14/
obj-$(CONFIG_TOUCHSCREEN_MTK_MSG5846)	+=  msg5846/
obj-$(CONFIG_TOUCHSCREEN_HIMAX_CHIPSET)	+= hxchipset/
obj-$(CONFIG_TOUCHSCREEN_MTK_MSG28XX)	+=  msg28xx/
