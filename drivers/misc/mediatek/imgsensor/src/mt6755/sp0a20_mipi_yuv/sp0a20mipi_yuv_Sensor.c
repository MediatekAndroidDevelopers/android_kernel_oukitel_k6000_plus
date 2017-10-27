/*****************************************************************************
 *
 * Filename:
 * ---------
 *   SP0A20yuv_Sensor.c
 *
 * Project:
 * --------
 *   MAUI
 *
 * Description:
 * ------------
 *   Image sensor driver function
 *   V1.2.3
 *
 * Author:
 * -------
 *   Leo
 *
 *=============================================================
 *             HISTORY
 * Below this line, this part is controlled by GCoreinc. DO NOT MODIFY!!
 *------------------------------------------------------------------------------
 * $Log$
 * 2012.02.29  kill bugs
 *   
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by GCoreinc. DO NOT MODIFY!!
 *=============================================================
 ******************************************************************************/
#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>

#include <linux/slab.h>
#include "kd_camera_typedef.h"
#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "kd_camera_feature.h"

#include "sp0a20mipi_yuv_Sensor.h"
#include <linux/hct_include/hct_project_all_config.h>


static DEFINE_SPINLOCK(SP0A20_drv_lock);


#define SP0A20YUV_DEBUG

#ifdef SP0A20YUV_DEBUG
#define SENSORDB printk
#else
#define SENSORDB(x,...)
#endif

#define SP0A20_TEST_PATTERN_CHECKSUM (0x9db2de6e)

kal_bool SP0A20_night_mode_enable = KAL_FALSE;
kal_uint16 SP0A20_CurStatus_AWB = 0;
#if __HCT_DUAL_CAMERA_USEDBY_YUV_MODE__

extern int GC0310_iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 *a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int GC0310_iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);
#endif


static void SP0A20_awb_enable(kal_bool enalbe);

extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);

kal_uint16 SP0A20_write_cmos_sensor(kal_uint8 addr, kal_uint8 para)
{
    char puSendCmd[2] = {(char)(addr & 0xFF) , (char)(para & 0xFF)};
	
#if  __HCT_DUAL_CAMERA_USEDBY_YUV_MODE__
	GC0310_iWriteRegI2C(puSendCmd , 2, SP0A20_WRITE_ID);
#else
	iWriteRegI2C(puSendCmd , 2, SP0A20_WRITE_ID);
#endif
	return 0;
}
kal_uint16 SP0A20_read_cmos_sensor(kal_uint8 addr)
{
	kal_uint16 get_byte=0;
    char puSendCmd = { (char)(addr & 0xFF) };
	
#if  __HCT_DUAL_CAMERA_USEDBY_YUV_MODE__
	GC0310_iReadRegI2C(&puSendCmd , 1, (u8*)&get_byte, 1, SP0A20_READ_ID);
#else
	iReadRegI2C(&puSendCmd , 1, (u8*)&get_byte, 1, SP0A20_READ_ID);
#endif
    return get_byte;
}


/*******************************************************************************
 * // Adapter for Winmo typedef
 ********************************************************************************/
#define WINMO_USE 0

#define Sleep(ms) mdelay(ms)
#define RETAILMSG(x,...)
#define TEXT

kal_bool   SP0A20_MPEG4_encode_mode = KAL_FALSE;
kal_uint16 SP0A20_dummy_pixels = 0, SP0A20_dummy_lines = 0;
kal_bool   SP0A20_MODE_CAPTURE = KAL_FALSE;
kal_bool   SP0A20_NIGHT_MODE = KAL_FALSE;



kal_uint32 SP0A20_isp_master_clock;
static kal_uint32 SP0A20_g_fPV_PCLK = 30 * 1000000;

kal_uint8 SP0A20_sensor_write_I2C_address = SP0A20_WRITE_ID;
kal_uint8 SP0A20_sensor_read_I2C_address = SP0A20_READ_ID;

UINT8 SP0A20PixelClockDivider=0;

MSDK_SENSOR_CONFIG_STRUCT SP0A20SensorConfigData;


/*************************************************************************
 * FUNCTION
 *	SP0A20_SetShutter
 *
 * DESCRIPTION
 *	This function set e-shutter of SP0A20 to change exposure time.
 *
 * PARAMETERS
 *   iShutter : exposured lines
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
void SP0A20_Set_Shutter(kal_uint16 iShutter)
{
} /* Set_SP0A20_Shutter */


/*************************************************************************
 * FUNCTION
 *	SP0A20_read_Shutter
 *
 * DESCRIPTION
 *	This function read e-shutter of SP0A20 .
 *
 * PARAMETERS
 *  None
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
kal_uint32 SP0A20_Read_Shutter(void)
{
	//kal_uint8 temp_reg1, temp_reg2;
	static kal_uint16 shutter;

	//SP0A20_write_cmos_sensor(0xfd,0x00);

//	temp_reg1 = SP0A20_read_cmos_sensor(0x04);
//	temp_reg2 = SP0A20_read_cmos_sensor(0x03);

//	shutter = ((temp_reg1 & 0xFF) | (temp_reg2 << 8))*2;
	
	SP0A20_write_cmos_sensor(0xfd,0x01);
	shutter = SP0A20_read_cmos_sensor(0xf1)*18;
	shutter=shutter^0xfff;
	//	printk("SP0A20_Read_Shutter shutter_1=%d\n",shutter_1);

    return shutter;
} /* SP0A20_read_shutter */

static void SP0A20_Set_Mirrorflip(kal_uint8 image_mirror)
{
	


}

static void SP0A20_set_AE_mode(kal_bool AE_enable)
{


}


void SP0A20_set_contrast(UINT16 para)
{   
   
}

UINT32 SP0A20_MIPI_SetMaxFramerateByScenario(
  MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 frameRate)
{
	SENSORDB("scenarioId = %d\n", scenarioId);
	return 0;
}


UINT32 SP0A20SetTestPatternMode(kal_bool bEnable)
{

	return ERROR_NONE;
}


void SP0A20_set_brightness(UINT16 para)
{


}
void SP0A20_set_saturation(UINT16 para)
{

    
}

void SP0A20_set_edge(UINT16 para)
{
    
}


void SP0A20_set_iso(UINT16 para)
{

}




void SP0A20_MIPI_GetDelayInfo(uintptr_t delayAddr)
{
    SENSOR_DELAY_INFO_STRUCT* pDelayInfo = (SENSOR_DELAY_INFO_STRUCT*)delayAddr;
    pDelayInfo->InitDelay = 2;
    pDelayInfo->EffectDelay = 2;
    pDelayInfo->AwbDelay = 2;
    pDelayInfo->AFSwitchDelayFrame = 50;
}

UINT32 SP0A20_MIPI_GetDefaultFramerateByScenario(
  MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 *pframeRate)
{
    switch (scenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
             *pframeRate = 300;
             break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
        case MSDK_SCENARIO_ID_CAMERA_ZSD:
             *pframeRate = 300;
             break;
        case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW: //added
        case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:
        case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE: //added
             *pframeRate = 300;
             break;
        default:
             *pframeRate = 300;
          break;
    }

  return ERROR_NONE;
}

