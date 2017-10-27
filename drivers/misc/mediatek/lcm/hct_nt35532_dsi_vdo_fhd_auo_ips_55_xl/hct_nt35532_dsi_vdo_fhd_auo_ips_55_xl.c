
/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2008
*
*  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
*  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
*  RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
*  AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
*  NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
*  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
*  SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
*  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
*  NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S
*  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
*  BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
*  LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
*  AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
*  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
*  MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*
*  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
*  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
*  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
*  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
*  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
*
*****************************************************************************/
#ifndef BUILD_LK
#include <linux/string.h>
#endif
#include "lcm_drv.h"

#ifdef BUILD_LK
	#include <platform/mt_gpio.h>
#elif defined(BUILD_UBOOT)
	#include <asm/arch/mt_gpio.h>
#else
//	#include <mach/mt_gpio.h>
#endif
// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  										(1080)
#define FRAME_HEIGHT 										(1920)
#define LCM_ID_NT35532 										(0x32)
#define REGFLAG_DELAY             							0xAB
#define REGFLAG_END_OF_TABLE      							0xAA   // END OF REGISTERS MARKER

#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif


#define   LCM_DSI_CMD_MODE							0

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))

#define UDELAY(n) 											(lcm_util.udelay(n))
#define MDELAY(n) 											(lcm_util.mdelay(n))


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	        lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)				lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)										lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)   			lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)    

#define changcmd(a,b,c,d) ((d<<24)|(c<<16)|(b<<8)|a)

#define   LCM_DSI_CMD_MODE									0

//int vcom=0x60;

static void lcm_power_5v_en(unsigned char enabled)
{
	if (enabled)
	{
	mt_dsi_pinctrl_set(LCM_POWER_ENN, 1);
	}	
	else
	{
	mt_dsi_pinctrl_set(LCM_POWER_ENN, 0);
	}
}

static void lcm_power_n5v_en(unsigned char enabled)
{
	if (enabled)
	{
	mt_dsi_pinctrl_set(LCM_POWER_ENP, 1);
	}	
	else
	{
	mt_dsi_pinctrl_set(LCM_POWER_ENP, 0);
	}
}


static void NT35596_DCS_write_1A_1P(unsigned char cmd, unsigned char para)
{
	unsigned int data_array[16];

    data_array[0] = (0x00023902);
    data_array[1] = (0x00000000 | (para << 8) | (cmd));
    dsi_set_cmdq(data_array, 2, 1);

}

static void NT35596_DCS_write_1A_0P(unsigned char cmd)
{
	unsigned int data_array[16];

	data_array[0]=(0x00000500 | (cmd<<16));
	dsi_set_cmdq(data_array, 1, 1);

}

