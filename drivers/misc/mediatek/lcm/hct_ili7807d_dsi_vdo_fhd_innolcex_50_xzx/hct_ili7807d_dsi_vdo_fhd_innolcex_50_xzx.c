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

			
{0xFF,3,{0x78,0x07,0x01}},//Page1
{0x40,1,{0x10}},//
{0x42,1,{0x11}},//
{0x43,1,{0xA8}},//VGH_CLP = 10.0V
{0x44,1,{0x9E}},//VGL_CLP = -8.0V

{0x45,1,{0x1B}},//VGHO = 8.4V
{0x46,1,{0x10}},//VGLO = -5.6V

{0x4A,1,{0x05}},//Default VSP=VSPR
{0x4B,1,{0x05}},//Default VSN=VSNR

{0x50,1,{0x55}},//GVDDP = 4.7V
{0x51,1,{0x55}},//GVDDN = - 4.7V
{0x5A,1,{0x33}},                 //LVD setting 

{0xA2,1,{0x01}},//VCOM1 = - 0.2V
{0xA3,1,{0x13}},//VCOM1 = - 0.2V

{0xB3,1,{0x70}},
{0xB4,1,{0x70}},

//GIP & SOURCE
{0xFF,3,{0x78,0x07,0x01}},//Page1

{0x22,1,{0x06}},                  // GS & SS setting
{0x36,1,{0x00}},                  //DEMUX setting 
{0x63,1,{0x05}},	          //for GIP
{0x64,1,{0x00}},                  //EQT
{0x6C,1,{0x40}},                 //for GIP

{0xFF,3,{0x78,0x07,0x06}},//Page6 for GIP
{0x00,1,{0x41}},
{0x01,1,{0x04}},
{0x02,1,{0x08}},
{0x03,1,{0x0A}},
{0x04,1,{0x00}},
{0x05,1,{0x00}},
{0x06,1,{0x00}},
{0x07,1,{0x00}},
{0x08,1,{0x01}},
{0x09,1,{0x08}},
{0x0A,1,{0x70}},
{0x0B,1,{0x00}},
{0x0C,1,{0x08}},
{0x0D,1,{0x08}},
{0x0E,1,{0x0A}},
{0x0F,1,{0x0A}},
{0x10,1,{0x00}},
{0x11,1,{0x00}},
{0x12,1,{0x00}},
{0x13,1,{0x00}},
{0x14,1,{0x00}},       
{0x15,1,{0x00}},      

{0x31,1,{0x08}},     //GoutR01
{0x32,1,{0x10}},
{0x33,1,{0x12}},
{0x34,1,{0x14}},
{0x35,1,{0x16}},
{0x36,1,{0x07}},
{0x37,1,{0x07}},
{0x38,1,{0x07}},
{0x39,1,{0x23}},
{0x3A,1,{0x22}},
{0x3B,1,{0x07}},
{0x3C,1,{0x07}},
{0x3D,1,{0x07}},
{0x3E,1,{0x28}},
{0x3F,1,{0x29}},
{0x40,1,{0x2a}},

{0x41,1,{0x09}},     //GoutL01
{0x42,1,{0x11}},
{0x43,1,{0x13}},
{0x44,1,{0x15}},
{0x45,1,{0x17}},
{0x46,1,{0x07}},
{0x47,1,{0x07}},
{0x48,1,{0x07}},
{0x49,1,{0x22}},
{0x4A,1,{0x22}},
{0x4B,1,{0x07}},
{0x4C,1,{0x07}},
{0x4D,1,{0x07}},
{0x4E,1,{0x28}},
{0x4F,1,{0x29}},
{0x50,1,{0x2a}},

{0x61,1,{0x09}},
{0x62,1,{0x17}},
{0x63,1,{0x15}},
{0x64,1,{0x13}},
{0x65,1,{0x11}},
{0x66,1,{0x07}},
{0x67,1,{0x07}},
{0x68,1,{0x07}},
{0x69,1,{0x23}},
{0x6A,1,{0x22}},
{0x6B,1,{0x07}},
{0x6C,1,{0x07}},
{0x6D,1,{0x07}},
{0x6E,1,{0x28}},
{0x6F,1,{0x29}},
{0x70,1,{0x2a}},
{0x71,1,{0x08}},
{0x72,1,{0x16}},
{0x73,1,{0x14}},
{0x74,1,{0x12}},
{0x75,1,{0x10}},
{0x76,1,{0x07}},
{0x77,1,{0x07}},
{0x78,1,{0x07}},
{0x79,1,{0x22}},
{0x7A,1,{0x22}},
{0x7B,1,{0x07}},
{0x7C,1,{0x07}},
{0x7D,1,{0x07}},
{0x7E,1,{0x28}},
{0x7F,1,{0x29}},
{0x80,1,{0x2a}},
{0x97,1,{0x11}}, 
{0xE5,1,{0x03}},    

