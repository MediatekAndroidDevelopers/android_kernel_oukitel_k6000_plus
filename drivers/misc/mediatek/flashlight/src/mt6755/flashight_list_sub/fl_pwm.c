
#include <linux/platform_device.h>
#include "flashlight_list.h"
#include <mt-plat/mt_pwm.h>
#include <linux/delay.h>
//#include <linux/hct_include/hct_project_all_config.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include "kd_flashlight.h"
#include "kd_flashlight_type.h"

//#include <linux/gpio.h>
//#include <linux/gpio/consumer.h>
#define PK_DBG(fmt, arg...)    pr_debug("%s: "fmt, __func__ , ##arg)


extern struct pinctrl *flashlightpinsctrl ;
extern struct pinctrl_state *flashlight_sub_as_pwm;
extern struct pinctrl_state *flashlight_sub_pwm_l;
extern struct pinctrl_state *flashlight_sub_pwm_h;

static kal_bool active_flag = KAL_FALSE;
//static struct gpio_desc *fl_sub_en_gpio = NULL;
static int pwm_no = -1;
static int flash_duty = -1;
static int torch_duty = -1;

static void flashlight_sub_gpio_mode_pwm_set(void){
	if(IS_ERR(flashlight_sub_as_pwm)){
	        pr_err("err: flashlight_sub_as_pwm is error!!!");
		return;
	}
	pinctrl_select_state(flashlightpinsctrl, flashlight_sub_as_pwm);
}

static int flashlight_sub_gpio_output_set(int level){
	if(IS_ERR(flashlight_sub_pwm_l)||IS_ERR(flashlight_sub_pwm_h))
	{
		pr_err("err: flashlight_sub_pwm_l or flashlight_sub_pwm_h is error!!!");
		return -1;
	} 
	pinctrl_select_state(flashlightpinsctrl, level ? flashlight_sub_pwm_h : flashlight_sub_pwm_l);

	return 0;
}

static S32 flashlight_sub_set_flash_mode(int duty){
	S32 ret = 0;

	if(duty == 10){
		flashlight_sub_gpio_output_set(1);
	}else
	{
		flashlight_sub_gpio_mode_pwm_set();
		ret = flashlight_gpio_pwm_start(pwm_no,duty);
	}
	return ret;
}

static	int flashlight_sub_pwm_init(struct platform_device *dev,struct device_node * node){
	int ret = 0;
	int sub_en_gpio = -1;

	of_property_read_u32(node, "fl-sub-en-gpio", &sub_en_gpio);
	of_property_read_u32(node, "fl-sub-flash-duty", &flash_duty);
	of_property_read_u32(node, "fl-sub-torch-duty", &torch_duty);	
	pwm_no = flashlight_gpio_as_get_pwm_no(sub_en_gpio);

	PK_DBG("sub_en_gpio:%d,pwm_no:%d,flash_duty:%d,torch_duty:%d\n",sub_en_gpio,pwm_no,flash_duty,torch_duty);
	if(sub_en_gpio == -1){
		PK_DBG("get sub_en_gpio error\n");
		ret = -1;
	}

	return ret;
}

static	int fl_enable(int g_duty){
	int ret = 0;
	if(active_flag == KAL_TRUE)
		return 0;
	if(g_duty){
		ret = flashlight_sub_set_flash_mode(flash_duty);
	}else{
		ret = flashlight_sub_set_flash_mode(torch_duty);
	}
	active_flag = KAL_TRUE;
	return ret;
}

static	int fl_disable(void){
	int ret = 0;
	if(active_flag == KAL_FALSE)
		return 0;
	ret = flashlight_gpio_pwm_end(pwm_no);
	ret = flashlight_sub_gpio_output_set(0);
	active_flag = KAL_FALSE;
	return ret;
}


flashlight_list_t flashlight_list_sub = {
	.fl_init = flashlight_sub_pwm_init,
	.fl_enable = fl_enable,
	.fl_disable = fl_disable,
	.fl_name = "fl_pwm",
};
