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




// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH                                         (1080)
#define FRAME_HEIGHT                                        (1920)
#define LCM_ID                       (0x7807)

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
    {REGFLAG_DELAY, milliseconds of time, {)},

    ...

    Setting ending by predefined flag
    
    {REGFLAG_END_OF_TABLE, 0x00, {}}
    */

			
   ////// AUO H546DAN09.1 /////
{0xFF,3,{0x78,0x07,0x01}},	//Page1
{0x42,1,{0x11}},	//VGH=x4 VGL=x4
{0x43,1,{0xA3}},	//VGH_CLP = 9.5V
{0x44,1,{0xA8}},	//VGL_CLP = -9V

{0x45,1,{0x19}},	//VGHO  = 8.5 V
{0x46,1,{0x28}},	//VGLO  = -8.0V

{0x4A,1,{0x12}},	//Disable VSPR 
{0x4B,1,{0x12}},	//Disable VSNR 

{0x50,1,{0x5A}},	//GVDDP  = 4.8V
{0x51,1,{0x5A}},	//GVDDN  = -4.8V

{0xB3,1,{0x70}},	//VGHO setting    
{0xB4,1,{0x70}},	//VGLO setting 

{0xA2,1,{0x01}},	//VCOM1 
{0xA3,1,{0x16}},	//VCOM1 

//GIP & SOURCE
{0xFF,3,{0x78,0x07,0x01}},	//Page1
{0x22,1,{0x06}},	//SS&NW
{0x36,1,{0x00}},        //2 mux 6
{0x63,1,{0x04}},        //SDT
{0x64,1,{0x08}},
{0x6C,1,{0x45}},	//PRC & PRCB
{0x6D,1,{0x00}},	//PCT2
{0x5A,1,{0x33}},	//LVD setting
//{0x59,1,31	//LVD setting


{0xFF,3,{0x78,0x07,0x06}},	//Page6
{0x00,1,{0x43}},    //  
{0x01,1,{0x12}},    //	
{0x02,1,{0x43}},    //jhc	
{0x03,1,{0x43}},    //jhc	
{0x04,1,{0x03}},    //	
{0x05,1,{0x12}},    //	
{0x06,1,{0x08}},    //	
{0x07,1,{0x0A}},    //	
{0x08,1,{0x83}},    //	
{0x09,1,{0x02}},    //	
{0x0A,1,{0x30}},    //	
{0x0B,1,{0x10}},    //  
{0x0C,1,{0x08}},    //	
{0x0D,1,{0x08}},    //	
{0x0E,1,{0x00}},    //	
{0x0F,1,{0x00}},    //	
{0x10,1,{0x00}},
{0x11,1,{0x00}},
{0x12,1,{0x00}},
{0x13,1,{0x00}},
{0x14,1,{0x84}},    // 
{0x15,1,{0x84}},    // 

{0x31,1,{0x08}},  // GOUTR_01_FW STV
{0x32,1,{0x01}},  // GOUTR_02_FW U2D
{0x33,1,{0x00}},  // GOUTR_03_FW D2U
{0x34,1,{0x11}},  // GOUTR_04_FW CLK_L
{0x35,1,{0x13}},  // GOUTR_05_FW XCLK_L
{0x36,1,{0x26}},  // GOUTR_06_FW RST
{0x37,1,{0x22}},  // GOUTR_07_FW XDONB = GAS
{0x38,1,{0x02}},  // GOUTR_08_FW CTRL jhc
{0x39,1,{0x0C}},  // GOUTR_09_FW V_END
{0x3A,1,{0x02}},  // GOUTR_10_FW CTRL
{0x3B,1,{0x02}},  // GOUTR_11_FW CTRL
{0x3C,1,{0x02}},  // GOUTR_12_FW CTRL
{0x3D,1,{0x02}},  // GOUTR_13_FW CTRL
{0x3E,1,{0x28}},  // GOUTR_14_FW SWR
{0x3F,1,{0x29}},  // GOUTR_15_FW SWG
{0x40,1,{0x2A}},  // GOUTR_16_FW SWB