{0xD0,1,{0x01}},
{0xD1,1,{0x00}},       
{0xD2,1,{0x01}},
{0xD9,1,{0x04}},
{0xDA,1,{0x04}},
{0xDB,1,{0x04}},

{0xA0,1,{0x08}},
{0xA1,1,{0x00}},
{0xA2,1,{0x08}},
{0xA3,1,{0x20}},
{0xA6,1,{0x11}},
{0xA7,1,{0x00}},

//============ power  seq===========

{0xD2,1,{0x01}},
{0xD9,1,{0x04}},
{0xDA,1,{0x04}},
{0xDD,1,{0x55}},

{0xFF,3,{0x78,0x07,0x08}},//Page8
{0x09,1,{0x0E}},

	
//============Gamma START=============

{0xFF,3,{0x78,0x07,0x02}},//

{0x00,1,{0x00}},////255
{0x01,1,{0xD9}},////255
{0x02,1,{0x00}},////254
{0x03,1,{0xE6}},////254
{0x04,1,{0x00}},////252
{0x05,1,{0xFC}},////252
{0x06,1,{0x01}},////250
{0x07,1,{0x0E}},////250
{0x08,1,{0x01}},////248
{0x09,1,{0x1D}},////248
{0x0A,1,{0x01}},////246
{0x0B,1,{0x2B}},////246
{0x0C,1,{0x01}},////244
{0x0D,1,{0x37}},////244
{0x0E,1,{0x01}},////242
{0x0F,1,{0x43}},////242
{0x10,1,{0x01}},////240
{0x11,1,{0x4D}},////240
{0x12,1,{0x01}},////232
{0x13,1,{0x71}},////232
{0x14,1,{0x01}},////224
{0x15,1,{0x8F}},////224
{0x16,1,{0x01}},////208
{0x17,1,{0xBE}},////208
{0x18,1,{0x01}},////192
{0x19,1,{0xE5}},////192
{0x1A,1,{0x02}},////160
{0x1B,1,{0x23}},////160
{0x1C,1,{0x02}},////128
{0x1D,1,{0x56}},////128
{0x1E,1,{0x02}},////127
{0x1F,1,{0x58}},////127
{0x20,1,{0x02}},////95
{0x21,1,{0x88}},////95
{0x22,1,{0x02}},////63
{0x23,1,{0xBD}},////63
{0x24,1,{0x02}},////47
{0x25,1,{0xDF}},////47
{0x26,1,{0x03}},////31
{0x27,1,{0x0B}},////31
{0x28,1,{0x03}},////23
{0x29,1,{0x28}},////23
{0x2A,1,{0x03}},////15
{0x2B,1,{0x4D}},////15
{0x2C,1,{0x03}},////13
{0x2D,1,{0x59}},////13
{0x2E,1,{0x03}},////11
{0x2F,1,{0x65}},////11
{0x30,1,{0x03}},////9
{0x31,1,{0x73}},////9
{0x32,1,{0x03}},////7
{0x33,1,{0x83}},////7
{0x34,1,{0x03}},////5
{0x35,1,{0x96}},////5
{0x36,1,{0x03}},////3
{0x37,1,{0xB1}},////3
{0x38,1,{0x03}},////1
{0x39,1,{0xD4}},////1
{0x3A,1,{0x03}},////0
{0x3B,1,{0xE6}},//   //0

