/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>


#include "kd_camera_hw.h"


#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_camera_feature.h"

#include <linux/hct_include/hct_project_all_config.h>


/******************************************************************************
 * Debug configuration
******************************************************************************/
#define PFX "[kd_camera_hw]"
#define PK_DBG_NONE(fmt, arg...)    do {} while (0)
#define PK_DBG_FUNC(fmt, arg...)    pr_debug(PFX fmt, ##arg)

#define DEBUG_CAMERA_HW_K
#define CONTROL_AF_POWER 1
#ifdef DEBUG_CAMERA_HW_K
#define PK_DBG PK_DBG_FUNC
#define PK_ERR(fmt, arg...)         pr_err(fmt, ##arg)
#define PK_XLOG_INFO(fmt, args...) \
		do {    \
		   pr_debug(PFX fmt, ##arg); \
		} while (0)
#else
#define PK_DBG(a, ...)
#define PK_ERR(a, ...)
#define PK_XLOG_INFO(fmt, args...)
#endif


#define IDX_PS_MODE 1
#define IDX_PS_ON   2
#define IDX_PS_OFF  3


#define IDX_PS_CMRST 0
#define IDX_PS_CMPDN 4

#ifndef __HCT_T975_POWER_SUPPORT__
#define __HCT_T975_POWER_SUPPORT__  0
#endif
extern void ISP_MCLK1_EN(BOOL En);
extern void ISP_MCLK2_EN(BOOL En);
extern void ISP_MCLK3_EN(BOOL En);


extern char 	g_MainSensorName[32];
extern char 	g_SubSensorName[32];
unsigned int	g_main_rst_output_when_sensor_disable 	= Vol_Low;
unsigned int	g_main_pwd_output_when_sensor_disable 	= Vol_High;
unsigned int	g_sub_rst_output_when_sensor_disable 	= Vol_Low;
unsigned int	g_sub_pwd_output_when_sensor_disable	= Vol_High;

u32 pinSetIdx = 0;		/* default main sensor */
u32 pinSet[3][8] = {
	/* for main sensor */
	{CAMERA_CMRST_PIN,
	 CAMERA_CMRST_PIN_M_GPIO,	/* mode */
	 GPIO_OUT_ONE,		/* ON state */
	 GPIO_OUT_ZERO,		/* OFF state */
	 CAMERA_CMPDN_PIN,
	 CAMERA_CMPDN_PIN_M_GPIO,
	 GPIO_OUT_ONE,
	 GPIO_OUT_ZERO,
	 },
	/* for sub sensor */
	{CAMERA_CMRST1_PIN,
	 CAMERA_CMRST1_PIN_M_GPIO,
	 GPIO_OUT_ONE,
	 GPIO_OUT_ZERO,
	 CAMERA_CMPDN1_PIN,
	 CAMERA_CMPDN1_PIN_M_GPIO,
	 GPIO_OUT_ONE,
	 GPIO_OUT_ZERO,
	 },
	/* for main2 sensor */
	{CAMERA_CMRST2_PIN,
	 CAMERA_CMRST2_PIN_M_GPIO,
	 GPIO_OUT_ONE,
	 GPIO_OUT_ZERO,
	 CAMERA_CMPDN2_PIN,
	 CAMERA_CMPDN2_PIN_M_GPIO,
	 GPIO_OUT_ONE,
	 GPIO_OUT_ZERO,
	 },
};
#ifndef CONFIG_MTK_LEGACY
#define CUST_AVDD AVDD - AVDD
#define CUST_DVDD DVDD - AVDD
#define CUST_DOVDD DOVDD - AVDD
#define CUST_AFVDD AFVDD - AVDD
#define CUST_SUB_DVDD SUB_DVDD - AVDD
#define CUST_MAIN2_DVDD MAIN2_DVDD - AVDD
#endif


PowerCust PowerCustList = {
	{
		{GPIO_UNSUPPORTED, GPIO_MODE_GPIO, Vol_Low},	/* for AVDD; */
		{GPIO_UNSUPPORTED, GPIO_MODE_GPIO, Vol_Low},	/* for DVDD; */
		{GPIO_UNSUPPORTED, GPIO_MODE_GPIO, Vol_Low},	/* for DOVDD; */
		{GPIO_UNSUPPORTED, GPIO_MODE_GPIO, Vol_Low},	/* for AFVDD; */
#if 0//def CONFIG_MTK_PMIC_CHIP_MT6353
		{GPIO_SUPPORTED, GPIO_MODE_GPIO, Vol_High},	/* for SUB_DVDD; */
#else
		{GPIO_UNSUPPORTED, GPIO_MODE_GPIO, Vol_Low},	/* for SUB_DVDD; */
#endif
		{GPIO_UNSUPPORTED, GPIO_MODE_GPIO, Vol_Low},	/* MAIN2_DVDD; */
		/*{GPIO_SUPPORTED, GPIO_MODE_GPIO, Vol_Low}, */
	 }
};



PowerUp PowerOnList = {
	{
#if defined(IMX219_MIPI_RAW_2LANE)
		{SENSOR_DRVNAME_IMX219_MIPI_RAW_2LANE,
			{
				{AVDD, Vol_2800, 1},
				{DVDD,	Vol_1200, 1},
				{DOVDD,	Vol_1800, 1},
				{SensorMCLK,Vol_High, 1},
				{AFVDD, Vol_2800, 5},
				{PDN,	Vol_Low,  1},
				{RST,	Vol_Low,  1},
				{PDN,	Vol_High, 1},
				{RST,	Vol_High, 1}
			},
		},
#endif

#if defined(S5K4H8_MIPI_RAW)
	 {SENSOR_DRVNAME_S5K4H8_MIPI_RAW,
	  {
	   {SensorMCLK, Vol_High, 0},
	   {DOVDD, Vol_1800, 0},
	   {AVDD, Vol_2800, 0},
	   {DVDD, Vol_1200, 0},
	   {AFVDD, Vol_2800, 5},
	   {PDN, Vol_Low, 4},
	   {PDN, Vol_High, 0},
	   {RST, Vol_Low, 1},
	   {RST, Vol_High, 0},
	   },
	  },
#endif

#if defined(S5K5E2YA_MIPI_RAW)
	 {SENSOR_DRVNAME_S5K5E2YA_MIPI_RAW,
	  {
	   {SensorMCLK, Vol_High, 0},
	   {DOVDD, Vol_1800, 0},
	   {AVDD, Vol_2800, 0},
	   {DVDD, Vol_1200, 0},
	   {AFVDD, Vol_2800, 5},
	   {PDN, Vol_Low, 4},
	   {PDN, Vol_High, 0},
	   {RST, Vol_Low, 1},
	   {RST, Vol_High, 0},
	   },
	  },
#endif
#if defined(S5K2P8_MIPI_RAW)
	 {SENSOR_DRVNAME_S5K2P8_MIPI_RAW,
	  {
	   {SensorMCLK, Vol_High, 0},
	   {DOVDD, Vol_1800, 0},
	   {AVDD, Vol_2800, 0},
	   {DVDD, Vol_1200, 0},
	   {AFVDD, Vol_2800, 5},
	   {PDN, Vol_Low, 4},
	   {PDN, Vol_High, 0},
	   {RST, Vol_Low, 1},
	   {RST, Vol_High, 0},
	   },
	  },
#endif 
#if defined(OV13850_MIPI_RAW)
		{SENSOR_DRVNAME_OV13850_MIPI_RAW,
			{
				{SensorMCLK,Vol_High, 0},
				{DOVDD, Vol_1800, 1},
				{AVDD,  Vol_2800, 1},
				{DVDD,  Vol_1200, 1},
				{AFVDD, Vol_2800, 5},
				{PDN,   Vol_Low,  1},
				{RST,   Vol_Low,  1},
				{PDN,   Vol_High, 0},
				{RST,   Vol_High, 0}
			},
		},
#endif
#if defined(OV13853_MIPI_RAW)
		{SENSOR_DRVNAME_OV13853_MIPI_RAW,
			{
				{SensorMCLK,Vol_High, 0},
				{DOVDD, Vol_1800, 1},
				{AVDD,  Vol_2800, 1},
				{DVDD,  Vol_1200, 1},
				{AFVDD, Vol_2800, 5},
				{PDN,   Vol_Low,  1},
				{RST,   Vol_Low,  1},
				{PDN,   Vol_High, 0},
				{RST,   Vol_High, 0}
			},
		},
#endif
#if defined(OV13850R1A_MIPI_RAW)
		{SENSOR_DRVNAME_OV13850R1A_MIPI_RAW,
			{
				{SensorMCLK,Vol_High, 0},
				{DOVDD, Vol_1800, 1},
				{AVDD,  Vol_2800, 1},
				{DVDD,  Vol_1200, 1},
				{AFVDD, Vol_2800, 5},
				{PDN,   Vol_Low,  1},
				{RST,   Vol_Low,  1},
				{PDN,   Vol_High, 0},
				{RST,   Vol_High, 0}
			},
		},
#endif
#if __HCT_T975_POWER_SUPPORT__  //t975项目中ov13850r2a模组上电需采用以下方式

#if defined(OV13850R2A_MIPI_RAW)
		{SENSOR_DRVNAME_OV13850R2A_MIPI_RAW,
			{
				{SensorMCLK,Vol_High, 0},
				{PDN,   Vol_Low,  1},
				{AVDD,  Vol_2800, 1},
				{DOVDD, Vol_1800, 1},
				{PDN,   Vol_High, 0},
				{DVDD,  Vol_1200, 1},
				{AFVDD, Vol_2800, 5},
				//{PDN,   Vol_Low,  1},
				{RST,   Vol_Low,  1},
				//{PDN,   Vol_High, 0},
				{RST,   Vol_High, 0}
			},
		},
#endif

#else

#if defined(OV13850R2A_MIPI_RAW)
		{SENSOR_DRVNAME_OV13850R2A_MIPI_RAW,
			{
				{SensorMCLK,Vol_High, 0},
				{DOVDD, Vol_1800, 1},
				{AVDD,  Vol_2800, 1},
				{DVDD,  Vol_1200, 1},
				{AFVDD, Vol_2800, 5},
				{PDN,   Vol_Low,  1},
				{RST,   Vol_Low,  1},
				{PDN,   Vol_High, 0},
				{RST,   Vol_High, 0}
			},
		},
#endif

#endif
#if defined(OV5648_MIPI_RAW)
	 {SENSOR_DRVNAME_OV5648_MIPI_RAW,
	  {
	   {SensorMCLK, Vol_High, 0},
	   {DOVDD, Vol_1800, 1},
	   {AVDD, Vol_2800, 1},
	   {DVDD, Vol_1500, 1},
	   {AFVDD, Vol_2800, 5},
	   {PDN, Vol_Low, 1},
	   {RST, Vol_Low, 1},
	   {PDN, Vol_High, 0},
	   {RST, Vol_High, 0}
	   },
	  },
#endif
#if defined(OV16825_MIPI_RAW)
	 {SENSOR_DRVNAME_OV16825_MIPI_RAW,
	  {
	   {SensorMCLK, Vol_High, 0},
	   {DOVDD, Vol_1800, 0},
	   {AVDD, Vol_2800, 0},
	   {DVDD, Vol_1200, 0},
	   {AFVDD, Vol_2800, 5},
	   {PDN, Vol_Low, 0},
	   {RST, Vol_Low, 0},
	   {RST, Vol_High, 0},
	   },
	  },
#endif

#if defined(OV16880_MIPI_RAW)
		{SENSOR_DRVNAME_OV16880_MIPI_RAW,
			{
				{SensorMCLK,Vol_High, 0},
				{DOVDD, Vol_1800, 1},
				{AVDD,  Vol_2800, 1},
				{DVDD,  Vol_1200, 1},
				{AFVDD, Vol_2800, 5},
				{PDN,   Vol_Low,  1},
				{RST,   Vol_Low,  1},
				{PDN,   Vol_High, 0},
				{RST,   Vol_High, 0}
			},
		},
#endif

#if defined(IMX135_MIPI_RAW)
	 {SENSOR_DRVNAME_IMX135_MIPI_RAW,
	  {
	   {SensorMCLK, Vol_High, 0},
	   {AVDD, Vol_2800, 10},
	   {DOVDD, Vol_1800, 10},
	   {DVDD, Vol_1000, 10},
	   {AFVDD, Vol_2800, 5},
	   {PDN, Vol_Low, 0},
	   {PDN, Vol_High, 0},
	   {RST, Vol_Low, 0},
	   {RST, Vol_High, 0}
	   },
	  },
#endif
#if defined(OV8858_MIPI_RAW)
	 {SENSOR_DRVNAME_OV8858_MIPI_RAW,
	  {
	   {SensorMCLK, Vol_High, 0},
	   {PDN, Vol_Low, 0},
	   {RST, Vol_Low, 0},
	   {DOVDD, Vol_1800, 1},
	   {AVDD, Vol_2800, 1},
	   {DVDD, Vol_1200, 5},
	   {AFVDD, Vol_2800, 1},
	   {PDN, Vol_High, 1},
	   {RST, Vol_High, 2}
	   },
	  },
#endif
#if defined(OV8856_MIPI_RAW)
	{SENSOR_DRVNAME_OV8856_MIPI_RAW,
	  {
	   {SensorMCLK, Vol_High, 0},
	   {DOVDD, Vol_1800, 1},
	   {AVDD, Vol_2800, 1},
	   {DVDD, Vol_1200, 1},
	   {AFVDD, Vol_2800, 5},
	   {PDN, Vol_Low, 1},
	   {RST, Vol_Low, 1},
	   {PDN, Vol_High, 0},
	   {RST, Vol_High, 0}
	   },
	  },
#endif

#if __HCT_T976_OV8856_SANYING_POWER_SUPPORT__ //add by huwenchang @20170803 T976 sanying ov8856 use this timing sepuence

#if defined(OV8856_MIPI_RAW_2LANE)
		{SENSOR_DRVNAME_OV8856_MIPI_RAW_2LANE,
			{
	   {SensorMCLK, Vol_High, 0},
	   {DOVDD, Vol_1800, 0},
	   {AVDD, Vol_2800, 0},
	   {DVDD, Vol_1200, 0},
	   {AFVDD, Vol_2800, 2},
	   {PDN, Vol_Low, 0},
	   {PDN, Vol_High, 0},
	   {RST, Vol_Low, 0},
	   {RST, Vol_High, 5},
			},
		},
#endif

#else

#if defined(OV8856_MIPI_RAW_2LANE)
		{SENSOR_DRVNAME_OV8856_MIPI_RAW_2LANE,
			{
	   {SensorMCLK, Vol_High, 0},
	   {DOVDD, Vol_1800, 1},
	   {AVDD, Vol_2800, 1},
	   {DVDD, Vol_1200, 1},
	   {AFVDD, Vol_2800, 5},
	   {PDN, Vol_Low, 1},
	   {RST, Vol_Low, 1},
	   {PDN, Vol_High, 0},
	   {RST, Vol_High, 0}
			},
		},
#endif


#endif

#if defined(IMX258_MIPI_RAW)
	  {SENSOR_DRVNAME_IMX258_MIPI_RAW,
	  {
	   {SensorMCLK, Vol_High, 0},
	   {PDN, Vol_Low, 0},
	   {RST, Vol_Low, 0},
	   {DOVDD, Vol_1800, 0},
	   {AVDD, Vol_2800, 0},
	   {DVDD, Vol_1200, 0},
	   {AFVDD, Vol_2800, 1},
	   {PDN, Vol_High, 0},
	   {RST, Vol_High, 0}
	   },
	  },
#endif
#if defined(IMX278_MIPI_RAW)
		{SENSOR_DRVNAME_IMX278_MIPI_RAW,
			{
				{AVDD, Vol_2800, 1},
				{DVDD,	Vol_1100, 1},
				{DOVDD,	Vol_1800, 1},
				{SensorMCLK,Vol_High, 1},
				{AFVDD, Vol_2800, 5},
				{PDN,	Vol_Low,  1},
				{RST,	Vol_Low,  1},
				{PDN,	Vol_High, 1},
				{RST,	Vol_High, 1}
			},
		},
#endif
#if defined(IMX219_MIPI_RAW)
		{SENSOR_DRVNAME_IMX219_MIPI_RAW,
			{
				{AVDD, Vol_2800, 1},
				{DVDD,	Vol_1200, 1},
				{DOVDD,	Vol_1800, 1},
				{SensorMCLK,Vol_High, 1},
				{AFVDD, Vol_2800, 5},
				{PDN,	Vol_Low,  1},
				{RST,	Vol_Low,  1},
				{PDN,	Vol_High, 1},
				{RST,	Vol_High, 1}
			},
		},
#endif
#if defined(IMX214_MIPI_RAW)
	  {SENSOR_DRVNAME_IMX214_MIPI_RAW,
	  {
	   {SensorMCLK, Vol_High, 0},
	   {AVDD, Vol_2800, 10},
	   {DOVDD, Vol_1800, 10},
	   {DVDD, Vol_1000, 10},
	   {AFVDD, Vol_2800, 5},
	   {PDN, Vol_Low, 0},
	   {PDN, Vol_High, 0},
	   {RST, Vol_Low, 0},
	   {RST, Vol_High, 0}
	   },
	  },
#endif
#if defined(S5K3P3SX_MIPI_RAW)
         {SENSOR_DRVNAME_S5K3P3SX_MIPI_RAW,
          {
           {SensorMCLK, Vol_High, 0},
           {DOVDD, Vol_1800, 0},
           {AVDD, Vol_2800, 0},
           {DVDD, Vol_1200, 0},
           {AFVDD, Vol_2800, 5},
           {PDN, Vol_Low, 4},
           {PDN, Vol_High, 0},
           {RST, Vol_Low, 1},
           {RST, Vol_High, 0},
           },
          },
#endif
#if defined(GC0310_MIPI_YUV)
		{SENSOR_DRVNAME_GC0310_MIPI_YUV,
			{
				{PDN,	Vol_High, 1},
				{RST,   Vol_Low,  1},
#if  __HCT_DUAL_CAMERA_USEDBY_YUV_MODE__
#else
				{DOVDD, Vol_1800, 1},
                                {DVDD,  Vol_1500, 1},
                                {AVDD,  Vol_2800, 1},
                                {AFVDD, Vol_2800, 5},
                                {SensorMCLK,Vol_High, 0},
#endif
				{PDN,	Vol_Low, 0},
				{RST,	Vol_High, 0}
			},
		},
#endif
#if defined(SP0A20_MIPI_YUV)
		{SENSOR_DRVNAME_SP0A20_MIPI_YUV,
			{
				{PDN,	Vol_Low, 10},
#if  __HCT_DUAL_CAMERA_USEDBY_YUV_MODE__
#else
                                {AVDD,  Vol_2800, 5},
                                {DOVDD, Vol_1800, 1},
                                {AFVDD, Vol_2800, 5},
                                {SensorMCLK,Vol_High, 0},
#endif
				{PDN,	Vol_High, 15},
				{PDN,	Vol_Low, 20},
			},
		},
#endif
#if defined(GC2355_MIPI_RAW)
		{SENSOR_DRVNAME_GC2355_MIPI_RAW,
			{
				{PDN,	Vol_High, 1},
				{RST,   Vol_Low,  1},
				{DOVDD,	Vol_1800, 1},
				{DVDD,	Vol_1500, 1},
				{AFVDD, Vol_2800, 5},
				{AVDD,	Vol_2800, 1},
				{SensorMCLK,Vol_High, 0},
				{PDN,	Vol_Low, 0},
				{RST,	Vol_High, 0}
			},
		},
#endif
#if defined(HI553_MIPI_RAW)
		{SENSOR_DRVNAME_HI553_MIPI_RAW,
			{
				{PDN, Vol_Low, 1},
				{RST, Vol_Low, 1},
				{DOVDD, Vol_1800, 1},
				{AVDD, Vol_2800, 1},
				{DVDD, Vol_1200, 10},
				{AFVDD, Vol_2800, 5},
				{PDN, Vol_High, 0},
				{SensorMCLK, Vol_High, 0},
				{RST, Vol_High, 0}
			},
		},
#endif
#if defined(SP0A09_MIPI_RAW)
		{SENSOR_DRVNAME_SP0A09_MIPI_RAW,
			{
				{PDN,	Vol_High, 1},
				{RST,   Vol_Low,  1},
				{DOVDD,	Vol_1800, 1},
				{DVDD,	Vol_1500, 1},//no used
				{AVDD,	Vol_2800, 1},
				{AFVDD, Vol_2800, 5},
				{SensorMCLK,Vol_High, 0},
				{PDN,	Vol_Low, 0},
				{RST,	Vol_High, 0}
			},
		},
#endif
	#ifdef S5K3L8_MIPI_RAW
	 {SENSOR_DRVNAME_S5K3L8_MIPI_RAW,
	  {
	   {SensorMCLK, Vol_High, 0},
	   {DOVDD, Vol_1800, 0},
	   {AVDD, Vol_2800, 0},
	   {DVDD, Vol_1200, 0},
	   {AFVDD, Vol_2800, 5},
	   {PDN, Vol_Low, 4},
	   {PDN, Vol_High, 0},
	   {RST, Vol_Low, 1},
	   {RST, Vol_High, 0},
	   },
	  },
	#endif 
	#ifdef S5K3L8_MIPI_RAW_HLT
	 {SENSOR_DRVNAME_S5K3L8_MIPI_RAW_HLT,
	  {
	   {SensorMCLK, Vol_High, 0},
	   {DOVDD, Vol_1800, 0},
	   {AVDD, Vol_2800, 0},
	   {DVDD, Vol_1200, 0},
	   {AFVDD, Vol_2800, 5},
	   {PDN, Vol_Low, 4},
	   {PDN, Vol_High, 0},
	   {RST, Vol_Low, 1},
	   {RST, Vol_High, 0},
	   },
	  },
	#endif 
#if defined(SP5507_MIPI_RAW)
		{SENSOR_DRVNAME_SP5507_MIPI_RAW,
			{
				{SensorMCLK,Vol_High, 0},
				{DOVDD, Vol_1800, 1},
				{AVDD,  Vol_2800, 1},
				{DVDD,  Vol_1200, 1},
				{AFVDD, Vol_2800, 5},
				{PDN,   Vol_Low,  1},
				{RST,   Vol_Low,  1},
				{PDN,   Vol_High, 0},
				{RST,	Vol_High, 0}
			},
		},
#endif
#if defined(SP5506_MIPI_RAW)
		{SENSOR_DRVNAME_SP5506_MIPI_RAW,
			{
				{SensorMCLK,Vol_High, 0},
				{DOVDD, Vol_1800, 1},
				{AVDD,  Vol_2800, 1},
				{DVDD,  Vol_1200, 1},
				{AFVDD, Vol_2800, 5},
				{PDN,   Vol_Low,  1},
				{RST,   Vol_Low,  1},
				{PDN,   Vol_High, 0},
				{RST,	Vol_High, 0}
			},
		},
#endif
#if defined(SP8407_MIPI_RAW)
	{SENSOR_DRVNAME_SP8407_MIPI_RAW,
	  	{
		   	{SensorMCLK, Vol_High, 0},
		   	{DOVDD, Vol_1800, 1},
		   	{AVDD, Vol_2800, 1},
		  	{DVDD, Vol_1200, 1},
		 	{AFVDD, Vol_2800, 5},
		 	{PDN, Vol_Low, 1},
		   	{RST, Vol_Low, 1},
		   	{PDN, Vol_High, 0},
		   	{RST, Vol_High, 0}
	   	},
	  },
#endif
#if defined(GC5005_MIPI_RAW)
		{SENSOR_DRVNAME_GC5005MIPI_RAW,
			{
				{PDN,	Vol_High, 1},
				{RST,   Vol_Low,  1},
				{DOVDD,	Vol_1800, 1},
				{DVDD,	Vol_1200, 1},
				{AVDD,	Vol_2800, 1},
				{AFVDD, Vol_2800, 5},
				{SensorMCLK,Vol_High, 0},
				{PDN,	Vol_Low, 0},
				{RST,	Vol_High, 0}
			},
		},
#endif
#if defined(GC5024MIPI_RAW)
		{SENSOR_DRVNAME_GC5024MIPI_RAW,
			{
				{PDN,	Vol_High, 10},
				{RST,   Vol_Low,  10},
				{DOVDD,	Vol_1800, 10},
				{DVDD,	Vol_1500, 10},
				{AVDD,	Vol_2800, 10},
				{AFVDD, Vol_2800, 5},
				{SensorMCLK,Vol_High, 10},
				{PDN,	Vol_Low, 10},
				{RST,	Vol_High, 10}
			},
		},
#endif
#if defined(GC5025_MIPI_RAW)
		{SENSOR_DRVNAME_GC5025MIPI_RAW,
			{
				{PDN,	Vol_Low, 1},
				{RST,   Vol_Low,  1},
				{DOVDD,	Vol_1800, 1},
				{DVDD,	Vol_1200, 1},
				{AVDD,	Vol_2800, 1},
				{AFVDD, Vol_2800, 5},
				{SensorMCLK,Vol_High, 0},
				{PDN,	Vol_High, 1},
				{RST,   Vol_High,  1},
			},
		},
#endif
#if defined(GC2365_MIPI_RAW)
		{SENSOR_DRVNAME_GC2365_MIPI_RAW,
			{
				{PDN,	Vol_High, 1},
				{RST,   Vol_Low,  1},
				{DOVDD,	Vol_1800, 1},
				{DVDD,	Vol_1500, 1},
				{AVDD,	Vol_2800, 1},
				{AFVDD, Vol_2800, 5},
				{SensorMCLK,Vol_High, 0},
				{PDN,	Vol_Low, 0},
				{RST,	Vol_High, 0}
			},
		},
#endif
#if defined(GC8024MIPI_RAW)
		{SENSOR_DRVNAME_GC8024MIPI_RAW,
			{
				{PDN,	Vol_High, 1},
				{RST,   Vol_Low,  1},
				{DOVDD,	Vol_1800, 1},
				{DVDD,	Vol_1200, 1},
				{AVDD,	Vol_2800, 1},
				{AFVDD, Vol_2800, 5},
				{SensorMCLK,Vol_High, 0},
				{PDN,	Vol_Low, 0},
				{RST,	Vol_High, 0}
			},
		},
#endif
#if defined(GC8024MIPI_RAW_SUB)
		{SENSOR_DRVNAME_GC8024MIPI_RAW_SUB,
			{
				{PDN,	Vol_High, 1},
				{RST,   Vol_Low,  1},
				{DOVDD,	Vol_1800, 1},
				{DVDD,	Vol_1200, 1},
				{AVDD,	Vol_2800, 1},
				{AFVDD, Vol_2800, 5},
				{SensorMCLK,Vol_High, 0},
				{PDN,	Vol_Low, 0},
				{RST,	Vol_High, 0}
			},
		},
#endif
	#ifdef HI843B_MIPI_RAW
	{SENSOR_DRVNAME_HI843B_MIPI_RAW,
		{
			{PDN, Vol_Low, 1},
			{RST, Vol_Low, 1},
			{DOVDD, Vol_1800, 1},
			{AVDD, Vol_2800, 1},
			{DVDD, Vol_1200, 10},
			{AFVDD, Vol_2800, 5},
			{SensorMCLK, Vol_High, 5},
			{PDN, Vol_High, 5},
			{RST, Vol_High, 5}
		},
	},
	#endif

	#ifdef HI843B_MIPI_RAW_2LANE
	{SENSOR_DRVNAME_HI843B_MIPI_RAW_2LANE,
		{
			{PDN, Vol_Low, 1},
			{RST, Vol_Low, 1},
			{DOVDD, Vol_1800, 1},
			{AVDD, Vol_2800, 1},
			{DVDD, Vol_1200, 10},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_High, 5},
			{SensorMCLK, Vol_High, 5},
			{RST, Vol_High, 5}
		},
	},
	#endif

#ifdef HI846_MIPI_RAW
	{SENSOR_DRVNAME_HI846_MIPI_RAW,
		{
			{PDN, Vol_Low, 1},
			{RST, Vol_Low, 1},
			{DOVDD, Vol_1800, 1},
			{AVDD, Vol_2800, 1},
			{DVDD, Vol_1200, 10},
			{AFVDD, Vol_2800, 5},
			{SensorMCLK, Vol_High, 5},
			{PDN, Vol_High, 5},
			{RST, Vol_High, 5}
		},
	},
#endif

#if defined(MN34152_MIPI_RAW)
		{SENSOR_DRVNAME_MN34152_MIPI_RAW,
			{
				{PDN,   Vol_Low,  1},
				{RST,   Vol_Low,  1},
				{SensorMCLK,Vol_High, 0},
				{DOVDD, Vol_1800, 1},
				{AVDD,  Vol_2800, 1},
				{DVDD,  Vol_1100, 5},
				{AFVDD, Vol_2800, 1},
				{PDN,   Vol_High, 1},
				{RST,   Vol_High, 20}
			},
		},
#endif
#if defined(S5K3L2_MIPI_RAW)
		{SENSOR_DRVNAME_S5K3L2_MIPI_RAW,
			{
				{SensorMCLK,Vol_High, 0},
				{DOVDD, Vol_1800, 0},
				{AVDD,  Vol_2800, 0},
				{DVDD,  Vol_1200, 0},
				{AFVDD, Vol_2800, 5},
				{PDN,   Vol_Low,  4},
				{RST,   Vol_Low,  1},
				{PDN,   Vol_High, 0},
				{RST,   Vol_High, 0},
			},
		},
#endif
#if defined(AR1335_MIPI_RAW)
		{SENSOR_DRVNAME_AR1335_MIPI_RAW,
			{

				{DOVDD,	Vol_1800, 1},
				{DVDD,	Vol_1200, 2},
				{AVDD,	Vol_2800, 1},
				{AFVDD, Vol_2800, 5},
				{PDN,	Vol_Low, 2},
				{RST,   Vol_Low,  2},
				{SensorMCLK,Vol_High, 0},
				{PDN,	Vol_High, 0},
				{RST,	Vol_High, 0}
			},
		},
#endif
#ifdef OV5695_MIPI_RAW
	 {SENSOR_DRVNAME_OV5695_MIPI_RAW,
	  {
	   {SensorMCLK, Vol_High, 0},
	   {PDN, Vol_Low, 1},
	   {RST, Vol_Low, 1},
	   {DOVDD, Vol_1800, 1},
	   {AVDD, Vol_2800, 1},
	   {DVDD, Vol_1200, 1},
	   {AFVDD, Vol_2800, 5},
	   {PDN, Vol_High, 1},
	   {RST, Vol_High, 0}
	   },
	  },
#endif
#if defined(S5K5E8YX_MIPI_RAW)
	{SENSOR_DRVNAME_S5K5E8YX_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_Low, 4},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 1},
			{RST, Vol_High, 0},
		},
	},
