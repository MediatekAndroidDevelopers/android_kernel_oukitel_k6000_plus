
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/hct_include/hct_project_all_config.h>
#include <linux/gpio/consumer.h>
#include <linux/kernel.h>
#include "kd_flashlight_type.h"
#include "flashlight_list.h"

#define PK_DBG(fmt, arg...)    pr_debug(CONFIG_HCT_FLASHLIGHT_SUB"%s: "fmt, __func__ , ##arg)

struct gpio_desc *fl_sub_en_gpio = NULL;

#ifndef __HCT_GPIO_FLASHLIGHT_SUB_EN_NUM__
	#error "__HCT_GPIO_FLASHLIGHT_SUB_EN_NUM__ is not defined in hct_board_config.h"
#endif
static kal_bool active_flag = KAL_FALSE;
static int fl_init(struct platform_device *dev,struct device_node * node){

	int ret = 0;
	fl_sub_en_gpio = gpio_to_desc(__HCT_GPIO_FLASHLIGHT_SUB_EN_NUM__);

	ret = gpiod_direction_output(fl_sub_en_gpio, 1);
	if(ret)
		PK_DBG("flashlight gpiod_direction_output failed,gpio_num: %d\n",__HCT_GPIO_FLASHLIGHT_SUB_EN_NUM__);
	ret = gpiod_direction_output(fl_sub_en_gpio, 1);
	gpiod_set_value(fl_sub_en_gpio, 0);
	return 0;
}

static int fl_enable(int g_duty){
	if(active_flag == KAL_TRUE)
		return 0;
	if(g_duty){
		gpiod_set_value(fl_sub_en_gpio, 1);
	}else{
		gpiod_set_value(fl_sub_en_gpio, 1);
	}
	active_flag = KAL_TRUE;
	return 0;
}

static int fl_disable(void){
	if(active_flag == KAL_FALSE)
		return -1;
	gpiod_set_value(fl_sub_en_gpio, 0);
	mdelay(1);
	active_flag = KAL_FALSE;
	return 0;
}


flashlight_list_t flashlight_list_sub = {
	.fl_init = fl_init,
	.fl_enable = fl_enable,
	.fl_disable = fl_disable,
	.fl_name = "fl_gpio",
};