{0x3C,1,{0x00}},////255
{0x3D,1,{0xD9}},////255
{0x3E,1,{0x00}},////254
{0x3F,1,{0xE6}},////254
{0x40,1,{0x00}},////252
{0x41,1,{0xFC}},////252
{0x42,1,{0x01}},////250
{0x43,1,{0x0E}},////250
{0x44,1,{0x01}},////248
{0x45,1,{0x1D}},////248
{0x46,1,{0x01}},////246
{0x47,1,{0x2B}},////246
{0x48,1,{0x01}},////244
{0x49,1,{0x37}},////244
{0x4A,1,{0x01}},////242
{0x4B,1,{0x43}},////242
{0x4C,1,{0x01}},////240
{0x4D,1,{0x4D}},////240
{0x4E,1,{0x01}},////232
{0x4F,1,{0x71}},////232
{0x50,1,{0x01}},////224
{0x51,1,{0x8F}},////224
{0x52,1,{0x01}},////208
{0x53,1,{0xBE}},////208
{0x54,1,{0x01}},////192
{0x55,1,{0xE5}},////192
{0x56,1,{0x02}},////160
{0x57,1,{0x23}},////160
{0x58,1,{0x02}},////128
{0x59,1,{0x56}},////128
{0x5A,1,{0x02}},////127
{0x5B,1,{0x58}},////127
{0x5C,1,{0x02}},////95
{0x5D,1,{0x88}},////95
{0x5E,1,{0x02}},////63
{0x5F,1,{0xBD}},////63
{0x60,1,{0x02}},////47
{0x61,1,{0xDF}},////47
{0x62,1,{0x03}},////31
{0x63,1,{0x0B}},////31
{0x64,1,{0x03}},////23
{0x65,1,{0x28}},////23
{0x66,1,{0x03}},////15
{0x67,1,{0x4D}},////15
{0x68,1,{0x03}},////13
{0x69,1,{0x59}},//   //13
{0x6A,1,{0x03}},////11
{0x6B,1,{0x65}},////11
{0x6C,1,{0x03}},////9
{0x6D,1,{0x73}},////9
{0x6E,1,{0x03}},////7
{0x6F,1,{0x83}},////7
{0x70,1,{0x03}},////5
{0x71,1,{0x96}},////5
{0x72,1,{0x03}},////3
{0x73,1,{0xB1}},////3
{0x74,1,{0x03}},////1
{0x75,1,{0xD4}},////1
{0x76,1,{0x03}},////0
{0x77,1,{0xE6}},////0

{0x78,1,{0x01}},//
{0x79,1,{0x01}},//

{0xFF,3,{0x78,0x07,0x03}},//

{0x00,1,{0x00}},////255
{0x01,1,{0xDE}},////255
{0x02,1,{0x00}},////254
{0x03,1,{0xEA}},////254
{0x04,1,{0x00}},////252
{0x05,1,{0xFF}},////252
{0x06,1,{0x01}},////250
{0x07,1,{0x0F}},////250
{0x08,1,{0x01}},////248
{0x09,1,{0x1E}},////248
{0x0A,1,{0x01}},//246
{0x0B,1,{0x2C}},//246
{0x0C,1,{0x01}},//244
{0x0D,1,{0x38}},//244
{0x0E,1,{0x01}},//242
{0x0F,1,{0x43}},//242
{0x10,1,{0x01}},//240
{0x11,1,{0x4D}},//240
{0x12,1,{0x01}},//232
{0x13,1,{0x71}},//232
{0x14,1,{0x01}},//224
{0x15,1,{0x8E}},//224
{0x16,1,{0x01}},//208
{0x17,1,{0xBE}},//208
{0x18,1,{0x01}},//192
{0x19,1,{0xE5}},//192
{0x1A,1,{0x02}},//160
{0x1B,1,{0x23}},//160
{0x1C,1,{0x02}},//128
{0x1D,1,{0x57}},//128
{0x1E,1,{0x02}},//127
{0x1F,1,{0x58}},//127
{0x20,1,{0x02}},//95
{0x21,1,{0x88}},//95
{0x22,1,{0x02}},//63
{0x23,1,{0xBE}},//63
{0x24,1,{0x02}},//47
{0x25,1,{0xDF}},//47
{0x26,1,{0x03}},//31
{0x27,1,{0x0B}},//31
{0x28,1,{0x03}},//23
{0x29,1,{0x28}},//23
{0x2A,1,{0x03}},//15
{0x2B,1,{0x4D}},//15
{0x2C,1,{0x03}},//13
{0x2D,1,{0x58}},//13
{0x2E,1,{0x03}},//11
{0x2F,1,{0x64}},//11
{0x30,1,{0x03}},//9
{0x31,1,{0x72}},//9
{0x32,1,{0x03}},//7
{0x33,1,{0x82}},//7
{0x34,1,{0x03}},//5
{0x35,1,{0x95}},//5
{0x36,1,{0x03}},//3
{0x37,1,{0xB1}},//3
{0x38,1,{0x03}},//1
{0x39,1,{0xD4}},//1
{0x3A,1,{0x03}},//0
{0x3B,1,{0xE6}},//0