{0x41,1,{0x08}},  // GOUTL_01_FW STV
{0x42,1,{0x01}},  // GOUTL_02_FW U2D
{0x43,1,{0x00}},  // GOUTL_03_FW D2U
{0x44,1,{0x10}},  // GOUTL_04_FW CLK_R
{0x45,1,{0x12}},  // GOUTL_05_FW XCLK_R
{0x46,1,{0x26}},  // GOUTL_06_FW RST
{0x47,1,{0x22}},  // GOUTL_07_FW XDONB = GAS
{0x48,1,{0x02}},  // GOUTL_08_FW CTRL jhc
{0x49,1,{0x0C}},  // GOUTL_09_FW V_END
{0x4A,1,{0x02}},  // GOUTL_10_FW CTRL
{0x4B,1,{0x02}},  // GOUTL_11_FW CTRL
{0x4C,1,{0x02}},  // GOUTL_12_FW CTRL
{0x4D,1,{0x02}},  // GOUTL_13_FW CTRL
{0x4E,1,{0x28}},  // GOUTL_14_FW SWR
{0x4F,1,{0x29}},  // GOUTL_15_FW SWG
{0x50,1,{0x2A}},  // GOUTL_16_FW SWB

{0x61,1,{0x0C}},  // GOUTR_01_BW STV
{0x62,1,{0x01}},  // GOUTR_02_BW U2D
{0x63,1,{0x00}},  // GOUTR_03_BW D2U
{0x64,1,{0x12}},  // GOUTR_04_BW CLK_L
{0x65,1,{0x10}},  // GOUTR_05_BW XCLK_L
{0x66,1,{0x26}},  // GOUTR_06_BW RST
{0x67,1,{0x22}},  // GOUTR_07_BW XCONB = GAS
{0x68,1,{0x02}},  // GOUTR_08_BW CTRL jhc
{0x69,1,{0x08}},  // GOUTR_09_BW V_END
{0x6A,1,{0x02}},  // GOUTR_10_BW CTRL
{0x6B,1,{0x02}},  // GOUTR_11_BW CTRL
{0x6C,1,{0x02}},  // GOUTR_12_BW CTRL
{0x6D,1,{0x02}},  // GOUTR_13_BW CTRL
{0x6E,1,{0x28}},  // GOUTR_14_BW SWR
{0x6F,1,{0x29}},  // GOUTR_15_BW SWG
{0x70,1,{0x2A}},  // GOUTR_16_BW SWG

{0x71,1,{0x0C}},  // GOUTL_01_BW STV
{0x72,1,{0x01}},  // GOUTL_02_BW U2D
{0x73,1,{0x00}},  // GOUTL_03_BW D2U
{0x74,1,{0x13}},  // GOUTL_04_BW CLK_R
{0x75,1,{0x11}},  // GOUTL_05_BW XCLK_R
{0x76,1,{0x26}},  // GOUTL_06_BW RST
{0x77,1,{0x22}},  // GOUTL_07_BW XDONB = GAS
{0x78,1,{0x02}},  // GOUTL_08_BW CTRL jhc
{0x79,1,{0x08}},  // GOUTL_09_BW V_END
{0x7A,1,{0x02}},  // GOUTL_10_BW CRTL
{0x7B,1,{0x02}},  // GOUTL_11_BW CTRL
{0x7C,1,{0x02}},  // GOUTL_12_BW CTRL
{0x7D,1,{0x02}},  // GOUTL_13_BW CTRL
{0x7E,1,{0x28}},  // GOUTL_14_BW SWR
{0x7F,1,{0x29}},  // GOUTL_15_BW SWG
{0x80,1,{0x2A}},  // GOUTL_16_BW SWB

{0xD0,1,{0x01}},
{0xD1,1,{0x01}},
{0xD2,1,{0x10}},   //jhc
{0xD3,1,{0x00}},
{0xD4,1,{0x00}},
{0xD5,1,{0x00}},
{0xD6,1,{0x00}},
{0xD7,1,{0x00}},
{0xD8,1,{0x00}},
{0xD9,1,{0x00}},
{0xDA,1,{0x00}},
{0xDB,1,{0x47}},
{0xDC,1,{0x02}},   //jhc
{0xDD,1,{0x55}},

{0x96,1,{0x80}},
{0x97,1,{0x33}},   //jhc