#endif

#if defined(HM8040_MIPI_RAW)
         {SENSOR_DRVNAME_HM8040_MIPI_RAW,
                {
                       {DOVDD, Vol_1800, 1},
                       {AVDD,  Vol_2800, 1},
                       {DVDD,  Vol_1200, 1},
                       {AFVDD, Vol_2800, 5},
                       {PDN,   Vol_Low,  1},
                       {RST,   Vol_Low,  1},
                       {SensorMCLK,Vol_High, 0},
                       {PDN,   Vol_High, 0},
                       {RST,   Vol_High, 0}
                 },
         },
#endif
	 /* add new sensor before this line */
	 {NULL,},
	 }
};


#define VOL_2800 2800000
#define VOL_1800 1800000
#define VOL_1500 1500000
#define VOL_1200 1200000
#define VOL_1220 1220000
#define VOL_1000 1000000

/* GPIO Pin control*/
struct platform_device *cam_plt_dev = NULL;
struct pinctrl *camctrl = NULL;
struct pinctrl_state *cam0_pnd_h = NULL;/* main cam */
struct pinctrl_state *cam0_pnd_l = NULL;
struct pinctrl_state *cam0_rst_h = NULL;
struct pinctrl_state *cam0_rst_l = NULL;
struct pinctrl_state *cam1_pnd_h = NULL;/* sub cam */
struct pinctrl_state *cam1_pnd_l = NULL;
struct pinctrl_state *cam1_rst_h = NULL;
struct pinctrl_state *cam1_rst_l = NULL;
struct pinctrl_state *cam2_pnd_h = NULL;/* main2 cam */
struct pinctrl_state *cam2_pnd_l = NULL;
struct pinctrl_state *cam2_rst_h = NULL;
struct pinctrl_state *cam2_rst_l = NULL;
struct pinctrl_state *cam_ldo_vcama_h = NULL;/* for AVDD */
struct pinctrl_state *cam_ldo_vcama_l = NULL;
struct pinctrl_state *cam_ldo_vcamd_h = NULL;/* for DVDD */
struct pinctrl_state *cam_ldo_vcamd_l = NULL;
struct pinctrl_state *cam_ldo_vcamio_h = NULL;/* for DOVDD */
struct pinctrl_state *cam_ldo_vcamio_l = NULL;
struct pinctrl_state *cam_ldo_vcamaf_h = NULL;/* for AFVDD */
struct pinctrl_state *cam_ldo_vcamaf_l = NULL;
struct pinctrl_state *cam_ldo_sub_vcamd_h = NULL;/* for SUB_DVDD */
struct pinctrl_state *cam_ldo_sub_vcamd_l = NULL;
struct pinctrl_state *cam_ldo_main2_vcamd_h = NULL;/* for MAIN2_DVDD */
struct pinctrl_state *cam_ldo_main2_vcamd_l = NULL;
struct pinctrl_state *cam_mipi_switch_en_h = NULL;/* for mipi switch enable */
struct pinctrl_state *cam_mipi_switch_en_l = NULL;
struct pinctrl_state *cam_mipi_switch_sel_h = NULL;/* for mipi switch select */
struct pinctrl_state *cam_mipi_switch_sel_l = NULL;
int has_mipi_switch = 0;

