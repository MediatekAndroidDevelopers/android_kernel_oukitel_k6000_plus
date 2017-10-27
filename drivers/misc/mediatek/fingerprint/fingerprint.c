#include <linux/cdev.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/kthread.h>
#include <linux/device.h>
#include <linux/compat.h>
#include <linux/platform_device.h>

#include <linux/hct_include/hct_project_all_config.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include "mt_spi.h"
#include "../../../spi/mediatek/mt6755/mt_spi.h" //modify zy
#include "fingerprint.h"



DECLARE_WAIT_QUEUE_HEAD(finger_init_waiter);

int hct_finger_probe_isok = 0;//add for hct finger jianrong
struct pinctrl *hct_finger_pinctrl;
struct pinctrl_state *hct_finger_power_on, *hct_finger_power_off,*hct_finger_reset_high,*hct_finger_reset_low,*hct_finger_spi0_mi_as_spi0_mi,*hct_finger_spi0_mi_as_gpio,
*hct_finger_spi0_mo_as_spi0_mo,*hct_finger_spi0_mo_as_gpio,*hct_finger_spi0_clk_as_spi0_clk,*hct_finger_spi0_clk_as_gpio,*hct_finger_spi0_cs_as_spi0_cs,*hct_finger_spi0_cs_as_gpio,
*hct_finger_int_as_int,*hct_finger_power_18v_on, *hct_finger_power_18v_off,*hct_finger_eint_en0,*hct_finger_eint_en1,*hct_finger_eint_en2;