void SP0A20_MIPI_SetMaxMinFps(UINT32 u2MinFrameRate, UINT32 u2MaxFrameRate)
{

	return;
}

void SP0A20_3ACtrl(ACDK_SENSOR_3A_LOCK_ENUM action)
{
	SENSORDB("[GC0329]enter ACDK_SENSOR_3A_LOCK_ENUM function:action=%d\n",action);
   switch (action)
   {
      case SENSOR_3A_AE_LOCK:
          SP0A20_set_AE_mode(KAL_FALSE);
      break;
      case SENSOR_3A_AE_UNLOCK:
          SP0A20_set_AE_mode(KAL_TRUE);
      break;

      case SENSOR_3A_AWB_LOCK:
          SP0A20_awb_enable(KAL_FALSE);       
      break;

      case SENSOR_3A_AWB_UNLOCK:
		   if (((AWB_MODE_OFF == SP0A20_CurStatus_AWB) ||
        		(AWB_MODE_AUTO == SP0A20_CurStatus_AWB)))
        	{	
         			 SP0A20_awb_enable(KAL_TRUE);
         	}
      break;
      default:
      	break;
   }
   SENSORDB("[GC0329]exit ACDK_SENSOR_3A_LOCK_ENUM function:action=%d\n",action);
   return;
}


void SP0A20_MIPI_GetExifInfo(uintptr_t exifAddr)
{
    SENSOR_EXIF_INFO_STRUCT* pExifInfo = (SENSOR_EXIF_INFO_STRUCT*)exifAddr;
    pExifInfo->FNumber = 28;
//    pExifInfo->AEISOSpeed = SP0A20_Driver.isoSpeed;
//    pExifInfo->AWBMode = S5K4ECGX_Driver.awbMode;
//    pExifInfo->CapExposureTime = S5K4ECGX_Driver.capExposureTime;
//    pExifInfo->FlashLightTimeus = 0;
//   pExifInfo->RealISOValue = (S5K4ECGX_MIPI_ReadGain()*57) >> 8;
        //S5K4ECGX_Driver.isoSpeed;
}

#if 0
void SP0A20_MIPI_get_AEAWB_lock(uintptr_t pAElockRet32, uintptr_t pAWBlockRet32)
{
    *pAElockRet32 = 1;
    *pAWBlockRet32 = 1;
    SENSORDB("[SP0A20]GetAEAWBLock,AE=%d ,AWB=%d\n,",(int)*pAElockRet32, (int)*pAWBlockRet32);
}
#endif

/*************************************************************************
 * FUNCTION
 *	SP0A20_write_reg
 *
 * DESCRIPTION
 *	This function set the register of SP0A20.
 *
 * PARAMETERS
 *	addr : the register index of SP0A20
 *  para : setting parameter of the specified register of SP0A20
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
void SP0A20_write_reg(kal_uint32 addr, kal_uint32 para)
{
	SP0A20_write_cmos_sensor(addr, para);
} /* SP0A20_write_reg() */


/*************************************************************************
 * FUNCTION
 *	SP0A20_read_cmos_sensor
 *
 * DESCRIPTION
 *	This function read parameter of specified register from SP0A20.
 *
 * PARAMETERS
 *	addr : the register index of SP0A20
 *
 * RETURNS
 *	the data that read from SP0A20
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
kal_uint32 SP0A20_read_reg(kal_uint32 addr)
{
	return SP0A20_read_cmos_sensor(addr);
} /* OV7670_read_reg() */


/*************************************************************************
* FUNCTION
*	SP0A20_awb_enable
*
* DESCRIPTION
*	This function enable or disable the awb (Auto White Balance).
*
* PARAMETERS
*	1. kal_bool : KAL_TRUE - enable awb, KAL_FALSE - disable awb.
*
* RETURNS
*	kal_bool : It means set awb right or not.
*
*************************************************************************/
static void SP0A20_awb_enable(kal_bool enalbe)
{	 
	

}


/*************************************************************************
* FUNCTION
*	SP0A20_GAMMA_Select
*
* DESCRIPTION
*	This function is served for FAE to select the appropriate GAMMA curve.
*
* PARAMETERS
*	None
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
void SP0A20GammaSelect(kal_uint32 GammaLvl)
{
	
}


/*************************************************************************
 * FUNCTION
 *	SP0A20_config_window
 *
 * DESCRIPTION
 *	This function config the hardware window of SP0A20 for getting specified
 *  data of that window.
 *
 * PARAMETERS
 *	start_x : start column of the interested window
 *  start_y : start row of the interested window
 *  width  : column widht of the itnerested window
 *  height : row depth of the itnerested window
 *
 * RETURNS
 *	the data that read from SP0A20
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
void SP0A20_config_window(kal_uint16 startx, kal_uint16 starty, kal_uint16 width, kal_uint16 height)
{
} /* SP0A20_config_window */


/*************************************************************************
 * FUNCTION
 *	SP0A20_SetGain
 *
 * DESCRIPTION
 *	This function is to set global gain to sensor.
 *
 * PARAMETERS
 *   iGain : sensor global gain(base: 0x40)
 *
 * RETURNS
 *	the actually gain set to sensor.
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
kal_uint16 SP0A20_SetGain(kal_uint16 iGain)
{
	return iGain;
}


/*************************************************************************
 * FUNCTION
 *	SP0A20_NightMode
 *
 * DESCRIPTION
 *	This function night mode of SP0A20.
 *
 * PARAMETERS
 *	bEnable: KAL_TRUE -> enable night mode, otherwise, disable night mode
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
void SP0A20NightMode(kal_bool bEnable)
{
	SENSORDB("Enter SP0A20NightMode!, bEnable = %d, SP0A20_MPEG4_encode_mode = %d\n", bEnable, SP0A20_MPEG4_encode_mode);

	
	
} /* SP0A20_NightMode */

