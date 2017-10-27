/*****************************************************************************
 *
 * Filename:
 * ---------
 *   gc0310yuv_Sensor.c
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

#include "kd_camera_typedef.h"
#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "kd_camera_feature.h"

#include "gc0310mipi_yuv_Sensor.h"
#include <linux/hct_include/hct_project_all_config.h>


static DEFINE_SPINLOCK(GC0310_drv_lock);


#define GC0310YUV_DEBUG

#ifdef GC0310YUV_DEBUG
#define SENSORDB printk
#else
#define SENSORDB(x,...)
#endif

#define GC0310_TEST_PATTERN_CHECKSUM (0x9db2de6e)

kal_bool GC0310_night_mode_enable = KAL_FALSE;
kal_uint16 GC0310_CurStatus_AWB = 0;
#if __HCT_DUAL_CAMERA_USEDBY_YUV_MODE__

extern int GC0310_iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 *a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int GC0310_iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);
#endif


static void GC0310_awb_enable(kal_bool enalbe);

extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);

kal_uint16 GC0310_write_cmos_sensor(kal_uint8 addr, kal_uint8 para)
{
    char puSendCmd[2] = {(char)(addr & 0xFF) , (char)(para & 0xFF)};
	
#if  __HCT_DUAL_CAMERA_USEDBY_YUV_MODE__
	GC0310_iWriteRegI2C(puSendCmd , 2, GC0310_WRITE_ID);
#else
	iWriteRegI2C(puSendCmd , 2, GC0310_WRITE_ID);
#endif
	return 0;
}
kal_uint16 GC0310_read_cmos_sensor(kal_uint8 addr)
{
	kal_uint16 get_byte=0;
    char puSendCmd = { (char)(addr & 0xFF) };
	
#if  __HCT_DUAL_CAMERA_USEDBY_YUV_MODE__
	GC0310_iReadRegI2C(&puSendCmd , 1, (u8*)&get_byte, 1, GC0310_READ_ID);
#else
	iReadRegI2C(&puSendCmd , 1, (u8*)&get_byte, 1, GC0310_READ_ID);
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

kal_bool   GC0310_MPEG4_encode_mode = KAL_FALSE;
kal_uint16 GC0310_dummy_pixels = 0, GC0310_dummy_lines = 0;
kal_bool   GC0310_MODE_CAPTURE = KAL_FALSE;
kal_bool   GC0310_NIGHT_MODE = KAL_FALSE;

kal_uint32 MINFramerate = 0;
kal_uint32 MAXFramerate = 0;

kal_uint32 GC0310_isp_master_clock;
static kal_uint32 GC0310_g_fPV_PCLK = 30 * 1000000;

kal_uint8 GC0310_sensor_write_I2C_address = GC0310_WRITE_ID;
kal_uint8 GC0310_sensor_read_I2C_address = GC0310_READ_ID;

UINT8 GC0310PixelClockDivider=0;

MSDK_SENSOR_CONFIG_STRUCT GC0310SensorConfigData;

#define GC0310_SET_PAGE0 	GC0310_write_cmos_sensor(0xfe, 0x00)
#define GC0310_SET_PAGE1 	GC0310_write_cmos_sensor(0xfe, 0x01)
#define GC0310_SET_PAGE2 	GC0310_write_cmos_sensor(0xfe, 0x02)
#define GC0310_SET_PAGE3 	GC0310_write_cmos_sensor(0xfe, 0x03)

/*************************************************************************
 * FUNCTION
 *	GC0310_SetShutter
 *
 * DESCRIPTION
 *	This function set e-shutter of GC0310 to change exposure time.
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
void GC0310_Set_Shutter(kal_uint16 iShutter)
{
} /* Set_GC0310_Shutter */


/*************************************************************************
 * FUNCTION
 *	GC0310_read_Shutter
 *
 * DESCRIPTION
 *	This function read e-shutter of GC0310 .
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
kal_uint32 GC0310_Read_Shutter(void)
{
    	kal_uint8 temp_reg1, temp_reg2, temp_reg3;
	kal_uint32 shutter;

	temp_reg1 = GC0310_read_cmos_sensor(0x04);
	temp_reg2 = GC0310_read_cmos_sensor(0x03);
	temp_reg3 = GC0310_read_cmos_sensor(0xef);

	shutter = (temp_reg1 & 0xFF) | (temp_reg2 << 8)| (temp_reg3 << 16);
printk("GC0310_Read_Shutter shutter=%d\n",shutter);
	return shutter;
} /* GC0310_read_shutter */