static void init_lcm_registers(void)
{

//LCD driver initialization

NT35596_DCS_write_1A_1P(0xFF,0x01); 
NT35596_DCS_write_1A_1P(0xFB,0x01); 

NT35596_DCS_write_1A_1P(0x00,0x01); 
NT35596_DCS_write_1A_1P(0x01,0x44); 
NT35596_DCS_write_1A_1P(0x02,0x59); 
NT35596_DCS_write_1A_1P(0x04,0x0C); 
NT35596_DCS_write_1A_1P(0x05,0x2B); 
NT35596_DCS_write_1A_1P(0x06,0x64); 
NT35596_DCS_write_1A_1P(0x07,0xC6); 
		
NT35596_DCS_write_1A_1P(0x0D,0x89); 
NT35596_DCS_write_1A_1P(0x0E,0x89); 

NT35596_DCS_write_1A_1P(0x0F,0xE0); 
NT35596_DCS_write_1A_1P(0x10,0x03); 
NT35596_DCS_write_1A_1P(0x11,0x5A); 
NT35596_DCS_write_1A_1P(0x12,0x5A); 
		
NT35596_DCS_write_1A_1P(0x15,0x60); 

NT35596_DCS_write_1A_1P(0x16,0x13); 
NT35596_DCS_write_1A_1P(0x17,0x13); 
NT35596_DCS_write_1A_1P(0x1C,0xA3); 
NT35596_DCS_write_1A_1P(0x6E,0x80); 
NT35596_DCS_write_1A_1P(0x60,0x77); 
NT35596_DCS_write_1A_1P(0x68,0x13); 

		
		
NT35596_DCS_write_1A_1P(0x75,0x00); 
NT35596_DCS_write_1A_1P(0x76,0x77); 
NT35596_DCS_write_1A_1P(0x77,0x00); 
NT35596_DCS_write_1A_1P(0x78,0x78); 
NT35596_DCS_write_1A_1P(0x79,0x00); 
NT35596_DCS_write_1A_1P(0x7A,0xB0); 
NT35596_DCS_write_1A_1P(0x7B,0x00); 
NT35596_DCS_write_1A_1P(0x7C,0xCC); 
NT35596_DCS_write_1A_1P(0x7D,0x00); 
NT35596_DCS_write_1A_1P(0x7E,0xE3); 
NT35596_DCS_write_1A_1P(0x7F,0x00); 
NT35596_DCS_write_1A_1P(0x80,0xF6); 
NT35596_DCS_write_1A_1P(0x81,0x01); 
NT35596_DCS_write_1A_1P(0x82,0x06); 
NT35596_DCS_write_1A_1P(0x83,0x01); 
NT35596_DCS_write_1A_1P(0x84,0x15); 
NT35596_DCS_write_1A_1P(0x85,0x01); 
NT35596_DCS_write_1A_1P(0x86,0x22); 
NT35596_DCS_write_1A_1P(0x87,0x01); 
NT35596_DCS_write_1A_1P(0x88,0x4E); 
NT35596_DCS_write_1A_1P(0x89,0x01); 
NT35596_DCS_write_1A_1P(0x8A,0x71); 
NT35596_DCS_write_1A_1P(0x8B,0x01); 
NT35596_DCS_write_1A_1P(0x8C,0xA7); 
NT35596_DCS_write_1A_1P(0x8D,0x01); 
NT35596_DCS_write_1A_1P(0x8E,0xD2); 
NT35596_DCS_write_1A_1P(0x8F,0x02); 
NT35596_DCS_write_1A_1P(0x90,0x14); 
NT35596_DCS_write_1A_1P(0x91,0x02); 
NT35596_DCS_write_1A_1P(0x92,0x49); 
NT35596_DCS_write_1A_1P(0x93,0x02); 
NT35596_DCS_write_1A_1P(0x94,0x4B); 
NT35596_DCS_write_1A_1P(0x95,0x02); 
NT35596_DCS_write_1A_1P(0x96,0x7D); 
NT35596_DCS_write_1A_1P(0x97,0x02); 
NT35596_DCS_write_1A_1P(0x98,0xB6); 
NT35596_DCS_write_1A_1P(0x99,0x02); 
NT35596_DCS_write_1A_1P(0x9A,0xD9); 
NT35596_DCS_write_1A_1P(0x9B,0x03); 
NT35596_DCS_write_1A_1P(0x9C,0x0D); 
NT35596_DCS_write_1A_1P(0x9D,0x03); 
NT35596_DCS_write_1A_1P(0x9E,0x2A); 
NT35596_DCS_write_1A_1P(0x9F,0x03); 
NT35596_DCS_write_1A_1P(0xA0,0x51); 
NT35596_DCS_write_1A_1P(0xA2,0x03); 
NT35596_DCS_write_1A_1P(0xA3,0x5D); 
NT35596_DCS_write_1A_1P(0xA4,0x03); 
NT35596_DCS_write_1A_1P(0xA5,0x6A); 
NT35596_DCS_write_1A_1P(0xA6,0x03); 
NT35596_DCS_write_1A_1P(0xA7,0x78); 
NT35596_DCS_write_1A_1P(0xA9,0x03); 
NT35596_DCS_write_1A_1P(0xAA,0x89); 
NT35596_DCS_write_1A_1P(0xAB,0x03); 
NT35596_DCS_write_1A_1P(0xAC,0x9E); 
NT35596_DCS_write_1A_1P(0xAD,0x03); 
NT35596_DCS_write_1A_1P(0xAE,0xB7); 
NT35596_DCS_write_1A_1P(0xAF,0x03); 
NT35596_DCS_write_1A_1P(0xB0,0xCE); 
NT35596_DCS_write_1A_1P(0xB1,0x03); 
NT35596_DCS_write_1A_1P(0xB2,0xD1); 
		
NT35596_DCS_write_1A_1P(0xB3,0x00); 
NT35596_DCS_write_1A_1P(0xB4,0x18); 
NT35596_DCS_write_1A_1P(0xB5,0x00); 
NT35596_DCS_write_1A_1P(0xB6,0x6A); 
NT35596_DCS_write_1A_1P(0xB7,0x00); 
NT35596_DCS_write_1A_1P(0xB8,0xB0); 
NT35596_DCS_write_1A_1P(0xB9,0x00); 
NT35596_DCS_write_1A_1P(0xBA,0xCC); 
NT35596_DCS_write_1A_1P(0xBB,0x00); 
NT35596_DCS_write_1A_1P(0xBC,0xE3); 
NT35596_DCS_write_1A_1P(0xBD,0x00); 
NT35596_DCS_write_1A_1P(0xBE,0xF6); 
NT35596_DCS_write_1A_1P(0xBF,0x01); 
NT35596_DCS_write_1A_1P(0xC0,0x06); 
NT35596_DCS_write_1A_1P(0xC1,0x01); 
NT35596_DCS_write_1A_1P(0xC2,0x15); 
NT35596_DCS_write_1A_1P(0xC3,0x01); 
NT35596_DCS_write_1A_1P(0xC4,0x22); 
NT35596_DCS_write_1A_1P(0xC5,0x01); 
NT35596_DCS_write_1A_1P(0xC6,0x4E); 
NT35596_DCS_write_1A_1P(0xC7,0x01); 
NT35596_DCS_write_1A_1P(0xC8,0x71); 
NT35596_DCS_write_1A_1P(0xC9,0x01); 
NT35596_DCS_write_1A_1P(0xCA,0xA7); 
NT35596_DCS_write_1A_1P(0xCB,0x01); 
NT35596_DCS_write_1A_1P(0xCC,0xD2); 
NT35596_DCS_write_1A_1P(0xCD,0x02); 
NT35596_DCS_write_1A_1P(0xCE,0x14); 
NT35596_DCS_write_1A_1P(0xCF,0x02); 
NT35596_DCS_write_1A_1P(0xD0,0x49); 
NT35596_DCS_write_1A_1P(0xD1,0x02); 
NT35596_DCS_write_1A_1P(0xD2,0x4B); 
NT35596_DCS_write_1A_1P(0xD3,0x02); 
NT35596_DCS_write_1A_1P(0xD4,0x7D); 
NT35596_DCS_write_1A_1P(0xD5,0x02); 
NT35596_DCS_write_1A_1P(0xD6,0xB6); 
NT35596_DCS_write_1A_1P(0xD7,0x02); 
NT35596_DCS_write_1A_1P(0xD8,0xD9); 
NT35596_DCS_write_1A_1P(0xD9,0x03); 
NT35596_DCS_write_1A_1P(0xDA,0x0D); 
NT35596_DCS_write_1A_1P(0xDB,0x03); 
NT35596_DCS_write_1A_1P(0xDC,0x2A); 
NT35596_DCS_write_1A_1P(0xDD,0x03); 
NT35596_DCS_write_1A_1P(0xDE,0x51); 
NT35596_DCS_write_1A_1P(0xDF,0x03); 
NT35596_DCS_write_1A_1P(0xE0,0x5D); 
NT35596_DCS_write_1A_1P(0xE1,0x03); 
NT35596_DCS_write_1A_1P(0xE2,0x6A); 
NT35596_DCS_write_1A_1P(0xE3,0x03); 
NT35596_DCS_write_1A_1P(0xE4,0x78); 
NT35596_DCS_write_1A_1P(0xE5,0x03); 
NT35596_DCS_write_1A_1P(0xE6,0x89); 
NT35596_DCS_write_1A_1P(0xE7,0x03); 
NT35596_DCS_write_1A_1P(0xE8,0x9E); 
NT35596_DCS_write_1A_1P(0xE9,0x03); 
NT35596_DCS_write_1A_1P(0xEA,0xB7); 
NT35596_DCS_write_1A_1P(0xEB,0x03); 
NT35596_DCS_write_1A_1P(0xEC,0xCE); 
NT35596_DCS_write_1A_1P(0xED,0x03); 
NT35596_DCS_write_1A_1P(0xEE,0xD1); 
		
NT35596_DCS_write_1A_1P(0xEF,0x00); 
NT35596_DCS_write_1A_1P(0xF0,0x77); 
NT35596_DCS_write_1A_1P(0xF1,0x00); 
NT35596_DCS_write_1A_1P(0xF2,0x78); 
NT35596_DCS_write_1A_1P(0xF3,0x00); 
NT35596_DCS_write_1A_1P(0xF4,0xB0); 
NT35596_DCS_write_1A_1P(0xF5,0x00); 
NT35596_DCS_write_1A_1P(0xF6,0xCC); 
NT35596_DCS_write_1A_1P(0xF7,0x00); 
NT35596_DCS_write_1A_1P(0xF8,0xE3); 
NT35596_DCS_write_1A_1P(0xF9,0x00); 
NT35596_DCS_write_1A_1P(0xFA,0xF6); 
		
NT35596_DCS_write_1A_1P(0xFF,0x02); 
NT35596_DCS_write_1A_1P(0xFB,0x01); 
		
NT35596_DCS_write_1A_1P(0x00,0x01); 
NT35596_DCS_write_1A_1P(0x01,0x06); 
NT35596_DCS_write_1A_1P(0x02,0x01); 
NT35596_DCS_write_1A_1P(0x03,0x15); 
NT35596_DCS_write_1A_1P(0x04,0x01); 
NT35596_DCS_write_1A_1P(0x05,0x22); 
NT35596_DCS_write_1A_1P(0x06,0x01); 
NT35596_DCS_write_1A_1P(0x07,0x4E); 
NT35596_DCS_write_1A_1P(0x08,0x01); 
NT35596_DCS_write_1A_1P(0x09,0x71); 
NT35596_DCS_write_1A_1P(0x0A,0x01); 
NT35596_DCS_write_1A_1P(0x0B,0xA7); 
NT35596_DCS_write_1A_1P(0x0C,0x01); 
NT35596_DCS_write_1A_1P(0x0D,0xD2); 
NT35596_DCS_write_1A_1P(0x0E,0x02); 
NT35596_DCS_write_1A_1P(0x0F,0x14); 
NT35596_DCS_write_1A_1P(0x10,0x02); 
NT35596_DCS_write_1A_1P(0x11,0x49); 
NT35596_DCS_write_1A_1P(0x12,0x02); 
NT35596_DCS_write_1A_1P(0x13,0x4B); 
NT35596_DCS_write_1A_1P(0x14,0x02); 
NT35596_DCS_write_1A_1P(0x15,0x7D); 
NT35596_DCS_write_1A_1P(0x16,0x02); 
NT35596_DCS_write_1A_1P(0x17,0xB6); 
NT35596_DCS_write_1A_1P(0x18,0x02); 
NT35596_DCS_write_1A_1P(0x19,0xD9); 
NT35596_DCS_write_1A_1P(0x1A,0x03); 
NT35596_DCS_write_1A_1P(0x1B,0x0D); 
NT35596_DCS_write_1A_1P(0x1C,0x03); 
NT35596_DCS_write_1A_1P(0x1D,0x2A); 
NT35596_DCS_write_1A_1P(0x1E,0x03); 
NT35596_DCS_write_1A_1P(0x1F,0x51); 
NT35596_DCS_write_1A_1P(0x20,0x03); 
NT35596_DCS_write_1A_1P(0x21,0x5D); 
NT35596_DCS_write_1A_1P(0x22,0x03); 
NT35596_DCS_write_1A_1P(0x23,0x6A); 
NT35596_DCS_write_1A_1P(0x24,0x03); 
NT35596_DCS_write_1A_1P(0x25,0x78); 
NT35596_DCS_write_1A_1P(0x26,0x03); 
NT35596_DCS_write_1A_1P(0x27,0x89); 
NT35596_DCS_write_1A_1P(0x28,0x03); 
NT35596_DCS_write_1A_1P(0x29,0x9E); 
NT35596_DCS_write_1A_1P(0x2A,0x03); 
NT35596_DCS_write_1A_1P(0x2B,0xB7); 
NT35596_DCS_write_1A_1P(0x2D,0x03); 
NT35596_DCS_write_1A_1P(0x2F,0xCE); 
NT35596_DCS_write_1A_1P(0x30,0x03); 
NT35596_DCS_write_1A_1P(0x31,0xD1); 
		
NT35596_DCS_write_1A_1P(0x32,0x00); 
NT35596_DCS_write_1A_1P(0x33,0x18); 
NT35596_DCS_write_1A_1P(0x34,0x00); 
NT35596_DCS_write_1A_1P(0x35,0x6A); 
NT35596_DCS_write_1A_1P(0x36,0x00); 
NT35596_DCS_write_1A_1P(0x37,0xB0); 
NT35596_DCS_write_1A_1P(0x38,0x00); 
NT35596_DCS_write_1A_1P(0x39,0xCC); 
NT35596_DCS_write_1A_1P(0x3A,0x00); 
NT35596_DCS_write_1A_1P(0x3B,0xE3); 
NT35596_DCS_write_1A_1P(0x3D,0x00); 
NT35596_DCS_write_1A_1P(0x3F,0xF6); 
NT35596_DCS_write_1A_1P(0x40,0x01); 
NT35596_DCS_write_1A_1P(0x41,0x06); 
NT35596_DCS_write_1A_1P(0x42,0x01); 
NT35596_DCS_write_1A_1P(0x43,0x15); 
NT35596_DCS_write_1A_1P(0x44,0x01); 
NT35596_DCS_write_1A_1P(0x45,0x22); 
NT35596_DCS_write_1A_1P(0x46,0x01); 
NT35596_DCS_write_1A_1P(0x47,0x4E); 
NT35596_DCS_write_1A_1P(0x48,0x01); 
NT35596_DCS_write_1A_1P(0x49,0x71); 
NT35596_DCS_write_1A_1P(0x4A,0x01); 
NT35596_DCS_write_1A_1P(0x4B,0xA7); 
NT35596_DCS_write_1A_1P(0x4C,0x01); 
NT35596_DCS_write_1A_1P(0x4D,0xD2); 
NT35596_DCS_write_1A_1P(0x4E,0x02); 
NT35596_DCS_write_1A_1P(0x4F,0x14); 
NT35596_DCS_write_1A_1P(0x50,0x02); 
NT35596_DCS_write_1A_1P(0x51,0x49); 
NT35596_DCS_write_1A_1P(0x52,0x02); 
NT35596_DCS_write_1A_1P(0x53,0x4B); 
NT35596_DCS_write_1A_1P(0x54,0x02); 
NT35596_DCS_write_1A_1P(0x55,0x7D); 
NT35596_DCS_write_1A_1P(0x56,0x02); 
NT35596_DCS_write_1A_1P(0x58,0xB6); 
NT35596_DCS_write_1A_1P(0x59,0x02); 
NT35596_DCS_write_1A_1P(0x5A,0xD9); 
NT35596_DCS_write_1A_1P(0x5B,0x03); 
NT35596_DCS_write_1A_1P(0x5C,0x0D); 
NT35596_DCS_write_1A_1P(0x5D,0x03); 
NT35596_DCS_write_1A_1P(0x5E,0x2A); 
NT35596_DCS_write_1A_1P(0x5F,0x03); 
NT35596_DCS_write_1A_1P(0x60,0x51); 
NT35596_DCS_write_1A_1P(0x61,0x03); 
NT35596_DCS_write_1A_1P(0x62,0x5D); 
NT35596_DCS_write_1A_1P(0x63,0x03); 
NT35596_DCS_write_1A_1P(0x64,0x6A); 
NT35596_DCS_write_1A_1P(0x65,0x03); 
NT35596_DCS_write_1A_1P(0x66,0x78); 
NT35596_DCS_write_1A_1P(0x67,0x03); 
NT35596_DCS_write_1A_1P(0x68,0x89); 
NT35596_DCS_write_1A_1P(0x69,0x03); 
NT35596_DCS_write_1A_1P(0x6A,0x9E); 
NT35596_DCS_write_1A_1P(0x6B,0x03); 
NT35596_DCS_write_1A_1P(0x6C,0xB7); 
NT35596_DCS_write_1A_1P(0x6D,0x03); 
NT35596_DCS_write_1A_1P(0x6E,0xCE); 
NT35596_DCS_write_1A_1P(0x6F,0x03); 
NT35596_DCS_write_1A_1P(0x70,0xD1); 
		
NT35596_DCS_write_1A_1P(0x71,0x00); 
NT35596_DCS_write_1A_1P(0x72,0x77); 
NT35596_DCS_write_1A_1P(0x73,0x00); 
NT35596_DCS_write_1A_1P(0x74,0x78); 
NT35596_DCS_write_1A_1P(0x75,0x00); 
NT35596_DCS_write_1A_1P(0x76,0xB0); 
NT35596_DCS_write_1A_1P(0x77,0x00); 
NT35596_DCS_write_1A_1P(0x78,0xCC); 
NT35596_DCS_write_1A_1P(0x79,0x00); 
NT35596_DCS_write_1A_1P(0x7A,0xE3); 
NT35596_DCS_write_1A_1P(0x7B,0x00); 
NT35596_DCS_write_1A_1P(0x7C,0xF6); 
NT35596_DCS_write_1A_1P(0x7D,0x01); 
NT35596_DCS_write_1A_1P(0x7E,0x06); 
NT35596_DCS_write_1A_1P(0x7F,0x01); 
NT35596_DCS_write_1A_1P(0x80,0x15); 
NT35596_DCS_write_1A_1P(0x81,0x01); 
NT35596_DCS_write_1A_1P(0x82,0x22); 
NT35596_DCS_write_1A_1P(0x83,0x01); 
NT35596_DCS_write_1A_1P(0x84,0x4E); 
NT35596_DCS_write_1A_1P(0x85,0x01); 
NT35596_DCS_write_1A_1P(0x86,0x71); 
NT35596_DCS_write_1A_1P(0x87,0x01); 
NT35596_DCS_write_1A_1P(0x88,0xA7); 
NT35596_DCS_write_1A_1P(0x89,0x01); 
NT35596_DCS_write_1A_1P(0x8A,0xD2); 
NT35596_DCS_write_1A_1P(0x8B,0x02); 
NT35596_DCS_write_1A_1P(0x8C,0x14); 
NT35596_DCS_write_1A_1P(0x8D,0x02); 
NT35596_DCS_write_1A_1P(0x8E,0x49); 
NT35596_DCS_write_1A_1P(0x8F,0x02); 
NT35596_DCS_write_1A_1P(0x90,0x4B); 
NT35596_DCS_write_1A_1P(0x91,0x02); 
NT35596_DCS_write_1A_1P(0x92,0x7D); 
NT35596_DCS_write_1A_1P(0x93,0x02); 
NT35596_DCS_write_1A_1P(0x94,0xB6); 
NT35596_DCS_write_1A_1P(0x95,0x02); 
NT35596_DCS_write_1A_1P(0x96,0xD9); 
NT35596_DCS_write_1A_1P(0x97,0x03); 
NT35596_DCS_write_1A_1P(0x98,0x0D); 
NT35596_DCS_write_1A_1P(0x99,0x03); 
NT35596_DCS_write_1A_1P(0x9A,0x2A); 
NT35596_DCS_write_1A_1P(0x9B,0x03); 
NT35596_DCS_write_1A_1P(0x9C,0x51); 
NT35596_DCS_write_1A_1P(0x9D,0x03); 
NT35596_DCS_write_1A_1P(0x9E,0x5D); 
NT35596_DCS_write_1A_1P(0x9F,0x03); 
NT35596_DCS_write_1A_1P(0xA0,0x6A); 
NT35596_DCS_write_1A_1P(0xA2,0x03); 
NT35596_DCS_write_1A_1P(0xA3,0x78); 
NT35596_DCS_write_1A_1P(0xA4,0x03); 
NT35596_DCS_write_1A_1P(0xA5,0x89); 
NT35596_DCS_write_1A_1P(0xA6,0x03); 
NT35596_DCS_write_1A_1P(0xA7,0x9E); 
NT35596_DCS_write_1A_1P(0xA9,0x03); 
NT35596_DCS_write_1A_1P(0xAA,0xB7); 
NT35596_DCS_write_1A_1P(0xAB,0x03); 
NT35596_DCS_write_1A_1P(0xAC,0xCE); 
NT35596_DCS_write_1A_1P(0xAD,0x03); 
NT35596_DCS_write_1A_1P(0xAE,0xD1); 
		
NT35596_DCS_write_1A_1P(0xAF,0x00); 
NT35596_DCS_write_1A_1P(0xB0,0x18); 
NT35596_DCS_write_1A_1P(0xB1,0x00); 
NT35596_DCS_write_1A_1P(0xB2,0x6A); 
NT35596_DCS_write_1A_1P(0xB3,0x00); 
NT35596_DCS_write_1A_1P(0xB4,0xB0); 
NT35596_DCS_write_1A_1P(0xB5,0x00); 
NT35596_DCS_write_1A_1P(0xB6,0xCC); 
NT35596_DCS_write_1A_1P(0xB7,0x00); 
NT35596_DCS_write_1A_1P(0xB8,0xE3); 
NT35596_DCS_write_1A_1P(0xB9,0x00); 
NT35596_DCS_write_1A_1P(0xBA,0xF6); 
NT35596_DCS_write_1A_1P(0xBB,0x01); 
NT35596_DCS_write_1A_1P(0xBC,0x06); 
NT35596_DCS_write_1A_1P(0xBD,0x01); 
NT35596_DCS_write_1A_1P(0xBE,0x15); 
NT35596_DCS_write_1A_1P(0xBF,0x01); 
NT35596_DCS_write_1A_1P(0xC0,0x22); 
NT35596_DCS_write_1A_1P(0xC1,0x01); 
NT35596_DCS_write_1A_1P(0xC2,0x4E); 
NT35596_DCS_write_1A_1P(0xC3,0x01); 
NT35596_DCS_write_1A_1P(0xC4,0x71); 
NT35596_DCS_write_1A_1P(0xC5,0x01); 
NT35596_DCS_write_1A_1P(0xC6,0xA7); 
NT35596_DCS_write_1A_1P(0xC7,0x01); 
NT35596_DCS_write_1A_1P(0xC8,0xD2); 
NT35596_DCS_write_1A_1P(0xC9,0x02); 
NT35596_DCS_write_1A_1P(0xCA,0x14); 
NT35596_DCS_write_1A_1P(0xCB,0x02); 
NT35596_DCS_write_1A_1P(0xCC,0x49); 
NT35596_DCS_write_1A_1P(0xCD,0x02); 
NT35596_DCS_write_1A_1P(0xCE,0x4B); 
NT35596_DCS_write_1A_1P(0xCF,0x02); 
NT35596_DCS_write_1A_1P(0xD0,0x7D); 
NT35596_DCS_write_1A_1P(0xD1,0x02); 
NT35596_DCS_write_1A_1P(0xD2,0xB6); 
NT35596_DCS_write_1A_1P(0xD3,0x02); 
NT35596_DCS_write_1A_1P(0xD4,0xD9); 
NT35596_DCS_write_1A_1P(0xD5,0x03); 
NT35596_DCS_write_1A_1P(0xD6,0x0D); 
NT35596_DCS_write_1A_1P(0xD7,0x03); 
NT35596_DCS_write_1A_1P(0xD8,0x2A); 
NT35596_DCS_write_1A_1P(0xD9,0x03); 
NT35596_DCS_write_1A_1P(0xDA,0x51); 
NT35596_DCS_write_1A_1P(0xDB,0x03); 
NT35596_DCS_write_1A_1P(0xDC,0x5D); 
NT35596_DCS_write_1A_1P(0xDD,0x03); 
NT35596_DCS_write_1A_1P(0xDE,0x6A); 
NT35596_DCS_write_1A_1P(0xDF,0x03); 
NT35596_DCS_write_1A_1P(0xE0,0x78); 
NT35596_DCS_write_1A_1P(0xE1,0x03); 
NT35596_DCS_write_1A_1P(0xE2,0x89); 
NT35596_DCS_write_1A_1P(0xE3,0x03); 
NT35596_DCS_write_1A_1P(0xE4,0x9E); 
NT35596_DCS_write_1A_1P(0xE5,0x03); 
NT35596_DCS_write_1A_1P(0xE6,0xB7); 
NT35596_DCS_write_1A_1P(0xE7,0x03); 
NT35596_DCS_write_1A_1P(0xE8,0xCE); 
NT35596_DCS_write_1A_1P(0xE9,0x03); 
NT35596_DCS_write_1A_1P(0xEA,0xD1); 


NT35596_DCS_write_1A_1P(0xFF,0x05); 
NT35596_DCS_write_1A_1P(0xFB,0x01); 

NT35596_DCS_write_1A_1P(0x00,0x38); 
NT35596_DCS_write_1A_1P(0x01,0x38); 
NT35596_DCS_write_1A_1P(0x02,0x07); 
NT35596_DCS_write_1A_1P(0x03,0x40); 
NT35596_DCS_write_1A_1P(0x04,0x40); 
NT35596_DCS_write_1A_1P(0x05,0x25); 
NT35596_DCS_write_1A_1P(0x06,0x1D); 
NT35596_DCS_write_1A_1P(0x07,0x23); 
NT35596_DCS_write_1A_1P(0x08,0x1B); 
NT35596_DCS_write_1A_1P(0x09,0x21); 
NT35596_DCS_write_1A_1P(0x0A,0x19); 
NT35596_DCS_write_1A_1P(0x0B,0x1F); 
NT35596_DCS_write_1A_1P(0x0C,0x17); 
NT35596_DCS_write_1A_1P(0x0D,0x05); 
NT35596_DCS_write_1A_1P(0x0E,0x04); 
NT35596_DCS_write_1A_1P(0x0F,0x0F); 
NT35596_DCS_write_1A_1P(0x10,0x38); 
NT35596_DCS_write_1A_1P(0x11,0x38); 
NT35596_DCS_write_1A_1P(0x12,0x38); 
NT35596_DCS_write_1A_1P(0x13,0x38); 
NT35596_DCS_write_1A_1P(0x14,0x38); 
NT35596_DCS_write_1A_1P(0x15,0x38); 
NT35596_DCS_write_1A_1P(0x16,0x06); 
NT35596_DCS_write_1A_1P(0x17,0x40); 
NT35596_DCS_write_1A_1P(0x18,0x40); 
NT35596_DCS_write_1A_1P(0x19,0x24); 
NT35596_DCS_write_1A_1P(0x1A,0x1C); 
NT35596_DCS_write_1A_1P(0x1B,0x22); 
NT35596_DCS_write_1A_1P(0x1C,0x1A); 
NT35596_DCS_write_1A_1P(0x1D,0x20); 
NT35596_DCS_write_1A_1P(0x1E,0x18); 
NT35596_DCS_write_1A_1P(0x1F,0x1E); 
NT35596_DCS_write_1A_1P(0x20,0x16); 
NT35596_DCS_write_1A_1P(0x21,0x05); 
NT35596_DCS_write_1A_1P(0x22,0x04); 
NT35596_DCS_write_1A_1P(0x23,0x0E); 
NT35596_DCS_write_1A_1P(0x24,0x38); 
NT35596_DCS_write_1A_1P(0x25,0x38); 
NT35596_DCS_write_1A_1P(0x26,0x38); 
NT35596_DCS_write_1A_1P(0x27,0x38); 
NT35596_DCS_write_1A_1P(0x28,0x38); 
NT35596_DCS_write_1A_1P(0x29,0x38); 
NT35596_DCS_write_1A_1P(0x2A,0x0E); 
NT35596_DCS_write_1A_1P(0x2B,0x40); 
NT35596_DCS_write_1A_1P(0x2D,0x40); 
NT35596_DCS_write_1A_1P(0x2F,0x16); 
NT35596_DCS_write_1A_1P(0x30,0x1E); 
NT35596_DCS_write_1A_1P(0x31,0x18); 
NT35596_DCS_write_1A_1P(0x32,0x20); 
NT35596_DCS_write_1A_1P(0x33,0x1A); 
NT35596_DCS_write_1A_1P(0x34,0x22); 
NT35596_DCS_write_1A_1P(0x35,0x1C); 
NT35596_DCS_write_1A_1P(0x36,0x24); 
NT35596_DCS_write_1A_1P(0x37,0x05); 
NT35596_DCS_write_1A_1P(0x38,0x04); 
NT35596_DCS_write_1A_1P(0x39,0x06); 
NT35596_DCS_write_1A_1P(0x3A,0x38); 
NT35596_DCS_write_1A_1P(0x3B,0x38); 
NT35596_DCS_write_1A_1P(0x3D,0x38); 
NT35596_DCS_write_1A_1P(0x3F,0x38); 
NT35596_DCS_write_1A_1P(0x40,0x38); 
NT35596_DCS_write_1A_1P(0x41,0x38); 
NT35596_DCS_write_1A_1P(0x42,0x0F); 
NT35596_DCS_write_1A_1P(0x43,0x40); 
NT35596_DCS_write_1A_1P(0x44,0x40); 
NT35596_DCS_write_1A_1P(0x45,0x17); 
NT35596_DCS_write_1A_1P(0x46,0x1F); 
NT35596_DCS_write_1A_1P(0x47,0x19); 
NT35596_DCS_write_1A_1P(0x48,0x21); 
NT35596_DCS_write_1A_1P(0x49,0x1B); 
NT35596_DCS_write_1A_1P(0x4A,0x23); 
NT35596_DCS_write_1A_1P(0x4B,0x1D); 
NT35596_DCS_write_1A_1P(0x4C,0x25); 
NT35596_DCS_write_1A_1P(0x4D,0x05); 
NT35596_DCS_write_1A_1P(0x4E,0x04); 
NT35596_DCS_write_1A_1P(0x4F,0x07); 
NT35596_DCS_write_1A_1P(0x50,0x38); 
NT35596_DCS_write_1A_1P(0x51,0x38); 
NT35596_DCS_write_1A_1P(0x52,0x38); 
NT35596_DCS_write_1A_1P(0x53,0x38); 
NT35596_DCS_write_1A_1P(0x5B,0x0A); 
NT35596_DCS_write_1A_1P(0x5C,0x0A); 
NT35596_DCS_write_1A_1P(0x63,0x0A); 
NT35596_DCS_write_1A_1P(0x64,0x15); 
NT35596_DCS_write_1A_1P(0x68,0x24); 
NT35596_DCS_write_1A_1P(0x69,0x24); 
NT35596_DCS_write_1A_1P(0x90,0x7B); 
NT35596_DCS_write_1A_1P(0x91,0x11); 
NT35596_DCS_write_1A_1P(0x92,0x14); 
NT35596_DCS_write_1A_1P(0x7E,0x10); 
NT35596_DCS_write_1A_1P(0x7F,0x10); 
NT35596_DCS_write_1A_1P(0x80,0x00); 
NT35596_DCS_write_1A_1P(0x98,0x00); 
NT35596_DCS_write_1A_1P(0x99,0x00); 
NT35596_DCS_write_1A_1P(0x54,0x2E); 
NT35596_DCS_write_1A_1P(0x59,0x38); 
NT35596_DCS_write_1A_1P(0x5D,0x01); 
NT35596_DCS_write_1A_1P(0x5E,0x27); 
NT35596_DCS_write_1A_1P(0x62,0x39); 
NT35596_DCS_write_1A_1P(0x66,0x88); 
NT35596_DCS_write_1A_1P(0x67,0x11); 
NT35596_DCS_write_1A_1P(0x6A,0x0E); 
NT35596_DCS_write_1A_1P(0x6B,0x20); 
NT35596_DCS_write_1A_1P(0x6C,0x08); 
NT35596_DCS_write_1A_1P(0x6D,0x00); 
NT35596_DCS_write_1A_1P(0x7D,0x01); 
NT35596_DCS_write_1A_1P(0xB7,0x01); 
NT35596_DCS_write_1A_1P(0xB8,0x0A); 
NT35596_DCS_write_1A_1P(0xBA,0x13); 
NT35596_DCS_write_1A_1P(0xBC,0x01); 
NT35596_DCS_write_1A_1P(0xBD,0x55); 
NT35596_DCS_write_1A_1P(0xBE,0x38); 
NT35596_DCS_write_1A_1P(0xBF,0x44); 
NT35596_DCS_write_1A_1P(0xCF,0x88); 
NT35596_DCS_write_1A_1P(0xC8,0x00); 
NT35596_DCS_write_1A_1P(0xC9,0x55); 
NT35596_DCS_write_1A_1P(0xCA,0x00); 
NT35596_DCS_write_1A_1P(0xCB,0x55); 
NT35596_DCS_write_1A_1P(0xCC,0xA2); 
NT35596_DCS_write_1A_1P(0xCE,0x88); 
NT35596_DCS_write_1A_1P(0xCF,0x88); 
NT35596_DCS_write_1A_1P(0xD0,0x00); 
NT35596_DCS_write_1A_1P(0xD1,0x00); 
NT35596_DCS_write_1A_1P(0xD3,0x00); 
NT35596_DCS_write_1A_1P(0xD5,0x00); 
NT35596_DCS_write_1A_1P(0xD6,0x22); 
NT35596_DCS_write_1A_1P(0xD7,0x31); 
NT35596_DCS_write_1A_1P(0xD8,0x7E); 

NT35596_DCS_write_1A_1P(0xFF,0x00); 
NT35596_DCS_write_1A_1P(0xFB,0x01); 
NT35596_DCS_write_1A_1P(0xD3,0x10); 
NT35596_DCS_write_1A_1P(0xD4,0x10); 
NT35596_DCS_write_1A_1P(0xD5,0x18); 
NT35596_DCS_write_1A_1P(0xD6,0xB8); 
NT35596_DCS_write_1A_1P(0xD7,0x00); 

//REGW 0xBA,0x02
NT35596_DCS_write_1A_1P(0x36,0x00);
NT35596_DCS_write_1A_0P(0x11);
MDELAY(120);	

NT35596_DCS_write_1A_0P(0x29);
MDELAY(50);

///{REGFLAG_END_OF_TABLE, 0x00, {}}
	// Note
	// Strongly recommend not to set Sleep out / Display On here. That will cause messed frame to be shown as later the backlight is on.
	// Setting ending by predefined flag
}

// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}

static void lcm_get_params(LCM_PARAMS *params)
{
    memset(params, 0, sizeof(LCM_PARAMS));
    params->type   = LCM_TYPE_DSI;
    params->width  = FRAME_WIDTH;
    params->height = FRAME_HEIGHT;

	// enable tearing-free
#if (LCM_DSI_CMD_MODE)
	params->dbi.te_mode 				= LCM_DBI_TE_MODE_VSYNC_ONLY;
	params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;
#endif

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode   = CMD_MODE;
#else
    params->dsi.mode = SYNC_PULSE_VDO_MODE; //SYNC_EVENT_VDO_MODE;
#endif
	
	// DSI
	/* Command mode setting */
	params->dsi.LANE_NUM				= LCM_FOUR_LANE;
	//The following defined the fomat for data coming from LCD engine.
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

	// Highly depends on LCD driver capability.
	// Not support in MT6573
	params->dsi.packet_size=256;

	// Video mode setting		
	params->dsi.intermediat_buffer_num = 0;// 0;

	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
	params->dsi.word_count=FRAME_WIDTH*3;	
#if 0		
	params->dsi.vertical_sync_active				= 4;   //5
	params->dsi.vertical_backporch					= 17;
	params->dsi.vertical_frontporch					= 16;  //20
	params->dsi.vertical_active_line				= FRAME_HEIGHT; 

	params->dsi.horizontal_sync_active				= 10;
	params->dsi.horizontal_backporch				= 118;
	params->dsi.horizontal_frontporch				= 118;
	params->dsi.horizontal_active_pixel				= FRAME_WIDTH;
#else	
	params->dsi.vertical_sync_active				= 2;   //5
	params->dsi.vertical_backporch					= 15;
	params->dsi.vertical_frontporch					= 16;  //20
	params->dsi.vertical_active_line				= FRAME_HEIGHT; 

	params->dsi.horizontal_sync_active				= 55;
	params->dsi.horizontal_backporch				= 40;
	params->dsi.horizontal_frontporch				= 55;
	params->dsi.horizontal_active_pixel				= FRAME_WIDTH;
#endif
   // params->dsi.TA_GO =5;
	//params->dsi.compatibility_for_nvk = 1;

	// Bit rate calculation
#if FRAME_WIDTH == 480	
	params->dsi.PLL_CLOCK=210;//254//247
#elif FRAME_WIDTH == 540
	params->dsi.PLL_CLOCK=230;
#elif FRAME_WIDTH == 720
	params->dsi.PLL_CLOCK=230;
#elif FRAME_WIDTH == 1080
	params->dsi.PLL_CLOCK=410;
#else
	params->dsi.PLL_CLOCK=230;
#endif
        params->dsi.ssc_disable=1;
    	params->dsi.cont_clock=0; //1;
params->dsi.clk_lp_per_line_enable=1;

params->dsi.esd_check_enable=1;
params->dsi.customization_esd_check_enable=1;
//params->dsi.noncont_clock=1;
params->dsi.lcm_esd_check_table[0].cmd=0x0a;
params->dsi.lcm_esd_check_table[0].count=1;
params->dsi.lcm_esd_check_table[0].para_list[0]=0x9c;
	
}