/*************************************************************************
* FUNCTION
*	SP0A20_Sensor_Init
*
* DESCRIPTION
*	This function apply all of the initial setting to sensor.
*
* PARAMETERS
*	None
*
* RETURNS
*	None
*
*************************************************************************/
static void SP0A20_Sensor_Init(void)
{
	SP0A20_write_cmos_sensor(0xfd,0x00);
	SP0A20_write_cmos_sensor(0x36,0x02);
	SP0A20_write_cmos_sensor(0x0c,0x00);
	SP0A20_write_cmos_sensor(0x92,0x11);
	SP0A20_write_cmos_sensor(0x96,0x02);
	SP0A20_write_cmos_sensor(0x97,0x0b);
	SP0A20_write_cmos_sensor(0x1b,0x27);
	SP0A20_write_cmos_sensor(0x12,0x02);
	SP0A20_write_cmos_sensor(0x13,0x2f);
	SP0A20_write_cmos_sensor(0x6d,0x32);
	SP0A20_write_cmos_sensor(0x6c,0x32);
	SP0A20_write_cmos_sensor(0x6f,0x33);
	SP0A20_write_cmos_sensor(0x6e,0x34);
	SP0A20_write_cmos_sensor(0x99,0x04);
	SP0A20_write_cmos_sensor(0x16,0x38);
	SP0A20_write_cmos_sensor(0x17,0x38);
	SP0A20_write_cmos_sensor(0x70,0x3a);
	SP0A20_write_cmos_sensor(0x14,0x02);
	SP0A20_write_cmos_sensor(0x15,0x20);
	SP0A20_write_cmos_sensor(0x71,0x23);
	SP0A20_write_cmos_sensor(0x69,0x25);
	SP0A20_write_cmos_sensor(0x6a,0x1a);
	SP0A20_write_cmos_sensor(0x72,0x1c);
	SP0A20_write_cmos_sensor(0x75,0x1e);
	SP0A20_write_cmos_sensor(0x73,0x3c);
	SP0A20_write_cmos_sensor(0x74,0x21);
	SP0A20_write_cmos_sensor(0x79,0x00);
	SP0A20_write_cmos_sensor(0x77,0x10);
	SP0A20_write_cmos_sensor(0x1a,0x4d);
	SP0A20_write_cmos_sensor(0x1c,0x07);
	SP0A20_write_cmos_sensor(0x1e,0x15);
	SP0A20_write_cmos_sensor(0x21,0x08);
	SP0A20_write_cmos_sensor(0x22,0x28);
	SP0A20_write_cmos_sensor(0x26,0x66);
	SP0A20_write_cmos_sensor(0x28,0x0b);
	SP0A20_write_cmos_sensor(0x37,0x4a);
	SP0A20_write_cmos_sensor(0xfd,0x02);
	SP0A20_write_cmos_sensor(0x01,0x80);
	SP0A20_write_cmos_sensor(0x52,0x10);
	SP0A20_write_cmos_sensor(0x54,0x00);
	SP0A20_write_cmos_sensor(0xfd,0x01);
	SP0A20_write_cmos_sensor(0x41,0x00);
	SP0A20_write_cmos_sensor(0x42,0x00);
	SP0A20_write_cmos_sensor(0x43,0x00);
	SP0A20_write_cmos_sensor(0x44,0x00);
	SP0A20_write_cmos_sensor(0xfd,0x00);
	SP0A20_write_cmos_sensor(0x03,0x03);
	SP0A20_write_cmos_sensor(0x04,0x2a);
	SP0A20_write_cmos_sensor(0x05,0x00);
	SP0A20_write_cmos_sensor(0x06,0x00);
	SP0A20_write_cmos_sensor(0x07,0x00);
	SP0A20_write_cmos_sensor(0x08,0x00);
	SP0A20_write_cmos_sensor(0x09,0x00);
	SP0A20_write_cmos_sensor(0x0a,0x2d);
	SP0A20_write_cmos_sensor(0xfd,0x01);
	SP0A20_write_cmos_sensor(0xf0,0x00);
	SP0A20_write_cmos_sensor(0xf7,0x87);
	SP0A20_write_cmos_sensor(0x02,0x0c);
	SP0A20_write_cmos_sensor(0x03,0x01);
	SP0A20_write_cmos_sensor(0x06,0x87);
	SP0A20_write_cmos_sensor(0x07,0x00);
	SP0A20_write_cmos_sensor(0x08,0x01);
	SP0A20_write_cmos_sensor(0x09,0x00);
	SP0A20_write_cmos_sensor(0xfd,0x02);
	SP0A20_write_cmos_sensor(0xbe,0x54);
	SP0A20_write_cmos_sensor(0xbf,0x06);
	SP0A20_write_cmos_sensor(0xd0,0x54);
	SP0A20_write_cmos_sensor(0xd1,0x06);
	SP0A20_write_cmos_sensor(0xfd,0x01);
	SP0A20_write_cmos_sensor(0x5a,0x40);
	SP0A20_write_cmos_sensor(0xfd,0x02);
	SP0A20_write_cmos_sensor(0xbc,0x70);
	SP0A20_write_cmos_sensor(0xbd,0x50);
	SP0A20_write_cmos_sensor(0xb8,0x66);
	SP0A20_write_cmos_sensor(0xb9,0x88);
	SP0A20_write_cmos_sensor(0xba,0x30);
	SP0A20_write_cmos_sensor(0xbb,0x45);
	SP0A20_write_cmos_sensor(0xfd,0x01);
	SP0A20_write_cmos_sensor(0xe0,0x60);
	SP0A20_write_cmos_sensor(0xe1,0x48);
	SP0A20_write_cmos_sensor(0xe2,0x40);
	SP0A20_write_cmos_sensor(0xe3,0x3a);
	SP0A20_write_cmos_sensor(0xe4,0x3a);
	SP0A20_write_cmos_sensor(0xe5,0x38);
	SP0A20_write_cmos_sensor(0xe6,0x38);
	SP0A20_write_cmos_sensor(0xe7,0x34);
	SP0A20_write_cmos_sensor(0xe8,0x34);
	SP0A20_write_cmos_sensor(0xe9,0x34);
	SP0A20_write_cmos_sensor(0xea,0x32);
	SP0A20_write_cmos_sensor(0xf3,0x32);
	SP0A20_write_cmos_sensor(0xf4,0x32);
	SP0A20_write_cmos_sensor(0xfd,0x01);
	SP0A20_write_cmos_sensor(0x04,0xa0);
	SP0A20_write_cmos_sensor(0x05,0x32);
	SP0A20_write_cmos_sensor(0x0a,0xa0);
	SP0A20_write_cmos_sensor(0x0b,0x32);
	SP0A20_write_cmos_sensor(0xfd,0x01);
	SP0A20_write_cmos_sensor(0xeb,0x98);
	SP0A20_write_cmos_sensor(0xec,0x6c);
	SP0A20_write_cmos_sensor(0xed,0x08);
	SP0A20_write_cmos_sensor(0xee,0x0c);
	SP0A20_write_cmos_sensor(0xfd,0x01);
	SP0A20_write_cmos_sensor(0xf2,0x4d);
	SP0A20_write_cmos_sensor(0xfd,0x02);
	SP0A20_write_cmos_sensor(0x5b,0x05);
	SP0A20_write_cmos_sensor(0x5c,0xa0);
	SP0A20_write_cmos_sensor(0xfd,0x01);
	SP0A20_write_cmos_sensor(0x26,0x80);
	SP0A20_write_cmos_sensor(0x27,0x4f);
	SP0A20_write_cmos_sensor(0x28,0x00);
	SP0A20_write_cmos_sensor(0x29,0x20);
	SP0A20_write_cmos_sensor(0x2a,0x00);
	SP0A20_write_cmos_sensor(0x2b,0x03);
	SP0A20_write_cmos_sensor(0x2c,0x00);
	SP0A20_write_cmos_sensor(0x2d,0x20);
	SP0A20_write_cmos_sensor(0x30,0x00);
	SP0A20_write_cmos_sensor(0x31,0x00);
	SP0A20_write_cmos_sensor(0xfd,0x01);
	SP0A20_write_cmos_sensor(0xa1,0x28);
	SP0A20_write_cmos_sensor(0xa2,0x28);
	SP0A20_write_cmos_sensor(0xa3,0x28);
	SP0A20_write_cmos_sensor(0xa4,0x28);
	SP0A20_write_cmos_sensor(0xa5,0x1c);
	SP0A20_write_cmos_sensor(0xa6,0x1e);
	SP0A20_write_cmos_sensor(0xa7,0x20);
	SP0A20_write_cmos_sensor(0xa8,0x20);
	SP0A20_write_cmos_sensor(0xa9,0x1a);
	SP0A20_write_cmos_sensor(0xaa,0x1c);
	SP0A20_write_cmos_sensor(0xab,0x20);
	SP0A20_write_cmos_sensor(0xac,0x20);
	SP0A20_write_cmos_sensor(0xad,0x08);
	SP0A20_write_cmos_sensor(0xae,0x04);
	SP0A20_write_cmos_sensor(0xaf,0x08);
	SP0A20_write_cmos_sensor(0xb0,0x04);
	SP0A20_write_cmos_sensor(0xb1,0x00);
	SP0A20_write_cmos_sensor(0xb2,0x00);
	SP0A20_write_cmos_sensor(0xb3,0x00);
	SP0A20_write_cmos_sensor(0xb4,0x00);
	SP0A20_write_cmos_sensor(0xb5,0x00);
	SP0A20_write_cmos_sensor(0xb6,0x00);
	SP0A20_write_cmos_sensor(0xb7,0x00);
	SP0A20_write_cmos_sensor(0xb8,0x00);
	SP0A20_write_cmos_sensor(0xfd,0x02);
	SP0A20_write_cmos_sensor(0x08,0x00);
	SP0A20_write_cmos_sensor(0x09,0x06);
	SP0A20_write_cmos_sensor(0x1d,0x03);
	SP0A20_write_cmos_sensor(0x1f,0x05);
	SP0A20_write_cmos_sensor(0xfd,0x01);
	SP0A20_write_cmos_sensor(0x32,0x00);
	SP0A20_write_cmos_sensor(0xfd,0x02);
	SP0A20_write_cmos_sensor(0x26,0xac);
	SP0A20_write_cmos_sensor(0x27,0xad);
	SP0A20_write_cmos_sensor(0x10,0x00);
	SP0A20_write_cmos_sensor(0x11,0xdd);
	SP0A20_write_cmos_sensor(0x1b,0x80);
	SP0A20_write_cmos_sensor(0x1a,0x80);
	SP0A20_write_cmos_sensor(0x18,0x27);
	SP0A20_write_cmos_sensor(0x19,0x26);
	SP0A20_write_cmos_sensor(0x2a,0x00);
	SP0A20_write_cmos_sensor(0x2b,0x00);
	SP0A20_write_cmos_sensor(0x28,0xf8);
	SP0A20_write_cmos_sensor(0x29,0x08);
	SP0A20_write_cmos_sensor(0x66,0x40);
	SP0A20_write_cmos_sensor(0x67,0x62);
	SP0A20_write_cmos_sensor(0x68,0xd4);
	SP0A20_write_cmos_sensor(0x69,0xf4);
	SP0A20_write_cmos_sensor(0x6a,0xa5);
	SP0A20_write_cmos_sensor(0x7c,0x20);
	SP0A20_write_cmos_sensor(0x7d,0x4b);
	SP0A20_write_cmos_sensor(0x7e,0xf4);
	SP0A20_write_cmos_sensor(0x7f,0x26);
	SP0A20_write_cmos_sensor(0x80,0xa6);
	SP0A20_write_cmos_sensor(0x70,0x26);
	SP0A20_write_cmos_sensor(0x71,0x4a);
	SP0A20_write_cmos_sensor(0x72,0x25);
	SP0A20_write_cmos_sensor(0x73,0x46);
	SP0A20_write_cmos_sensor(0x74,0xaa);
	SP0A20_write_cmos_sensor(0x6b,0x0a);
	SP0A20_write_cmos_sensor(0x6c,0x31);
	SP0A20_write_cmos_sensor(0x6d,0x2e);
	SP0A20_write_cmos_sensor(0x6e,0x52);
	SP0A20_write_cmos_sensor(0x6f,0xaa);
	SP0A20_write_cmos_sensor(0x61,0x00);
	SP0A20_write_cmos_sensor(0x62,0x20);
	SP0A20_write_cmos_sensor(0x63,0x40);
	SP0A20_write_cmos_sensor(0x64,0x70);
	SP0A20_write_cmos_sensor(0x65,0x6a);
	SP0A20_write_cmos_sensor(0x75,0x80);
	SP0A20_write_cmos_sensor(0x76,0x09);
	SP0A20_write_cmos_sensor(0x77,0x02);
	SP0A20_write_cmos_sensor(0x24,0x25);
	SP0A20_write_cmos_sensor(0x0e,0x16);
	SP0A20_write_cmos_sensor(0x3b,0x09);
	SP0A20_write_cmos_sensor(0xfd,0x02);
	SP0A20_write_cmos_sensor(0xde,0x0f);
	SP0A20_write_cmos_sensor(0xd7,0x08);
	SP0A20_write_cmos_sensor(0xd8,0x08);
	SP0A20_write_cmos_sensor(0xd9,0x10);
	SP0A20_write_cmos_sensor(0xda,0x14);
	SP0A20_write_cmos_sensor(0xe8,0x1b);
	SP0A20_write_cmos_sensor(0xe9,0x1b);
	SP0A20_write_cmos_sensor(0xea,0x1b);
	SP0A20_write_cmos_sensor(0xeb,0x1b);
	SP0A20_write_cmos_sensor(0xec,0x1c);
	SP0A20_write_cmos_sensor(0xed,0x1c);
	SP0A20_write_cmos_sensor(0xee,0x1c);
	SP0A20_write_cmos_sensor(0xef,0x1c);
	SP0A20_write_cmos_sensor(0xd3,0x20);
	SP0A20_write_cmos_sensor(0xd4,0x48);
	SP0A20_write_cmos_sensor(0xd5,0x20);
	SP0A20_write_cmos_sensor(0xd6,0x08);
	SP0A20_write_cmos_sensor(0xfd,0x01);
	SP0A20_write_cmos_sensor(0xd1,0x20);
	SP0A20_write_cmos_sensor(0xfd,0x02);
	SP0A20_write_cmos_sensor(0xdc,0x05);
	SP0A20_write_cmos_sensor(0x05,0x20);
	SP0A20_write_cmos_sensor(0xfd,0x02);
	SP0A20_write_cmos_sensor(0x81,0x00);
	SP0A20_write_cmos_sensor(0xfd,0x01);
	SP0A20_write_cmos_sensor(0xfc,0x00);
	SP0A20_write_cmos_sensor(0x7d,0x05);
	SP0A20_write_cmos_sensor(0x7e,0x05);
	SP0A20_write_cmos_sensor(0x7f,0x09);
	SP0A20_write_cmos_sensor(0x80,0x08);
	SP0A20_write_cmos_sensor(0xfd,0x02);
	SP0A20_write_cmos_sensor(0xdd,0x0f);
	SP0A20_write_cmos_sensor(0xfd,0x01);
	SP0A20_write_cmos_sensor(0x6d,0x05);
	SP0A20_write_cmos_sensor(0x6e,0x05);
	SP0A20_write_cmos_sensor(0x6f,0x10);
	SP0A20_write_cmos_sensor(0x70,0x18);
	SP0A20_write_cmos_sensor(0x86,0x18);
	SP0A20_write_cmos_sensor(0x71,0x08);
	SP0A20_write_cmos_sensor(0x72,0x08);
	SP0A20_write_cmos_sensor(0x73,0x14);
	SP0A20_write_cmos_sensor(0x74,0x14);
	SP0A20_write_cmos_sensor(0x75,0x08);
	SP0A20_write_cmos_sensor(0x76,0x0a);
	SP0A20_write_cmos_sensor(0x77,0x06);
	SP0A20_write_cmos_sensor(0x78,0x06);
	SP0A20_write_cmos_sensor(0x79,0x27);
	SP0A20_write_cmos_sensor(0x7a,0x25);
	SP0A20_write_cmos_sensor(0x7b,0x24);
	SP0A20_write_cmos_sensor(0x7c,0x02);
	SP0A20_write_cmos_sensor(0x81,0x0d);
	SP0A20_write_cmos_sensor(0x82,0x18);
	SP0A20_write_cmos_sensor(0x83,0x20);
	SP0A20_write_cmos_sensor(0x84,0x24);
	SP0A20_write_cmos_sensor(0xfd,0x02);
	SP0A20_write_cmos_sensor(0x83,0x12);
	SP0A20_write_cmos_sensor(0x84,0x14);
	SP0A20_write_cmos_sensor(0x86,0x04);
	SP0A20_write_cmos_sensor(0xfd,0x01);
	SP0A20_write_cmos_sensor(0x61,0x60);
	SP0A20_write_cmos_sensor(0x62,0x28);
	SP0A20_write_cmos_sensor(0x8a,0x10);
	SP0A20_write_cmos_sensor(0xfd,0x01);
	SP0A20_write_cmos_sensor(0x8b,0x00);
	SP0A20_write_cmos_sensor(0x8c,0x0a);
	SP0A20_write_cmos_sensor(0x8d,0x17);
	SP0A20_write_cmos_sensor(0x8e,0x23);
	SP0A20_write_cmos_sensor(0x8f,0x2f);
	SP0A20_write_cmos_sensor(0x90,0x44);
	SP0A20_write_cmos_sensor(0x91,0x55);
	SP0A20_write_cmos_sensor(0x92,0x63);
	SP0A20_write_cmos_sensor(0x93,0x71);
	SP0A20_write_cmos_sensor(0x94,0x87);
	SP0A20_write_cmos_sensor(0x95,0x96);
	SP0A20_write_cmos_sensor(0x96,0xa4);
	SP0A20_write_cmos_sensor(0x97,0xb0);
	SP0A20_write_cmos_sensor(0x98,0xbb);
	SP0A20_write_cmos_sensor(0x99,0xc5);
	SP0A20_write_cmos_sensor(0x9a,0xcf);
	SP0A20_write_cmos_sensor(0x9b,0xd8);
	SP0A20_write_cmos_sensor(0x9c,0xe0);
	SP0A20_write_cmos_sensor(0x9d,0xe9);
	SP0A20_write_cmos_sensor(0x9e,0xf1);
	SP0A20_write_cmos_sensor(0x9f,0xf8);
	SP0A20_write_cmos_sensor(0xa0,0xff);
	SP0A20_write_cmos_sensor(0xfd,0x02);
	SP0A20_write_cmos_sensor(0x15,0xc0);
	SP0A20_write_cmos_sensor(0x16,0x8c);
	SP0A20_write_cmos_sensor(0xa0,0xa6);
	SP0A20_write_cmos_sensor(0xa1,0x0b);
	SP0A20_write_cmos_sensor(0xa2,0xcf);
	SP0A20_write_cmos_sensor(0xa3,0xe4);
	SP0A20_write_cmos_sensor(0xa4,0xb5);
	SP0A20_write_cmos_sensor(0xa5,0xe7);
	SP0A20_write_cmos_sensor(0xa6,0xff);
	SP0A20_write_cmos_sensor(0xa7,0xd4);
	SP0A20_write_cmos_sensor(0xa8,0xad);
	SP0A20_write_cmos_sensor(0xa9,0x30);
	SP0A20_write_cmos_sensor(0xaa,0x33);
	SP0A20_write_cmos_sensor(0xab,0x0f);
	SP0A20_write_cmos_sensor(0xac,0x9e);
	SP0A20_write_cmos_sensor(0xad,0xf0);
	SP0A20_write_cmos_sensor(0xae,0xf2);
	SP0A20_write_cmos_sensor(0xaf,0xdf);
	SP0A20_write_cmos_sensor(0xb0,0xc1);
	SP0A20_write_cmos_sensor(0xb1,0xe0);
	SP0A20_write_cmos_sensor(0xb2,0xf4);
	SP0A20_write_cmos_sensor(0xb3,0x84);
	SP0A20_write_cmos_sensor(0xb4,0x08);
	SP0A20_write_cmos_sensor(0xb5,0x3c);
	SP0A20_write_cmos_sensor(0xb6,0x33);
	SP0A20_write_cmos_sensor(0xb7,0x1f);
	SP0A20_write_cmos_sensor(0xfd,0x01);
	SP0A20_write_cmos_sensor(0xd3,0x60);
	SP0A20_write_cmos_sensor(0xd4,0x60);
	SP0A20_write_cmos_sensor(0xd5,0x50);
	SP0A20_write_cmos_sensor(0xd6,0x4c);
	SP0A20_write_cmos_sensor(0xd7,0x60);
	SP0A20_write_cmos_sensor(0xd8,0x60);
	SP0A20_write_cmos_sensor(0xd9,0x50);
	SP0A20_write_cmos_sensor(0xda,0x4c);
	SP0A20_write_cmos_sensor(0xfd,0x01);
	SP0A20_write_cmos_sensor(0xdd,0x30);
	SP0A20_write_cmos_sensor(0xde,0x10);
	SP0A20_write_cmos_sensor(0xdf,0xff);
	SP0A20_write_cmos_sensor(0x00,0x00);
	SP0A20_write_cmos_sensor(0xfd,0x01);
	SP0A20_write_cmos_sensor(0xc2,0xee);
	SP0A20_write_cmos_sensor(0xc3,0xee);
	SP0A20_write_cmos_sensor(0xc4,0x99);
	SP0A20_write_cmos_sensor(0xc5,0x77);
	SP0A20_write_cmos_sensor(0xfd,0x01);
	SP0A20_write_cmos_sensor(0xcd,0x10);
	SP0A20_write_cmos_sensor(0xce,0x1f);
	SP0A20_write_cmos_sensor(0xcf,0x30);
	SP0A20_write_cmos_sensor(0xd0,0x45);
	SP0A20_write_cmos_sensor(0xfd,0x02);
	SP0A20_write_cmos_sensor(0x30,0x80);
	SP0A20_write_cmos_sensor(0x31,0x60);
	SP0A20_write_cmos_sensor(0x32,0x60);
	SP0A20_write_cmos_sensor(0x33,0xc0);
	SP0A20_write_cmos_sensor(0x35,0x60);
	SP0A20_write_cmos_sensor(0x37,0x13);
	SP0A20_write_cmos_sensor(0xfd,0x01);
	SP0A20_write_cmos_sensor(0x0e,0x80);
	SP0A20_write_cmos_sensor(0x0f,0x20);
	SP0A20_write_cmos_sensor(0x10,0x70);
	SP0A20_write_cmos_sensor(0x11,0x70);
	SP0A20_write_cmos_sensor(0x12,0x8a);
	SP0A20_write_cmos_sensor(0x13,0x8a);
	SP0A20_write_cmos_sensor(0x14,0x85);
	SP0A20_write_cmos_sensor(0x15,0x84);
	SP0A20_write_cmos_sensor(0x16,0x84);
	SP0A20_write_cmos_sensor(0x17,0x85);
	SP0A20_write_cmos_sensor(0xfd,0x00);
	SP0A20_write_cmos_sensor(0x31,0x00);
	SP0A20_write_cmos_sensor(0xfd,0x01);
	SP0A20_write_cmos_sensor(0x32,0x15);
	SP0A20_write_cmos_sensor(0x33,0xef);
	SP0A20_write_cmos_sensor(0x34,0x07);
	SP0A20_write_cmos_sensor(0xd2,0x01);
	SP0A20_write_cmos_sensor(0xfb,0x25);
	SP0A20_write_cmos_sensor(0xf2,0x49);
	SP0A20_write_cmos_sensor(0x35,0x40);
	SP0A20_write_cmos_sensor(0x5d,0x11);

}