int hct_finger_get_gpio_info(struct platform_device *pdev)
{
	struct device_node *node;
	int ret;
	node = of_find_compatible_node(NULL, NULL, "mediatek,hct_finger");
	printk("node.name %s full name %s",node->name,node->full_name);

	wake_up_interruptible(&finger_init_waiter);

	hct_finger_pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(hct_finger_pinctrl)) {
		ret = PTR_ERR(hct_finger_pinctrl);
		dev_err(&pdev->dev, "hct_finger cannot find pinctrl\n");
		return ret;
	}

	printk("[%s] hct_finger_pinctrl+++++++++++++++++\n",pdev->name);

	hct_finger_power_on = pinctrl_lookup_state(hct_finger_pinctrl, "finger_power_en1");
	if (IS_ERR(hct_finger_power_on)) {
		ret = PTR_ERR(hct_finger_power_on);
		dev_err(&pdev->dev, " Cannot find hct_finger pinctrl hct_finger_power_on!\n");
		return ret;
	}
	hct_finger_power_off = pinctrl_lookup_state(hct_finger_pinctrl, "finger_power_en0");
	if (IS_ERR(hct_finger_power_off)) {
		ret = PTR_ERR(hct_finger_power_off);
		dev_err(&pdev->dev, " Cannot find hct_finger pinctrl hct_finger_power_off!\n");
		return ret;
	}
	hct_finger_reset_high = pinctrl_lookup_state(hct_finger_pinctrl, "finger_reset_en1");
	if (IS_ERR(hct_finger_reset_high)) {
		ret = PTR_ERR(hct_finger_reset_high);
		dev_err(&pdev->dev, " Cannot find hct_finger pinctrl hct_finger_reset_high!\n");
		return ret;
	}
	hct_finger_reset_low = pinctrl_lookup_state(hct_finger_pinctrl, "finger_reset_en0");
	if (IS_ERR(hct_finger_reset_low)) {
		ret = PTR_ERR(hct_finger_reset_low);
		dev_err(&pdev->dev, " Cannot find hct_finger pinctrl hct_finger_reset_low!\n");
		return ret;
	}
	hct_finger_spi0_mi_as_spi0_mi = pinctrl_lookup_state(hct_finger_pinctrl, "finger_spi0_mi_as_spi0_mi");
	if (IS_ERR(hct_finger_spi0_mi_as_spi0_mi)) {
		ret = PTR_ERR(hct_finger_spi0_mi_as_spi0_mi);
		dev_err(&pdev->dev, " Cannot find hct_finger pinctrl hct_finger_spi0_mi_as_spi0_mi!\n");
		return ret;
	}
	hct_finger_spi0_mi_as_gpio = pinctrl_lookup_state(hct_finger_pinctrl, "finger_spi0_mi_as_gpio");
	if (IS_ERR(hct_finger_spi0_mi_as_gpio)) {
		ret = PTR_ERR(hct_finger_spi0_mi_as_gpio);
		dev_err(&pdev->dev, " Cannot find hct_finger pinctrl hct_finger_spi0_mi_as_gpio!\n");
		return ret;
	}
	hct_finger_spi0_mo_as_spi0_mo = pinctrl_lookup_state(hct_finger_pinctrl, "finger_spi0_mo_as_spi0_mo");
	if (IS_ERR(hct_finger_spi0_mo_as_spi0_mo)) {
		ret = PTR_ERR(hct_finger_spi0_mo_as_spi0_mo);
		dev_err(&pdev->dev, " Cannot find hct_finger pinctrl hct_finger_spi0_mo_as_spi0_mo!\n");
		return ret;
	}
	hct_finger_spi0_mo_as_gpio = pinctrl_lookup_state(hct_finger_pinctrl, "finger_spi0_mo_as_gpio");
	if (IS_ERR(hct_finger_spi0_mo_as_gpio)) {
		ret = PTR_ERR(hct_finger_spi0_mo_as_gpio);
		dev_err(&pdev->dev, " Cannot find hct_finger pinctrl hct_finger_spi0_mo_as_gpio!\n");
		return ret;
	}
	hct_finger_spi0_clk_as_spi0_clk = pinctrl_lookup_state(hct_finger_pinctrl, "finger_spi0_clk_as_spi0_clk");
	if (IS_ERR(hct_finger_spi0_clk_as_spi0_clk)) {
		ret = PTR_ERR(hct_finger_spi0_clk_as_spi0_clk);
		dev_err(&pdev->dev, " Cannot find hct_finger pinctrl hct_finger_spi0_clk_as_spi0_clk!\n");
		return ret;
	}
	hct_finger_spi0_clk_as_gpio = pinctrl_lookup_state(hct_finger_pinctrl, "finger_spi0_clk_as_gpio");
	if (IS_ERR(hct_finger_spi0_clk_as_gpio)) {
		ret = PTR_ERR(hct_finger_spi0_clk_as_gpio);
		dev_err(&pdev->dev, " Cannot find hct_finger pinctrl hct_finger_spi0_clk_as_gpio!\n");
		return ret;
	}
	hct_finger_spi0_cs_as_spi0_cs = pinctrl_lookup_state(hct_finger_pinctrl, "finger_spi0_cs_as_spi0_cs");
	if (IS_ERR(hct_finger_spi0_cs_as_spi0_cs)) {
		ret = PTR_ERR(hct_finger_spi0_cs_as_spi0_cs);
		dev_err(&pdev->dev, " Cannot find hct_finger pinctrl hct_finger_spi0_cs_as_spi0_cs!\n");
		return ret;
	}
	hct_finger_spi0_cs_as_gpio = pinctrl_lookup_state(hct_finger_pinctrl, "finger_spi0_cs_as_gpio");
	if (IS_ERR(hct_finger_spi0_cs_as_gpio)) {
		ret = PTR_ERR(hct_finger_spi0_cs_as_gpio);
		dev_err(&pdev->dev, " Cannot find hct_finger pinctrl hct_finger_spi0_cs_as_gpio!\n");
		return ret;
	}
	hct_finger_int_as_int = pinctrl_lookup_state(hct_finger_pinctrl, "finger_int_as_int");
	if (IS_ERR(hct_finger_int_as_int)) {
		ret = PTR_ERR(hct_finger_int_as_int);
		dev_err(&pdev->dev, " Cannot find hct_finger pinctrl hct_finger_spi0_cs_as_gpio!\n");
		return ret;
	}
/***************************************add by wanghanfeng for gf5216(start 2016-9-24)(porting by zz 10-25)*******************************************************/
    	hct_finger_eint_en0 = pinctrl_lookup_state(hct_finger_pinctrl, "finger_eint_en0");
    	if (IS_ERR(hct_finger_eint_en0)) {
    		ret = PTR_ERR(hct_finger_eint_en0);
    		dev_err(&pdev->dev, " Cannot find fp pinctrl finger_eint_en0!\n");
    		return ret;
    	}
    	hct_finger_eint_en1= pinctrl_lookup_state(hct_finger_pinctrl, "finger_eint_en1");
    	if (IS_ERR(hct_finger_eint_en1)) {
    		ret = PTR_ERR(hct_finger_eint_en1);
    		dev_err(&pdev->dev, " Cannot find fp pinctrl finger_eint_en1!\n");
    		return ret;
    	}
    
    	hct_finger_eint_en2 = pinctrl_lookup_state(hct_finger_pinctrl, "finger_eint_en2");
    	if (IS_ERR(hct_finger_eint_en2)) {
    		ret = PTR_ERR(hct_finger_eint_en2);
    		dev_err(&pdev->dev, " Cannot find fp pinctrl finger_eint_en2!\n");
    		return ret;
    	}
