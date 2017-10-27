

#include "hct_common_config.h"

/*************** lcm fhd/amoled *********************/

// +5v
#define __HCT_GPIO_LCM_POWER_ENN_PINMUX__   PINMUX_GPIO20__FUNC_GPIO20
// -5v
#define __HCT_GPIO_LCM_POWER_ENP_PINMUX__   PINMUX_GPIO19__FUNC_GPIO19
//1.8v ldo
#define __HCT_GPIO_LCM_POWER_DM_PINMUX__        PINMUX_GPIO16__FUNC_GPIO16
//2.8v ldo
#define __HCT_GPIO_LCM_POWER_DP_PINMUX__        PINMUX_GPIO0__FUNC_GPIO0 


/**************************SD CARD hotplug Related*********/
#if __HCT_SD_CARD_HOTPLUG_SUPPORT__
#define __HCT_MSDC_CD_EINT_NUM__  		3
#define __HCT_MSDC_CD_GPIO_PIN_NUM__  	3
#endif


/**************************  Hall  Related*********/
#if __HCT_HALL_SUPPORT__
#define __HCT_HALL_EINT_PIN_NUM__           4
#define __HCT_KPD_SLIDE_EINT_PIN__          PINMUX_GPIO4__FUNC_GPIO4
#define __HCT_HALL_EINT_GPIO_NUM__          4
#endif
  
/**************************Camera Related*********/
#define __HCT_GPIO_CAMERA_LDO_EN_PINMUX__  PUNMUX_GPIO_NONE_FUNC_NONE
//-----------main camera
#define __HCT_GPIO_CAMERA_CMRST_PINMUX__ PINMUX_GPIO110__FUNC_GPIO110
#define __HCT_GPIO_CAMERA_CMPDN_PINMUX__ PINMUX_GPIO107__FUNC_GPIO107
//-----------sub camera
#define __HCT_GPIO_CAMERA_CMRST1_PINMUX__ PINMUX_GPIO111__FUNC_GPIO111
#define __HCT_GPIO_CAMERA_CMPDN1_PINMUX__ PINMUX_GPIO108__FUNC_GPIO108


/**************************FlashLight Related*********/

#ifdef CONFIG_FLASHLIGHT_AW3641E
	#define  __HCT_AW3641E_GPIO_EN_NUM__         8
	#define  __HCT_AW3641E_GPIO_MODE_NUM__       9
#endif

#ifdef CONFIG_FLASHLIGHT_SGM3785
	#define __HCT_SGM3785_ENF_GPIO_NUM__          		9
	#define __HCT_SGM3785_ENM_GPIO_NUM__  			8
	#define __HCT_SGM3785_ENM_GPIO_AS_PWM_PIN__		PINMUX_GPIO8__FUNC_PWM_A
	#define __HCT_SGM3785_PWM_F_DUTY__      9        
	#define __HCT_SGM3785_PWM_T_DUTY__      5
#endif

#ifdef CONFIG_FLASHLIGHT_SUB_GPIO
	#define  __HCT_GPIO_FLASHLIGHT_SUB_EN_NUM__     12
#endif

#ifdef CONFIG_FLASHLIGHT_SUB_PWM
	#define __HCT_FLASHLIGHT_SUB_GPIO_PWM_AS_PWM_PIN__	PINMUX_GPIO12__FUNC_PWM_C
	#define __HCT_FLASHLIGHT_SUB_GPIO_EN_NUM__   		12
	#define __HCT_FLASHLIGHT_SUB_PWM_F_DUTY__      10        
	#define __HCT_FLASHLIGHT_SUB_PWM_T_DUTY__      2
#endif

/**************************Audio EXMP Related*********/
#define __HCT_GPIO_EXTAMP_EN_PIN__     PINMUX_GPIO100__FUNC_GPIO100
#define __HCT_GPIO_EXTAMP2_EN_PIN__    PUNMUX_GPIO_NONE_FUNC_NONE



/**************************CTP Related****************/
#define __HCT_CTP_EINT_ENT_PINNUX__         PINMUX_GPIO1__FUNC_GPIO1
#define __HCT_CTP_EINT_EN_PIN_NUM__      	1 

#define __HCT_CTP_RESET_PINNUX__          PINMUX_GPIO10__FUNC_GPIO10

#define __HCT_CTP_FT_ALL_SENSOR_ADDR__     0x38