{0x3C,1,{0x00}},//255
{0x3D,1,{0xDE}},//255
{0x3E,1,{0x00}},//254
{0x3F,1,{0xEA}},//254
{0x40,1,{0x00}},//252
{0x41,1,{0xFF}},//252
{0x42,1,{0x01}},//250
{0x43,1,{0x0F}},//250
{0x44,1,{0x01}},//248
{0x45,1,{0x1E}},//248
{0x46,1,{0x01}},//246
{0x47,1,{0x2C}},//246
{0x48,1,{0x01}},//244
{0x49,1,{0x38}},//244
{0x4A,1,{0x01}},//242
{0x4B,1,{0x43}},//242
{0x4C,1,{0x01}},//240
{0x4D,1,{0x4D}},//240
{0x4E,1,{0x01}},//232
{0x4F,1,{0x71}},//232
{0x50,1,{0x01}},//224
{0x51,1,{0x8E}},//224
{0x52,1,{0x01}},//208
{0x53,1,{0xBE}},//208
{0x54,1,{0x01}},//192
{0x55,1,{0xE5}},//192
{0x56,1,{0x02}},//160
{0x57,1,{0x23}},//160
{0x58,1,{0x02}},//128
{0x59,1,{0x57}},//128
{0x5A,1,{0x02}},//127
{0x5B,1,{0x58}},//127
{0x5C,1,{0x02}},//95
{0x5D,1,{0x88}},//95
{0x5E,1,{0x02}},//63
{0x5F,1,{0xBE}},//63
{0x60,1,{0x02}},//47
{0x61,1,{0xDF}},//47
{0x62,1,{0x03}},//31
{0x63,1,{0x0B}},//31
{0x64,1,{0x03}},//23
{0x65,1,{0x28}},//23
{0x66,1,{0x03}},//15
{0x67,1,{0x4D}},//15
{0x68,1,{0x03}},//13
{0x69,1,{0x58}},//13
{0x6A,1,{0x03}},//11
{0x6B,1,{0x64}},//11
{0x6C,1,{0x03}},//9
{0x6D,1,{0x72}},//9
{0x6E,1,{0x03}},//7
{0x6F,1,{0x82}},//7
{0x70,1,{0x03}},//5
{0x71,1,{0x95}},//5
{0x72,1,{0x03}},//3
{0x73,1,{0xB1}},//3
{0x74,1,{0x03}},//1
{0x75,1,{0xD4}},//1
{0x76,1,{0x03}},//0
{0x77,1,{0xE6}},//0

{0x78,1,{0x01}},
{0x79,1,{0x01}},

{0xFF,3,{0x78,0x07,0x04}},

{0x00,1,{0x00}},//255
{0x01,1,{0x95}},//255
{0x02,1,{0x00}},//254
{0x03,1,{0xAC}},//254
{0x04,1,{0x00}},//252
{0x05,1,{0xCA}},//252
{0x06,1,{0x00}},//250
{0x07,1,{0xE1}},//250
{0x08,1,{0x00}},//248
{0x09,1,{0xF4}},//248
{0x0A,1,{0x01}},//246
{0x0B,1,{0x05}},//246
{0x0C,1,{0x01}},//244
{0x0D,1,{0x13}},//244
{0x0E,1,{0x01}},//242
{0x0F,1,{0x20}},//242
{0x10,1,{0x01}},//240
{0x11,1,{0x2C}},//240
{0x12,1,{0x01}},//232
{0x13,1,{0x55}},//232
{0x14,1,{0x01}},//224
{0x15,1,{0x76}},//224
{0x16,1,{0x01}},//208
{0x17,1,{0xAB}},//208
{0x18,1,{0x01}},//192
{0x19,1,{0xD5}},//192
{0x1A,1,{0x02}},//160
{0x1B,1,{0x18}},//160
{0x1C,1,{0x02}},//128
{0x1D,1,{0x4E}},//128
{0x1E,1,{0x02}},//127
{0x1F,1,{0x50}},//127
{0x20,1,{0x02}},//95
{0x21,1,{0x81}},//95
{0x22,1,{0x02}},//63
{0x23,1,{0xB8}},//63
{0x24,1,{0x02}},//47
{0x25,1,{0xDB}},//47
{0x26,1,{0x03}},//31
{0x27,1,{0x09}},//31
{0x28,1,{0x03}},//23
{0x29,1,{0x2A}},//23
{0x2A,1,{0x03}},//15
{0x2B,1,{0x5C}},//15
{0x2C,1,{0x03}},//13
{0x2D,1,{0x6A}},//13
{0x2E,1,{0x03}},//11
{0x2F,1,{0x76}},//11
{0x30,1,{0x03}},//9
{0x31,1,{0x83}},//9
{0x32,1,{0x03}},//7
{0x33,1,{0x91}},//7
{0x34,1,{0x03}},//5
{0x35,1,{0xA4}},//5
{0x36,1,{0x03}},//3
{0x37,1,{0xBD}},//3
{0x38,1,{0x03}},//1
{0x39,1,{0xD8}},//1
{0x3A,1,{0x03}},//0
{0x3B,1,{0xE6}},//0