int mtkcam_gpio_init(struct platform_device *pdev)
{
	int ret = 0;
	has_mipi_switch = 1;

	camctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(camctrl)) {
		dev_err(&pdev->dev, "Cannot find camera pinctrl!");
		ret = PTR_ERR(camctrl);
	}
	/*Cam0 Power/Rst Ping initialization */
	cam0_pnd_h = pinctrl_lookup_state(camctrl, "cam0_pnd1");
	if (IS_ERR(cam0_pnd_h)) {
		ret = PTR_ERR(cam0_pnd_h);
		PK_DBG("%s : pinctrl err, cam0_pnd_h\n", __func__);
	}

	cam0_pnd_l = pinctrl_lookup_state(camctrl, "cam0_pnd0");
	if (IS_ERR(cam0_pnd_l)) {
		ret = PTR_ERR(cam0_pnd_l);
		PK_DBG("%s : pinctrl err, cam0_pnd_l\n", __func__);
	}


	cam0_rst_h = pinctrl_lookup_state(camctrl, "cam0_rst1");
	if (IS_ERR(cam0_rst_h)) {
		ret = PTR_ERR(cam0_rst_h);
		PK_DBG("%s : pinctrl err, cam0_rst_h\n", __func__);
	}

	cam0_rst_l = pinctrl_lookup_state(camctrl, "cam0_rst0");
	if (IS_ERR(cam0_rst_l)) {
		ret = PTR_ERR(cam0_rst_l);
		PK_DBG("%s : pinctrl err, cam0_rst_l\n", __func__);
	}

	/*Cam1 Power/Rst Ping initialization */
	cam1_pnd_h = pinctrl_lookup_state(camctrl, "cam1_pnd1");
	if (IS_ERR(cam1_pnd_h)) {
		ret = PTR_ERR(cam1_pnd_h);
		PK_DBG("%s : pinctrl err, cam1_pnd_h\n", __func__);
	}

	cam1_pnd_l = pinctrl_lookup_state(camctrl, "cam1_pnd0");
	if (IS_ERR(cam1_pnd_l)) {
		ret = PTR_ERR(cam1_pnd_l);
		PK_DBG("%s : pinctrl err, cam1_pnd_l\n", __func__);
	}


	cam1_rst_h = pinctrl_lookup_state(camctrl, "cam1_rst1");
	if (IS_ERR(cam1_rst_h)) {
		ret = PTR_ERR(cam1_rst_h);
		PK_DBG("%s : pinctrl err, cam1_rst_h\n", __func__);
	}


	cam1_rst_l = pinctrl_lookup_state(camctrl, "cam1_rst0");
	if (IS_ERR(cam1_rst_l)) {
		ret = PTR_ERR(cam1_rst_l);
		PK_DBG("%s : pinctrl err, cam1_rst_l\n", __func__);
	}
	/*Cam2 Power/Rst Ping initialization */
	cam2_pnd_h = pinctrl_lookup_state(camctrl, "cam2_pnd1");
	if (IS_ERR(cam2_pnd_h)) {
		ret = PTR_ERR(cam2_pnd_h);
		PK_DBG("%s : pinctrl err, cam2_pnd_h\n", __func__);
	}

	cam2_pnd_l = pinctrl_lookup_state(camctrl, "cam2_pnd0");
	if (IS_ERR(cam2_pnd_l)) {
		ret = PTR_ERR(cam2_pnd_l);
		PK_DBG("%s : pinctrl err, cam2_pnd_l\n", __func__);
	}


	cam2_rst_h = pinctrl_lookup_state(camctrl, "cam2_rst1");
	if (IS_ERR(cam2_rst_h)) {
		ret = PTR_ERR(cam2_rst_h);
		PK_DBG("%s : pinctrl err, cam2_rst_h\n", __func__);
	}


	cam2_rst_l = pinctrl_lookup_state(camctrl, "cam2_rst0");
	if (IS_ERR(cam2_rst_l)) {
		ret = PTR_ERR(cam2_rst_l);
		PK_DBG("%s : pinctrl err, cam2_rst_l\n", __func__);
	}

	/*externel LDO enable */
	cam_ldo_vcama_h = pinctrl_lookup_state(camctrl, "cam_ldo_vcama_1");
	if (IS_ERR(cam_ldo_vcama_h)) {
		ret = PTR_ERR(cam_ldo_vcama_h);
		PK_DBG("%s : pinctrl err, cam_ldo_vcama_h\n", __func__);
	}

	cam_ldo_vcama_l = pinctrl_lookup_state(camctrl, "cam_ldo_vcama_0");
	if (IS_ERR(cam_ldo_vcama_l)) {
		ret = PTR_ERR(cam_ldo_vcama_l);
		PK_DBG("%s : pinctrl err, cam_ldo_vcama_l\n", __func__);
	}

	cam_ldo_vcamd_h = pinctrl_lookup_state(camctrl, "cam_ldo_vcamd_1");
	if (IS_ERR(cam_ldo_vcamd_h)) {
		ret = PTR_ERR(cam_ldo_vcamd_h);
		PK_DBG("%s : pinctrl err, cam_ldo_vcamd_h\n", __func__);
	}

	cam_ldo_vcamd_l = pinctrl_lookup_state(camctrl, "cam_ldo_vcamd_0");
	if (IS_ERR(cam_ldo_vcamd_l)) {
		ret = PTR_ERR(cam_ldo_vcamd_l);
		PK_DBG("%s : pinctrl err, cam_ldo_vcamd_l\n", __func__);
	}

	cam_ldo_vcamio_h = pinctrl_lookup_state(camctrl, "cam_ldo_vcamio_1");
	if (IS_ERR(cam_ldo_vcamio_h)) {
		ret = PTR_ERR(cam_ldo_vcamio_h);
		PK_DBG("%s : pinctrl err, cam_ldo_vcamio_h\n", __func__);
	}

	cam_ldo_vcamio_l = pinctrl_lookup_state(camctrl, "cam_ldo_vcamio_0");
	if (IS_ERR(cam_ldo_vcamio_l)) {
		ret = PTR_ERR(cam_ldo_vcamio_l);
		PK_DBG("%s : pinctrl err, cam_ldo_vcamio_l\n", __func__);
	}

	cam_ldo_vcamaf_h = pinctrl_lookup_state(camctrl, "cam_ldo_vcamaf_1");
	if (IS_ERR(cam_ldo_vcamaf_h)) {
		ret = PTR_ERR(cam_ldo_vcamaf_h);
		PK_DBG("%s : pinctrl err, cam_ldo_vcamaf_h\n", __func__);
	}

	cam_ldo_vcamaf_l = pinctrl_lookup_state(camctrl, "cam_ldo_vcamaf_0");
	if (IS_ERR(cam_ldo_vcamaf_l)) {
		ret = PTR_ERR(cam_ldo_vcamaf_l);
		PK_DBG("%s : pinctrl err, cam_ldo_vcamaf_l\n", __func__);
	}

	cam_ldo_sub_vcamd_h = pinctrl_lookup_state(camctrl, "cam_ldo_sub_vcamd_1");
	if (IS_ERR(cam_ldo_sub_vcamd_h)) {
		ret = PTR_ERR(cam_ldo_sub_vcamd_h);
		PK_DBG("%s : pinctrl err, cam_ldo_sub_vcamd_h\n", __func__);
	}

	cam_ldo_sub_vcamd_l = pinctrl_lookup_state(camctrl, "cam_ldo_sub_vcamd_0");
	if (IS_ERR(cam_ldo_sub_vcamd_l)) {
		ret = PTR_ERR(cam_ldo_sub_vcamd_l);
		PK_DBG("%s : pinctrl err, cam_ldo_sub_vcamd_l\n", __func__);
	}

	cam_ldo_main2_vcamd_h = pinctrl_lookup_state(camctrl, "cam_ldo_main2_vcamd_1");
	if (IS_ERR(cam_ldo_main2_vcamd_h)) {
		ret = PTR_ERR(cam_ldo_main2_vcamd_h);
		PK_DBG("%s : pinctrl err, cam_ldo_main2_vcamd_h\n", __func__);
	}

	cam_ldo_main2_vcamd_l = pinctrl_lookup_state(camctrl, "cam_ldo_main2_vcamd_0");
	if (IS_ERR(cam_ldo_main2_vcamd_l)) {
		ret = PTR_ERR(cam_ldo_main2_vcamd_l);
		PK_DBG("%s : pinctrl err, cam_ldo_main2_vcamd_l\n", __func__);
	}

	cam_mipi_switch_en_h = pinctrl_lookup_state(camctrl, "cam_mipi_switch_en_1");
	if (IS_ERR(cam_mipi_switch_en_h)) {
		has_mipi_switch = 0;
		ret = PTR_ERR(cam_mipi_switch_en_h);
		PK_DBG("%s : pinctrl err, cam_mipi_switch_en_h\n", __func__);
	}

	cam_mipi_switch_en_l = pinctrl_lookup_state(camctrl, "cam_mipi_switch_en_0");
	if (IS_ERR(cam_mipi_switch_en_l)) {
		has_mipi_switch = 0;
		ret = PTR_ERR(cam_mipi_switch_en_l);
		PK_DBG("%s : pinctrl err, cam_mipi_switch_en_l\n", __func__);
	}
	cam_mipi_switch_sel_h = pinctrl_lookup_state(camctrl, "cam_mipi_switch_sel_1");
	if (IS_ERR(cam_mipi_switch_sel_h)) {
		has_mipi_switch = 0;
		ret = PTR_ERR(cam_mipi_switch_sel_h);
		PK_DBG("%s : pinctrl err, cam_mipi_switch_sel_h\n", __func__);
	}

	cam_mipi_switch_sel_l = pinctrl_lookup_state(camctrl, "cam_mipi_switch_sel_0");
	if (IS_ERR(cam_mipi_switch_sel_l)) {
		has_mipi_switch = 0;
		ret = PTR_ERR(cam_mipi_switch_sel_l);
		PK_DBG("%s : pinctrl err, cam_mipi_switch_sel_l\n", __func__);
	}
	return ret;
}

