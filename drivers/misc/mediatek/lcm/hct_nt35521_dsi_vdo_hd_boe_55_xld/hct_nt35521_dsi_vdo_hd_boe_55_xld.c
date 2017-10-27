/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

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
#include <linux/kernel.h>
#endif

#include "lcm_drv.h"

#ifdef BUILD_LK
	#include <platform/mt_gpio.h>
	#include <string.h>
#elif defined(BUILD_UBOOT)
	#include <asm/arch/mt_gpio.h>
#else
	//#include <mach/mt_gpio.h>
#endif


// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH                                         (720)
#define FRAME_HEIGHT                                        (1280)
#define LCM_ID                       (0x5521)

#define REGFLAG_DELAY               (0XFE)
#define REGFLAG_END_OF_TABLE        (0x100) // END OF REGISTERS MARKER


#define LCM_DSI_CMD_MODE                                    0



// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v)                                    (lcm_util.set_reset_pin((v)))

#define UDELAY(n)                                           (lcm_util.udelay(n))
#define MDELAY(n)                                           (lcm_util.mdelay(n))


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------
#define dsi_set_cmdq_V3(para_tbl,size,force_update)        lcm_util.dsi_set_cmdq_V3(para_tbl,size,force_update)
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)    lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)       lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)                                      lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)                  lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)											lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)               lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

#define   LCM_DSI_CMD_MODE							0
 struct LCM_setting_table {
    unsigned  cmd;
    unsigned char count;
    unsigned char para_list[64];
};



static struct LCM_setting_table lcm_initialization_setting[] = {
    
    /*
    Note :

    Data ID will depends on the following rule.
    
        count of parameters > 1 => Data ID = 0x39
        count of parameters = 1 => Data ID = 0x15
        count of parameters = 0 => Data ID = 0x05

    Structure Format :

    {DCS command, count of parameters, {parameter list}}
    {REGFLAG_DELAY, milliseconds of time, {}},

    ...

    Setting ending by predefined flag
    
    {REGFLAG_END_OF_TABLE, 0x00, {}}
    */
/*
{0xB9, 3 ,{0xF1,0x12,0x83}},

{0xBA, 27 ,{0x33,0x81,0x05,0xF9,0x0E,0x0E,0x00,0x00,0x00,0x00,
            0x00,0x00,0x00,0x00,0x44,0x25,0x00,0x91,0x0A,0x00,
            0x00,0x02,0x4F,0xD1,0x00,0x00,0x37}},

{0xB8, 2 ,{0x26,0x22}},	//0x75 for 3 Power Mode,0x25 for Power IC Mode

{0xBF, 3 ,{0x02,0x11,0x00}},

{0xB3, 10 ,{0x0C,0x10,0x0A,0x50,0x03,0xFF,0x00,0x00,0x00,0x00}},

{0xC0, 9 ,{0x73,0x73,0x50,0x50,0x00,0x00,0x08,0x70,0x00}},

{0xBC, 1 ,{0x46}},

{0xCC, 1 ,{0x0B}},

{0xB4, 1 ,{0x80}},

{0xB2, 3 ,{0xC8,0x12,0x30}},

{0xE3, 14 ,{0x07,0x07,0x0B,0x0B,0x03,0x0B,0x00,0x00,0x00,0x00,
            0xFF,0x00,0xc0,0x10}},

{0xC1, 12 ,{0x53,0x00,0x1E,0x1E,0x77,0xC1,0xFF,0xFF,0xAF,0xAF,0x7F,0x7F}},

{0xB5, 2 ,{0x07,0x07}},

{0xB6, 2 ,{0x66,0x66}},

{0xC6, 6 ,{0x00,0x00,0xFF,0xFF,0x01,0xFF}},

{0xE9, 63 ,{0xC2,0x10,0x05,0x04,0xFE,0x02,0xA1,0x12,0x31,0x45,
	0x3F,0x83,0x12,0xB1,0x3B,0x2A,0x08,0x05,0x00,0x00,
	0x00,0x00,0x08,0x05,0x00,0x00,0x00,0x00,0xFF,0x02,
	0x46,0x02,0x48,0x68,0x88,0x88,0x88,0x80,0x88,0xFF,
	0x13,0x57,0x13,0x58,0x78,0x88,0x88,0x88,0x81,0x88,
	0x00,0x00,0x00,0x00,0x00,0x12,0xB1,0x3B,0x00,0x00,
	0x00,0x00,0x00}},

{0xEA, 61 ,{0x00,0x1A,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0xFF,0x31,0x75,0x31,0x18,0x78,0x88,0x88,
	0x88,0x85,0x88,0xFF,0x20,0x64,0x20,0x08,0x68,0x88,
	0x88,0x88,0x84,0x88,0x20,0x10,0x00,0x00,0x54,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0xC0,0x00,0x00,0x0C,
	0x00,0x00,0x00,0x00,0x30,0x02,0xA1,0x00,0x00,0x00,
	0x00}},

{0xE0, 34 ,{0x00,0x05,0x07,0x1A,0x39,0x3F,0x33,0x2C,0x06,0x0B,0x0D,0x11,0x13,0x12,0x14,0x10,0x1A,
            0x00,0x05,0x07,0x1A,0x39,0x3F,0x33,0x2C,0x06,0x0B,0x0D,0x11,0x13,0x12,0x14,0x10,0x1A}},
*/


