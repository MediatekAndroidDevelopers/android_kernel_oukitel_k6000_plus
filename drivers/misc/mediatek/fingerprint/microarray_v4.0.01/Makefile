# Copyright (C) Microarray
# MicroArray Fprint Driver Code
# MAKEFILE
# Date: 2016-12-18
# Version: v4.0.04 
# Author: guq
# Contact: guq@microarray.com.cn


subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include \
					-I$(srctree)/drivers/misc/mediatek/include/mt-plat

MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
subdir-ccflags-y += -Werror
subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/include
subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include
subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat
subdir-ccflags-y += -I$(srctree)/drivers/spi/mediatek/$(MTK_PLATFORM)
subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/tkcore/core/
#ifeq ($(CONFIG_FINGERPRINT_MICROARRAY),y)
obj-$(CONFIG_FINGERPRINT_MICROARRAY)		+= mtk-settings.o madev.o
#else ifeq ($(CONFIG_FINGERPRINT_MICROARRAY_A80T),y)
obj-$(CONFIG_FINGERPRINT_MICROARRAY_A80T)		+= mtk-settings.o madev.o
#else ifeq ($(CONFIG_FINGERPRINT_MICROARRAY_A80T_TEE),y)
obj-$(CONFIG_FINGERPRINT_MICROARRAY_A80T_TEE)		+= mtk-settings.o madev_tee.o
#else ifeq ($(CONFIG_FINGERPRINT_MICROARRAY_TEE),y)
obj-$(CONFIG_FINGERPRINT_MICROARRAY_TEE)	+= mtk-settings.o madev_tee.o
#endif