int mtkcam_gpio_set(int PinIdx, int PwrType, int Val)
{
	int ret = 0;
	if (IS_ERR(camctrl)) {
		return -1;
	}
	switch (PwrType) {
	case RST:
		if (PinIdx == 0) {
			if (Val == 0 && !IS_ERR(cam0_rst_l))
				pinctrl_select_state(camctrl, cam0_rst_l);
			else if (Val == 1 && !IS_ERR(cam0_rst_h))
				pinctrl_select_state(camctrl, cam0_rst_h);
			else
				PK_ERR("%s : pinctrl err, PinIdx %d, Val %d, RST\n", __func__,PinIdx ,Val);
		} else if (PinIdx == 1) {
			if (Val == 0 && !IS_ERR(cam1_rst_l))
				pinctrl_select_state(camctrl, cam1_rst_l);
			else if (Val == 1 && !IS_ERR(cam1_rst_h))
				pinctrl_select_state(camctrl, cam1_rst_h);
			else
				PK_ERR("%s : pinctrl err, PinIdx %d, Val %d, RST\n", __func__,PinIdx ,Val);
		} else {
			if (Val == 0 && !IS_ERR(cam2_rst_l))
				pinctrl_select_state(camctrl, cam2_rst_l);
			else if (Val == 1 && !IS_ERR(cam2_rst_h))
				pinctrl_select_state(camctrl, cam2_rst_h);
			else
				PK_ERR("%s : pinctrl err, PinIdx %d, Val %d, RST\n", __func__,PinIdx ,Val);
		}
		break;
	case PDN:
		if (PinIdx == 0) {
			if (Val == 0 && !IS_ERR(cam0_pnd_l))
				pinctrl_select_state(camctrl, cam0_pnd_l);
			else if (Val == 1 && !IS_ERR(cam0_pnd_h))
				pinctrl_select_state(camctrl, cam0_pnd_h);
			else
				PK_ERR("%s : pinctrl err, PinIdx %d, Val %d, PDN\n", __func__,PinIdx ,Val);
		} else if (PinIdx == 1) {
			if (Val == 0 && !IS_ERR(cam1_pnd_l))
				pinctrl_select_state(camctrl, cam1_pnd_l);
			else if (Val == 1 && !IS_ERR(cam1_pnd_h))
				pinctrl_select_state(camctrl, cam1_pnd_h);
			else
				PK_ERR("%s : pinctrl err, PinIdx %d, Val %d, PDN\n", __func__,PinIdx ,Val);
		} else {
			if (Val == 0 && !IS_ERR(cam2_pnd_l))
				pinctrl_select_state(camctrl, cam2_pnd_l);
			else if (Val == 1 && !IS_ERR(cam2_pnd_h))
				pinctrl_select_state(camctrl, cam2_pnd_h);
			else
				PK_ERR("%s : pinctrl err, PinIdx %d, Val %d, PDN\n", __func__,PinIdx ,Val);
		}
		break;
	case AVDD:
		if (Val == 0 && !IS_ERR(cam_ldo_vcama_l))
			pinctrl_select_state(camctrl, cam_ldo_vcama_l);
		else if (Val == 1 && !IS_ERR(cam0_rst_h))
			pinctrl_select_state(camctrl, cam_ldo_vcama_h);
		break;
	case DVDD:
		if (Val == 0 && !IS_ERR(cam_ldo_vcamd_l))
			pinctrl_select_state(camctrl, cam_ldo_vcamd_l);
		else if (Val == 1 && !IS_ERR(cam_ldo_vcamd_h))
			pinctrl_select_state(camctrl, cam_ldo_vcamd_h);
		break;
	case DOVDD:
		if (Val == 0 && !IS_ERR(cam_ldo_vcamio_l))
			pinctrl_select_state(camctrl, cam_ldo_vcamio_l);
		else if (Val == 1 && !IS_ERR(cam_ldo_vcamio_h))
			pinctrl_select_state(camctrl, cam_ldo_vcamio_h);
		break;
	case AFVDD:
		if (Val == 0 && !IS_ERR(cam_ldo_vcamaf_l))
			pinctrl_select_state(camctrl, cam_ldo_vcamaf_l);
		else if (Val == 1 && !IS_ERR(cam_ldo_vcamaf_h))
			pinctrl_select_state(camctrl, cam_ldo_vcamaf_h);
		break;
	case SUB_DVDD:
		if (Val == 0 && !IS_ERR(cam_ldo_sub_vcamd_l))
			pinctrl_select_state(camctrl, cam_ldo_sub_vcamd_l);
		else if (Val == 1 && !IS_ERR(cam_ldo_sub_vcamd_h))
			pinctrl_select_state(camctrl, cam_ldo_sub_vcamd_h);
		break;
	case MAIN2_DVDD:
		if (Val == 0 && !IS_ERR(cam_ldo_main2_vcamd_l))
			pinctrl_select_state(camctrl, cam_ldo_main2_vcamd_l);
		else if (Val == 1 && !IS_ERR(cam_ldo_main2_vcamd_h))
			pinctrl_select_state(camctrl, cam_ldo_main2_vcamd_h);
		break;
	default:
		PK_DBG("PwrType(%d) is invalid !!\n", PwrType);
		break;
	};

	/* PK_DBG("PinIdx(%d) PwrType(%d) val(%d)\n", PinIdx, PwrType, Val); */

	return ret;
}