static void GC0310_Set_Mirrorflip(kal_uint8 image_mirror)
{
	


}

static void GC0310_set_AE_mode(kal_bool AE_enable)
{


}


void GC0310_set_contrast(UINT16 para)
{   
   
}

UINT32 GC0310_MIPI_SetMaxFramerateByScenario(
  MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 frameRate)
{
	SENSORDB("scenarioId = %d\n", scenarioId);
	return 0;
}


UINT32 GC0310SetTestPatternMode(kal_bool bEnable)
{

	return ERROR_NONE;
}


void GC0310_set_brightness(UINT16 para)
{


}
void GC0310_set_saturation(UINT16 para)
{

    
}

void GC0310_set_edge(UINT16 para)
{
    
}


void GC0310_set_iso(UINT16 para)
{

}



void GC0310StreamOn(void)
{
	//Sleep(150);
    GC0310_write_cmos_sensor(0xfe,0x03);
    GC0310_write_cmos_sensor(0x10,0x94); 
    GC0310_write_cmos_sensor(0xfe,0x00);    
    Sleep(50);	
}

void GC0310_MIPI_GetDelayInfo(uintptr_t delayAddr)
{
    SENSOR_DELAY_INFO_STRUCT* pDelayInfo = (SENSOR_DELAY_INFO_STRUCT*)delayAddr;
    pDelayInfo->InitDelay = 2;
    pDelayInfo->EffectDelay = 2;
    pDelayInfo->AwbDelay = 2;
    pDelayInfo->AFSwitchDelayFrame = 50;
}

