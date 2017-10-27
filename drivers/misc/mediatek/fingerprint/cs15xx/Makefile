#include $(srctree)/drivers/misc/mediatek/Makefile.custom
# Makefile for the fingerprint device.

obj-y	+= cs15xx-spi.o

# subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/include/mt-plat/

ldflags-y += --whole-archive $(ROOTDIR)/kernel-3.18/drivers/misc/mediatek/fingerprint/cs15xx/cs15xx_nav.a  --no-whole-archive
