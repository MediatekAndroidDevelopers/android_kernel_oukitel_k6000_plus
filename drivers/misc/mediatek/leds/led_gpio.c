
#include <linux/hct_include/hct_project_all_config.h>

#if (__HCT_RED_LED_MODE__ == __HCT_MT65XX_LED_MODE_GPIO__) || (__HCT_GREEN_LED_MODE__ == __HCT_MT65XX_LED_MODE_GPIO__) || (__HCT_BLUE_LED_MODE__ == __HCT_MT65XX_LED_MODE_GPIO__) || (__HCT_BUTTON_BACKLIGHT_LED_MODE__ == __HCT_MT65XX_LED_MODE_GPIO__) \
  || (__HCT_RED_LED_MODE__ == __HCT_MT65XX_LED_MODE_PWM__) || (__HCT_GREEN_LED_MODE__ == __HCT_MT65XX_LED_MODE_PWM__) || (__HCT_BLUE_LED_MODE__ == __HCT_MT65XX_LED_MODE_PWM__) || (__HCT_BUTTON_BACKLIGHT_LED_MODE__ == __HCT_MT65XX_LED_MODE_PWM__)  
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
#define LED_GPIO_RGB_RED		0x1
#define LED_GPIO_RGB_GREEN		0x2
#define LED_GPIO_RGB_BLUE		0x4
#define LED_GPIO_KPD_LED		0x8


struct pinctrl *led_gpio_pinsctrl = NULL;
struct pinctrl_state *led_gpio_default = NULL;
struct pinctrl_state *led_gpio_rgb_red_low = NULL;
struct pinctrl_state *led_gpio_rgb_red_high = NULL;
struct pinctrl_state *led_gpio_rgb_green_low = NULL;
struct pinctrl_state *led_gpio_rgb_green_high = NULL;
struct pinctrl_state *led_gpio_rgb_blue_low = NULL;
struct pinctrl_state *led_gpio_rgb_blue_high = NULL;
struct pinctrl_state *led_gpio_kpdled_low = NULL;
struct pinctrl_state *led_gpio_kpdled_high = NULL;

struct pinctrl_state *led_gpio_rgb_red_as_pwm = NULL;
struct pinctrl_state *led_gpio_rgb_green_as_pwm = NULL;
struct pinctrl_state *led_gpio_rgb_blue_as_pwm = NULL;
struct pinctrl_state *led_gpio_kpdled_as_pwm = NULL;

static int led_gpio_init(struct platform_device *pdev)
{
	int ret = 0;

	led_gpio_pinsctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(led_gpio_pinsctrl)) {
		dev_err(&pdev->dev, "Cannot find led_gpio pinctrl!");
		ret = PTR_ERR(led_gpio_pinsctrl);
		return ret;
	}

#if (__HCT_RED_LED_MODE__ == __HCT_MT65XX_LED_MODE_GPIO__) || (__HCT_RED_LED_MODE__ == __HCT_MT65XX_LED_MODE_PWM__)
	/*led_gpio rgb red Pin initialization */
	led_gpio_rgb_red_low = pinctrl_lookup_state(led_gpio_pinsctrl, "state_led_gpio_rgb_red_low");
	if (IS_ERR(led_gpio_rgb_red_low)) {
		ret = PTR_ERR(led_gpio_rgb_red_low);
		pr_err("%s : pinctrl err, led_gpio_rgb_red_low\n", __func__);
	}
	led_gpio_rgb_red_high = pinctrl_lookup_state(led_gpio_pinsctrl, "state_led_gpio_rgb_red_high");
	if (IS_ERR(led_gpio_rgb_red_high)) {
		ret = PTR_ERR(led_gpio_rgb_red_high);
		pr_err("%s : pinctrl err, led_gpio_rgb_red_high\n", __func__);
	}
#endif