static void config_sensor_rst_pwd(char *currSensorName)
{
	int pwListIdx,pwIdx;
	for(pwListIdx=0 ; pwListIdx<16; pwListIdx++)
	{
		//currSensorName
		if(currSensorName && (PowerOnList.PowerSeq[pwListIdx].SensorName!=NULL) && (0 == strcmp(PowerOnList.PowerSeq[pwListIdx].SensorName,currSensorName)))
		{
			for(pwIdx=0;pwIdx<12;pwIdx++)
			{  
				if(PowerOnList.PowerSeq[pwListIdx].PowerInfo[pwIdx].PowerType == RST)
				{
					if(pinSetIdx==0)
						g_main_rst_output_when_sensor_disable = PowerOnList.PowerSeq[pwListIdx].PowerInfo[pwIdx].Voltage;
					else
						g_sub_rst_output_when_sensor_disable = PowerOnList.PowerSeq[pwListIdx].PowerInfo[pwIdx].Voltage;
					break;
				}
			}
			for(pwIdx=0;pwIdx<12;pwIdx++)
			{  
				if(PowerOnList.PowerSeq[pwListIdx].PowerInfo[pwIdx].PowerType == PDN)
				{
					if(pinSetIdx==0)
						g_main_pwd_output_when_sensor_disable = PowerOnList.PowerSeq[pwListIdx].PowerInfo[pwIdx].Voltage;
					else
						g_sub_pwd_output_when_sensor_disable = PowerOnList.PowerSeq[pwListIdx].PowerInfo[pwIdx].Voltage;
					break;
				}
			}
		}
	}

	for(pwListIdx=0 ; pwListIdx<16; pwListIdx++)
	{
		//g_MainSensorName
		if( (PowerOnList.PowerSeq[pwListIdx].SensorName!=NULL) && (0 == strcmp(PowerOnList.PowerSeq[pwListIdx].SensorName,g_MainSensorName)))
		{
			for(pwIdx=0;pwIdx<12;pwIdx++)
			{  
				if(PowerOnList.PowerSeq[pwListIdx].PowerInfo[pwIdx].PowerType == RST)
				{
					g_main_rst_output_when_sensor_disable = PowerOnList.PowerSeq[pwListIdx].PowerInfo[pwIdx].Voltage;
					break;
				}
			}
			for(pwIdx=0;pwIdx<12;pwIdx++)
			{  
				if(PowerOnList.PowerSeq[pwListIdx].PowerInfo[pwIdx].PowerType == PDN)
				{
					g_main_pwd_output_when_sensor_disable = PowerOnList.PowerSeq[pwListIdx].PowerInfo[pwIdx].Voltage;
					break;
				}
			}
		}
		//g_SubSensorName
		if( (PowerOnList.PowerSeq[pwListIdx].SensorName!=NULL) && (0 == strcmp(PowerOnList.PowerSeq[pwListIdx].SensorName,g_SubSensorName)))
		{
			for(pwIdx=0;pwIdx<12;pwIdx++)
			{  
				if(PowerOnList.PowerSeq[pwListIdx].PowerInfo[pwIdx].PowerType == RST)
				{
					g_sub_rst_output_when_sensor_disable = PowerOnList.PowerSeq[pwListIdx].PowerInfo[pwIdx].Voltage;
					break;
				}
			}
			for(pwIdx=0;pwIdx<12;pwIdx++)
			{  
				if(PowerOnList.PowerSeq[pwListIdx].PowerInfo[pwIdx].PowerType == PDN)
				{
					g_sub_pwd_output_when_sensor_disable = PowerOnList.PowerSeq[pwListIdx].PowerInfo[pwIdx].Voltage;
					break;
				}
			}
		}
	}
	PK_DBG("[imgsensor_poweron] config_sensor_rst_pwd mainrst=%d, mainpwd=%d, subrst=%d, subpwd=%d\n",g_main_rst_output_when_sensor_disable, 
        g_main_pwd_output_when_sensor_disable, g_sub_rst_output_when_sensor_disable, g_sub_pwd_output_when_sensor_disable); 
	
      mtkcam_gpio_set(0, RST,g_main_rst_output_when_sensor_disable);
      mtkcam_gpio_set(0, PDN,g_main_pwd_output_when_sensor_disable);
  
      mtkcam_gpio_set(1, RST,g_sub_rst_output_when_sensor_disable);
      mtkcam_gpio_set(1, PDN,g_sub_pwd_output_when_sensor_disable);
  


}