/**************************RGB GPIO Related****************/
#if __HCT_GREEN_LED_MODE__ == __HCT_MT65XX_LED_MODE_GPIO__
#define  __HCT_RGB_GREEN_GPIO_EN_PIN__ 		PINMUX_GPIO12__FUNC_GPIO12
#endif

#if __HCT_BUTTON_BACKLIGHT_LED_MODE__ == __HCT_MT65XX_LED_MODE_GPIO__
#define  __HCT_KPDLED_GPIO_EN_PIN__ 		PINMUX_GPIO5__FUNC_GPIO5
#endif

#if __HCT_BUTTON_BACKLIGHT_LED_MODE__ == __HCT_MT65XX_LED_MODE_PWM__ || __HCT_BUTTON_BACKLIGHT_LED_MODE__ == __HCT_MT65XX_LED_MODE_GPIO__
#define __HCT_KPDLED_GPIO_EN_PIN__            PINMUX_GPIO5__FUNC_GPIO5
#define __HCT_KPDLED_GPIO_AS_PWM_PIN__            PINMUX_GPIO5__FUNC_PWM_C
#define __HCT_KPDLED_PWM_DUTY__                2
#endif


#define __HCT_BREATHLIGHT_AW2013_ADDR__		0x45
#define __HCT_GPIO_AW2013_LDO_PIN__    		PINMUX_GPIO42__FUNC_GPIO42
/**************************Accdet Related****************/

#define __HCT_ACCDET_EINT_PIN__     	PINMUX_GPIO2__FUNC_GPIO2
#define __HCT_ACCDET_EINT_PIN_NUM__     2


/**************************Gsensor Related****************/
/* Gsensor releated*/
/*Step1: define this macro*/
/*Step2: need define dws int tab with the right bus num*/
#define __HCT_GSENSOR_I2C_BUS_NUM__       	1

#define __HCT__KXTJ2_SENSOR_ADDR__     0x40
#define __HCT__KXTJ2_SENSOR_DIRECTION__     5
#define __HCT__KXTJ2_SENSOR_BATCH_SUPPORT__     0

#define __HCT__MXC400X_SENSOR_ADDR__	0x15
#define __HCT__MXC400X_SENSOR_DIRECTION__	5
#define __HCT__MXC400X_SENSOR_BATCH_SUPPORT__ 0

#define __HCT__BMA156_NEW_SENSOR_ADDR__			0x10
#define __HCT__BMA156_NEW_SENSOR_DIRECTION__		1
#define __HCT__BMA156_NEW_SENSOR_BATCH_SUPPORT__ 	0

#ifdef CONFIG_MTK_BMI160_ACC
#define __HCT__BMI160_ACC_SENSOR_ADDR__			0x69
#define __HCT__BMI160_ACC_SENSOR_DIRECTION__		2
#define __HCT__BMI160_ACC_SENSOR_BATCH_SUPPORT__ 	0
#endif


/**************************Gyro sensor Related****************/
#define __HCT_GYRO_I2C_BUS_NUM__       1
#define __HCT__BMG160_NEW_SENSOR_ADDR__     0x68
#define __HCT__BMG160_NEW_SENSOR_DIRECTION__		0


#ifdef CONFIG_MTK_BMI160_GYRO
#define __HCT__BMI160_GYRO_SENSOR_ADDR__     0x66
#define __HCT__BMI160_GYRO_SENSOR_DIRECTION__		2
#endif

#define __HCT_GYRO_EINT_PINMUX__     0
/**************************ALSPS Related****************/
/*Step1: define this macro*/
/*Step2: need define dws int tab with the right bus num*/
#define __HCT_ALSPS_I2C_BUS_NUM__               1
#define __HCT_STK3X1X_SENSOR_ADDR__			0x48
#define __HCT_HCT_STK3X1X_PS_THRELD_HIGH__		1700
#define __HCT_HCT_STK3X1X_PS_THRELD_LOW__		1500
/*EPL259X Sensor Customize ----start*****/
/*EPL259X Sensor I2C addr*/
#define __HCT_EPL259X_SENSOR_ADDR__            0x49
#define __HCT_EPL2182_SENSOR_ADDR__            0x49
#define __HCT_EM3071X_SENSOR_ADDR__            0x24