#if (__HCT_GREEN_LED_MODE__ == __HCT_MT65XX_LED_MODE_GPIO__) || (__HCT_GREEN_LED_MODE__ == __HCT_MT65XX_LED_MODE_PWM__)
	/*led_gpio rgb green Pin initialization */
	led_gpio_rgb_green_low = pinctrl_lookup_state(led_gpio_pinsctrl, "state_led_gpio_rgb_green_low");
	if (IS_ERR(led_gpio_rgb_green_low)) {
		ret = PTR_ERR(led_gpio_rgb_green_low);
		pr_err("%s : pinctrl err, led_gpio_rgb_green_low\n", __func__);
	}
	led_gpio_rgb_green_high = pinctrl_lookup_state(led_gpio_pinsctrl, "state_led_gpio_rgb_green_high");
	if (IS_ERR(led_gpio_rgb_green_high)) {
		ret = PTR_ERR(led_gpio_rgb_green_high);
		pr_err("%s : pinctrl err, led_gpio_rgb_green_high\n", __func__);
	}
#endif

#if (__HCT_BLUE_LED_MODE__ == __HCT_MT65XX_LED_MODE_GPIO__) || (__HCT_BLUE_LED_MODE__ == __HCT_MT65XX_LED_MODE_PWM__)
	/*led_gpio rgb blue Pin initialization */
	led_gpio_rgb_blue_low = pinctrl_lookup_state(led_gpio_pinsctrl, "state_led_gpio_rgb_blue_low");
	if (IS_ERR(led_gpio_rgb_blue_low)) {
		ret = PTR_ERR(led_gpio_rgb_blue_low);
		pr_err("%s : pinctrl err, led_gpio_rgb_blue_low\n", __func__);
	}
	led_gpio_rgb_blue_high = pinctrl_lookup_state(led_gpio_pinsctrl, "state_led_gpio_rgb_blue_high");
	if (IS_ERR(led_gpio_rgb_blue_high)) {
		ret = PTR_ERR(led_gpio_rgb_blue_high);
		pr_err("%s : pinctrl err, led_gpio_rgb_blue_high\n", __func__);
	}
#endif

#if (__HCT_BUTTON_BACKLIGHT_LED_MODE__ == __HCT_MT65XX_LED_MODE_GPIO__) || (__HCT_BUTTON_BACKLIGHT_LED_MODE__ == __HCT_MT65XX_LED_MODE_PWM__)
	/*led_gpio kpdled Pin initialization */
	led_gpio_kpdled_low = pinctrl_lookup_state(led_gpio_pinsctrl, "state_led_gpio_kpdled_low");
	if (IS_ERR(led_gpio_kpdled_low)) {
		ret = PTR_ERR(led_gpio_kpdled_low);
		pr_err("%s : pinctrl err, led_gpio_kpdled_low\n", __func__);
	}
	led_gpio_kpdled_high = pinctrl_lookup_state(led_gpio_pinsctrl, "state_led_gpio_kpdled_high");
	if (IS_ERR(led_gpio_kpdled_high)) {
		ret = PTR_ERR(led_gpio_kpdled_high);
		pr_err("%s : pinctrl err, led_gpio_kpdled_high\n", __func__);
	}
#endif

#if (__HCT_RED_LED_MODE__ == __HCT_MT65XX_LED_MODE_PWM__)
	led_gpio_rgb_red_as_pwm = pinctrl_lookup_state(led_gpio_pinsctrl, "state_led_rgb_red_pins_as_pwm");
	if (IS_ERR(led_gpio_rgb_red_as_pwm)) {
		ret = PTR_ERR(led_gpio_rgb_red_as_pwm);
		pr_err("%s : pinctrl err, led_gpio_rgb_red_as_pwm\n", __func__);
	}
#endif
#if (__HCT_GREEN_LED_MODE__ == __HCT_MT65XX_LED_MODE_PWM__)
	led_gpio_rgb_green_as_pwm = pinctrl_lookup_state(led_gpio_pinsctrl, "state_led_rgb_green_pins_as_pwm");
	if (IS_ERR(led_gpio_rgb_green_as_pwm)) {
		ret = PTR_ERR(led_gpio_rgb_green_as_pwm);
		pr_err("%s : pinctrl err, led_gpio_rgb_green_as_pwm\n", __func__);
	}
