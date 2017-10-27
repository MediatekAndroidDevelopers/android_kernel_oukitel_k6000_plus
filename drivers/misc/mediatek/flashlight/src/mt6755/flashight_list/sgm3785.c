
#include <linux/kernel.h>
#include <linux/types.h>

#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/errno.h>

#include <linux/of.h>
#include <linux/wakelock.h>//Hct drv zs add for torch
#include <linux/gpio.h>
#include <mt-plat/mt_pwm.h>
//#include <linux/hct_include/hct_project_all_config.h>
#include "kd_flashlight_type.h"
#include "kd_flashlight.h"
#include "flashlight_list.h"

#define PK_DBG(fmt, arg...)    pr_debug("%s: "fmt, __func__ , ##arg)
static kal_bool active_flag = KAL_FALSE;

static struct wake_lock sgm_pwm_lock;//Hct drv zs add for torch

static U16 sgm_flash_duty;
static U16 sgm_torch_duty;

extern struct pinctrl *flashlightpinsctrl;
extern struct pinctrl_state *flashlight_as_pwm;
extern struct pinctrl_state *flashlight_pwm_l;
extern struct pinctrl_state *flashlight_pwm_h;
extern const struct of_device_id FLASHLIGHT_of_match[];

static struct gpio_desc *fl_ENF_gpio = NULL;

static int pwm_no = -1;


static inline int sgm3785_ENF_gpio_en_output_set(int level){
	if (fl_ENF_gpio != NULL){
		gpiod_set_value(fl_ENF_gpio, level);
		return 0;
	}else{
		PK_DBG("error: fl_ENF_gpio is NULL!");
		return -1;
	}
}

static inline int sgm3785_ENM_gpio_mode_pwm_set(void){
	if(IS_ERR(flashlight_as_pwm)){
	        PK_DBG("err: flashlight_as_pwm is error!!!");
		return -1;
	}
	pinctrl_select_state(flashlightpinsctrl, flashlight_as_pwm);
	return 0;
}

static inline void sgm3785_ENM_gpio_pwm_output_set(int level){

	if(IS_ERR(flashlight_pwm_h)||IS_ERR(flashlight_pwm_l))
	{
	     PK_DBG("err: flashlight_pwm_h or flashlight_pwm_h is error!!!");
	     return;
	}
	pinctrl_select_state(flashlightpinsctrl, level ? flashlight_pwm_h : flashlight_pwm_l);

}


static int sgm3785_init(struct platform_device *pdev,struct device_node * node)
{
	int ret = 0;
	int gpio_ENF_gpio_num = -1;
	int gpio_ENM_gpio_num = -1;
	int flash_duty = -1;
	int torch_duty = -1;

	of_property_read_u32(node, "fl-ENF-gpio", &gpio_ENF_gpio_num);
	of_property_read_u32(node, "fl-ENM-gpio", &gpio_ENM_gpio_num);
	of_property_read_u32(node, "fl-flash-duty", &flash_duty);
	of_property_read_u32(node, "fl-torch-duty", &torch_duty);

    wake_lock_init(&sgm_pwm_lock, WAKE_LOCK_SUSPEND, "sgm pwm wakelock");//Hct drv zs add for torch
	
    pwm_no = flashlight_gpio_as_get_pwm_no(gpio_ENM_gpio_num);
	PK_DBG("fl-ENF-gpio:%d,fl-ENM-gpio:%d,sgm_flash_duty:%d,sgm_torch_duty:%d,pwm_no:%d\n",
		gpio_ENF_gpio_num,gpio_ENM_gpio_num,flash_duty,torch_duty,pwm_no);

	sgm_flash_duty= flash_duty;
	sgm_torch_duty= torch_duty;
	if(gpio_ENF_gpio_num >= 0){
		fl_ENF_gpio = gpio_to_desc(gpio_ENF_gpio_num);
		if(IS_ERR(fl_ENF_gpio)){
			PK_DBG("gpio_ENF_gpio_num get failed");
			ret = -1;
		}else{
			ret = gpiod_direction_output(fl_ENF_gpio, 1);
			gpiod_set_value(fl_ENF_gpio, 0);
		}
	}
	sgm3785_ENM_gpio_pwm_output_set(0);
	return ret;
}


static S32 sgm3785_set_flash_mode(U16 duty)
{	
	S32 ret = 0;
	PK_DBG("duty:%d\n",duty);

	if(active_flag == KAL_TRUE)
		return ret;

	sgm3785_ENF_gpio_en_output_set(0);
	if (duty == 10){
		sgm3785_ENM_gpio_pwm_output_set(1);	
	}else{
		sgm3785_ENM_gpio_mode_pwm_set();
		flashlight_gpio_pwm_start(pwm_no,duty);
	}

	sgm3785_ENF_gpio_en_output_set(1);

	active_flag = KAL_TRUE;
	return ret;
}

/*movie/torch mode
ENF: 1
          0 _____________________________________
ENM:1	     ______   _   _   _   _   _	
         0 _| >5ms |_| |_| |_| |_| |_| |________
*/	
static S32 sgm3785_set_torch_mode(U16 duty)
{
	S32 ret = 0;
	printk("%s,duty:%d\n",__func__,duty);
	if(active_flag == KAL_TRUE)
		return ret;
	if (duty == 10){
		sgm3785_ENM_gpio_pwm_output_set(1);	
	}else{
		sgm3785_ENF_gpio_en_output_set(0);

		sgm3785_ENM_gpio_pwm_output_set(0);
		mdelay(1);

		sgm3785_ENM_gpio_pwm_output_set(1);
		mdelay(6);

		sgm3785_ENM_gpio_mode_pwm_set();
		flashlight_gpio_pwm_start(pwm_no,duty);
	}
	active_flag = KAL_TRUE;
	return ret;
}

static S32 sgm3785_shutdown(void)
{
	S32 ret = 0;
	if(active_flag == KAL_FALSE)
		return ret;

	flashlight_gpio_pwm_end(pwm_no);

	sgm3785_ENF_gpio_en_output_set(0);
	sgm3785_ENM_gpio_pwm_output_set(0);
	mdelay(5);

	active_flag = KAL_FALSE;
	return ret;
}

static int sgm3785_FL_Enable(int duty)
{
    wake_lock(&sgm_pwm_lock);//Hct drv zs add for torch
	if(duty > 0)//flashlight mode
		sgm3785_set_flash_mode(sgm_flash_duty);
	else //torch mode
		sgm3785_set_torch_mode(sgm_torch_duty);
	return 0;
}

static int sgm3785_FL_Disable(void)
{
    wake_unlock(&sgm_pwm_lock);//Hct drv zs add for torch
	sgm3785_shutdown();
	return 0;
}

flashlight_list_t flashlight_list = {
	.fl_init = sgm3785_init,
	.fl_enable = sgm3785_FL_Enable,
	.fl_disable = sgm3785_FL_Disable,
	.fl_name = "sgm3785",
};