{0xA0,1,{0x10}},  //T8
{0xA1,1,{0x00}},  //T7P  Pre-CKH
{0xA2,1,{0x08}},  //T9P  T9= non overlap time
{0xA3,1,{0x1E}},  //T7 = CHK width 1.9us
{0xA7,1,{0x00}},
{0xA6,1,{0x32}},
{0xA7,1,{0x03}},
{0xAE,1,{0x14}},

{0xE5,1,{0x80}},


//===CKH Modulation====
//{0xB1,1,33
{0xB2,1,{0x04}},
{0xB3,1,{0x04}},
//{0xB4,1,33
//{0xB5,1,08
//{0xB6,1,08


//============Gamma START=============

{0xFF,03,{0x78,0x07,0x02}},

{0x00,01,{0x00}},   //255
{0x01,01,{0x00}},   //255
{0x02,01,{0x00}},   //254
{0x03,01,{0x1C}},   //254
{0x04,01,{0x00}},   //252
{0x05,01,{0x4B}},   //252
{0x06,01,{0x00}},   //250
{0x07,01,{0x70}},   //250
{0x08,01,{0x00}},   //248
{0x09,01,{0x8C}},   //248
{0x0A,01,{0x00}},   //246
{0x0B,01,{0xA6}},   //246
{0x0C,01,{0x00}},   //244
{0x0D,01,{0xBA}},   //244
{0x0E,01,{0x00}},   //242
{0x0F,01,{0xCD}},   //242
{0x10,01,{0x00}},   //240
{0x11,01,{0xDE}},   //240
{0x12,01,{0x01}},   //232
{0x13,01,{0x14}},   //232
{0x14,01,{0x01}},   //224
{0x15,01,{0x3C}},   //224
{0x16,01,{0x01}},   //208
{0x17,01,{0x77}},   //208
{0x18,01,{0x01}},   //192
{0x19,01,{0xA3}},   //192
{0x1A,01,{0x01}},   //160
{0x1B,01,{0xE3}},   //160
{0x1C,01,{0x02}},   //128
{0x1D,01,{0x14}},   //128
{0x1E,01,{0x02}},   //127
{0x1F,01,{0x15}},   //127
{0x20,01,{0x02}},   //95
{0x21,01,{0x45}},   //95
{0x22,01,{0x02}},   //63
{0x23,01,{0x7F}},   //63
{0x24,01,{0x02}},   //47
{0x25,01,{0xA8}},   //47
{0x26,01,{0x02}},   //31
{0x27,01,{0xE0}},   //31
{0x28,01,{0x03}},   //23
{0x29,01,{0x05}},   //23
{0x2A,01,{0x03}},   //15
{0x2B,01,{0x33}},   //15
{0x2C,01,{0x03}},   //13
{0x2D,01,{0x41}},   //13
{0x2E,01,{0x03}},   //11
{0x2F,01,{0x50}},   //11
{0x30,01,{0x03}},   //9
{0x31,01,{0x61}},   //9
{0x32,01,{0x03}},   //7
{0x33,01,{0x74}},   //7
{0x34,01,{0x03}},   //5
{0x35,01,{0x8C}},   //5
{0x36,01,{0x03}},   //3
{0x37,01,{0xAF}},   //3
{0x38,01,{0x03}},   //1
{0x39,01,{0xD4}},   //1
{0x3A,01,{0x03}},   //0
{0x3B,01,{0xE6}},   //0