#endif
#if (__HCT_BLUE_LED_MODE__ == __HCT_MT65XX_LED_MODE_PWM__)
	led_gpio_rgb_blue_as_pwm = pinctrl_lookup_state(led_gpio_pinsctrl, "state_led_rgb_blue_pins_as_pwm");
	if (IS_ERR(led_gpio_rgb_blue_as_pwm)) {
		ret = PTR_ERR(led_gpio_rgb_blue_as_pwm);
		pr_err("%s : pinctrl err, led_gpio_rgb_blue_as_pwm\n", __func__);
	}
#endif
#if (__HCT_BUTTON_BACKLIGHT_LED_MODE__ == __HCT_MT65XX_LED_MODE_PWM__)
	led_gpio_kpdled_as_pwm = pinctrl_lookup_state(led_gpio_pinsctrl, "state_kpdled_pins_as_pwm");
	if (IS_ERR(led_gpio_kpdled_as_pwm)) {
		ret = PTR_ERR(led_gpio_kpdled_as_pwm);
		pr_err("%s : pinctrl err, led_gpio_kpdled_as_pwm\n", __func__);
	}
#endif

	return ret;
}

int led_gpio_output(int ledtype, int level)
{
	switch(ledtype)
	{
	#if (__HCT_RED_LED_MODE__ == __HCT_MT65XX_LED_MODE_GPIO__) || (__HCT_RED_LED_MODE__ == __HCT_MT65XX_LED_MODE_PWM__)
	case __HCT_MT65XX_LED_MODE_GPIO_R__ :
		if ((IS_ERR(led_gpio_rgb_red_low))||(IS_ERR(led_gpio_rgb_red_high))) {
			pr_err("%s : pinctrl err, led_gpio_rgb_red_low || led_gpio_rgb_red_high\n", __func__);
			return 1;
			
		}
		if (level)
			pinctrl_select_state(led_gpio_pinsctrl, led_gpio_rgb_red_high);
		else
			pinctrl_select_state(led_gpio_pinsctrl, led_gpio_rgb_red_low);
		break;
	#endif

	#if (__HCT_GREEN_LED_MODE__ == __HCT_MT65XX_LED_MODE_GPIO__) || (__HCT_GREEN_LED_MODE__ == __HCT_MT65XX_LED_MODE_PWM__)
	case __HCT_MT65XX_LED_MODE_GPIO_G__ :
		if ((IS_ERR(led_gpio_rgb_green_low))||(IS_ERR(led_gpio_rgb_green_high))) {
			pr_err("%s : pinctrl err, led_gpio_rgb_green_low || led_gpio_rgb_green_high\n", __func__);
			return 1;
		}
		if (level)
			pinctrl_select_state(led_gpio_pinsctrl, led_gpio_rgb_green_high);
		else
			pinctrl_select_state(led_gpio_pinsctrl, led_gpio_rgb_green_low);
		break;
	#endif

	#if (__HCT_BLUE_LED_MODE__ ==__HCT_MT65XX_LED_MODE_GPIO__) || (__HCT_BLUE_LED_MODE__ == __HCT_MT65XX_LED_MODE_PWM__)
	case __HCT_MT65XX_LED_MODE_GPIO_B__ :
		if ((IS_ERR(led_gpio_rgb_blue_low))||(IS_ERR(led_gpio_rgb_blue_high))) {
			pr_err("%s : pinctrl err, led_gpio_rgb_blue_low || led_gpio_rgb_blue_high\n", __func__);
			return 1;
		}
		if (level)
			pinctrl_select_state(led_gpio_pinsctrl, led_gpio_rgb_blue_high);
		else
			pinctrl_select_state(led_gpio_pinsctrl, led_gpio_rgb_blue_low);
		break;
	#endif

	#if (__HCT_BUTTON_BACKLIGHT_LED_MODE__ == __HCT_MT65XX_LED_MODE_GPIO__) || (__HCT_BUTTON_BACKLIGHT_LED_MODE__ == __HCT_MT65XX_LED_MODE_PWM__)
	case __HCT_MT65XX_LED_MODE_GPIO_KPD__ :
		if ((IS_ERR(led_gpio_kpdled_low))||(IS_ERR(led_gpio_kpdled_high))) {
			pr_err("%s : pinctrl err, led_gpio_kpdled_low || led_gpio_kpdled_high\n", __func__);
			return 1;
		}
		if (level)
			pinctrl_select_state(led_gpio_pinsctrl, led_gpio_kpdled_high);
		else
			pinctrl_select_state(led_gpio_pinsctrl, led_gpio_kpdled_low);
		break;
	#endif
	default:;
	}

	return 0;	
}

