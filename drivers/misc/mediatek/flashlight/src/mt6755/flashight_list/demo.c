
#include <linux/platform_device.h>
#include "flashlight_list.h"
#include <linux/delay.h>
//#include <linux/hct_board_config.h>
#include <linux/kernel.h>
#include "kd_flashlight_type.h"
//#include <linux/gpio.h>
//#include <linux/gpio/consumer.h>
#define PK_DBG(fmt, arg...)    pr_debug(CONFIG_HCT_FLASHLIGHT"%s: "fmt, __func__ , ##arg)

static kal_bool active_flag = KAL_FALSE;

static	int fl_init(struct platform_device *dev,struct device_node * node){


	return 0;
}


static	int fl_enable(int g_duty){
	if(active_flag == KAL_TRUE)
		return;
	if(g_duty){
	

	}else{
	

	}
	active_flag = KAL_TRUE;
	return 0;
}

static	int fl_disable(void){
	if(active_flag == KAL_TRUE)
		return;


	active_flag = KAL_TRUE;
	return 0;
}


flashlight_list_t flashlight_list = {
	.fl_init = fl_init,
	.fl_enable = fl_enable,
	.fl_disable = fl_disable,
	.fl_name = "fl_demo",
};