/*****************************************add by wanghanfeng (end 2016-9-24)******************************************************/
	hct_finger_power_18v_on = pinctrl_lookup_state(hct_finger_pinctrl, "finger_power_18v_en1");
	if (IS_ERR(hct_finger_power_18v_on)) {
		ret = PTR_ERR(hct_finger_power_18v_on);
		dev_err(&pdev->dev, " Cannot find hct_finger pinctrl hct_finger_power_18v_on!\n");
		return ret;
	}
	hct_finger_power_18v_off = pinctrl_lookup_state(hct_finger_pinctrl, "finger_power_18v_en0");
	if (IS_ERR(hct_finger_power_18v_off)) {
		ret = PTR_ERR(hct_finger_power_18v_off);
		dev_err(&pdev->dev, " Cannot find hct_finger pinctrl hct_finger_power_18v_off!\n");
		return ret;
	}
	printk("hct_finger get gpio info ok--------");
	return 0;
}

int hct_finger_set_power(int cmd)
{

	if(IS_ERR(hct_finger_power_off)||IS_ERR(hct_finger_power_on))
	{
		 pr_err( "err: hct_finger_power_on or hct_finger_power_off is error!!!");
		 return -1;
	}	


	switch (cmd)
	{
	case 0 : 		
		pinctrl_select_state(hct_finger_pinctrl, hct_finger_power_off);
	break;
	case 1 : 		
		pinctrl_select_state(hct_finger_pinctrl, hct_finger_power_on);
	break;
	}
	return 0;
}

int hct_finger_set_18v_power(int cmd)
{

	if(IS_ERR(hct_finger_power_18v_off)||IS_ERR(hct_finger_power_18v_on))
	{
		 pr_err( "err: hct_finger_power_on or hct_finger_power_18v_off is error!!!");
		 return -1;
	}	


	switch (cmd)
	{
	case 0 : 		
		pinctrl_select_state(hct_finger_pinctrl, hct_finger_power_18v_off);
	break;
	case 1 : 		
		pinctrl_select_state(hct_finger_pinctrl, hct_finger_power_18v_on);
	break;
	}
	return 0;
}

int hct_finger_set_reset(int cmd)
{

	if(IS_ERR(hct_finger_reset_low)||IS_ERR(hct_finger_reset_high))
	{
		 pr_err( "err: hct_finger_reset_low or hct_finger_reset_high is error!!!");
		 return -1;
	}	
	

	switch (cmd)
	{
	case 0 : 		
		pinctrl_select_state(hct_finger_pinctrl, hct_finger_reset_low);
	break;
	case 1 : 		
		pinctrl_select_state(hct_finger_pinctrl, hct_finger_reset_high);
	break;
	}
	return 0;
}
/*********************************************add by wanghanfeng for gf5216(start 2016-9-24)*************************************/
int hct_finger_set_irq(int cmd)
{
	
	
	
        if(IS_ERR(hct_finger_eint_en2)||IS_ERR(hct_finger_eint_en0)||IS_ERR(hct_finger_eint_en1))
	{
		 pr_err( "err: hct_finger_int_as_gpio is error!!!!");
		 return -1;
	}	

	

	switch (cmd)
	{
	case 0 : 		
		pinctrl_select_state(hct_finger_pinctrl, hct_finger_eint_en0);
	break;
	case 1 : 		
		pinctrl_select_state(hct_finger_pinctrl, hct_finger_eint_en1);
	break;
	case 2 : 		
		pinctrl_select_state(hct_finger_pinctrl, hct_finger_eint_en2);
	break;
	}
	return 0;


}
/*********************************************add by wanghanfeng(end  2016-9-24)**************************************/