#define __HCT_ALSPS_EINT_PINMUX__                  PINMUX_GPIO6__FUNC_GPIO6
#define __HCT_ALSPS_EINT_PIN_NUM__              6
/**************************msensor sensor Related****************/
#define __HCT_MSENSOR_I2C_BUS_NUM__       	1

#define __HCT__BMM156_NEW_SENSOR_ADDR__			0x12
#define __HCT__BMM156_NEW_SENSOR_DIRECTION__		1
#define __HCT__BMM156_NEW_SENSOR_BATCH_SUPPORT__ 	0

#define __HCT_QMC7983_SENSOR_ADDR__			0x2D
#define __HCT_QMC7983_SENSOR_DIRECTION__		1	
/**************************Switch Charging Related****************/
/*Step1: define this macro*/
/*Step2: need define dws int tab with the right bus num*/

#define __HCT_SWITCH_CHARGER_SUB_I2C_BUS_NUM__ 0
#define __HCT_SWITCH_CHARGER_I2C_BUS_NUM__ 3
#define __HCT_NCP1854_CHARGER_ADDR__             0X36
#define __HCT_NCP1851_CHARGER_ADDR__             0X36

/**************************Usb type C Related****************/
/*Step1: define this macro*/
/*Step2: need define dws int tab with the right bus num*/
#if __HCT_TYPEC_PI5USB_SUPPORT__
#define __HCT_TYPEC_PI5USB_I2C_BUS_NUM__       	3
#define __HCT_TYPEC_PI5USB_ADDR__             		0x1d
/*EINT*/
#define __HCT_TYPEC_EINT_PIN_NUM__					60
#define __HCT_TYPEC_EINT_PIN__          			PINMUX_GPIO60__FUNC_GPIO60
/*USB ID as EINT*/
#define __HCT_TYPEC_USBID_EINT_PIN_NUM__			23
#define __HCT_TYPEC_USB_ID_GPIO_NUM__				23
#define __HCT_TYPEC_USB_ID_PIN__          			PINMUX_GPIO23__FUNC_GPIO23
#endif
/**************************USB OTG Related****************/
#if __HCT_TYPEC_PI5USB_SUPPORT__
	#if __HCT_USB_MTK_OTG_SUPPORT__
	#define __HCT_GPIO_DRV_VBUS_PIN__     			PINMUX_GPIO43__FUNC_USB_DRVVBUS
	#define __HCT_GPIO_USB_IDDIG_PINMUX_PIN__       PINMUX_GPIO42__FUNC_IDDIG

	#define __HCT_USB_VBUS_EN_PIN__     			43
	#define __HCT_USB_VBUS_EN_PIN_MODE__     		0

	#define __HCT_USB_IDDIG_PIN__     				42
	#define __HCT_USB_IDDIG_PIN_MODE__     	    1
	#endif
#else
	#if __HCT_USB_MTK_OTG_SUPPORT__
	#define __HCT_GPIO_DRV_VBUS_PIN__     			PUNMUX_GPIO_NONE_FUNC_NONE
	#define __HCT_GPIO_USB_IDDIG_PINMUX_PIN__       PINMUX_GPIO23__FUNC_IDDIG

	//#define __HCT_USB_VBUS_EN_PIN__     			115
	#define __HCT_USB_VBUS_EN_PIN_MODE__     		0
	
	
	#define __HCT_USB_IDDIG_PIN__     				23
	#define __HCT_USB_IDDIG_PIN_MODE__     	    2
	#endif
#endif
#define __HCT_GPIO_OZ115_RST_PIN__        43
/**************************GPS LNA Related****************/
#define __HCT_GPS_LNA_EN_PINMUX__     			PINMUX_GPIO114__FUNC_GPIO114

/**************************FINGERPRINT SUPPORT****************/
#ifdef CONFIG_HCT_FINGERPRINT_SUPPORT
#define __HCT_FINGERPRINT_EINT_EN_PIN_NUM__	56
#define __HCT_FINGERPRINT_EINT_PIN__	PINMUX_GPIO56__FUNC_GPIO56
#define __HCT_FINGERPRINT_POWER_PIN__	PINMUX_GPIO99__FUNC_GPIO99
#define __HCT_FINGERPRINT_RESET_PIN__	PINMUX_GPIO62__FUNC_GPIO62
#endif

//////////*****************customise end******************//////////
#include "hct_custom_config.h"