UINT32 GC0310_MIPI_GetDefaultFramerateByScenario(
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

void GC0310_MIPI_SetMaxMinFps(UINT32 u2MinFrameRate, UINT32 u2MaxFrameRate)
{
	SENSORDB("GC0310_MIPI_SetMaxMinFps+ :FrameRate= %d %d\n",u2MinFrameRate,u2MaxFrameRate);
	spin_lock(&GC0310_drv_lock);
	MINFramerate = u2MinFrameRate;
	MAXFramerate = u2MaxFrameRate;
	spin_unlock(&GC0310_drv_lock);
	return;
}

void GC0310_3ACtrl(ACDK_SENSOR_3A_LOCK_ENUM action)
{
	SENSORDB("[GC0329]enter ACDK_SENSOR_3A_LOCK_ENUM function:action=%d\n",action);
   switch (action)
   {
      case SENSOR_3A_AE_LOCK:
          GC0310_set_AE_mode(KAL_FALSE);
      break;
      case SENSOR_3A_AE_UNLOCK:
          GC0310_set_AE_mode(KAL_TRUE);
      break;

      case SENSOR_3A_AWB_LOCK:
          GC0310_awb_enable(KAL_FALSE);       
      break;

      case SENSOR_3A_AWB_UNLOCK:
		   if (((AWB_MODE_OFF == GC0310_CurStatus_AWB) ||
        		(AWB_MODE_AUTO == GC0310_CurStatus_AWB)))
        	{	
         			 GC0310_awb_enable(KAL_TRUE);
         	}
      break;
      default:
      	break;
   }
   SENSORDB("[GC0329]exit ACDK_SENSOR_3A_LOCK_ENUM function:action=%d\n",action);
   return;
}


void GC0310_MIPI_GetExifInfo(uintptr_t exifAddr)
{
    SENSOR_EXIF_INFO_STRUCT* pExifInfo = (SENSOR_EXIF_INFO_STRUCT*)exifAddr;
    pExifInfo->FNumber = 28;
//    pExifInfo->AEISOSpeed = GC0310_Driver.isoSpeed;
//    pExifInfo->AWBMode = S5K4ECGX_Driver.awbMode;
//    pExifInfo->CapExposureTime = S5K4ECGX_Driver.capExposureTime;
//    pExifInfo->FlashLightTimeus = 0;
//   pExifInfo->RealISOValue = (S5K4ECGX_MIPI_ReadGain()*57) >> 8;
        //S5K4ECGX_Driver.isoSpeed;
}

#if 0
void GC0310_MIPI_get_AEAWB_lock(uintptr_t pAElockRet32, uintptr_t pAWBlockRet32)
{
    *pAElockRet32 = 1;
    *pAWBlockRet32 = 1;
    SENSORDB("[GC0310]GetAEAWBLock,AE=%d ,AWB=%d\n,",(int)*pAElockRet32, (int)*pAWBlockRet32);
}
#endif

/*************************************************************************
 * FUNCTION
 *	GC0310_write_reg
 *
 * DESCRIPTION
 *	This function set the register of GC0310.
 *
 * PARAMETERS
 *	addr : the register index of GC0310
 *  para : setting parameter of the specified register of GC0310
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
void GC0310_write_reg(kal_uint32 addr, kal_uint32 para)
{
	GC0310_write_cmos_sensor(addr, para);
} /* GC0310_write_reg() */


/*************************************************************************
 * FUNCTION
 *	GC0310_read_cmos_sensor
 *
 * DESCRIPTION
 *	This function read parameter of specified register from GC0310.
 *
 * PARAMETERS
 *	addr : the register index of GC0310
 *
 * RETURNS
 *	the data that read from GC0310
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
kal_uint32 GC0310_read_reg(kal_uint32 addr)
{
	return GC0310_read_cmos_sensor(addr);
} /* OV7670_read_reg() */


/*************************************************************************
* FUNCTION
*	GC0310_awb_enable
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
static void GC0310_awb_enable(kal_bool enalbe)
{	 
	

}


/*************************************************************************
* FUNCTION
*	GC0310_GAMMA_Select
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
void GC0310GammaSelect(kal_uint32 GammaLvl)
{
	
}


/*************************************************************************
 * FUNCTION
 *	GC0310_config_window
 *
 * DESCRIPTION
 *	This function config the hardware window of GC0310 for getting specified
 *  data of that window.
 *
 * PARAMETERS
 *	start_x : start column of the interested window
 *  start_y : start row of the interested window
 *  width  : column widht of the itnerested window
 *  height : row depth of the itnerested window
 *
 * RETURNS
 *	the data that read from GC0310
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
void GC0310_config_window(kal_uint16 startx, kal_uint16 starty, kal_uint16 width, kal_uint16 height)
{
} /* GC0310_config_window */


/*************************************************************************
 * FUNCTION
 *	GC0310_SetGain
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
kal_uint16 GC0310_SetGain(kal_uint16 iGain)
{
	return iGain;
}


/*************************************************************************
 * FUNCTION
 *	GC0310_NightMode
 *
 * DESCRIPTION
 *	This function night mode of GC0310.
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
void GC0310NightMode(kal_bool bEnable)
{
	SENSORDB("Enter GC0310NightMode!, bEnable = %d, GC0310_MPEG4_encode_mode = %d\n", bEnable, GC0310_MPEG4_encode_mode);

	
	
} /* GC0310_NightMode */

/*************************************************************************
* FUNCTION
*	GC0310_Sensor_Init
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
void GC0310_Sensor_Init(void)
{
    GC0310_write_cmos_sensor(0xfe,0xf0);
    GC0310_write_cmos_sensor(0xfe,0xf0);
    GC0310_write_cmos_sensor(0xfe,0x00);
    GC0310_write_cmos_sensor(0xfc,0x0e);
    GC0310_write_cmos_sensor(0xfc,0x0e);
    GC0310_write_cmos_sensor(0xf2,0x80);
    GC0310_write_cmos_sensor(0xf3,0x00);
    GC0310_write_cmos_sensor(0xf7,0x1b);
    GC0310_write_cmos_sensor(0xf8,0x04);  // from 03 to 04
    GC0310_write_cmos_sensor(0xf9,0x8e);
    GC0310_write_cmos_sensor(0xfa,0x11);
     /////////////////////////////////////////////////      
	///////////////////   MIPI   ////////////////////      
	/////////////////////////////////////////////////      
	GC0310_write_cmos_sensor(0xfe,0x03);
	GC0310_write_cmos_sensor(0x40,0x08);
	GC0310_write_cmos_sensor(0x42,0x00);
	GC0310_write_cmos_sensor(0x43,0x00);
	GC0310_write_cmos_sensor(0x01,0x03);
	GC0310_write_cmos_sensor(0x10,0x84);
                                        
	GC0310_write_cmos_sensor(0x01,0x03);             
	GC0310_write_cmos_sensor(0x02,0x00);             
	GC0310_write_cmos_sensor(0x03,0x94);             
	GC0310_write_cmos_sensor(0x04,0x01);            
	GC0310_write_cmos_sensor(0x05,0x40);  // 40      20     
	GC0310_write_cmos_sensor(0x06,0x80);             
	GC0310_write_cmos_sensor(0x11,0x1e);             
	GC0310_write_cmos_sensor(0x12,0x00);      
	GC0310_write_cmos_sensor(0x13,0x05);             
	GC0310_write_cmos_sensor(0x15,0x10);                                                                    
	GC0310_write_cmos_sensor(0x21,0x10);             
	GC0310_write_cmos_sensor(0x22,0x01);             
	GC0310_write_cmos_sensor(0x23,0x10);                                             
	GC0310_write_cmos_sensor(0x24,0x02);                                             
	GC0310_write_cmos_sensor(0x25,0x10);                                             
	GC0310_write_cmos_sensor(0x26,0x03);                                             
	GC0310_write_cmos_sensor(0x29,0x02); //02                                            
	GC0310_write_cmos_sensor(0x2a,0x0a);   //0a                                          
	GC0310_write_cmos_sensor(0x2b,0x04);                                             
	GC0310_write_cmos_sensor(0xfe,0x00);
        /////////////////////////////////////////////////
        /////////////////   CISCTL reg  /////////////////
        /////////////////////////////////////////////////
    GC0310_write_cmos_sensor(0x00,0x2f);
    GC0310_write_cmos_sensor(0x01,0x0f);
    GC0310_write_cmos_sensor(0x02,0x04);
    GC0310_write_cmos_sensor(0x03,0x04);
    GC0310_write_cmos_sensor(0x04,0xd0);
    GC0310_write_cmos_sensor(0x09,0x00);
    GC0310_write_cmos_sensor(0x0a,0x00);
    GC0310_write_cmos_sensor(0x0b,0x00);
    GC0310_write_cmos_sensor(0x0c,0x06);
    GC0310_write_cmos_sensor(0x0d,0x01);
    GC0310_write_cmos_sensor(0x0e,0xe8);
    GC0310_write_cmos_sensor(0x0f,0x02);
    GC0310_write_cmos_sensor(0x10,0x88);
    GC0310_write_cmos_sensor(0x16,0x00);
    GC0310_write_cmos_sensor(0x17,0x14);
    GC0310_write_cmos_sensor(0x18,0x1a);
    GC0310_write_cmos_sensor(0x19,0x14);
    GC0310_write_cmos_sensor(0x1b,0x48);
    GC0310_write_cmos_sensor(0x1e,0x6b);
    GC0310_write_cmos_sensor(0x1f,0x28);
    GC0310_write_cmos_sensor(0x20,0x8b);  // from 89 to 8b
    GC0310_write_cmos_sensor(0x21,0x49);
    GC0310_write_cmos_sensor(0x22,0xb0);
    GC0310_write_cmos_sensor(0x23,0x04);
    GC0310_write_cmos_sensor(0x24,0x16);
    GC0310_write_cmos_sensor(0x34,0x20);
        
        /////////////////////////////////////////////////
        ////////////////////   BLK   ////////////////////
        /////////////////////////////////////////////////
    GC0310_write_cmos_sensor(0x26,0x23); 
    GC0310_write_cmos_sensor(0x28,0xff); 
    GC0310_write_cmos_sensor(0x29,0x00); 
    GC0310_write_cmos_sensor(0x33,0x10); 
    GC0310_write_cmos_sensor(0x37,0x20); 
    GC0310_write_cmos_sensor(0x38,0x10); 
    GC0310_write_cmos_sensor(0x47,0x80); 
    GC0310_write_cmos_sensor(0x4e,0x66); 
    GC0310_write_cmos_sensor(0xa8,0x02); 
    GC0310_write_cmos_sensor(0xa9,0x80);
        
        /////////////////////////////////////////////////
        //////////////////   ISP reg  ///////////////////
        /////////////////////////////////////////////////
    GC0310_write_cmos_sensor(0x40,0xff); 
    GC0310_write_cmos_sensor(0x41,0x21); 
    GC0310_write_cmos_sensor(0x42,0xcf); 
    GC0310_write_cmos_sensor(0x44,0x01); // 02 yuv 
    GC0310_write_cmos_sensor(0x45,0xa0); // from a8 - a4 a4-a0
    GC0310_write_cmos_sensor(0x46,0x03); 
    GC0310_write_cmos_sensor(0x4a,0x11);
    GC0310_write_cmos_sensor(0x4b,0x01);
    GC0310_write_cmos_sensor(0x4c,0x20); 
    GC0310_write_cmos_sensor(0x4d,0x05); 
    GC0310_write_cmos_sensor(0x4f,0x01);
    GC0310_write_cmos_sensor(0x50,0x01); 
    GC0310_write_cmos_sensor(0x55,0x01); 
    GC0310_write_cmos_sensor(0x56,0xe0);
    GC0310_write_cmos_sensor(0x57,0x02); 
    GC0310_write_cmos_sensor(0x58,0x80);
        
        /////////////////////////////////////////////////  
        ///////////////////   GAIN   ////////////////////
        /////////////////////////////////////////////////
    GC0310_write_cmos_sensor(0x70,0x70); 
    GC0310_write_cmos_sensor(0x5a,0x84); 
    GC0310_write_cmos_sensor(0x5b,0xc9); 
    GC0310_write_cmos_sensor(0x5c,0xed); 
    GC0310_write_cmos_sensor(0x77,0x74); 
    GC0310_write_cmos_sensor(0x78,0x40); 
    GC0310_write_cmos_sensor(0x79,0x5f); 
        
      
      
        

        /////////////////////////////////////////////////
        ////////////////////   AEC   ////////////////////
        /////////////////////////////////////////////////
    GC0310_write_cmos_sensor(0xfe,0x01);
    GC0310_write_cmos_sensor(0x05,0x30); 
    GC0310_write_cmos_sensor(0x06,0x75); 
    GC0310_write_cmos_sensor(0x07,0x40); 
    GC0310_write_cmos_sensor(0x08,0xb0); 
    GC0310_write_cmos_sensor(0x0a,0xc5); 
    GC0310_write_cmos_sensor(0x0b,0x11);
    GC0310_write_cmos_sensor(0x0c,0x00); 
    GC0310_write_cmos_sensor(0x12,0x52);
    GC0310_write_cmos_sensor(0x13,0x38); 
    GC0310_write_cmos_sensor(0x18,0x95);
    GC0310_write_cmos_sensor(0x19,0x96);
    GC0310_write_cmos_sensor(0x1f,0x20);
    GC0310_write_cmos_sensor(0x20,0xc0); 
    GC0310_write_cmos_sensor(0x3e,0x40); 
    GC0310_write_cmos_sensor(0x3f,0x57); 
    GC0310_write_cmos_sensor(0x40,0x7d); 
    GC0310_write_cmos_sensor(0x03,0x60); 
    GC0310_write_cmos_sensor(0x44,0x02); 
    
    GC0310_write_cmos_sensor(0xde,0x00); 
        
     
        
        /////////////////////////////////////////////////
        //////////////   Measure Window   ///////////////
        /////////////////////////////////////////////////
    GC0310_write_cmos_sensor(0xcc,0x0c);  
    GC0310_write_cmos_sensor(0xcd,0x10); 
    GC0310_write_cmos_sensor(0xce,0xa0); 
    GC0310_write_cmos_sensor(0xcf,0xe6); 
        

        
        /////////////////////////////////////////////////
        ///////////////////  banding  ///////////////////
        /////////////////////////////////////////////////
    GC0310_write_cmos_sensor(0xfe,0x00);
    GC0310_write_cmos_sensor(0x05,0x01);
    GC0310_write_cmos_sensor(0x06,0x18); //HB
#if 1    
    GC0310_write_cmos_sensor(0x07,0x00);
    GC0310_write_cmos_sensor(0x08,0x10); //VB  from 10 to 50
#else
	GC0310_write_cmos_sensor(0x07,0x01);
	GC0310_write_cmos_sensor(0x08,0xe0); //VB
#endif
    GC0310_write_cmos_sensor(0xfe,0x01);
    GC0310_write_cmos_sensor(0x25,0x00); //step 
    GC0310_write_cmos_sensor(0x26,0x9a); 
    GC0310_write_cmos_sensor(0x27,0x01); //30fps
    GC0310_write_cmos_sensor(0x28,0xce);  
    GC0310_write_cmos_sensor(0x29,0x04); //12.5fps
	GC0310_write_cmos_sensor(0x2a,0x36); 
	GC0310_write_cmos_sensor(0x2b,0x06); //10fps
	GC0310_write_cmos_sensor(0x2c,0x04); 
	GC0310_write_cmos_sensor(0x2d,0x0c); //5fps
	GC0310_write_cmos_sensor(0x2e,0x08);
    GC0310_write_cmos_sensor(0x3c,0x30);
        
        /////////////////////////////////////////////////
        ///////////////////   MIPI   ////////////////////
        /////////////////////////////////////////////////
   GC0310_write_cmos_sensor(0xfe,0x03);
   GC0310_write_cmos_sensor(0x10,0x94);  
   GC0310_write_cmos_sensor(0xfe,0x00); 
}


UINT32 GC0310GetSensorID(UINT32 *sensorID)
{
    int  retry = 3; 
    // check if sensor ID correct
    do {
        *sensorID=((GC0310_read_cmos_sensor(0xf0)<< 8)|GC0310_read_cmos_sensor(0xf1));
        if (*sensorID == GC0310_SENSOR_ID)
            break; 
        SENSORDB("Read Sensor ID Fail = 0x%04x\n", *sensorID); 
        retry--; 
    } while (retry > 0);

    if (*sensorID != GC0310_SENSOR_ID) {
        *sensorID = 0xFFFFFFFF; 
        return ERROR_SENSOR_CONNECT_FAIL;
    }
    return ERROR_NONE;    
}

UINT32 GC0310_GetSensorID(void)
{
    int  retry = 3; 
	UINT32 sensorID;
    // check if sensor ID correct
    do {
        sensorID=((GC0310_read_cmos_sensor(0xf0)<< 8)|GC0310_read_cmos_sensor(0xf1));
        if (sensorID == GC0310_SENSOR_ID)
            break; 
        SENSORDB("Read Sensor ID Fail = 0x%04x\n", sensorID); 
        retry--; 
    } while (retry > 0);

    if (sensorID != GC0310_SENSOR_ID) {
        sensorID = 0xFFFFFFFF; 
        return ERROR_SENSOR_CONNECT_FAIL;
    }
    return ERROR_NONE;    
}



/*************************************************************************
* FUNCTION
*	GC0310_Write_More_Registers
*
* DESCRIPTION
*	This function is served for FAE to modify the necessary Init Regs. Do not modify the regs
*     in init_GC0310() directly.
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
void GC0310_Write_More_Registers(void)
{

}


/*************************************************************************
 * FUNCTION
 *	GC0310Open
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
UINT32 GC0310Open(void)
{
	volatile signed char i;
	kal_uint16 sensor_id=0;

	//SENSORDB("<Jet> Entry GC0310Open!!!\r\n");

    for(i=0;i<3;i++)
		{
		sensor_id = ((GC0310_read_cmos_sensor(0xf0) << 8) | GC0310_read_cmos_sensor(0xf1));
		if(sensor_id == GC0310_SENSOR_ID)  
		{
			SENSORDB("GC0310mipi Read Sensor ID ok[open] = 0x%x\n", sensor_id); 
			break;
		}
		}
	//  Read sensor ID to adjust I2C is OK?
	if(sensor_id != GC0310_SENSOR_ID)  
	{
		SENSORDB("GC0310mipi Read Sensor ID Fail[open] = 0x%x\n", sensor_id); 
		return ERROR_SENSOR_CONNECT_FAIL;
	}
	GC0310_Sensor_Init();

	return ERROR_NONE;
} /* GC0310Open */


/*************************************************************************
 * FUNCTION
 *	GC0310Close
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
UINT32 GC0310Close(void)
{
    return ERROR_NONE;
} /* GC0310Close */


/*************************************************************************
 * FUNCTION
 * GC0310Preview
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
UINT32 GC0310Preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
        MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)

{
 //   kal_uint32 iTemp;
//    kal_uint16 iStartX = 0, iStartY = 1;

	SENSORDB("Enter GC0310Preview function!!!\r\n");
//	GC0310StreamOn();

    image_window->GrabStartX= IMAGE_SENSOR_VGA_GRAB_PIXELS;
    image_window->GrabStartY= IMAGE_SENSOR_VGA_GRAB_LINES;
    image_window->ExposureWindowWidth = IMAGE_SENSOR_PV_WIDTH;
    image_window->ExposureWindowHeight =IMAGE_SENSOR_PV_HEIGHT;

    GC0310_Set_Mirrorflip(IMAGE_NORMAL);
    

    // copy sensor_config_data
    memcpy(&GC0310SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
//    GC0310NightMode(GC0310_night_mode_enable);
    return ERROR_NONE;
} /* GC0310Preview */


/*************************************************************************
 * FUNCTION
 *	GC0310Capture
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
UINT32 GC0310Capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
        MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)

{
	spin_lock(&GC0310_drv_lock);
    GC0310_MODE_CAPTURE=KAL_TRUE;
	spin_unlock(&GC0310_drv_lock);


    image_window->GrabStartX = IMAGE_SENSOR_VGA_GRAB_PIXELS;
    image_window->GrabStartY = IMAGE_SENSOR_VGA_GRAB_LINES;
    image_window->ExposureWindowWidth= IMAGE_SENSOR_FULL_WIDTH;
    image_window->ExposureWindowHeight = IMAGE_SENSOR_FULL_HEIGHT;

    // copy sensor_config_data
    memcpy(&GC0310SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
    return ERROR_NONE;
} /* GC0310_Capture() */



UINT32 GC0310GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
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
} /* GC0310GetResolution() */