int hct_finger_set_eint(int cmd)
{

	if(IS_ERR(hct_finger_int_as_int))
	{
		 pr_err( "err: hct_finger_int_as_int is error!!!");
		 return -1;
	}	

	

	switch (cmd)
	{
	case 0 : 		
		return -1;
	break;
	case 1 : 		
		pinctrl_select_state(hct_finger_pinctrl, hct_finger_int_as_int);
	break;
	}
	return 0;
}

int hct_finger_set_spi_mode(int cmd)
{

	if(IS_ERR(hct_finger_spi0_clk_as_gpio)||IS_ERR(hct_finger_spi0_cs_as_gpio)||IS_ERR(hct_finger_spi0_mi_as_gpio) \
		||IS_ERR(hct_finger_spi0_mo_as_gpio)||IS_ERR(hct_finger_spi0_clk_as_spi0_clk)||IS_ERR(hct_finger_spi0_cs_as_spi0_cs) \
		||IS_ERR(hct_finger_spi0_mi_as_spi0_mi)||IS_ERR(hct_finger_spi0_mo_as_spi0_mo))
	{
		 pr_err( "err: hct_finger_reset_low or hct_finger_reset_high is error!!!");
		 return -1;
	}	

	

	switch (cmd)
	{
	case 0 : 		
		pinctrl_select_state(hct_finger_pinctrl, hct_finger_spi0_clk_as_gpio);
		pinctrl_select_state(hct_finger_pinctrl, hct_finger_spi0_cs_as_gpio);
		pinctrl_select_state(hct_finger_pinctrl, hct_finger_spi0_mi_as_gpio);
		pinctrl_select_state(hct_finger_pinctrl, hct_finger_spi0_mo_as_gpio);
	break;
	case 1 : 		
		pinctrl_select_state(hct_finger_pinctrl, hct_finger_spi0_clk_as_spi0_clk);
		pinctrl_select_state(hct_finger_pinctrl, hct_finger_spi0_cs_as_spi0_cs);
		pinctrl_select_state(hct_finger_pinctrl, hct_finger_spi0_mi_as_spi0_mi);
		pinctrl_select_state(hct_finger_pinctrl, hct_finger_spi0_mo_as_spi0_mo);
	break;
	}
	return 0;
}

#ifdef CONFIG_OF
static struct of_device_id hct_finger_match[] = {
	{ .compatible = "mediatek,hct_finger", },
	{}
};
MODULE_DEVICE_TABLE(of, hct_finger_match);
#endif

static struct platform_device *hct_finger_plat = NULL;


void hct_waite_for_finger_dts_paser(void)
{
    if(hct_finger_plat==NULL)
    {
        wait_event_interruptible_timeout(finger_init_waiter, hct_finger_plat != NULL, 3 * HZ);
    }
    else
	return;
	
}

int hct_get_max_finger_spi_cs_number(void)
{
    return MAX_FINGER_CHIP_CS_NUMBER;
}

int hct_finger_plat_probe(struct platform_device *pdev) {
	hct_finger_plat = pdev;

	hct_finger_get_gpio_info(pdev);
	return 0;
}

int hct_finger_plat_remove(struct platform_device *pdev) {
	hct_finger_plat = NULL;
	return 0;
} 


#ifndef CONFIG_OF
static struct platform_device hct_finger_dev = {
	.name		  = "hct_finger",
	.id		  = -1,
};
#endif


static struct platform_driver hct_finger_pdrv = {
	.probe	  = hct_finger_plat_probe,
	.remove	 = hct_finger_plat_remove,
	.driver = {
		.name  = "hct_finger",
		.owner = THIS_MODULE,			
#ifdef CONFIG_OF
		.of_match_table = hct_finger_match,
#endif
	}
};


static int __init hct_finger_init(void)
{

#ifndef CONFIG_OF
    int retval=0;
    retval = platform_device_register(&hct_finger_dev);
    if (retval != 0){
        return retval;
    }
#endif

    if(platform_driver_register(&hct_finger_pdrv))
    {
    	printk("failed to register driver");
    	return -ENODEV;
    }
    
    return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit hct_finger_exit(void)
{
	platform_driver_unregister(&hct_finger_pdrv);
}


rootfs_initcall(hct_finger_init);
module_exit(hct_finger_exit);

MODULE_AUTHOR("Jay_zhou");
MODULE_DESCRIPTION("for hct fingerprint driver");
MODULE_LICENSE("GPL");


