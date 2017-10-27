#ifndef __HCT_BOARD_CONFIG_H__
#define __HCT_BOARD_CONFIG_H__


#undef HCT_YES
#define HCT_YES 1
#undef HCT_NO
#define HCT_NO 0

/*audio releated */
/* phone mic mode



*/
#define __HCT_PHONE_MIC_MODE__   2
 
 /*phone use exp audio pa*/
#define __HCT_USING_EXTAMP_HP__  HCT_YES



/**************************ACCDET MIC Related*********/
#define __HCT_ACCDET_MIC_MODE__  1
/**###########################audio gpio define##################***/

#if  __HCT_USING_EXTAMP_HP__  
    #define __HCT_EXTAMP_HP_MODE__    2
    #define __HCT_EXTAMP_GPIO_NUM__    100
#endif

/****POWER RELEATED****/
#define __HCT_CAR_TUNE_VALUE__     85
#define __HCT_5G_WIFI_SUPPORT__   HCT_YES
#define __HCT_SHUTDOWN_SYSTEM_VOLTAGE__  3300

/*############### USB OTG releated config--START #####################*/

#define __HCT_USB_MTK_OTG_SUPPORT__  HCT_YES

/*for mt6750 and 6755 tp power default use Vio28*/
#define __HCT_GTP_VIO28_SUPPORT__  HCT_YES


// add feature control o2 dual charger support or not , in lk
#define __HCT_O2_DUAL_CHARGER_SUPPORT__  HCT_YES
/*############### USB OTG releated config--END #####################*/

#define __HCT_MSDC_CD_POLARITY_HIGH__ HCT_YES



#endif