	{0xFF,4,{0xAA,0x55,0x25,0x01}},
{0x6F,1,{0x03}},
{0xF4,1,{0x60}},
{0x6F,1,{0x06}},
{0xF4,1,{0x01}},
{0x6F,1,{0x21}},
{0xF7,1,{0x01}},
{REGFLAG_DELAY, 1, {}},
{0x6F,1,{0x21}},
{0xF7,1,{0x00}},
{0xFC,1,{0x08}},
{REGFLAG_DELAY, 1, {}},
{0xFC,1,{0x00}},
//{0x6F,1,{0x16}},  //3lane
//{0xF7,1,{0x10}},  //3lane
{0xFF,4,{0xAA,0x55,0x25,0x00}},

{0xFF,4,{0xAA,0x55,0xA5,0x80}},
{0x6F,2,{0x11,0x00}},
{0xF7,2,{0x20,0x00}},
{0x6F,1,{0x06}},
{0xF7,1,{0xA0}},
{0x6F,1,{0x19}},
{0xF7,1,{0x12}},
{0x6F,1,{0x02}},
{0xF7,1,{0x47}},
{0x6F,1,{0x17}},
{0xF4,1,{0x70}},
{0x6F,1,{0x01}},
{0xF9,1,{0x46}},

{0xF0,5,{0x55,0xAA,0x52,0x08,0x00}},
{0xBD,5,{0x01,0xA0,0x10,0x10,0x01}},
{0xB8,4,{0x01,0x02,0x0C,0x02}},
{0xBB,2,{0x11,0x11}},
{0xBC,2,{0x00,0x00}},
{0xB6,1,{0x04}},
{0xC8,1,{0x80}},
{0xD9,2,{0x01,0x01}},
{0xD4,1,{0xC7}},
{0xB1,2,{0x60,0x21}},

{0xF0,5,{0x55,0xAA,0x52,0x08,0x01}},
{0xB0,2,{0x09,0x09}},
{0xB1,2,{0x09,0x09}},
{0xBC,2,{0x90,0x00}},
{0xBD,2,{0x90,0x00}},
{0xBE,1,{0x4f}},//3f 3a 46 4b 4f
{0xCA,1,{0x00}},
{0xC0,1,{0x0C}},
{0xB5,2,{0x03,0x03}},
{0xB3,2,{0x19,0x19}},
{0xB4,2,{0x19,0x19}},
{0xB9,2,{0x36,0x36}},
{0xBA,2,{0x34,0x34}},
 
{0xF0,5,{0x55,0xAA,0x52,0x08,0x02}}, 
//{0xEE,1,{0x01}},  //0x00==>3Gamma 0x01==>单Gamma
//3gama2.2//R(+) MCR cmd
{0xB0,16,{0x00,0x0E,0x00,0x69,0x00,0x96,0x00,0xB5,0x00,0xD0,0x00,0xF4,0x01,0x11,0x01,0x3F}},
{0xB1,16,{0x01,0x62,0x01,0x9C,0x01,0xC9,0x02,0x0E,0x02,0x44,0x02,0x48,0x02,0x74,0x02,0xA9}},
{0xB2,16,{0x02,0xCB,0x02,0xF6,0x03,0x16,0x03,0x3F,0x03,0x5C,0x03,0x73,0x03,0x9B,0x03,0x9F}},
{0xB3, 4,{0x03,0xD7,0x03,0xE8}},

{0xB4,16,{0x00,0x05,0x00,0x50,0x00,0x8D,0x00,0xAD,0x00,0xC4,0x00,0xEB,0x01,0x09,0x01,0x39}},
{0xB5,16,{0x01,0x5E,0x01,0x97,0x01,0xC4,0x02,0x08,0x02,0x3D,0x02,0x3E,0x02,0x70,0x02,0xA4}},
{0xB6,16,{0x02,0xC5,0x02,0xF2,0x03,0x11,0x03,0x3B,0x03,0x58,0x03,0x6C,0x03,0x96,0x03,0xCA}},
{0xB7, 4,{0x03,0xF5,0x03,0xF8}},

{0xB8,16,{0x00,0x14,0x00,0x3B,0x00,0x6F,0x00,0x8E,0x00,0xA9,0x00,0xD1,0x00,0xF1,0x01,0x24}},
{0xB9,16,{0x01,0x4C,0x01,0x8A,0x01,0xB9,0x02,0x03,0x02,0x3A,0x02,0x3B,0x02,0x6E,0x02,0xA4}},
{0xBA,16,{0x02,0xC5,0x02,0xF4,0x03,0x16,0x03,0x4D,0x03,0x81,0x03,0xF9,0x03,0xFA,0x03,0xFB}},
{0xBB, 4,{0x03,0xFD,0x03,0xFE}},

{0xBC,16,{0x00,0x0E,0x00,0x69,0x00,0x96,0x00,0xB5,0x00,0xD0,0x00,0xF4,0x01,0x11,0x01,0x3F}},
{0xBD,16,{0x01,0x62,0x01,0x9C,0x01,0xC9,0x02,0x0E,0x02,0x44,0x02,0x48,0x02,0x74,0x02,0xA9}},
{0xBE,16,{0x02,0xCB,0x02,0xF6,0x03,0x16,0x03,0x3F,0x03,0x5C,0x03,0x73,0x03,0x9B,0x03,0x9F}},
{0xBF, 4,{0x03,0xD7,0x03,0xE8}},

{0xC0,16,{0x00,0x05,0x00,0x50,0x00,0x8D,0x00,0xAD,0x00,0xC4,0x00,0xEB,0x01,0x09,0x01,0x39}},
{0xC1,16,{0x01,0x5E,0x01,0x97,0x01,0xC4,0x02,0x08,0x02,0x3D,0x02,0x3E,0x02,0x70,0x02,0xA4}},
{0xC2,16,{0x02,0xC5,0x02,0xF2,0x03,0x11,0x03,0x3B,0x03,0x58,0x03,0x6C,0x03,0x96,0x03,0xCA}},
{0xC3, 4,{0x03,0xF5,0x03,0xF8}},

{0xC4,16,{0x00,0x14,0x00,0x3B,0x00,0x6F,0x00,0x8E,0x00,0xA9,0x00,0xD1,0x00,0xF1,0x01,0x24}},
{0xC5,16,{0x01,0x4C,0x01,0x8A,0x01,0xB9,0x02,0x03,0x02,0x3A,0x02,0x3B,0x02,0x6E,0x02,0xA4}},
{0xC6,16,{0x02,0xC5,0x02,0xF4,0x03,0x16,0x03,0x4D,0x03,0x81,0x03,0xF9,0x03,0xFA,0x03,0xFB}},
{0xC7, 4,{0x03,0xFD,0x03,0xFE}},
                                            
{0xF0,5,{0x55,0xAA,0x52,0x08,0x06}},
{0xB0,2,{0x31,0x2E}},
{0xB1,2,{0x10,0x12}},
{0xB2,2,{0x16,0x18}},
{0xB3,2,{0x31,0x31}},
{0xB4,2,{0x31,0x34}},
{0xB5,2,{0x34,0x34}},
{0xB6,2,{0x34,0x34}},
{0xB7,2,{0x34,0x34}},
{0xB8,2,{0x33,0x2D}},
{0xB9,2,{0x00,0x02}},
{0xBA,2,{0x03,0x01}},
{0xBB,2,{0x2D,0x33}},
{0xBC,2,{0x34,0x34}},
{0xBD,2,{0x34,0x34}},
{0xBE,2,{0x34,0x34}},
{0xBF,2,{0x34,0x31}},
{0xC0,2,{0x31,0x31}},
{0xC1,2,{0x19,0x17}},
{0xC2,2,{0x13,0x11}},
{0xC3,2,{0x2E,0x31}},
{0xE5,2,{0x31,0x31}},
{0xC4,2,{0x31,0x2D}},
{0xC5,2,{0x19,0x17}},
{0xC6,2,{0x13,0x11}},
{0xC7,2,{0x31,0x31}},
{0xC8,2,{0x31,0x34}},
{0xC9,2,{0x34,0x34}},
{0xCA,2,{0x34,0x34}},
{0xCB,2,{0x34,0x34}},
{0xCC,2,{0x33,0x2E}},
{0xCD,2,{0x03,0x01}},
{0xCE,2,{0x00,0x02}},
{0xCF,2,{0x2E,0x33}},
{0xD0,2,{0x34,0x34}},
{0xD1,2,{0x34,0x34}},
{0xD2,2,{0x34,0x34}},
{0xD3,2,{0x34,0x31}},
{0xD4,2,{0x31,0x31}},
{0xD5,2,{0x10,0x12}},
{0xD6,2,{0x16,0x18}},
{0xD7,2,{0x2D,0x31}},
{0xE6,2,{0x31,0x31}},
{0xD8,5,{0x00,0x00,0x00,0x00,0x00}},
{0xD9,5,{0x00,0x00,0x00,0x00,0x00}},
{0xE7,1,{0x00}},

{0xF0,5,{0x55,0xAA,0x52,0x08,0x05}},
{0xED,1,{0x30}},
{0xB0,2,{0x17,0x06}},
{0xB8,1,{0x00}},
{0xC0,1,{0x0D}},
{0xC1,1,{0x0B}},
{0xC2,1,{0x00}},
{0xC3,1,{0x00}},
{0xC4,1,{0x84}},
{0xC5,1,{0x82}},
{0xC6,1,{0x82}},
{0xC7,1,{0x80}},
{0xC8,2,{0x0B,0x20}},
{0xC9,2,{0x07,0x20}},
{0xCA,2,{0x01,0x10}},
//{0xCB,2,{0x01,0x10}},
{0xD1,5,{0x03,0x05,0x05,0x07,0x00}},
{0xD2,5,{0x03,0x05,0x09,0x03,0x00}},
{0xD3,5,{0x00,0x00,0x6A,0x07,0x10}},
{0xD4,5,{0x30,0x00,0x6A,0x07,0x10}},

{0xF0,5,{0x55,0xAA,0x52,0x08,0x03}},
{0xB0,2,{0x00,0x00}},
{0xB1,2,{0x00,0x00}},
{0xB2,5,{0x05,0x01,0x13,0x00,0x00}},  
{0xB3,5,{0x05,0x01,0x13,0x00,0x00}},  
{0xB4,5,{0x05,0x01,0x13,0x00,0x00}},  
{0xB5,5,{0x05,0x01,0x13,0x00,0x00}},  
{0xB6,5,{0x02,0x01,0x13,0x00,0x00}},  
{0xB7,5,{0x02,0x01,0x13,0x00,0x00}},  
{0xB8,5,{0x02,0x01,0x13,0x00,0x00}},  
{0xB9,5,{0x02,0x01,0x13,0x00,0x00}},  
{0xBA,5,{0x53,0x01,0x13,0x00,0x00}},  
{0xBB,5,{0x53,0x01,0x13,0x00,0x00}},  
{0xBC,5,{0x53,0x01,0x13,0x00,0x00}},  
{0xBD,5,{0x53,0x01,0x13,0x00,0x00}},  
{0xC4,1,{0x60}},
{0xC5,1,{0x40}},
{0xC6,1,{0x64}},
{0xC7,1,{0x44}},
{0x6F,1,{0x11}},
{0xF3,1,{0x01}},

{0x35, 1, {0x00}},

{0xF0,5,{0x55,0xAA,0x52,0x08,0x00}},
{0xEF,4,{0x00,0x00,0x20,0xFF}},
{0xF0,4,{0x87,0x78,0x02,0x40}},


{0x11, 0, {0x00}},
{REGFLAG_DELAY, 120, {}},
{0x29, 0,{0x00}},
{REGFLAG_DELAY, 20, {}}, 
{REGFLAG_END_OF_TABLE, 0, {}}
};