UINT32 SP0A20_GetSensorID(void)
{
    int  retry = 3; 
	UINT32 sensorID;
    // check if sensor ID correct
    do {
		 SP0A20_write_cmos_sensor(0xfd,0x00);
        sensorID=SP0A20_read_cmos_sensor(0x02);
        if (sensorID == SP0A20_SENSOR_ID)
            break; 
        SENSORDB("Read Sensor ID Fail = 0x%04x\n", sensorID); 
        retry--; 
    } while (retry > 0);

    if (sensorID != SP0A20_SENSOR_ID) {
        sensorID = 0xFFFFFFFF; 
        return ERROR_SENSOR_CONNECT_FAIL;
    }
    return ERROR_NONE;    
}


UINT32 SP0A20GetSensorID(UINT32 *sensorID)
{
    int  retry = 3; 
    // check if sensor ID correct
    do {
		 SP0A20_write_cmos_sensor(0xfd,0x00);
        *sensorID=SP0A20_read_cmos_sensor(0x02);
        if (*sensorID == SP0A20_SENSOR_ID)
            break; 
        SENSORDB("Read Sensor ID Fail = 0x%04x\n", *sensorID); 
        retry--; 
    } while (retry > 0);

    if (*sensorID != SP0A20_SENSOR_ID) {
        *sensorID = 0xFFFFFFFF; 
        return ERROR_SENSOR_CONNECT_FAIL;
    }
    return ERROR_NONE;    
}




