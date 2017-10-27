#include <linux/platform_device.h>
#include <linux/of.h>
#include "flashlight_list.h"
#include <linux/delay.h>
#include <linux/gpio.h>
//#include <linux/hct_include/hct_project_all_config.h>
#include <linux/gpio/consumer.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include "kd_flashlight_type.h"

#define PK_DBG(fmt, arg...)    pr_debug("%s: "fmt, __func__ , ##arg)

static kal_bool active_flag = KAL_FALSE;
static struct gpio_desc *fl_en_gpio = NULL;
static struct gpio_desc *fl_mode_gpio = NULL;
extern const struct of_device_id FLASHLIGHT_of_match[];

static	int aw3641e_init(struct platform_device *dev,struct device_node * node){

	int ret = 0;
	int gpio_en_num = -1;
	int gpio_mode_num = -1;
//	struct device_node *node1 = NULL;
//	node1 = of_find_matching_node(node1, FLASHLIGHT_of_match);
	of_property_read_u32(node, "fl-en-gpio", &gpio_en_num);
	of_property_read_u32(node, "fl-mode-gpio", &gpio_mode_num);
	PK_DBG("gpio_en_num:%d,fl-mode-gpio:%d\n",gpio_en_num,gpio_mode_num);

	if(gpio_en_num < 0){
		PK_DBG("gpio_en_num get failed\n");
		ret = -1;
	}else{
		fl_en_gpio = gpio_to_desc(gpio_en_num);
		if(IS_ERR(fl_en_gpio)){
			PK_DBG("gpio_to_desc(gpio_en_num:%d)  error!\n",gpio_en_num);
			ret = -1;
		}else{
			ret = gpiod_direction_output(fl_en_gpio, 1);
			gpiod_set_value(fl_en_gpio, 0);
		}
	}
	if(gpio_mode_num < 0){
		PK_DBG("gpio_mode_num get failed\n");
		ret = -1;
	}else{
		fl_mode_gpio = gpio_to_desc(gpio_mode_num);
		if(IS_ERR(fl_mode_gpio)){
			PK_DBG("gpio_to_desc(gpio_mode_num:%d)  error!\n",gpio_mode_num);
			ret = -1;
		}else{
			ret = gpiod_direction_output(fl_mode_gpio, 1);
			gpiod_set_value(fl_mode_gpio, 0);
		}
	}
	return 0;
}

static inline void aw3641e_set_flash_mode(int en){
	int i;
	if(active_flag == KAL_TRUE)
		return;
	gpiod_set_value(fl_mode_gpio, 1);
	for(i=0; i<en; i++)
	{
		gpiod_set_value(fl_en_gpio, 0);
		udelay(2);
		gpiod_set_value(fl_en_gpio, 1);
		udelay(2);
	}
	active_flag = KAL_TRUE;
}

static inline void aw3641e_set_torch_mode(void){
	if(active_flag == KAL_TRUE)
		return;
	gpiod_set_value(fl_mode_gpio, 0);
	gpiod_set_value(fl_en_gpio, 1);
	active_flag = KAL_TRUE;
}

static	int fl_enable(int g_duty){
	if(IS_ERR(fl_mode_gpio)||IS_ERR(fl_en_gpio)){
		PK_DBG("fl_mode_gpio or fl_en_gpio is error!!!\n");
		return -1;
	}
	if(g_duty){
		aw3641e_set_flash_mode(9);
	}else{
		aw3641e_set_torch_mode();
	}
	return 0;
}

static	int fl_disable(void){
	if(active_flag == KAL_FALSE)
		return -1;
	if(IS_ERR(fl_mode_gpio)||IS_ERR(fl_en_gpio)){
		PK_DBG("fl_mode_gpio or fl_en_gpio is error!!!\n");
		return -1;
	}
	gpiod_set_value(fl_en_gpio, 0);
	gpiod_set_value(fl_mode_gpio, 0);
	mdelay(1);
	active_flag = KAL_FALSE;
	return 0;
}


flashlight_list_t flashlight_list = {
	.fl_init = aw3641e_init,
	.fl_enable = fl_enable,
	.fl_disable = fl_disable,
	.fl_name = "3641e",
};