{0x3C,01,{0x00}},   //255
{0x3D,01,{0x00}},   //255
{0x3E,01,{0x00}},   //254
{0x3F,01,{0x1C}},   //254
{0x40,01,{0x00}},   //252
{0x41,01,{0x4B}},   //252
{0x42,01,{0x00}},   //250
{0x43,01,{0x70}},   //250
{0x44,01,{0x00}},   //248
{0x45,01,{0x8C}},   //248
{0x46,01,{0x00}},   //246
{0x47,01,{0xA6}},   //246
{0x48,01,{0x00}},   //244
{0x49,01,{0xBA}},   //244
{0x4A,01,{0x00}},   //242
{0x4B,01,{0xCD}},   //242
{0x4C,01,{0x00}},   //240
{0x4D,01,{0xDE}},   //240
{0x4E,01,{0x01}},   //232
{0x4F,01,{0x14}},   //232
{0x50,01,{0x01}},   //224
{0x51,01,{0x3C}},   //224
{0x52,01,{0x01}},   //208
{0x53,01,{0x77}},   //208
{0x54,01,{0x01}},   //192
{0x55,01,{0xA3}},   //192
{0x56,01,{0x01}},   //160
{0x57,01,{0xE3}},   //160
{0x58,01,{0x02}},   //128
{0x59,01,{0x14}},   //128
{0x5A,01,{0x02}},   //127
{0x5B,01,{0x15}},   //127
{0x5C,01,{0x02}},   //95
{0x5D,01,{0x45}},   //95
{0x5E,01,{0x02}},   //63
{0x5F,01,{0x7F}},   //63
{0x60,01,{0x02}},   //47
{0x61,01,{0xA8}},   //47
{0x62,01,{0x02}},   //31
{0x63,01,{0xE0}},   //31
{0x64,01,{0x03}},   //23
{0x65,01,{0x05}},   //23
{0x66,01,{0x03}},   //15
{0x67,01,{0x33}},   //15
{0x68,01,{0x03}},   //13
{0x69,01,{0x41}},   //13
{0x6A,01,{0x03}},   //11
{0x6B,01,{0x50}},   //11
{0x6C,01,{0x03}},   //9
{0x6D,01,{0x61}},   //9
{0x6E,01,{0x03}},   //7
{0x6F,01,{0x74}},   //7
{0x70,01,{0x03}},   //5
{0x71,01,{0x8C}},   //5
{0x72,01,{0x03}},   //3
{0x73,01,{0xAF}},   //3
{0x74,01,{0x03}},   //1
{0x75,01,{0xD4}},   //1
{0x76,01,{0x03}},   //0
{0x77,01,{0xE6}},   //0

{0x78,01,{0x01}},
{0x79,01,{0x01}},

{0xFF,03,{0x78,0x07,0x03}},

{0x00,01,{0x00}},   //255
{0x01,01,{0x00}},   //255
{0x02,01,{0x00}},   //254
{0x03,01,{0x1E}},   //254
{0x04,01,{0x00}},   //252
{0x05,01,{0x4F}},   //252
{0x06,01,{0x00}},   //250
{0x07,01,{0x74}},   //250
{0x08,01,{0x00}},   //248
{0x09,01,{0x91}},   //248
{0x0A,01,{0x00}},   //246
{0x0B,01,{0xAA}},   //246
{0x0C,01,{0x00}},   //244
{0x0D,01,{0xBE}},   //244
{0x0E,01,{0x00}},   //242
{0x0F,01,{0xD1}},   //242
{0x10,01,{0x00}},   //240
{0x11,01,{0xE2}},   //240
{0x12,01,{0x01}},   //232
{0x13,01,{0x17}},   //232
{0x14,01,{0x01}},   //224
{0x15,01,{0x3F}},   //224
{0x16,01,{0x01}},   //208
{0x17,01,{0x79}},   //208
{0x18,01,{0x01}},   //192
{0x19,01,{0xA4}},   //192
{0x1A,01,{0x01}},   //160
{0x1B,01,{0xE4}},   //160
{0x1C,01,{0x02}},   //128
{0x1D,01,{0x15}},   //128
{0x1E,01,{0x02}},   //127
{0x1F,01,{0x16}},   //127
{0x20,01,{0x02}},   //95
{0x21,01,{0x45}},   //95
{0x22,01,{0x02}},   //63
{0x23,01,{0x80}},   //63
{0x24,01,{0x02}},   //47
{0x25,01,{0xA9}},   //47
{0x26,01,{0x02}},   //31
{0x27,01,{0xE1}},   //31
{0x28,01,{0x03}},   //23
{0x29,01,{0x06}},   //23
{0x2A,01,{0x03}},   //15
{0x2B,01,{0x35}},   //15
{0x2C,01,{0x03}},   //13
{0x2D,01,{0x42}},   //13
{0x2E,01,{0x03}},   //11
{0x2F,01,{0x52}},   //11
{0x30,01,{0x03}},   //9
{0x31,01,{0x64}},   //9
{0x32,01,{0x03}},   //7
{0x33,01,{0x78}},   //7
{0x34,01,{0x03}},   //5
{0x35,01,{0x8E}},   //5
{0x36,01,{0x03}},   //3
{0x37,01,{0xAE}},   //3
{0x38,01,{0x03}},   //1
{0x39,01,{0xD3}},   //1
{0x3A,01,{0x03}},   //0
{0x3B,01,{0xE6}},   //0