/*************************************************************************
* FUNCTION
*	SP0A20_Write_More_Registers
*
* DESCRIPTION
*	This function is served for FAE to modify the necessary Init Regs. Do not modify the regs
*     in init_SP0A20() directly.
*
* PARAMETERS
*	None
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
void SP0A20_Write_More_Registers(void)
{

}


/*************************************************************************
 * FUNCTION
 *	SP0A20Open
 *
 * DESCRIPTION
 *	This function initialize the registers of CMOS sensor
 *
 * PARAMETERS
 *	None
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
UINT32 SP0A20Open(void)
{
	volatile signed char i;
	kal_uint16 sensor_id=0;
	printk("zengyuan SP0A20Open\n");

	//SENSORDB("<Jet> Entry SP0A20Open!!!\r\n");

    for(i=0;i<3;i++)
		{
		SP0A20_write_cmos_sensor(0xfd,0x00);
		sensor_id = SP0A20_read_cmos_sensor(0x02);
		if(sensor_id == SP0A20_SENSOR_ID)  
		{
			SENSORDB("SP0A20mipi Read Sensor ID ok[open] = 0x%x\n", sensor_id); 
			break;
		}
		}
	//  Read sensor ID to adjust I2C is OK?
	if(sensor_id != SP0A20_SENSOR_ID)  
	{
		SENSORDB("SP0A20mipi Read Sensor ID Fail[open] = 0x%x\n", sensor_id); 
		return ERROR_SENSOR_CONNECT_FAIL;
	}
	SP0A20_Sensor_Init();

	return ERROR_NONE;
} /* SP0A20Open */


