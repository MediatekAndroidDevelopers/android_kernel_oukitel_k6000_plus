#ifndef __HCT_CUSTOM_DTS_CONFIG_H__
#define __HCT_CUSTOM_DTS_CONFIG_H__

/** it's used for dts ,add by zz **/
#define STR_AND_STR(a,b)    	a##b

/** Example for dts: *******
#define __HCT_XXX_I2C_BUS_NUM__ 1
&I2C(__HCT_XXX_I2C_BUS_NUM__) {
--->output: &i2c1 {
******/
#define I2C(num)   		STR_AND_STR(i2c,num)


#define PUNMUX_GPIO_NONE_FUNC_NONE    0xFFFF

/*############### led releated config #####################*/
/*led mode define*/

#define __HCT_MT65XX_LED_MODE_NONE__      0
#define __HCT_MT65XX_LED_MODE_PWM__       1
#define __HCT_MT65XX_LED_MODE_GPIO__      2
#define __HCT_MT65XX_LED_MODE_PMIC__      3
#define __HCT_MT65XX_LED_MODE_CUST_LCM__  4 
#define __HCT_MT65XX_LED_MODE_CUST_BLS_PWM__  5
#define __HCT_MT65XX_LED_MODE_I2C__       6

#define __HCT_MT65XX_LED_MODE_GPIO_R__     	1
#define __HCT_MT65XX_LED_MODE_GPIO_G__      	2
#define __HCT_MT65XX_LED_MODE_GPIO_B__      	4
#define __HCT_MT65XX_LED_MODE_GPIO_KPD__      	8
#define __HCT_MT65XX_LED_MODE_BREATHLIGHT__      6

#define __HCT_MT65XX_LED_PMIC_LCD_ISINK__      0
#define __HCT_MT65XX_LED_PMIC_NLED_ISINK0__      1
#define __HCT_MT65XX_LED_PMIC_NLED_ISINK1__      2
#define __HCT_MT65XX_LED_PMIC_NLED_ISINK2__      3
#define __HCT_MT65XX_LED_PMIC_NLED_ISINK3__      4

#define __HCT_MT65XX_LED_PWM_NLED_PWM1__      0
#define __HCT_MT65XX_LED_PWM_NLED_PWM2__      1
#define __HCT_MT65XX_LED_PWM_NLED_PWM3__      2
#define __HCT_MT65XX_LED_PWM_NLED_PWM4__      3
#define __HCT_MT65XX_LED_PWM_NLED_PWM5__      4

/*##############  PMIC releated config #####################*/
#define __HCT_PMIC_ISINK_0_4MA__  	0
#define __HCT_PMIC_ISINK_1_8MA__  	1
#define __HCT_PMIC_ISINK_2_12MA__  	2
#define __HCT_PMIC_ISINK_3_16MA__  	3
#define __HCT_PMIC_ISINK_4_20MA__  	4
#define __HCT_PMIC_ISINK_5_24MA__  	5

#endif

