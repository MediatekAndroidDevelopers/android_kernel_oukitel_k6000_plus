#ifndef  __HCT_DEVICES_IOCTL_H__
#define  __HCT_DEVICES_IOCTL_H__

#include <linux/ioctl.h>

#define HCT_IOC_MAGIC 0xCC

#define HCTIOC_GET_EXTAMP_SOUND_MODE        _IOR(HCT_IOC_MAGIC, 0x01, uint32_t)
#define HCTIOC_SET_EXTAMP_SOUND_MODE        _IOW(HCT_IOC_MAGIC, 0x02, uint32_t)
#define HCTIOC_GET_ACCDET_MIC_MODE          _IOR(HCT_IOC_MAGIC, 0x03, uint32_t)
#define HCTIOC_SET_ACCDET_MIC_MODE          _IOW(HCT_IOC_MAGIC, 0x04, uint32_t)
#define HCTIOC_GET_ADC_VALUE		    _IOR(HCT_IOC_MAGIC, 0x05, uint32_t)

#define HCT_IOCQDIR            _IOR(HCT_IOC_MAGIC, 0x06, uint32_t)
#define HCT_IOCSDIRIN          _IOW(HCT_IOC_MAGIC, 0x07, uint32_t)
#define HCT_IOCSDIROUT         _IOW(HCT_IOC_MAGIC, 0x08, uint32_t)
#define HCT_IOCQPULLEN         _IOR(HCT_IOC_MAGIC, 0x09, uint32_t)
#define HCT_IOCSPULLENABLE     _IOW(HCT_IOC_MAGIC, 0x0A, uint32_t)
#define HCT_IOCSPULLDISABLE    _IOW(HCT_IOC_MAGIC, 0x0B, uint32_t)
#define HCT_IOCQPULL           _IOR(HCT_IOC_MAGIC, 0x0C, uint32_t)
#define HCT_IOCSPULLDOWN       _IOW(HCT_IOC_MAGIC, 0x0D, uint32_t)
#define HCT_IOCSPULLUP         _IOW(HCT_IOC_MAGIC, 0x0E, uint32_t)
#define HCT_IOCQINV            _IOR(HCT_IOC_MAGIC, 0x0F, uint32_t)
#define HCT_IOCSINVENABLE      _IOW(HCT_IOC_MAGIC, 0x10, uint32_t)
#define HCT_IOCSINVDISABLE     _IOW(HCT_IOC_MAGIC, 0x11, uint32_t)
#define HCT_IOCQDATAIN         _IOR(HCT_IOC_MAGIC, 0x12, uint32_t)
#define HCT_IOCQDATAOUT        _IOR(HCT_IOC_MAGIC, 0x13, uint32_t)
#define HCT_IOCSDATALOW        _IOW(HCT_IOC_MAGIC, 0x14, uint32_t)
#define HCT_IOCSDATAHIGH       _IOW(HCT_IOC_MAGIC, 0x15, uint32_t)



#endif