int led_gpio_pwm_set(int ledtype)
{
	switch(ledtype)
	{
	#if (__HCT_RED_LED_MODE__ == __HCT_MT65XX_LED_MODE_PWM__)
	case __HCT_MT65XX_LED_MODE_GPIO_R__ :
		if (IS_ERR(led_gpio_rgb_red_as_pwm)) {
			pr_err("%s : pinctrl err, led_gpio_rgb_red_as_pwm\n", __func__);
			return 1;
		}
		pinctrl_select_state(led_gpio_pinsctrl, led_gpio_rgb_red_as_pwm);
		break;
	#endif

	#if (__HCT_GREEN_LED_MODE__ == __HCT_MT65XX_LED_MODE_PWM__)
	case __HCT_MT65XX_LED_MODE_GPIO_G__ :
		if (IS_ERR(led_gpio_rgb_green_as_pwm)) {
			pr_err("%s : pinctrl err, led_gpio_rgb_green_as_pwm\n", __func__);
			return 1;
		}
		pinctrl_select_state(led_gpio_pinsctrl, led_gpio_rgb_green_as_pwm);
		break;
	#endif

	#if (__HCT_BLUE_LED_MODE__ == __HCT_MT65XX_LED_MODE_PWM__)
	case __HCT_MT65XX_LED_MODE_GPIO_B__ :
		if (IS_ERR(led_gpio_rgb_blue_as_pwm)) {
			pr_err("%s : pinctrl err, led_gpio_rgb_blue_as_pwm\n", __func__);
			return 1;
		}
		pinctrl_select_state(led_gpio_pinsctrl, led_gpio_rgb_blue_as_pwm);
		break;
	#endif

	#if (__HCT_BUTTON_BACKLIGHT_LED_MODE__ == __HCT_MT65XX_LED_MODE_PWM__)
	case __HCT_MT65XX_LED_MODE_GPIO_KPD__ :
		if (IS_ERR(led_gpio_kpdled_as_pwm)) {
			pr_err("%s : pinctrl err, led_gpio_kpdled_as_pwm\n", __func__);
			return 1;
		}
		pinctrl_select_state(led_gpio_pinsctrl, led_gpio_kpdled_as_pwm);
		break;
	#endif
	default:
		pr_err("%s : pinctrl err, %#x is not support\n", __func__, ledtype);
		break;
	}

	return 0;	
}

//led_gpio device 
#ifdef CONFIG_OF
struct of_device_id led_gpio_of_match[] = {
	{ .compatible = "mediatek,led_gpio_dev", },
	{},
};
#endif

static int led_gpio_probe(struct platform_device *pdev)
{
	int ret;
	ret = led_gpio_init(pdev);

	return 0;
}

static int led_gpio_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver led_gpio_driver = {
	.probe = led_gpio_probe,
	.remove = led_gpio_remove,
	.shutdown = NULL,
	.driver = {
			.name = "led_gpio_dev",
			.owner = THIS_MODULE,
#ifdef CONFIG_OF
			.of_match_table = led_gpio_of_match,
#endif
	},
};

static int __init led_gpio_driver_init(void) 
{
	pr_err("%s: led_gpio driver init\n", __func__);
	if (platform_driver_register(&led_gpio_driver) != 0) {
		pr_err("unable to register led_gpio driver.\n");
	}
	return 0;
}

/* should never be called */
static void __exit led_gpio_driver_exit(void) 
{
	pr_err("%s: led_gpio driver exit\n", __func__);
	platform_driver_unregister(&led_gpio_driver);
}

module_init(led_gpio_driver_init);
module_exit(led_gpio_driver_exit);
MODULE_DESCRIPTION("Linux HW direct control driver");
MODULE_AUTHOR("David.wang(softnow@live.cn)");
MODULE_LICENSE("GPL");

#endif