{0x3C,1,{0x00}},//255
{0x3D,1,{0x95}},//255
{0x3E,1,{0x00}},//254
{0x3F,1,{0xAC}},//254
{0x40,1,{0x00}},//252
{0x41,1,{0xCA}},//252
{0x42,1,{0x00}},//250
{0x43,1,{0xE1}},//250
{0x44,1,{0x00}},//248
{0x45,1,{0xF4}},//248
{0x46,1,{0x01}},//246
{0x47,1,{0x05}},//246
{0x48,1,{0x01}},//244
{0x49,1,{0x13}},//244
{0x4A,1,{0x01}},//242
{0x4B,1,{0x20}},//242
{0x4C,1,{0x01}},//240
{0x4D,1,{0x2C}},//240
{0x4E,1,{0x01}},//232
{0x4F,1,{0x55}},//232
{0x50,1,{0x01}},//224
{0x51,1,{0x76}},//224
{0x52,1,{0x01}},//208
{0x53,1,{0xAB}},//208
{0x54,1,{0x01}},//192
{0x55,1,{0xD5}},//192
{0x56,1,{0x02}},//160
{0x57,1,{0x18}},//160
{0x58,1,{0x02}},//128
{0x59,1,{0x4E}},//128
{0x5A,1,{0x02}},//127
{0x5B,1,{0x50}},//127
{0x5C,1,{0x02}},//95
{0x5D,1,{0x81}},//95
{0x5E,1,{0x02}},//63
{0x5F,1,{0xB8}},//63
{0x60,1,{0x02}},//47
{0x61,1,{0xDB}},//47
{0x62,1,{0x03}},//31
{0x63,1,{0x09}},//31
{0x64,1,{0x03}},//23
{0x65,1,{0x2A}},//23
{0x66,1,{0x03}},//15
{0x67,1,{0x5C}},//15
{0x68,1,{0x03}},//13
{0x69,1,{0x6A}},//13
{0x6A,1,{0x03}},//11
{0x6B,1,{0x76}},//11
{0x6C,1,{0x03}},//9
{0x6D,1,{0x83}},//9
{0x6E,1,{0x03}},//7
{0x6F,1,{0x91}},//7
{0x70,1,{0x03}},//5
{0x71,1,{0xA4}},//5
{0x72,1,{0x03}},//3
{0x73,1,{0xBD}},//3
{0x74,1,{0x03}},//1
{0x75,1,{0xD8}},//1
{0x76,1,{0x03}},//0
{0x77,1,{0xE6}},//0

{0x78,1,{0x01}},
{0x79,1,{0x01}},

{0xFF,3,{0x78,0x07,0x00}},
{0x36,1,{0x00}},
			
{0x11,01,{0x00}},                
{REGFLAG_DELAY, 120, {}},        
{0x29,01,{0x00}},	               
{REGFLAG_DELAY, 20, {}},         
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
LCM_DRIVER hct_ili7807d_dsi_vdo_fhd_innolcex_50_xzx = 
{
    .name           = "hct_ili7807d_dsi_vdo_fhd_innolcex_50_xzx",
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