//static int vcom=0x80;

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
    unsigned int i;

    for(i = 0; i < count; i++) {
        
        unsigned cmd;
        cmd = table[i].cmd;
        
        switch (cmd) {
	/*			case 0xb6:
		        table[i].para_list[0]=vcom;
			table[i].para_list[1]=vcom;
			dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
            vcom-=1;
			break;
*/
			
            case REGFLAG_DELAY :
                MDELAY(table[i].count);
                break;
                
            case REGFLAG_END_OF_TABLE :
                break;
                
            default:
                dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
                MDELAY(1);
        }
    }
    
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
    params->dbi.te_mode             = LCM_DBI_TE_MODE_DISABLED;
    params->dbi.te_edge_polarity        = LCM_POLARITY_RISING;



    params->dsi.mode   = SYNC_EVENT_VDO_MODE;


    // DSI
    /* Command mode setting */
    params->dsi.LANE_NUM                = LCM_FOUR_LANE;
    //The following defined the fomat for data coming from LCD engine. 
    params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
    params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST; 
    params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;
    // Highly depends on LCD driver capability.
    // Not support in MT6573
    params->dsi.packet_size=256;
    // Video mode setting       
    params->dsi.intermediat_buffer_num = 2;
    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
	params->dsi.vertical_sync_active				= 8; //8;	//2;
	params->dsi.vertical_backporch					= 16; //18;	//14;
	params->dsi.vertical_frontporch					= 16; //20;	//16;
	params->dsi.vertical_active_line				= FRAME_HEIGHT; 

	params->dsi.horizontal_sync_active				= 10;	//2;
	params->dsi.horizontal_backporch				= 100;//120;	//60;	//42;
	params->dsi.horizontal_frontporch				= 100;//100;	//60;	//44;
	params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

        params->dsi.clk_lp_per_line_enable = 1;  
        params->dsi.esd_check_enable = 1; 
        params->dsi.customization_esd_check_enable = 1;   
        params->dsi.lcm_esd_check_table[0].cmd          = 0x0A;    
        params->dsi.lcm_esd_check_table[0].count        = 1;     
        params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9c;
        //params->dsi.LPX=8;
        params->dsi.noncont_clock = 1;   
        params->dsi.noncont_clock_period = 2;
        params->dsi.ssc_disable = 1;