UINT32 GC0310GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
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
    GC0310PixelClockDivider=pSensorInfo->SensorPixelClockCount;
    memcpy(pSensorConfigData, &GC0310SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
    return ERROR_NONE;
} /* GC0310GetInfo() */


UINT32 GC0310Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
        MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{

	SENSORDB("Entry GC0310Control, ScenarioId = %d!!!\r\n", ScenarioId);
    switch (ScenarioId)
    {
    case MSDK_SCENARIO_ID_VIDEO_PREVIEW:   
    	SENSORDB("GC0310 Camera Video Preview!\n");
		spin_lock(&GC0310_drv_lock);         
        GC0310_MPEG4_encode_mode = KAL_TRUE;
        spin_unlock(&GC0310_drv_lock); 
		GC0310Preview(pImageWindow, pSensorConfigData);        
        break;
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:       
    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
    default:
        spin_lock(&GC0310_drv_lock);  
        GC0310_MPEG4_encode_mode = KAL_FALSE;
        spin_unlock(&GC0310_drv_lock);
		GC0310Preview(pImageWindow, pSensorConfigData);        
        break;
    }
    
    return ERROR_NONE;
}	/* GC0310Control() */

BOOL GC0310_set_param_wb(UINT16 para)
{
	return TRUE;
} /* GC0310_set_param_wb */