#if 0
static void config_subsensor_rst_pwd_when_nosub(char *currSensorName)
{
	int pwListIdx,pwIdx;
	if((0==strcmp(g_SubSensorName,KDIMGSENSOR_NOSENSOR)) && (pinSetIdx==0))
	{
	       
		for(pwListIdx=0 ; pwListIdx<16; pwListIdx++)
		{
			//currSensorName
			if(currSensorName && (PowerOnList.PowerSeq[pwListIdx].SensorName!=NULL) && (0 == strcmp(PowerOnList.PowerSeq[pwListIdx].SensorName,currSensorName)))
			{
				for(pwIdx=0;pwIdx<12;pwIdx++)
				{  
					if(PowerOnList.PowerSeq[pwListIdx].PowerInfo[pwIdx].PowerType == RST)
					{
						g_sub_rst_output_when_sensor_disable = PowerOnList.PowerSeq[pwListIdx].PowerInfo[pwIdx].Voltage;
						break;
					}
				}
				for(pwIdx=0;pwIdx<12;pwIdx++)
				{  
					if(PowerOnList.PowerSeq[pwListIdx].PowerInfo[pwIdx].PowerType == PDN)
					{
						g_sub_pwd_output_when_sensor_disable = PowerOnList.PowerSeq[pwListIdx].PowerInfo[pwIdx].Voltage;
						break;
					}
				}
			}
		
		}
		PK_DBG("[imgsensor_poweron] config_subsensor_rst_pwd_when_nosub subrst=%d, subpwd=%d\n",g_sub_rst_output_when_sensor_disable, g_sub_pwd_output_when_sensor_disable);	

              mtkcam_gpio_set(1, RST,g_sub_rst_output_when_sensor_disable);
              mtkcam_gpio_set(1, PDN,g_sub_pwd_output_when_sensor_disable);


	}
}
#endif