{0x3C,01,{0x00}},   //255
{0x3D,01,{0x00}},   //255
{0x3E,01,{0x00}},   //254
{0x3F,01,{0x1E}},   //254
{0x40,01,{0x00}},   //252
{0x41,01,{0x4F}},   //252
{0x42,01,{0x00}},   //250
{0x43,01,{0x74}},   //250
{0x44,01,{0x00}},   //248
{0x45,01,{0x91}},   //248
{0x46,01,{0x00}},   //246
{0x47,01,{0xAA}},   //246
{0x48,01,{0x00}},   //244
{0x49,01,{0xBE}},   //244
{0x4A,01,{0x00}},   //242
{0x4B,01,{0xD1}},   //242
{0x4C,01,{0x00}},   //240
{0x4D,01,{0xE2}},   //240
{0x4E,01,{0x01}},   //232
{0x4F,01,{0x17}},   //232
{0x50,01,{0x01}},   //224
{0x51,01,{0x3F}},   //224
{0x52,01,{0x01}},   //208
{0x53,01,{0x79}},   //208
{0x54,01,{0x01}},   //192
{0x55,01,{0xA4}},   //192
{0x56,01,{0x01}},   //160
{0x57,01,{0xE4}},   //160
{0x58,01,{0x02}},   //128
{0x59,01,{0x15}},   //128
{0x5A,01,{0x02}},   //127
{0x5B,01,{0x16}},   //127
{0x5C,01,{0x02}},   //95
{0x5D,01,{0x45}},   //95
{0x5E,01,{0x02}},   //63
{0x5F,01,{0x80}},   //63
{0x60,01,{0x02}},   //47
{0x61,01,{0xA9}},   //47
{0x62,01,{0x02}},   //31
{0x63,01,{0xE1}},   //31
{0x64,01,{0x03}},   //23
{0x65,01,{0x06}},   //23
{0x66,01,{0x03}},   //15
{0x67,01,{0x35}},   //15
{0x68,01,{0x03}},   //13
{0x69,01,{0x42}},   //13
{0x6A,01,{0x03}},   //11
{0x6B,01,{0x52}},   //11
{0x6C,01,{0x03}},   //9
{0x6D,01,{0x64}},   //9
{0x6E,01,{0x03}},   //7
{0x6F,01,{0x78}},   //7
{0x70,01,{0x03}},   //5
{0x71,01,{0x8E}},   //5
{0x72,01,{0x03}},   //3
{0x73,01,{0xAE}},   //3
{0x74,01,{0x03}},   //1
{0x75,01,{0xD3}},   //1
{0x76,01,{0x03}},   //0
{0x77,01,{0xE6}},   //0

{0x78,01,{0x01}},
{0x79,01,{0x01}},

{0xFF,03,{0x78,0x07,0x04}},