// Bit rate calculation
//1 Every lane speed
//params->dsi.pll_div1=0;		// div1=0,1,2,3;div1_real=1,2,4,4 ----0: 546Mbps  1:273Mbps
//params->dsi.pll_div2=0;		// div2=0,1,2,3;div1_real=1,2,4,4	
//params->dsi.fbk_div =0x12;    // fref=26MHz, fvco=fref*(fbk_div+1)*2/(div1_real*div2_real)	

// zhangxiaofei add for test
params->dsi.PLL_CLOCK = 230;//208;

		//params->physical_width = 62;
		//params->physical_height = 110;
}

static void lcm_init(void)
{
    SET_RESET_PIN(1);
    MDELAY(10);
    SET_RESET_PIN(0);
    MDELAY(50);
    SET_RESET_PIN(1);
    MDELAY(120);

    push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_suspend(void)
{

    unsigned int data_array[16];

    data_array[0]=0x00280500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(20);
    data_array[0]=0x00100500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(120);
  //  SET_RESET_PIN(1);
   // MDELAY(120);
    SET_RESET_PIN(0);
	MDELAY(20); // 1ms
	//SET_RESET_PIN(1);
	//MDELAY(120); 
}


    //for LGE backlight IC mapping table


    // Refresh value of backlight level.


static void lcm_resume(void)
{
     lcm_init();

    /// please notice: the max return packet size is 1
    /// if you want to change it, you can refer to the following marked code
    /// but read_reg currently only support read no more than 4 bytes....
    /// if you need to read more, please let BinHan knows.
    /*
            unsigned int data_array[16];
            unsigned int max_return_size = 1;
            
            data_array[0]= 0x00003700 | (max_return_size << 16);    
            
            dsi_set_cmdq(&data_array, 1, 1);
    


	unsigned int data_array[16];

    MDELAY(100);
    data_array[0]=0x00290500;
    dsi_set_cmdq(data_array,1,1);
    MDELAY(10);
*/
}


static unsigned int lcm_compare_id(void)
{


    unsigned int id=0;
    unsigned char buffer[3];
    unsigned int array[16]; 
    unsigned int data_array[16];

    SET_RESET_PIN(1);
        MDELAY(10);
    SET_RESET_PIN(0);
    MDELAY(50);
   
    SET_RESET_PIN(1);
    MDELAY(120);

  //   return 1;

    data_array[0] = 0x00063902;
    data_array[1] = 0x52AA55F0; 
    data_array[2] = 0x00000108;               
    dsi_set_cmdq(data_array, 3, 1);

    array[0] = 0x00033700;// read id return two byte,version and id
    dsi_set_cmdq(array, 1, 1);
   
    read_reg_v2(0xC5, buffer, 3);
    id = buffer[1]; //we only need ID
    #ifdef BUILD_LK
	printf("%s, LK nt35521 debug: nt35521 id = 0x%08x buffer[0]=0x%08x,buffer[1]=0x%08x,buffer[2]=0x%08x\n", __func__, id,buffer[0],buffer[1],buffer[2]);
    #else
	printk("%s, LK nt35521 debug: nt35521 id = 0x%08x buffer[0]=0x%08x,buffer[1]=0x%08x,buffer[2]=0x%08x\n", __func__, id,buffer[0],buffer[1],buffer[2]);
    #endif

   // if(id == LCM_ID_NT35521)
    if(buffer[0]==0x55 && buffer[1]==0x21)
        return 1;
    else
        return 0;


/*	int   array[4];
		char  buffer[5];
		unsigned int id_high;
		unsigned int id_low;
		unsigned int id=0;

	
        //Do reset here
	SET_RESET_PIN(1);
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(120);
     
        array[0]=0x00043700;
        dsi_set_cmdq(array, 1, 1);
    
        read_reg_v2(0x04, buffer,2);
	id_high = buffer[0]; 
	id_low=buffer[1]; 
	id = (id_high << 8) | id_low;
#ifdef BUILD_LK
//	printf("st7703 id = 0x%08x\n",  id);
#else
	//printk("st7703 id = 0x%08x\n",  id);
#endif

	return (LCM_ID == id)?1:0;//id1
  //return 1;
*/
}

// ---------------------------------------------------------------------------
//  Get LCM Driver Hooks
// ---------------------------------------------------------------------------
LCM_DRIVER hct_nt35521_dsi_vdo_hd_boe_55_xld = 
{
    .name           = "hct_nt35521_dsi_vdo_hd_boe_55_xld",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,   
    .compare_id    = lcm_compare_id,    
#if 0//defined(LCM_DSI_CMD_MODE)
    //.set_backlight    = lcm_setbacklight,
    //.esd_check   = lcm_esd_check, 
    //.esd_recover   = lcm_esd_recover, 
    .update         = lcm_update,
#endif  //wqtao
};