BOOL hwpoweron(PowerInformation pwInfo, char *mode_name)
{
	if (pwInfo.PowerType == AVDD) {
		if (PowerCustList.PowerCustInfo[CUST_AVDD].Gpio_Pin == GPIO_UNSUPPORTED) {
			if (TRUE != _hwPowerOn(pwInfo.PowerType, pwInfo.Voltage)) {
				PK_ERR("[CAMERA SENSOR] Fail to enable analog power\n");
				return FALSE;
			}
		} else {
			if (mtkcam_gpio_set(pinSetIdx, pwInfo.PowerType, PowerCustList.PowerCustInfo[CUST_AVDD].Voltage)) {
				PK_ERR("[CAMERA CUST_AVDD] set gpio failed!!\n");
			}
		}
	} else if (pwInfo.PowerType == DVDD) {
		if (pinSetIdx == 2) {
			if (PowerCustList.PowerCustInfo[CUST_MAIN2_DVDD].Gpio_Pin == GPIO_UNSUPPORTED) {
				/*PK_DBG("[CAMERA SENSOR] main2 camera VCAM_D power on");*/
				if (pwInfo.Voltage == Vol_1200) {
					pwInfo.Voltage = Vol_1220;
					PK_DBG("[CAMERA SENSOR] Main2 camera VCAM_D power 1.2V to 1.22V\n");
				}
				if (TRUE != _hwPowerOn(MAIN2_DVDD, pwInfo.Voltage)) {
					PK_ERR("[CAMERA SENSOR] Fail to enable digital power\n");
					return FALSE;
				}
			} else {
				if (mtkcam_gpio_set(pinSetIdx, MAIN2_DVDD, PowerCustList.PowerCustInfo[CUST_MAIN2_DVDD].Voltage)) {
					PK_ERR("[CAMERA CUST_MAIN2_DVDD] set gpio failed!!\n");
				}
			}
		} else if (pinSetIdx == 1) {
/****
mtk 原来的前摄 DVDD 上电没有实际的动作,
前后摄是一路电,将后摄的上电移过来
**/
#if 0
			if (PowerCustList.PowerCustInfo[CUST_SUB_DVDD].Gpio_Pin == GPIO_UNSUPPORTED) {
				/* PK_DBG("[CAMERA SENSOR] Sub camera VCAM_D power on"); */
				if (pwInfo.Voltage == Vol_1200) {
					pwInfo.Voltage = Vol_1220;
					PK_DBG("[CAMERA SENSOR] Sub camera VCAM_D power 1.2V to 1.22V\n");
				}
				if (TRUE != _hwPowerOn(SUB_DVDD, pwInfo.Voltage)) {
					PK_ERR("[CAMERA SENSOR] Fail to enable digital power\n");
					return FALSE;
				}
			} else {
				if (mtkcam_gpio_set(pinSetIdx, SUB_DVDD, PowerCustList.PowerCustInfo[CUST_SUB_DVDD].Voltage)) {
					PK_ERR("[CAMERA CUST_SUB_DVDD] set gpio failed!!\n");
#endif
			if (PowerCustList.PowerCustInfo[CUST_DVDD].Gpio_Pin == GPIO_UNSUPPORTED) {
				/* PK_DBG("[CAMERA SENSOR] Sub camera VCAM_D power on"); */
				PK_ERR("GPIO_UNSUPPORTED\n");
				if (TRUE != _hwPowerOn(pwInfo.PowerType, pwInfo.Voltage)) {
					PK_ERR("[CAMERA SENSOR] Fail to enable digital power\n");
					return FALSE;
				}
			} else {
				PK_ERR("GPIO_yes\n");
				if (mtkcam_gpio_set(pinSetIdx, pwInfo.PowerType, PowerCustList.PowerCustInfo[CUST_DVDD].Voltage)) {
					PK_ERR("[CAMERA CUST_DVDD] set gpio failed!!\n");
				}
			}
		} else {
			if (PowerCustList.PowerCustInfo[CUST_DVDD].Gpio_Pin == GPIO_UNSUPPORTED) {
				/* PK_DBG("[CAMERA SENSOR] Main camera VCAM_D power on"); */
				if (TRUE != _hwPowerOn(pwInfo.PowerType, pwInfo.Voltage)) {
					PK_ERR("[CAMERA SENSOR] Fail to enable digital power\n");
					return FALSE;
				}
			} else {
				if (mtkcam_gpio_set(pinSetIdx, pwInfo.PowerType, PowerCustList.PowerCustInfo[CUST_DVDD].Voltage)) {
					PK_ERR("[CAMERA CUST_DVDD] set gpio failed!!\n");
				}
			}
		}
	} else if (pwInfo.PowerType == DOVDD) {
		if (PowerCustList.PowerCustInfo[CUST_DOVDD].Gpio_Pin == GPIO_UNSUPPORTED) {
			if (TRUE != _hwPowerOn(pwInfo.PowerType, pwInfo.Voltage)) {
				PK_ERR("[CAMERA SENSOR] Fail to enable io power\n");
				return FALSE;
			}
		} else {
			if (mtkcam_gpio_set(pinSetIdx, pwInfo.PowerType, PowerCustList.PowerCustInfo[CUST_DOVDD].Voltage)) {
					PK_ERR("[CAMERA CUST_DOVDD] set gpio failed!!\n");
			}

		}
	} else if (pwInfo.PowerType == AFVDD) {
#if CONTROL_AF_POWER
		if (PowerCustList.PowerCustInfo[CUST_AFVDD].Gpio_Pin == GPIO_UNSUPPORTED) {
			if (TRUE != _hwPowerOn(pwInfo.PowerType, pwInfo.Voltage)) {
				PK_ERR("[CAMERA SENSOR] Fail to enable af power\n");
				return FALSE;
			}
		} else {
			if (mtkcam_gpio_set(pinSetIdx, pwInfo.PowerType, PowerCustList.PowerCustInfo[CUST_AFVDD].Voltage)) {
					PK_ERR("[CAMERA CUST_AFVDD] set gpio failed!!\n");
			}
		}
#endif
	} else if (pwInfo.PowerType == PDN) {
		PK_DBG("%s hwPowerOn: PDN %d \n",pinSetIdx?"SubCam-":"MainCam", pwInfo.Voltage);

             mtkcam_gpio_set(pinSetIdx, PDN ,pwInfo.Voltage);

	} else if (pwInfo.PowerType == RST) {
		PK_DBG("%s hwPowerOn: RST %d \n",pinSetIdx?"SubCam-":"MainCam",pwInfo.Voltage);

             mtkcam_gpio_set(pinSetIdx, RST,pwInfo.Voltage);
		
	} else if (pwInfo.PowerType == SensorMCLK) {
		if (pinSetIdx == 0) {
			PK_DBG("Sensor MCLK1 On");
			ISP_MCLK1_EN(TRUE);
		} else {
			PK_DBG("Sensor MCLK2 On");
			ISP_MCLK2_EN(TRUE);
		}
	} else {
	}
	if (pwInfo.Delay > 0)
		mdelay(pwInfo.Delay);
	return TRUE;
}



BOOL hwpowerdown(PowerInformation pwInfo, char *mode_name)
{
	if (pwInfo.PowerType == AVDD) {
		if (PowerCustList.PowerCustInfo[CUST_AVDD].Gpio_Pin == GPIO_UNSUPPORTED) {
			if (TRUE != _hwPowerDown(pwInfo.PowerType)) {
				PK_ERR("[CAMERA SENSOR] Fail to disable analog power\n");
				return FALSE;
			}
		} else {
			if (mtkcam_gpio_set(pinSetIdx, AVDD, 1-PowerCustList.PowerCustInfo[CUST_AVDD].Voltage)) {
					PK_ERR("[CAMERA CUST_AVDD] set gpio failed!!\n");/* 1-voltage for reverse*/
			}
		}
	} else if (pwInfo.PowerType == DVDD) {
		if (pinSetIdx == 2) {
			if (PowerCustList.PowerCustInfo[CUST_MAIN2_DVDD].Gpio_Pin == GPIO_UNSUPPORTED) {
				if (TRUE != _hwPowerDown(MAIN2_DVDD)) {
					PK_ERR("[CAMERA SENSOR] Fail to disable main2 digital power\n");
					return FALSE;
				}
			} else {
				if (mtkcam_gpio_set(pinSetIdx, MAIN2_DVDD, 1-PowerCustList.PowerCustInfo[CUST_MAIN2_DVDD].Voltage)) {
					PK_ERR("[CAMERA CUST_MAIN2_DVDD] off set gpio failed!!\n");
				}
			}
		} else if (pinSetIdx == 1) {
			if (PowerCustList.PowerCustInfo[CUST_DVDD].Gpio_Pin == GPIO_UNSUPPORTED) {
//				if (TRUE != _hwPowerDown(SUB_DVDD)) {
				if (TRUE != _hwPowerDown(DVDD)) {
					PK_ERR("[CAMERA SENSOR] Fail to enable sub digital power\n");
					return FALSE;
				}
			} else {
//				if (mtkcam_gpio_set(pinSetIdx, SUB_DVDD, 1-PowerCustList.PowerCustInfo[CUST_SUB_DVDD].Voltage)) {
				if (mtkcam_gpio_set(pinSetIdx, DVDD, 1-PowerCustList.PowerCustInfo[CUST_SUB_DVDD].Voltage)) {
					PK_ERR("[CAMERA CUST_SUB_DVDD] off set gpio failed!!\n");
				}
			}
		} else {
			if (PowerCustList.PowerCustInfo[CUST_DVDD].Gpio_Pin == GPIO_UNSUPPORTED) {
				if (TRUE != _hwPowerDown(DVDD)) {
					PK_ERR("[CAMERA SENSOR] Fail to disable main digital power\n");
					return FALSE;
				}
			} else {
				if (mtkcam_gpio_set(pinSetIdx, DVDD, 1-PowerCustList.PowerCustInfo[CUST_DVDD].Voltage)) {
					PK_ERR("[CAMERA CUST_DVDD] off set gpio failed!!\n");
				}
			}
		}

	} else if (pwInfo.PowerType == DOVDD) {
		if (PowerCustList.PowerCustInfo[CUST_DOVDD].Gpio_Pin == GPIO_UNSUPPORTED) {
			if (TRUE != _hwPowerDown(DOVDD)) {
				PK_ERR("[CAMERA SENSOR] Fail to disable io power\n");
				return FALSE;
			}
		} else {
			if (mtkcam_gpio_set(pinSetIdx, DOVDD, 1-PowerCustList.PowerCustInfo[CUST_DOVDD].Voltage)) {
				PK_ERR("[CAMERA CUST_AVDD] set gpio failed!!\n");/* 1-voltage for reverse*/
			}
		}
	} else if (pwInfo.PowerType == AFVDD) {
#if CONTROL_AF_POWER
		if (PowerCustList.PowerCustInfo[CUST_AFVDD].Gpio_Pin == GPIO_UNSUPPORTED) {
			if (TRUE != _hwPowerDown(AFVDD)) {
				PK_ERR("[CAMERA SENSOR] Fail to disable af power\n");
				return FALSE;
			}
		} else {
			if (mtkcam_gpio_set(pinSetIdx, AFVDD, 1-PowerCustList.PowerCustInfo[CUST_AFVDD].Voltage)) {
				PK_ERR("[CAMERA CUST_AFVDD] set gpio failed!!\n");/* 1-voltage for reverse*/
			}
		}
#endif
	} else if (pwInfo.PowerType == PDN) {
		PK_DBG("hwPowerDown: PDN %d \n",pwInfo.Voltage);
            mtkcam_gpio_set(pinSetIdx, PDN ,pwInfo.Voltage);
        
	} else if (pwInfo.PowerType == RST) {
		PK_DBG("hwPowerDown: RST %d \n",pwInfo.Voltage);
             mtkcam_gpio_set(pinSetIdx, RST,pwInfo.Voltage);

	} else if (pwInfo.PowerType == SensorMCLK) {
		if (pinSetIdx == 0) {
			ISP_MCLK1_EN(FALSE);
		} else {
			ISP_MCLK2_EN(FALSE);
		}
	} else {
	}
	return TRUE;
}




int kdCISModulePowerOn(CAMERA_DUAL_CAMERA_SENSOR_ENUM SensorIdx, char *currSensorName, BOOL On,
		       char *mode_name)
{

	int pwListIdx, pwIdx;
	BOOL sensorInPowerList = KAL_FALSE;

	if (DUAL_CAMERA_MAIN_SENSOR == SensorIdx) {
		pinSetIdx = 0;
	} else if (DUAL_CAMERA_SUB_SENSOR == SensorIdx) {
		pinSetIdx = 1;
	} else if (DUAL_CAMERA_MAIN_2_SENSOR == SensorIdx) {
		pinSetIdx = 2;
	}
	/* power ON */
	if (On) {
		PK_DBG("kdCISModulePowerOn -on:currSensorName=%s pinSetIdx=%d\n", currSensorName, pinSetIdx);


    /* MIPI SWITCH */
	if(has_mipi_switch){
		if (DUAL_CAMERA_MAIN_SENSOR == SensorIdx) {
			pinctrl_select_state(camctrl, cam_mipi_switch_en_h);
		} else if (DUAL_CAMERA_SUB_SENSOR == SensorIdx) {
			pinctrl_select_state(camctrl, cam_mipi_switch_en_l);
			pinctrl_select_state(camctrl, cam_mipi_switch_sel_h);

		} else if (DUAL_CAMERA_MAIN_2_SENSOR == SensorIdx) {
			pinctrl_select_state(camctrl, cam_mipi_switch_en_l);
			pinctrl_select_state(camctrl, cam_mipi_switch_sel_l);
		}
	}

		for (pwListIdx = 0; pwListIdx < 16; pwListIdx++) {
			if (currSensorName && (PowerOnList.PowerSeq[pwListIdx].SensorName != NULL) && (0 == strcmp(PowerOnList.PowerSeq[pwListIdx].SensorName,   currSensorName))) {
				PK_DBG("kdCISModulePowerOn  powerOn get in--- \n");
				PK_DBG("sensorIdx:%d\n", pwListIdx);

				sensorInPowerList = KAL_TRUE;

				for (pwIdx = 0; pwIdx < 10; pwIdx++) 
				{
					if (PowerOnList.PowerSeq[pwListIdx].PowerInfo[pwIdx].PowerType != VDD_None) 
					{
						if (hwpoweron(PowerOnList.PowerSeq[pwListIdx].PowerInfo[pwIdx], mode_name) == FALSE)
							goto _kdCISModulePowerOn_exit_;
					} else {
						PK_DBG("pwIdx=%d\n", pwIdx);
						break;
					}
				}
				break;
			} else if (PowerOnList.PowerSeq[pwListIdx].SensorName == NULL) {
				break;
			} else {
			}
		}
#if 0
		/* Temp solution: default power on/off sequence */
		if (KAL_FALSE == sensorInPowerList) {
			PK_DBG("Default power on sequence");

			if (pinSetIdx == 0) {
				ISP_MCLK1_EN(1);
			} else if (pinSetIdx == 1) {
				ISP_MCLK2_EN(1);
			}
			/* First Power Pin low and Reset Pin Low */
			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN]) {
				if (mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN], pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_MODE])) {
					PK_DBG("[CAMERA LENS] set gpio mode failed!! (CMPDN)\n");
				}
				if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN], GPIO_DIR_OUT)) {
					PK_DBG("[CAMERA LENS] set gpio dir failed!! (CMPDN)\n");
				}
				if (mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN], pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_OFF])) {
					PK_DBG("[CAMERA LENS] set gpio failed!! (CMPDN)\n");
				}
			}

			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
				if (0 == pinSetIdx) {
#ifndef CONFIG_MTK_MT6306_SUPPORT
					if (mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST], pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_MODE])) {
						PK_DBG("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");
					}
					if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)) {
						PK_DBG("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");
					}
					if (mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST], pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF])) {
						PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");
					}