BOOL GC0310_set_param_effect(UINT16 para)
{
	
	return TRUE;

} /* GC0310_set_param_effect */


BOOL GC0310_set_param_banding(UINT16 para)
{
return TRUE;

} /* GC0310_set_param_banding */

BOOL GC0310_set_param_exposure(UINT16 para)
{
	
	return TRUE;
} /* GC0310_set_param_exposure */


UINT32 GC0310YUVSetVideoMode(UINT16 u2FrameRate)    // lanking add
{


    return TRUE;

}


UINT32 GC0310YUVSensorSetting(FEATURE_ID iCmd, UINT16 iPara)
{
	SENSORDB("GC0310YUVSensorSetting: icmd is  %d and para is %d!!\n", iCmd, iPara);
    switch (iCmd) {
    case FID_AWB_MODE:
        GC0310_set_param_wb(iPara);
        break;
    case FID_COLOR_EFFECT:
        GC0310_set_param_effect(iPara);
        break;
    case FID_AE_EV:
        GC0310_set_param_exposure(iPara);
        break;
    case FID_AE_FLICKER:
        GC0310_set_param_banding(iPara);
		break;
    case FID_SCENE_MODE:
	 	GC0310NightMode(iPara);
        break;
        
	case FID_ISP_CONTRAST:
		GC0310_set_contrast(iPara);
		break;
	case FID_ISP_BRIGHT:
		GC0310_set_brightness(iPara);
		break;
	case FID_ISP_SAT:
		GC0310_set_saturation(iPara);
		break; 
	case FID_ISP_EDGE:
		GC0310_set_edge(iPara);
		break; 
	case FID_AE_ISO:
		GC0310_set_iso(iPara);
		break;	
	case FID_AE_SCENE_MODE: 
		GC0310_set_AE_mode(iPara);
		break; 
		
    default:
        break;
    }
    return TRUE;
} /* GC0310YUVSensorSetting */