{0x00,01,{0x00}},   //255
{0x01,01,{0x00}},   //255
{0x02,01,{0x00}},   //254
{0x03,01,{0x2D}},   //254
{0x04,01,{0x00}},   //252
{0x05,01,{0x67}},   //252
{0x06,01,{0x00}},   //250
{0x07,01,{0x8C}},   //250
{0x08,01,{0x00}},   //248
{0x09,01,{0xAA}},   //248
{0x0A,01,{0x00}},   //246
{0x0B,01,{0xC1}},   //246
{0x0C,01,{0x00}},   //244
{0x0D,01,{0xD5}},   //244
{0x0E,01,{0x00}},   //242
{0x0F,01,{0xE7}},   //242
{0x10,01,{0x00}},   //240
{0x11,01,{0xF7}},   //240
{0x12,01,{0x01}},   //232
{0x13,01,{0x29}},   //232
{0x14,01,{0x01}},   //224
{0x15,01,{0x4E}},   //224
{0x16,01,{0x01}},   //208
{0x17,01,{0x85}},   //208
{0x18,01,{0x01}},   //192
{0x19,01,{0xAD}},   //192
{0x1A,01,{0x01}},   //160
{0x1B,01,{0xE9}},   //160
{0x1C,01,{0x02}},   //128
{0x1D,01,{0x18}},   //128
{0x1E,01,{0x02}},   //127
{0x1F,01,{0x1A}},   //127
{0x20,01,{0x02}},   //95
{0x21,01,{0x48}},   //95
{0x22,01,{0x02}},   //63
{0x23,01,{0x82}},   //63
{0x24,01,{0x02}},   //47
{0x25,01,{0xAC}},   //47
{0x26,01,{0x02}},   //31
{0x27,01,{0xE5}},   //31
{0x28,01,{0x03}},   //23
{0x29,01,{0x0D}},   //23
{0x2A,01,{0x03}},   //15
{0x2B,01,{0x42}},   //15
{0x2C,01,{0x03}},   //13
{0x2D,01,{0x54}},   //13
{0x2E,01,{0x03}},   //11
{0x2F,01,{0x65}},   //11
{0x30,01,{0x03}},   //9
{0x31,01,{0x75}},   //9
{0x32,01,{0x03}},   //7
{0x33,01,{0x86}},   //7
{0x34,01,{0x03}},   //5
{0x35,01,{0x9A}},   //5
{0x36,01,{0x03}},   //3
{0x37,01,{0xB8}},   //3
{0x38,01,{0x03}},   //1
{0x39,01,{0xD7}},   //1
{0x3A,01,{0x03}},   //0
{0x3B,01,{0xE6}},   //0

{0x3C,01,{0x00}},   //255
{0x3D,01,{0x00}},   //255
{0x3E,01,{0x00}},   //254
{0x3F,01,{0x2D}},   //254
{0x40,01,{0x00}},   //252
{0x41,01,{0x67}},   //252
{0x42,01,{0x00}},   //250
{0x43,01,{0x8C}},   //250
{0x44,01,{0x00}},   //248
{0x45,01,{0xAA}},   //248
{0x46,01,{0x00}},   //246
{0x47,01,{0xC1}},   //246
{0x48,01,{0x00}},   //244
{0x49,01,{0xD5}},   //244
{0x4A,01,{0x00}},   //242
{0x4B,01,{0xE7}},   //242
{0x4C,01,{0x00}},   //240
{0x4D,01,{0xF7}},   //240
{0x4E,01,{0x01}},   //232
{0x4F,01,{0x29}},   //232
{0x50,01,{0x01}},   //224
{0x51,01,{0x4E}},   //224
{0x52,01,{0x01}},   //208
{0x53,01,{0x85}},   //208
{0x54,01,{0x01}},   //192
{0x55,01,{0xAD}},   //192
{0x56,01,{0x01}},   //160
{0x57,01,{0xE9}},   //160
{0x58,01,{0x02}},   //128
{0x59,01,{0x18}},   //128
{0x5A,01,{0x02}},   //127
{0x5B,01,{0x1A}},   //127
{0x5C,01,{0x02}},   //95
{0x5D,01,{0x48}},   //95
{0x5E,01,{0x02}},   //63
{0x5F,01,{0x82}},   //63
{0x60,01,{0x02}},   //47
{0x61,01,{0xAC}},   //47
{0x62,01,{0x02}},   //31
{0x63,01,{0xE5}},   //31
{0x64,01,{0x03}},   //23
{0x65,01,{0x0D}},   //23
{0x66,01,{0x03}},   //15
{0x67,01,{0x42}},   //15
{0x68,01,{0x03}},   //13
{0x69,01,{0x54}},   //13
{0x6A,01,{0x03}},   //11
{0x6B,01,{0x65}},   //11
{0x6C,01,{0x03}},   //9
{0x6D,01,{0x75}},   //9
{0x6E,01,{0x03}},   //7
{0x6F,01,{0x86}},   //7
{0x70,01,{0x03}},   //5
{0x71,01,{0x9A}},   //5
{0x72,01,{0x03}},   //3
{0x73,01,{0xB8}},   //3
{0x74,01,{0x03}},   //1
{0x75,01,{0xD7}},   //1
{0x76,01,{0x03}},   //0
{0x77,01,{0xE6}},   //0

{0x78,01,{0x01}},
{0x79,01,{0x01}},

//============Gamma END=============