/*************************************************************************
 * FUNCTION
 *	SP0A20Close
 *
 * DESCRIPTION
 *	This function is to turn off sensor module power.
 *
 * PARAMETERS
 *	None
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
UINT32 SP0A20Close(void)
{
    return ERROR_NONE;
} /* SP0A20Close */


/*************************************************************************
 * FUNCTION
 * SP0A20Preview
 *
 * DESCRIPTION
 *	This function start the sensor preview.
 *
 * PARAMETERS
 *	*image_window : address pointer of pixel numbers in one period of HSYNC
 *  *sensor_config_data : address pointer of line numbers in one period of VSYNC
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
UINT32 SP0A20Preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
        MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)

{
 //   kal_uint32 iTemp;
//    kal_uint16 iStartX = 0, iStartY = 1;

	SENSORDB("Enter SP0A20Preview function!!!\r\n");

    image_window->GrabStartX= IMAGE_SENSOR_VGA_GRAB_PIXELS;
    image_window->GrabStartY= IMAGE_SENSOR_VGA_GRAB_LINES;
    image_window->ExposureWindowWidth = IMAGE_SENSOR_PV_WIDTH;
    image_window->ExposureWindowHeight =IMAGE_SENSOR_PV_HEIGHT;

    SP0A20_Set_Mirrorflip(IMAGE_NORMAL);
    

    // copy sensor_config_data
    memcpy(&SP0A20SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
//    SP0A20NightMode(SP0A20_night_mode_enable);
    return ERROR_NONE;
} /* SP0A20Preview */


/*************************************************************************
 * FUNCTION
 *	SP0A20Capture
 *
 * DESCRIPTION
 *	This function setup the CMOS sensor in capture MY_OUTPUT mode
 *
 * PARAMETERS
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
UINT32 SP0A20Capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
        MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)

{
	spin_lock(&SP0A20_drv_lock);
    SP0A20_MODE_CAPTURE=KAL_TRUE;
	spin_unlock(&SP0A20_drv_lock);


    image_window->GrabStartX = IMAGE_SENSOR_VGA_GRAB_PIXELS;
    image_window->GrabStartY = IMAGE_SENSOR_VGA_GRAB_LINES;
    image_window->ExposureWindowWidth= IMAGE_SENSOR_FULL_WIDTH;
    image_window->ExposureWindowHeight = IMAGE_SENSOR_FULL_HEIGHT;

    // copy sensor_config_data
    memcpy(&SP0A20SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
    return ERROR_NONE;
} /* SP0A20_Capture() */