static void lcm_init(void)
{
//power on
	lcm_power_5v_en(1);
    MDELAY(10);
	lcm_power_n5v_en(1);
    MDELAY(10);


    SET_RESET_PIN(1);
    MDELAY(10);

    SET_RESET_PIN(0);
    MDELAY(10);
	
    SET_RESET_PIN(1);
    MDELAY(120);

//	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
	init_lcm_registers();

}



static void lcm_suspend(void)
{	
	unsigned int data_array[16];
    
	data_array[0]=0x00280500; // Display Off
	dsi_set_cmdq(data_array, 1, 1);
    MDELAY(120);

	data_array[0] = 0x00100500; // Sleep In
	dsi_set_cmdq(data_array, 1, 1);
    MDELAY(120);
    SET_RESET_PIN(1);
    MDELAY(5);
    SET_RESET_PIN(0);
    MDELAY(20);
    SET_RESET_PIN(1);
    MDELAY(120);

    NT35596_DCS_write_1A_1P(0xFF,0x05);
    NT35596_DCS_write_1A_1P(0xFB,0x01);
    NT35596_DCS_write_1A_1P(0xC5,0x00);

//power off
	lcm_power_n5v_en(0);
    MDELAY(10);
   	lcm_power_5v_en(0);
    MDELAY(10);
	
}


