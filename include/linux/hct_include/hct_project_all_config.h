
#ifndef __HCT_PROJECT_ALL_CONFIG_H__
#define __HCT_PROJECT_ALL_CONFIG_H__

#include "hct_common_config.h"
#include "hct_board_config.h"
#include "hct_custom_config.h"



//Jay: when you add new macro for project , you need defined default macro valut here, so you need not defined in code logic
//Next is Begin :::::::::

// Note: default: sd card detection is low level
#ifndef __HCT_SD_CARD_DET_LEVEL__
#define __HCT_SD_CARD_DET_LEVEL__ HCT_NO
#endif

// Note: default: c2k config is off
/*
    when you switch c2k on, you need config __HCT_C2K_DTS_SUPPORT__ HCT_YES
*/
#ifndef __HCT_C2K_DTS_SUPPORT__
#define __HCT_C2K_DTS_SUPPORT__  HCT_NO
#endif

#ifndef __HCT_T637_DK_C2_OPPO_MODULE_SUPPORT__
#define __HCT_T637_DK_C2_OPPO_MODULE_SUPPORT__  HCT_NO
#endif


#ifndef __HCT_USB_MTK_OTG_SUPPORT__
#define __HCT_USB_MTK_OTG_SUPPORT__ HCT_NO
#endif


#ifndef  __HCT_DISCRETE_MEMERY_SUPPORT__
#define  __HCT_DISCRETE_MEMERY_SUPPORT__  HCT_NO
#endif


#ifndef __HCT_DUAL_CAMERA_USEDBY_YUV_MODE__
#define __HCT_DUAL_CAMERA_USEDBY_YUV_MODE__  HCT_NO
#endif

#ifndef __HCT_CAMERA_I2C_IS_SAME__
#define __HCT_CAMERA_I2C_IS_SAME__  HCT_NO
#endif

#ifndef __HCT_TYPEC_ACCDET_MIC_SUPPORT__
#define __HCT_TYPEC_ACCDET_MIC_SUPPORT__  HCT_NO
#endif

#ifndef __HCT_OZ115_CHARGER_4200MA__
#define __HCT_OZ115_CHARGER_4200MA__   HCT_NO
#endif

#ifndef __HCT_T976_OZ115_CHARGE_UPDATE_AVGVBAT__
#define __HCT_T976_OZ115_CHARGE_UPDATE_AVGVBAT__    HCT_NO
#endif


#ifndef __HCT_T976_OV8856_SANYING_POWER_SUPPORT__
#define __HCT_T976_OV8856_SANYING_POWER_SUPPORT__ HCT_NO
#endif

///////////////////
/////  next pls do not modify
////////////////////
#ifndef PINMUX_GPIO1__FUNC_GPIO1
#include "mt67xx-pinfunc.h"
#endif

#include "hct_board_dts_config.h"


#ifndef __HCT_GPIO_LCM_POWER_ENN_PINMUX__
#define __HCT_GPIO_LCM_POWER_ENN_PINMUX__ PUNMUX_GPIO_NONE_FUNC_NONE
#endif

#ifndef __HCT_GPIO_LCM_POWER_ENP_PINMUX__
#define __HCT_GPIO_LCM_POWER_ENP_PINMUX__ PUNMUX_GPIO_NONE_FUNC_NONE
#endif

#ifndef __HCT_GPIO_LCM_POWER_DM_PINMUX__
#define __HCT_GPIO_LCM_POWER_DM_PINMUX__ PUNMUX_GPIO_NONE_FUNC_NONE
#endif

#ifndef __HCT_GPIO_LCM_POWER_DP_PINMUX__
#define __HCT_GPIO_LCM_POWER_DP_PINMUX__ PUNMUX_GPIO_NONE_FUNC_NONE
#endif


#endif