UINT32 SP0A20GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
    pSensorResolution->SensorFullWidth=IMAGE_SENSOR_FULL_WIDTH;
    pSensorResolution->SensorFullHeight=IMAGE_SENSOR_FULL_HEIGHT;
    pSensorResolution->SensorPreviewWidth=IMAGE_SENSOR_PV_WIDTH;
    pSensorResolution->SensorPreviewHeight=IMAGE_SENSOR_PV_HEIGHT;
    pSensorResolution->SensorVideoWidth=IMAGE_SENSOR_PV_WIDTH;
    pSensorResolution->SensorVideoHeight=IMAGE_SENSOR_PV_HEIGHT;
    
    pSensorResolution->SensorHighSpeedVideoWidth=IMAGE_SENSOR_PV_WIDTH;
    pSensorResolution->SensorHighSpeedVideoHeight=IMAGE_SENSOR_PV_HEIGHT;
    
    pSensorResolution->SensorSlimVideoWidth=IMAGE_SENSOR_PV_WIDTH;
    pSensorResolution->SensorSlimVideoHeight=IMAGE_SENSOR_PV_HEIGHT;    
    return ERROR_NONE;
} /* SP0A20GetResolution() */


UINT32 SP0A20GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
        MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
        MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
    pSensorInfo->SensorPreviewResolutionX=IMAGE_SENSOR_PV_WIDTH;
    pSensorInfo->SensorPreviewResolutionY=IMAGE_SENSOR_PV_HEIGHT;
    pSensorInfo->SensorFullResolutionX=IMAGE_SENSOR_FULL_WIDTH;
    pSensorInfo->SensorFullResolutionY=IMAGE_SENSOR_FULL_HEIGHT;

    pSensorInfo->SensorCameraPreviewFrameRate=30;
    pSensorInfo->SensorVideoFrameRate=30;
    pSensorInfo->SensorStillCaptureFrameRate=10;
    pSensorInfo->SensorWebCamCaptureFrameRate=15;
    pSensorInfo->SensorResetActiveHigh=FALSE;
    pSensorInfo->SensorResetDelayCount=1;
    pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_VYUY;
    pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorInterruptDelayLines = 1;
    pSensorInfo->SensroInterfaceType=SENSOR_INTERFACE_TYPE_MIPI;//MIPI setting
    pSensorInfo->CaptureDelayFrame = 2;
    pSensorInfo->PreviewDelayFrame = 2;
    pSensorInfo->VideoDelayFrame = 4;

    pSensorInfo->SensorMasterClockSwitch = 0;
    pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_6MA;

    pSensorInfo->HighSpeedVideoDelayFrame = 4;
    pSensorInfo->SlimVideoDelayFrame = 4;
    pSensorInfo->SensorModeNum = 5;

    switch (ScenarioId)
    {
    case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
    case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
    default:
        pSensorInfo->SensorClockFreq=24;
        pSensorInfo->SensorClockDividCount= 3;
        pSensorInfo->SensorClockRisingCount=0;
        pSensorInfo->SensorClockFallingCount=2;
        pSensorInfo->SensorPixelClockCount=3;
        pSensorInfo->SensorDataLatchCount=2;
        pSensorInfo->SensorGrabStartX = IMAGE_SENSOR_VGA_GRAB_PIXELS;
        pSensorInfo->SensorGrabStartY = IMAGE_SENSOR_VGA_GRAB_LINES;
	//MIPI setting
	pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_1_LANE; 	
	pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
	pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14;   
	pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
	pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
	pSensorInfo->SensorHightSampling = 0;	// 0 is default 1x 
	pSensorInfo->SensorPacketECCOrder = 1;

        break;
    }
    SP0A20PixelClockDivider=pSensorInfo->SensorPixelClockCount;
    memcpy(pSensorConfigData, &SP0A20SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
    return ERROR_NONE;
} /* SP0A20GetInfo() */


UINT32 SP0A20Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
        MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{

	SENSORDB("Entry SP0A20Control, ScenarioId = %d!!!\r\n", ScenarioId);
    switch (ScenarioId)
    {
    case MSDK_SCENARIO_ID_VIDEO_PREVIEW:   
    	SENSORDB("SP0A20 Camera Video Preview!\n");
		spin_lock(&SP0A20_drv_lock);         
        SP0A20_MPEG4_encode_mode = KAL_TRUE;
        spin_unlock(&SP0A20_drv_lock); 
		SP0A20Preview(pImageWindow, pSensorConfigData);        
        break;
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:       
    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
    default:
        spin_lock(&SP0A20_drv_lock);  
        SP0A20_MPEG4_encode_mode = KAL_FALSE;
        spin_unlock(&SP0A20_drv_lock);
		SP0A20Preview(pImageWindow, pSensorConfigData);        
        break;
    }
    
    return ERROR_NONE;
}	/* SP0A20Control() */

BOOL SP0A20_set_param_wb(UINT16 para)
{
	return TRUE;
} /* SP0A20_set_param_wb */


BOOL SP0A20_set_param_effect(UINT16 para)
{
	
	return TRUE;

} /* SP0A20_set_param_effect */


BOOL SP0A20_set_param_banding(UINT16 para)
{
return TRUE;

} /* SP0A20_set_param_banding */

BOOL SP0A20_set_param_exposure(UINT16 para)
{
	
	return TRUE;
} /* SP0A20_set_param_exposure */


UINT32 SP0A20YUVSetVideoMode(UINT16 u2FrameRate)    // lanking add
{


    return TRUE;

}


UINT32 SP0A20YUVSensorSetting(FEATURE_ID iCmd, UINT16 iPara)
{
	SENSORDB("SP0A20YUVSensorSetting: icmd is  %d and para is %d!!\n", iCmd, iPara);
    switch (iCmd) {
    case FID_AWB_MODE:
        SP0A20_set_param_wb(iPara);
        break;
    case FID_COLOR_EFFECT:
        SP0A20_set_param_effect(iPara);
        break;
    case FID_AE_EV:
        SP0A20_set_param_exposure(iPara);
        break;
    case FID_AE_FLICKER:
        SP0A20_set_param_banding(iPara);
		break;
    case FID_SCENE_MODE:
	 	SP0A20NightMode(iPara);
        break;
        
	case FID_ISP_CONTRAST:
		SP0A20_set_contrast(iPara);
		break;
	case FID_ISP_BRIGHT:
		SP0A20_set_brightness(iPara);
		break;
	case FID_ISP_SAT:
		SP0A20_set_saturation(iPara);
		break; 
	case FID_ISP_EDGE:
		SP0A20_set_edge(iPara);
		break; 
	case FID_AE_ISO:
		SP0A20_set_iso(iPara);
		break;	
	case FID_AE_SCENE_MODE: 
		SP0A20_set_AE_mode(iPara);
		break; 
		
    default:
        break;
    }
    return TRUE;
} /* SP0A20YUVSensorSetting */


