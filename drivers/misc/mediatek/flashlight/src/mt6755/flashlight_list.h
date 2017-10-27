#ifndef __FLASHLIAHT_LIST_H__
#define __FLASHLIAHT_LIST_H__

#include <mach/mt_pwm_hal.h>
#include <linux/of.h>

typedef struct {
	int (*fl_init)(struct platform_device *,struct device_node *);
	int (*fl_enable)(int);
	int (*fl_disable)(void);
	// int (*fl_open)(void *);
	// int (*fl_close)(void *);
	const char * fl_name;
} flashlight_list_t;

//#define FLASHLIGHT_TEST

enum HCT_PWM_NO{
	PWM_A = PWM1,
	PWM_B = PWM2,
	PWM_C = PWM3,
};

typedef struct {
	int gpio_no;
	int pwm_no;
}gpio_pwm_t;


#endif