UINT32 GC0310FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
        UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{
    UINT16 *pFeatureReturnPara16=(UINT16 *) pFeaturePara;
//    UINT16 *pFeatureData16=(UINT16 *) pFeaturePara;
    UINT32 *pFeatureReturnPara32=(UINT32 *) pFeaturePara;
    UINT32 *pFeatureData32=(UINT32 *) pFeaturePara;
 //   UINT32 **ppFeatureData=(UINT32 **) pFeaturePara;
    unsigned long long *feature_data=(unsigned long long *) pFeaturePara;
 //   unsigned long long *feature_return_para=(unsigned long long *) pFeaturePara;
	
   // UINT32 GC0310SensorRegNumber;
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
        *pFeatureReturnPara16++=(VGA_PERIOD_PIXEL_NUMS)+GC0310_dummy_pixels;
        *pFeatureReturnPara16=(VGA_PERIOD_LINE_NUMS)+GC0310_dummy_lines;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
        *pFeatureReturnPara32 = GC0310_g_fPV_PCLK;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_SET_ESHUTTER:
        break;
    case SENSOR_FEATURE_SET_NIGHTMODE:
        GC0310NightMode((BOOL) *feature_data);
        break;
    case SENSOR_FEATURE_SET_GAIN:
    case SENSOR_FEATURE_SET_FLASHLIGHT:
        break;
    case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
        GC0310_isp_master_clock=*pFeatureData32;
        break;
    case SENSOR_FEATURE_SET_REGISTER:
        GC0310_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
        break;
    case SENSOR_FEATURE_GET_REGISTER:
        pSensorRegData->RegData = GC0310_read_cmos_sensor(pSensorRegData->RegAddr);
        break;
    case SENSOR_FEATURE_GET_CONFIG_PARA:
        memcpy(pSensorConfigData, &GC0310SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
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
        GC0310YUVSensorSetting((FEATURE_ID)*feature_data, *(feature_data+1));
        
        break;
    case SENSOR_FEATURE_SET_VIDEO_MODE:    //  lanking
	 GC0310YUVSetVideoMode(*feature_data);
	 break;
    case SENSOR_FEATURE_CHECK_SENSOR_ID:
	GC0310GetSensorID(pFeatureData32);
	break;
	case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE:
		*pFeatureReturnPara32=GC0310_TEST_PATTERN_CHECKSUM;
		*pFeatureParaLen=4;
		break;

	case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
		 SENSORDB("[GC0310] F_SET_MAX_FRAME_RATE_BY_SCENARIO.\n");
		 GC0310_MIPI_SetMaxFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*feature_data, *(feature_data+1));
		 break;

//	case SENSOR_CMD_SET_VIDEO_FRAME_RATE:
//		SENSORDB("[GC0310] Enter SENSOR_CMD_SET_VIDEO_FRAME_RATE\n");
//		//GC0310_MIPI_SetVideoFrameRate(*pFeatureData32);
//		break;

	case SENSOR_FEATURE_SET_TEST_PATTERN:			 
		GC0310SetTestPatternMode((BOOL)*feature_data);			
		break;

    case SENSOR_FEATURE_GET_DELAY_INFO:
        SENSORDB("[GC0310] F_GET_DELAY_INFO\n");
        GC0310_MIPI_GetDelayInfo((uintptr_t)*feature_data);
    break;

    case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
         SENSORDB("[GC0310] F_GET_DEFAULT_FRAME_RATE_BY_SCENARIO\n");
         GC0310_MIPI_GetDefaultFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*feature_data, (MUINT32 *)(uintptr_t)(*(feature_data+1)));
    break;   
    
	case SENSOR_FEATURE_SET_YUV_3A_CMD:
		 SENSORDB("[GC0310] SENSOR_FEATURE_SET_YUV_3A_CMD ID:%d\n", *pFeatureData32);
		 GC0310_3ACtrl((ACDK_SENSOR_3A_LOCK_ENUM)*feature_data);
		 break;


	case SENSOR_FEATURE_GET_EXIF_INFO:
		 //SENSORDB("[4EC] F_GET_EXIF_INFO\n");
		 GC0310_MIPI_GetExifInfo((uintptr_t)*feature_data);
		 break;


	case SENSOR_FEATURE_GET_AE_AWB_LOCK_INFO:
		 SENSORDB("[GC0310] F_GET_AE_AWB_LOCK_INFO\n");
		 //GC0310_MIPI_get_AEAWB_lock((uintptr_t)(*feature_data), (uintptr_t)*(feature_data+1));
	break;    

	case SENSOR_FEATURE_SET_MIN_MAX_FPS:
		SENSORDB("SENSOR_FEATURE_SET_MIN_MAX_FPS:[%d,%d]\n",*pFeatureData32,*(pFeatureData32+1)); 
		GC0310_MIPI_SetMaxMinFps((UINT32)*feature_data, (UINT32)*(feature_data+1));
	break; 
	
    default:
        break;
	}
return ERROR_NONE;
}	/* GC0310FeatureControl() */


SENSOR_FUNCTION_STRUCT	SensorFuncGC0310YUV=
{
	GC0310Open,
	GC0310GetInfo,
	GC0310GetResolution,
	GC0310FeatureControl,
	GC0310Control,
	GC0310Close
};
SENSOR_FUNCTION_STRUCT_1	SensorFuncGC0310YUV_1=
{
	GC0310Open,
	GC0310_Read_Shutter,
	GC0310_GetSensorID,
};


UINT32 GC0310_YUV_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&SensorFuncGC0310YUV;
	return ERROR_NONE;
} /* SensorInit() */
