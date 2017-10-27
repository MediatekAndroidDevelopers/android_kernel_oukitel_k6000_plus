obj-y += bluewhale_charger.o

obj-$(CONFIG_MEDIATEK_SOLUTION) += charging_hw_oz115.o
ifeq ($(CONFIG_OZ115_DUAL_SUPPORT),y)
	obj-$(CONFIG_MTK_SMART_BATTERY) += oz115_sub.o
endif
