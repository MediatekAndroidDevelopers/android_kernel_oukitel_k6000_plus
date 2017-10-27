/************************************************************************************************
*							*file name : sgm3785_drv.c
*							*Version : v1.0
*							*Author : erick
*							*Date : 2015.4.16
*************************************************************************************************/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <mach/upmu_sw.h>
#include <linux/of.h>

#include <sgm3785_drv.h>


#if  __HCT_FLASHLIGHT_SGM3785_SUPPORT__

static kal_bool active_flag = KAL_FALSE;

U16 sgm_flash_duty;
U16 sgm_torch_duty;


struct pwm_spec_config sgm3785_config = {
//    .pwm_no = PWM_NO,
    .mode = PWM_MODE_OLD,
    .clk_div = CLK_DIV32,//CLK_DIV16: 147.7kHz    CLK_DIV32: 73.8kHz
    .clk_src = PWM_CLK_OLD_MODE_BLOCK,
    .pmic_pad = false,
    .PWM_MODE_OLD_REGS.IDLE_VALUE = IDLE_FALSE,
    .PWM_MODE_OLD_REGS.GUARD_VALUE = GUARD_FALSE,
    .PWM_MODE_OLD_REGS.GDURATION = 0,
    .PWM_MODE_OLD_REGS.WAVE_NUM = 0,
    .PWM_MODE_OLD_REGS.DATA_WIDTH = 10,
    .PWM_MODE_OLD_REGS.THRESH = 5,
};

/*flash mode
ENF: 1                           _____________
         0 ____________|       660ms       |____
ENM:1    _      _     _     _      _     _     _      _     _
         0 _| |_| |_| |_| |_| |_| |_| |_| |_| |_
*/

static S32 sgm3785_set_flash_mode(U16 duty)
{	
	S32 ret = 0;

	if(active_flag == KAL_TRUE)
		return ret;

	flashlight_gpio_output(FL_EN_PIN, 0);

	flashlight_gpio_as_pwm(FL_PWM_PIN, 0);

	sgm3785_config.PWM_MODE_OLD_REGS.THRESH = duty;
	ret = pwm_set_spec_config(&sgm3785_config);	

	flashlight_gpio_output(FL_EN_PIN, 1);

	active_flag = KAL_TRUE;
	return ret;
}

/*movie/torch mode
ENF: 1
          0 _____________________________________
ENM:1	 ______	     _     _      _     _      _	
         0 _| >5ms |_| |_| |_| |_| |_| |________
*/	
static S32 sgm3785_set_torch_mode(U16 duty)
{
	S32 ret = 0;

	if(active_flag == KAL_TRUE)
		return ret;

	flashlight_gpio_output(FL_EN_PIN, 0);

	flashlight_gpio_output(FL_PWM_PIN, 0);
	mdelay(1);
	flashlight_gpio_output(FL_PWM_PIN, 1);
	mdelay(6);

	flashlight_gpio_as_pwm(FL_PWM_PIN, 0);

	sgm3785_config.PWM_MODE_OLD_REGS.THRESH = duty;
	ret = pwm_set_spec_config(&sgm3785_config);

	active_flag = KAL_TRUE;
	return ret;
}

static S32 sgm3785_shutdown(void)
{
	S32 ret = 0;
	if(active_flag == KAL_FALSE)
		return ret;

	mt_pwm_disable(sgm3785_config.pwm_no, false);

	flashlight_gpio_output(FL_EN_PIN, 0);
	flashlight_gpio_output(FL_PWM_PIN, 0);
	mdelay(5);

	active_flag = KAL_FALSE;
	return ret;
}

int sgm3785_ioctr(U16 mode, U16 duty)
{
	sgm3138_dbg("[sgm3785] mode %d , duty %d\n", mode, duty);

	if(mode < MODE_MIN || mode >= MODE_MAX)
	{
		sgm3138_dbg("[sgm3785] mode error!!!\n");
		return 1;
	}
	
	if(duty < 0 || duty > 10)
	{
		sgm3138_dbg("[sgm3785] duty error!!!\n");
		return 1;
	}

	switch(mode){
		case FLASH_M:
			sgm3785_set_flash_mode(duty);
			break;

		case TORCH_M:
			sgm3785_set_torch_mode(duty);
			break;

		case PWD_M:
			sgm3785_shutdown();
			break;

		default:
			sgm3138_dbg("[sgm3785] error ioctr!!!\n");
			break;
	}

	return 0;
}

void sgm3785_get_dts_info(const struct of_device_id *maches )
{
        struct device_node *node1 = NULL;
        u32   pwm_number;
        node1 = of_find_matching_node(node1, maches);
    
          if (node1) 
          {
              of_property_read_u32(node1, "flashlight_pwmnum", &pwm_number);
              sgm3785_config.pwm_no=pwm_number; 
              sgm3138_dbg("[sgm3785_get_dts_info] PWM_NO=%d!!!\n",pwm_number);
          }else
          {
              sgm3785_config.pwm_no=PWM3;
              printk("sgm3785_get_dts_info, error, pwm_no is error pls check\n");
          }
          sgm_flash_duty= F_DUTY;
          sgm_torch_duty= T_DUTY;
}

void sgm3785_FL_Enable(int duty)
{
	if(duty > 0)//flashlight mode
		sgm3785_ioctr(FLASH_M, sgm_flash_duty);
	else //torch mode
		sgm3785_ioctr(TORCH_M, sgm_torch_duty);
}

void sgm3785_FL_Disable(void)
{
	sgm3785_ioctr(PWD_M, 0);
}

#endif

