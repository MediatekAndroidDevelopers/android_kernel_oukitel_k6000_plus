#ifndef __HW_CTL_DRVIER__

#define __HW_CTL_DRVIER__

#define HWCTL_DBG

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/vmalloc.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/ctype.h>

#include <linux/semaphore.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/workqueue.h>
#include <linux/switch.h>
#include <linux/delay.h>

#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/kthread.h>
#include <linux/input.h>
#include <linux/wakelock.h>
#include <linux/time.h>
#include <linux/string.h>
#include <mach/irqs.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#define HW_DEVICE_MINOR	(0)
#define HW_DEVICE_COUNT	(1)
#define HW_DEVICE_NAME	"hwdctl"

#define HW_CTL_IO_TEST			_IO('H', 0x00)
#define HW_CTL_IO_ENB_KBD		_IO('H', 0x01)
#define HW_CTL_IO_DIS_KBD		_IO('H', 0x02)
#define HW_CTL_IO_EN_MSG_NTF	_IO('H', 0x03)
#define HW_CTL_IO_DIS_MSG_NTF	_IO('H', 0x04)
#define HW_CTL_IO_EN_CALL_NTF	_IO('H', 0x05)
#define HW_CTL_IO_DIS_CALL_NTF	_IO('H', 0x06)
#define HW_CTL_IO_EN_BAT_NTF	_IO('H', 0x07)
#define HW_CTL_IO_DIS_BAT_NTF	_IO('H', 0x08)
#define HW_CTL_IO_CHARGING_EN_NTF	_IO('H', 0x09)
#define HW_CTL_IO_CHARGING_DIS_NTF	_IO('H', 0x0A)
#define HW_CTL_IO_LEFT_HAND_NTF		_IO('H', 0x0B)/////added by liyunpen 20130219
#define HW_CTL_IO_RIGHT_HAND_NTF	_IO('H', 0x0C)/////added by liyunpen20130219
#define HW_CTL_IO_CHARGING_FULL_EN_NTF	_IO('H', 0x0D)
#define HW_CTL_IO_CHARGING_FULL_DIS_NTF	_IO('H', 0x0E)

#define HW_CTL_IO_FORCE_REFRESH_LEDS	_IO('H', 0xF0)

typedef enum{
	DEV_NONE_STAGE = 0x0,
	DEV_ALLOC_REGION,
	DEV_ALLOC_CDEV,	
	DEV_ADD_CDEV,
	DEV_ALLOC_CLASS,
	DEV_INIT_ALL
}HWDEV_INIT_STAGE;

typedef struct __HW_DEVICE{
	dev_t hwctl_dev_no;
	struct cdev * hw_cdev;
	struct class *hw_class;
	struct device *hw_device;
	char init_stage;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif				/* CONFIG_HAS_EARLYSUSPEND */	
}HW_DEVICE , * p_HW_DEVICE;
#endif