UINT32 SP0A20FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
        UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{
    UINT16 *pFeatureReturnPara16=(UINT16 *) pFeaturePara;
//    UINT16 *pFeatureData16=(UINT16 *) pFeaturePara;
    UINT32 *pFeatureReturnPara32=(UINT32 *) pFeaturePara;
    UINT32 *pFeatureData32=(UINT32 *) pFeaturePara;
 //   UINT32 **ppFeatureData=(UINT32 **) pFeaturePara;
    unsigned long long *feature_data=(unsigned long long *) pFeaturePara;
 //   unsigned long long *feature_return_para=(unsigned long long *) pFeaturePara;
	
   // UINT32 SP0A20SensorRegNumber;
  //  UINT32 i;
    MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData=(MSDK_SENSOR_CONFIG_STRUCT *) pFeaturePara;
    MSDK_SENSOR_REG_INFO_STRUCT *pSensorRegData=(MSDK_SENSOR_REG_INFO_STRUCT *) pFeaturePara;

    switch (FeatureId)
    {
    case SENSOR_FEATURE_GET_RESOLUTION:
        *pFeatureReturnPara16++=IMAGE_SENSOR_FULL_WIDTH;
        *pFeatureReturnPara16=IMAGE_SENSOR_FULL_HEIGHT;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_GET_PERIOD:
        *pFeatureReturnPara16++=(VGA_PERIOD_PIXEL_NUMS)+SP0A20_dummy_pixels;
        *pFeatureReturnPara16=(VGA_PERIOD_LINE_NUMS)+SP0A20_dummy_lines;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
        *pFeatureReturnPara32 = SP0A20_g_fPV_PCLK;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_SET_ESHUTTER:
        break;
    case SENSOR_FEATURE_SET_NIGHTMODE:
        SP0A20NightMode((BOOL) *feature_data);
        break;
    case SENSOR_FEATURE_SET_GAIN:
    case SENSOR_FEATURE_SET_FLASHLIGHT:
        break;
    case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
        SP0A20_isp_master_clock=*pFeatureData32;
        break;
    case SENSOR_FEATURE_SET_REGISTER:
        SP0A20_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
        break;
    case SENSOR_FEATURE_GET_REGISTER:
        pSensorRegData->RegData = SP0A20_read_cmos_sensor(pSensorRegData->RegAddr);
        break;
    case SENSOR_FEATURE_GET_CONFIG_PARA:
        memcpy(pSensorConfigData, &SP0A20SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
        *pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
        break;
    case SENSOR_FEATURE_SET_CCT_REGISTER:
    case SENSOR_FEATURE_GET_CCT_REGISTER:
    case SENSOR_FEATURE_SET_ENG_REGISTER:
    case SENSOR_FEATURE_GET_ENG_REGISTER:
    case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
    case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
    case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
    case SENSOR_FEATURE_GET_GROUP_COUNT:
    case SENSOR_FEATURE_GET_GROUP_INFO:
    case SENSOR_FEATURE_GET_ITEM_INFO:
    case SENSOR_FEATURE_SET_ITEM_INFO:
    case SENSOR_FEATURE_GET_ENG_INFO:
        break;
    case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
        // get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
        // if EEPROM does not exist in camera module.
        *pFeatureReturnPara32=LENS_DRIVER_ID_DO_NOT_CARE;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_SET_YUV_CMD:
        SP0A20YUVSensorSetting((FEATURE_ID)*feature_data, *(feature_data+1));
        
        break;
    case SENSOR_FEATURE_SET_VIDEO_MODE:    //  lanking
	 SP0A20YUVSetVideoMode(*feature_data);
	 break;
    case SENSOR_FEATURE_CHECK_SENSOR_ID:
	SP0A20GetSensorID(pFeatureData32);
	break;
	case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE:
		*pFeatureReturnPara32=SP0A20_TEST_PATTERN_CHECKSUM;
		*pFeatureParaLen=4;
		break;

	case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
		 SENSORDB("[SP0A20] F_SET_MAX_FRAME_RATE_BY_SCENARIO.\n");
		 SP0A20_MIPI_SetMaxFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*feature_data, *(feature_data+1));
		 break;

//	case SENSOR_CMD_SET_VIDEO_FRAME_RATE:
//		SENSORDB("[SP0A20] Enter SENSOR_CMD_SET_VIDEO_FRAME_RATE\n");
//		//SP0A20_MIPI_SetVideoFrameRate(*pFeatureData32);
//		break;

	case SENSOR_FEATURE_SET_TEST_PATTERN:			 
		SP0A20SetTestPatternMode((BOOL)*feature_data);			
		break;

    case SENSOR_FEATURE_GET_DELAY_INFO:
        SENSORDB("[SP0A20] F_GET_DELAY_INFO\n");
        SP0A20_MIPI_GetDelayInfo((uintptr_t)*feature_data);
    break;

    case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
         SENSORDB("[SP0A20] F_GET_DEFAULT_FRAME_RATE_BY_SCENARIO\n");
         SP0A20_MIPI_GetDefaultFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*feature_data, (MUINT32 *)(uintptr_t)(*(feature_data+1)));
    break;   
    
	case SENSOR_FEATURE_SET_YUV_3A_CMD:
		 SENSORDB("[SP0A20] SENSOR_FEATURE_SET_YUV_3A_CMD ID:%d\n", *pFeatureData32);
		 SP0A20_3ACtrl((ACDK_SENSOR_3A_LOCK_ENUM)*feature_data);
		 break;


	case SENSOR_FEATURE_GET_EXIF_INFO:
		 //SENSORDB("[4EC] F_GET_EXIF_INFO\n");
		 SP0A20_MIPI_GetExifInfo((uintptr_t)*feature_data);
		 break;


	case SENSOR_FEATURE_GET_AE_AWB_LOCK_INFO:
		 SENSORDB("[SP0A20] F_GET_AE_AWB_LOCK_INFO\n");
		 //SP0A20_MIPI_get_AEAWB_lock((uintptr_t)(*feature_data), (uintptr_t)*(feature_data+1));
	break;    

	case SENSOR_FEATURE_SET_MIN_MAX_FPS:
		SENSORDB("SENSOR_FEATURE_SET_MIN_MAX_FPS:[%d,%d]\n",*pFeatureData32,*(pFeatureData32+1)); 
		SP0A20_MIPI_SetMaxMinFps((UINT32)*feature_data, (UINT32)*(feature_data+1));
	break; 
	
    default:
        break;
	}
return ERROR_NONE;
}	/* SP0A20FeatureControl() */


SENSOR_FUNCTION_STRUCT	SensorFuncSP0A20YUV=
{
	SP0A20Open,
	SP0A20GetInfo,
	SP0A20GetResolution,
	SP0A20FeatureControl,
	SP0A20Control,
	SP0A20Close
};

SENSOR_FUNCTION_STRUCT_1	SensorFuncSP0A20YUV_1=
{
	SP0A20Open,
	SP0A20_Read_Shutter,
	SP0A20_GetSensorID,
};


UINT32 SP0A20_MIPI_YUV_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&SensorFuncSP0A20YUV;
	return ERROR_NONE;
} /* SensorInit() */