{0xFF,3,{0x78,0x07,0x00}},	//Page0 
{REGFLAG_DELAY, 10, {}},
{0x11,1,{0x0}},
{REGFLAG_DELAY, 120, {}},
{0x29,1,{0x0}},
{REGFLAG_DELAY, 10, {}},
{0x35,1,{0x0}},
{REGFLAG_DELAY, 10, {}},         
{REGFLAG_END_OF_TABLE, 0x00, {}}, 

};






//static int vcom=0x40;
static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
    unsigned int i;

    for(i = 0; i < count; i++) {
        
        unsigned cmd;
        cmd = table[i].cmd;
        
        switch (cmd) {
			/*case 0xd9:
			table[i].para_list[0]=vcom;
			dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
            vcom+=2;
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


    params->dsi.mode = BURST_VDO_MODE;//SYNC_EVENT_VDO_MODE; SYNC_EVENT_VDO_MODE;


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
 //    	params->physical_width = 69;
  //   	params->physical_height = 122;

    params->dsi.vertical_sync_active                   = 4;
    params->dsi.vertical_backporch                    = 21;
    params->dsi.vertical_frontporch                    = 32; 
    params->dsi.vertical_active_line                = FRAME_HEIGHT; 
 
    params->dsi.horizontal_sync_active                = 4;    //2;
    params->dsi.horizontal_backporch                = 20;//120;    //60;    //42;
    params->dsi.horizontal_frontporch                = 18;
    params->dsi.horizontal_active_pixel                = FRAME_WIDTH; 


// zhangxiaofei add for test
  params->dsi.PLL_CLOCK = 410;//208;220
  params->dsi.ssc_disable =1;	
  params->dsi.esd_check_enable = 1;
  params->dsi.customization_esd_check_enable = 1;
  params->dsi.lcm_esd_check_table[0].cmd          = 0x0a;
  params->dsi.lcm_esd_check_table[0].count        = 1;
  params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;
}

static void lcm_init(void)
{
    SET_RESET_PIN(1);
    MDELAY(10);
    SET_RESET_PIN(0);
    MDELAY(20);
    SET_RESET_PIN(1);
    MDELAY(120);

    push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_suspend(void)
{





    unsigned int data_array[16];


    data_array[0]=0x00280500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(10);
    data_array[0]=0x00100500;

    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(100);
    SET_RESET_PIN(1);
    MDELAY(120);
    SET_RESET_PIN(0);
	MDELAY(1); // 1ms
	SET_RESET_PIN(1);
	MDELAY(120); 
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
        int array[4];
        char buffer[5];
		unsigned int id_high;
		unsigned int id_low;
		unsigned int id=0;
        //Do reset here
        SET_RESET_PIN(1);
        MDELAY(10);
        SET_RESET_PIN(0);
		MDELAY(10);
        SET_RESET_PIN(1);
		MDELAY(30);
       
        array[0]=0x00043902;
	array[1]=0x018198ff;
	dsi_set_cmdq(array, 2, 1);
        MDELAY(10);
        array[0]=0x00023700;
        dsi_set_cmdq(array, 1, 1);
        //read_reg_v2(0x04, buffer, 3);
    
        read_reg_v2(0x00, buffer,1);
        id_high = buffer[0]; ///////////////////////0x98
        read_reg_v2(0x01, buffer,1);
	id_low = buffer[0]; ///////////////////////0x06
       // id = (id_midd &lt;&lt; 8) | id_low;
	id = (id_high << 8) | id_low;

		#if defined(BUILD_LK)
		printf("ILI7807 %s id_high = 0x%04x, id_low = 0x%04x\n,id=0x%x\n", __func__, id_high, id_low,id);
#else
		printk("ILI7807 %s id_high = 0x%04x, id_low = 0x%04x\n,id=0x%x\n", __func__, id_high, id_low,id);
#endif
	//return 1;	
	 return (LCM_ID == id)?1:0;

}

// ---------------------------------------------------------------------------
//  Get LCM Driver Hooks
// ---------------------------------------------------------------------------
LCM_DRIVER hct_ili7807d_dsi_vdo_fhd_auo_55_sh = 
{
    .name           = "hct_ili7807d_dsi_vdo_fhd_auo_55_sh",
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