static void lcm_resume(void)
{
		lcm_init();
		//vcom ++;
}
         
#if (LCM_DSI_CMD_MODE)
static void lcm_update(unsigned int x, unsigned int y,
                       unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0>>8)&0xFF);
	unsigned char x0_LSB = (x0&0xFF);
	unsigned char x1_MSB = ((x1>>8)&0xFF);
	unsigned char x1_LSB = (x1&0xFF);
	unsigned char y0_MSB = ((y0>>8)&0xFF);
	unsigned char y0_LSB = (y0&0xFF);
	unsigned char y1_MSB = ((y1>>8)&0xFF);
	unsigned char y1_LSB = (y1&0xFF);

	unsigned int data_array[16];

	data_array[0]= 0x00053902;
	data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
	data_array[2]= (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);
	
	data_array[0]= 0x00053902;
	data_array[1]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
	data_array[2]= (y1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0]= 0x002c3909;
	dsi_set_cmdq(data_array, 1, 0);

}
#endif

static unsigned int lcm_compare_id(void)
{
	unsigned int id=0;
	unsigned char buffer[2];
	unsigned int array[16];

//power on
	lcm_power_5v_en(1);
    MDELAY(10);
	lcm_power_n5v_en(1);
    MDELAY(10);


    SET_RESET_PIN(1);
    MDELAY(10);

    SET_RESET_PIN(0);
    MDELAY(10);
	
    SET_RESET_PIN(1);

    SET_RESET_PIN(1);
    MDELAY(120);
	array[0] = 0x00023700;// read id return two byte,version and id
	dsi_set_cmdq(array, 1, 1);

		read_reg_v2(0xF4, buffer, 2);
		id = buffer[0]; //we only need ID
    	#ifdef BUILD_LK
			printf("%s, LK nt35596 debug: id = 0x%08x\n", __func__, id);
    	#else
			printk("%s, kernel nt35596 debug: id = 0x%08x\n", __func__, id);
    	#endif

//power off
   	MDELAY(10);
	lcm_power_n5v_en(0);
    MDELAY(10);
    lcm_power_5v_en(0);
    MDELAY(10);
    SET_RESET_PIN(0);
    if(id == LCM_ID_NT35532)
    	return 1;
    else
        return 0;
}




LCM_DRIVER hct_nt35532_dsi_vdo_fhd_auo_ips_55_xl = 
{
    .name			= "hct_nt35532_dsi_vdo_fhd_auo_ips_55_xl",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,   
    .compare_id    = lcm_compare_id,    
#if (LCM_DSI_CMD_MODE)
    //.set_backlight    = lcm_setbacklight,
    .update         = lcm_update,
#endif  //wqtao
};