#else
					if (mt6306_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)) {
						PK_DBG("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");
					}
					if (mt6306_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST], pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF])) {
						PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");
					}
#endif
				} else {
					if (mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST], pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_MODE])) {
						PK_DBG("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");
					}
					if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)) {
						PK_DBG("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");
					}
					if (mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST], pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF])) {
						PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");
					}
				}
			}
			/* VCAM_IO */
			if (TRUE != hwPowerOn(CAMERA_POWER_VCAM_D2, VOL_1800*1000, mode_name)) {
				PK_DBG("[CAMERA SENSOR] Fail to enable digital power (VCAM_IO), power id = %d\n", CAMERA_POWER_VCAM_D2);
				goto _kdCISModulePowerOn_exit_;
			}
			/* VCAM_A */
			if (TRUE != hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800*1000, mode_name)) {
				PK_DBG("[CAMERA SENSOR] Fail to enable analog power (VCAM_A), power id = %d\n", CAMERA_POWER_VCAM_A);
				goto _kdCISModulePowerOn_exit_;
			}

			if (TRUE != hwPowerOn(CAMERA_POWER_VCAM_D, VOL_1800*1000, mode_name)) {
				PK_DBG("[CAMERA SENSOR] Fail to enable digital power (VCAM_D), power id = %d\n", CAMERA_POWER_VCAM_D);
				goto _kdCISModulePowerOn_exit_;
			}
			/* AF_VCC */
			if (TRUE != hwPowerOn(CAMERA_POWER_VCAM_A2, VOL_2800*1000, mode_name)) {
				PK_DBG("[CAMERA SENSOR] Fail to enable analog power (VCAM_AF), power id = %d\n", CAMERA_POWER_VCAM_A2);
				goto _kdCISModulePowerOn_exit_;
			}

			mdelay(5);

			/* enable active sensor */
			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN]) {
				if (mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN], pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_MODE])) {
					PK_DBG("[CAMERA LENS] set gpio mode failed!! (CMPDN)\n");
				}
				if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN], GPIO_DIR_OUT)) {
					PK_DBG("[CAMERA LENS] set gpio dir failed!! (CMPDN)\n");
				}
				if (mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN], pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_ON])) {
					PK_DBG("[CAMERA LENS] set gpio failed!! (CMPDN)\n");
				}
			}

			mdelay(1);

			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
				if (0 == pinSetIdx) {
#ifndef CONFIG_MTK_MT6306_SUPPORT
					if (mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST], pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_MODE])) {
						PK_DBG("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");
					}
					if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)) {
						PK_DBG("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");
					}
					if (mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST], pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_ON])) {
						PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");
					}
#else
					if (mt6306_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)) {
						PK_DBG("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");
					}
					if (mt6306_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST], pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_ON])) {
						PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");
					}
#endif

				} else {
					if (mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST], pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_MODE])) {
						PK_DBG("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");
					}
					if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)) {
						PK_DBG("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");
					}
					if (mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST], pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_ON])) {
						PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");
					}
				}
			}




		}
		/*
		   if(pinSetIdx==0)
		   for(;;)
		   {}
		 */
		/*
		   if(pinSetIdx==1)
		   for(;;)
		   {}
		 */
 #endif
	} else {		/* power OFF */
		if(has_mipi_switch){
			pinctrl_select_state(camctrl, cam_mipi_switch_en_h);
		}
		for (pwListIdx = 0; pwListIdx < 16; pwListIdx++) {
			if (currSensorName && (PowerOnList.PowerSeq[pwListIdx].SensorName != NULL) && (0 == strcmp(PowerOnList.PowerSeq[pwListIdx].SensorName,   currSensorName))) {
				PK_DBG("kdCISModulePowerOn -off:currSensorName=%s pinSetIdx=%d\n", currSensorName, pinSetIdx);

				sensorInPowerList = KAL_TRUE;
				for (pwIdx = 9; pwIdx >= 0; pwIdx--) 
				{
					if (PowerOnList.PowerSeq[pwListIdx].PowerInfo[pwIdx].PowerType != VDD_None) 
					{
						if (hwpowerdown(PowerOnList.PowerSeq[pwListIdx].PowerInfo[pwIdx], mode_name) == FALSE)
							goto _kdCISModulePowerOn_exit_;
						if (pwIdx > 0) 
						{
							if (PowerOnList.PowerSeq[pwListIdx].PowerInfo[pwIdx - 1].Delay > 0)
								mdelay(PowerOnList.PowerSeq[pwListIdx].PowerInfo[pwIdx - 1].Delay);
						}
					}
					else
					{
						PK_DBG("pwIdx=%d\n", pwIdx);
					}
				}
			} else if (PowerOnList.PowerSeq[pwListIdx].SensorName == NULL) {
				break;
			} else {
			}
		}
		config_sensor_rst_pwd(currSensorName);
		
#if 0
		/* Temp solution: default power on/off sequence */
		if (KAL_FALSE == sensorInPowerList) {
			PK_DBG("Default power off sequence");

			if (pinSetIdx == 0) {
				ISP_MCLK1_EN(0);
			} else if (pinSetIdx == 1) {
				ISP_MCLK2_EN(0);
			}
			/* Set Power Pin low and Reset Pin Low */
			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN]) {
				if (mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN], pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_MODE])) {
					PK_DBG("[CAMERA LENS] set gpio mode failed!! (CMPDN)\n");
				}
				if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN], GPIO_DIR_OUT)) {
					PK_DBG("[CAMERA LENS] set gpio dir failed!! (CMPDN)\n");
				}
				if (mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN], pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_OFF])) {
					PK_DBG("[CAMERA LENS] set gpio failed!! (CMPDN)\n");
				}
			}


			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
				if (0 == pinSetIdx) {
#ifndef CONFIG_MTK_MT6306_SUPPORT
					if (mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST], pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_MODE])) {
						PK_DBG("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");
					}
					if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)) {
						PK_DBG("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");
					}
					if (mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST], pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF])) {
						PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");
					}
#else
					if (mt6306_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)) {
						PK_DBG("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");
					}
					if (mt6306_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST], pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF])) {
						PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");
					}
#endif
				} else {
					if (mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST], pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_MODE])) {
						PK_DBG("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");
					}
					if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)) {
						PK_DBG("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");
					}
					if (mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST], pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF])) {
						PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");
					}
				}
			}


			if (TRUE != hwPowerDown(CAMERA_POWER_VCAM_D, mode_name)) {
				PK_DBG("[CAMERA SENSOR] Fail to OFF core power (VCAM_D), power id = %d\n", CAMERA_POWER_VCAM_D);
				goto _kdCISModulePowerOn_exit_;
			}
			/* VCAM_A */
			if (TRUE != hwPowerDown(CAMERA_POWER_VCAM_A, mode_name)) {
				PK_DBG("[CAMERA SENSOR] Fail to OFF analog power (VCAM_A), power id= (%d)\n", CAMERA_POWER_VCAM_A);
				/* return -EIO; */
				goto _kdCISModulePowerOn_exit_;
			}
			/* VCAM_IO */
			if (TRUE != hwPowerDown(CAMERA_POWER_VCAM_D2, mode_name)) {
				PK_DBG("[CAMERA SENSOR] Fail to OFF digital power (VCAM_IO), power id = %d\n", CAMERA_POWER_VCAM_D2);
				/* return -EIO; */
				goto _kdCISModulePowerOn_exit_;
			}
			/* AF_VCC */
			if (TRUE != hwPowerDown(CAMERA_POWER_VCAM_A2, mode_name)) {
				PK_DBG("[CAMERA SENSOR] Fail to OFF AF power (VCAM_AF), power id = %d\n", CAMERA_POWER_VCAM_A2);
				/* return -EIO; */
				goto _kdCISModulePowerOn_exit_;
			}







		}
#endif
	}			/*  */

	return 0;

_kdCISModulePowerOn_exit_:
	return -EIO;
}


EXPORT_SYMBOL(kdCISModulePowerOn);


/* !-- */
/*  */
