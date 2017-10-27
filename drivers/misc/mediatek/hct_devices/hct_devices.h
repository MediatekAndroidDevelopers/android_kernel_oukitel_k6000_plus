
#include <hwmsen_dev.h>
#include <sensors_io.h>
#include <asm/io.h>
#include <hwmsen_helper.h>
#include <linux/list.h>
#include <linux/seq_file.h>

//#include <mach/mt_gpio.h>
#include <hwmsensor.h>

#include "lcm_drv.h"
#include "kd_imgsensor_define.h"
#include "tpd.h"
#include "accel.h"
#include "alsps.h"
#include "mag.h"

#define ID_LCM_TYPE       		MAX_ANDROID_SENSOR_NUM    	// 11
#define ID_CAMERA_TYPE       		(MAX_ANDROID_SENSOR_NUM + 1)   	// 12
#define ID_TOUCH_TYPE         		(MAX_ANDROID_SENSOR_NUM + 2)   	// 13
#define ID_ACCSENSOR_TYPE       	(MAX_ANDROID_SENSOR_NUM + 3)   	// 14
#define ID_MSENSOR_TYPE         	(MAX_ANDROID_SENSOR_NUM + 4)   	// 15
#define ID_ALSPSSENSOR_TYPE     	(MAX_ANDROID_SENSOR_NUM + 5)   	// 16

typedef enum 
{ 
    DEVICE_SUPPORTED = 0,        
    DEVICE_USED = 1,
}campatible_type;



extern int hct_set_touch_device_used(char * module_name, int pdata);
extern int hct_touchpanel_device_add(struct tpd_driver_t* mTouch, campatible_type isUsed);

extern int hct_set_camera_device_used(char * module_name, int pdata);
extern int hct_camera_device_add(ACDK_KD_SENSOR_INIT_FUNCTION_STRUCT* mCamera, campatible_type isUsed);

extern int hct_set_accsensor_device_used(char * module_name, int pdata);
extern int hct_accsensor_device_add(struct acc_init_info* maccsensor, campatible_type isUsed);

extern int hct_set_msensor_device_used(char * module_name, int pdata);
extern int hct_msensor_device_add(struct mag_init_info* mmsensor, campatible_type isUsed);

extern int hct_set_alspssensor_device_used(char * module_name, int pdata);
extern int hct_alspssensor_device_add(struct alsps_init_info* malspssensor, campatible_type isUsed);

