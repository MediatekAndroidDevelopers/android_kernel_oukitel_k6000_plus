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

#define FRAME_WIDTH  										(720)
#define FRAME_HEIGHT 										(1280)
#define LCM_ID                       						(0x1284)

#define REGFLAG_DELAY             							(0XFFFE)
#define REGFLAG_END_OF_TABLE      							(0xFff0)	// END OF REGISTERS MARKER


#define LCM_DSI_CMD_MODE									0

#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))

#define UDELAY(n) 											(lcm_util.udelay(n))
#define MDELAY(n) 											(lcm_util.mdelay(n))

#define LCM_RM68200_ID 		(0x6820)

//static unsigned int lcm_esd_test = FALSE;      ///only for ESD test

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)											lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

 struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};


static struct LCM_setting_table lcm_initialization_setting[] = {

	/*
Note :

Data ID will depends on the following rule.

count of parameters > 1      => Data ID = 0x39
count of parameters = 1      => Data ID = 0x15
count of parameters = 0      => Data ID = 0x05

Struclcm_deep_sleep_mode_in_settingture Format :

{DCS command, count of parameters, {parameter list}}
{REGFLAG_DELAY, milliseconds of time, {}},

...

Setting ending by predefined flag

{REGFLAG_END_OF_TABLE, 0x00, {}}
*/

/**************************************************
IC Name: F056A4
Panel Maker/Size: AUO546
Panel Product No.: H546TAN01
Version: V0
Date: 20140618_PFM
**************************************************/
 
{0xFE, 1,{0x01}},

{0x24, 1,{0xC0}},

{0x25, 1,{0x53}},//53

{0x26, 1,{0x00}},

{0x2B, 1,{0xE5}},

{0x27, 1,{0x0A}},

{0x29, 1,{0x0A}},

{0x16, 1,{0x52}},

{0x2F, 1,{0x53}},

{0x34, 1,{0x5A}},

{0x1B, 1,{0x00}},//00 50

{0x12, 1,{0x0A}},

{0x1A, 1,{0x06}},

{0x46, 1,{0x5C}},//56--64---74----6a---65----60-

{0x52, 1,{0x78}},//a0

{0x53, 1,{0x00}},

{0x54, 1,{0x78}},//a0

{0x55, 1,{0x00}},

{0x5F, 1,{0x13}},

{0xFE, 1,{0x03}},

{0x00, 1,{0x05}},

{0x01, 1,{0x16}},

{0x02, 1,{0x0B}},

{0x03, 1,{0x0F}},

{0x04, 1,{0x7D}},

{0x05, 1,{0x00}},

{0x06, 1,{0x50}},

{0x07, 1,{0x05}},

{0x08, 1,{0x16}},

{0x09, 1,{0x0D}},

{0x0A, 1,{0x11}},

{0x0B, 1,{0x7D}},

{0x0C, 1,{0x00}},

{0x0D, 1,{0x50}},

{0x0E, 1,{0x07}},

{0x0F, 1,{0x08}},

{0x10, 1,{0x01}},

{0x11, 1,{0x02}},

{0x12, 1,{0x00}},

{0x13, 1,{0x7D}},

{0x14, 1,{0x00}},

{0x15, 1,{0x85}},

{0x16, 1,{0x08}},

{0x17, 1,{0x03}},

{0x18, 1,{0x04}},

{0x19, 1,{0x05}},

{0x1A, 1,{0x06}},

{0x1B, 1,{0x00}},

{0x1C, 1,{0x7D}},

{0x1D, 1,{0x00}},

{0x1E, 1,{0x85}},

{0x1F, 1,{0x08}},

{0x20, 1,{0x00}},

{0x21, 1,{0x00}},

{0x22, 1,{0x00}},

{0x23, 1,{0x00}},

{0x24, 1,{0x00}},

{0x25, 1,{0x00}},

{0x26, 1,{0x00}},

{0x27, 1,{0x00}},

{0x28, 1,{0x00}},

{0x29, 1,{0x00}},

{0x2A, 1,{0x07}},

{0x2B, 1,{0x08}},

{0x2D, 1,{0x01}},

{0x2F, 1,{0x02}},

{0x30, 1,{0x00}},

{0x31, 1,{0x40}},

{0x32, 1,{0x05}},

{0x33, 1,{0x08}},

{0x34, 1,{0x54}},

{0x35, 1,{0x7D}},

{0x36, 1,{0x00}},

{0x37, 1,{0x03}},

{0x38, 1,{0x04}},

{0x39, 1,{0x05}},

{0x3A, 1,{0x06}},

{0x3B, 1,{0x00}},

{0x3D, 1,{0x40}},

{0x3F, 1,{0x05}},

{0x40, 1,{0x08}},

{0x41, 1,{0x54}},

{0x42, 1,{0x7D}},

{0x43, 1,{0x00}},

{0x44, 1,{0x00}},

{0x45, 1,{0x00}},

{0x46, 1,{0x00}},

{0x47, 1,{0x00}},

{0x48, 1,{0x00}},

{0x49, 1,{0x00}},

{0x4A, 1,{0x00}},

{0x4B, 1,{0x00}},

{0x4C, 1,{0x00}},

{0x4D, 1,{0x00}},

{0x4E, 1,{0x00}},

{0x4F, 1,{0x00}},

{0x50, 1,{0x00}},

{0x51, 1,{0x00}},

{0x52, 1,{0x00}},

{0x53, 1,{0x00}},

{0x54, 1,{0x00}},

{0x55, 1,{0x00}},

{0x56, 1,{0x00}},

{0x58, 1,{0x00}},/////////////////

{0x59, 1,{0x00}},

{0x5A, 1,{0x00}},

{0x5B, 1,{0x00}},

{0x5C, 1,{0x00}},

{0x5D, 1,{0x00}},

{0x5E, 1,{0x00}},

{0x5F, 1,{0x00}},

{0x60, 1,{0x00}},

{0x61, 1,{0x00}},

{0x62, 1,{0x00}},

{0x63, 1,{0x00}},

{0x64, 1,{0x00}},

{0x65, 1,{0x00}},

{0x66, 1,{0x00}},

{0x67, 1,{0x00}},

{0x68, 1,{0x00}},

{0x69, 1,{0x00}},

{0x6A, 1,{0x00}},

{0x6B, 1,{0x00}},

{0x6C, 1,{0x00}},

{0x6D, 1,{0x00}},

{0x6E, 1,{0x00}},

{0x6F, 1,{0x00}},

{0x70, 1,{0x00}},

{0x71, 1,{0x00}},

{0x72, 1,{0x20}},

{0x73, 1,{0x00}},

{0x74, 1,{0x08}},

{0x75, 1,{0x08}},

{0x76, 1,{0x08}},

{0x77, 1,{0x08}},

{0x78, 1,{0x08}},

{0x79, 1,{0x08}},

{0x7A, 1,{0x00}},

{0x7B, 1,{0x00}},

{0x7C, 1,{0x00}},

{0x7D, 1,{0x00}},

{0x7E, 1,{0xBF}},

{0x7F, 1,{0x02}},

{0x80, 1,{0x06}},

{0x81, 1,{0x14}},

{0x82, 1,{0x10}},

{0x83, 1,{0x16}},

{0x84, 1,{0x12}},

{0x85, 1,{0x08}},

{0x86, 1,{0x3F}},

{0x87, 1,{0x3F}},

{0x88, 1,{0x3F}},

{0x89, 1,{0x3F}},

{0x8A, 1,{0x3F}},

{0x8B, 1,{0x0C}},

{0x8C, 1,{0x0A}},

{0x8D, 1,{0x0E}},

{0x8E, 1,{0x3F}},

{0x8F, 1,{0x3F}},

{0x90, 1,{0x00}},

{0x91, 1,{0x04}},

{0x92, 1,{0x3F}},

{0x93, 1,{0x3F}},

{0x94, 1,{0x3F}},

{0x95, 1,{0x3F}},

{0x96, 1,{0x05}},

{0x97, 1,{0x01}},

{0x98, 1,{0x3F}},

{0x99, 1,{0x3F}},

{0x9A, 1,{0x0F}},

{0x9B, 1,{0x0B}},

{0x9C, 1,{0x0D}},

{0x9D, 1,{0x3F}},

{0x9E, 1,{0x3F}},

{0x9F, 1,{0x3F}},

{0xA0, 1,{0x3F}},

{0xA2, 1,{0x3F}},

{0xA3, 1,{0x09}},

{0xA4, 1,{0x13}},

{0xA5, 1,{0x17}},

{0xA6, 1,{0x11}},

{0xA7, 1,{0x15}},

{0xA9, 1,{0x07}},

{0xAA, 1,{0x03}},

{0xAB, 1,{0x3F}},

{0xAC, 1,{0x3F}},

{0xAD, 1,{0x05}},

{0xAE, 1,{0x01}},

{0xAF, 1,{0x17}},

{0xB0, 1,{0x13}},

{0xB1, 1,{0x15}},

{0xB2, 1,{0x11}},

{0xB3, 1,{0x0F}},

{0xB4, 1,{0x3F}},

{0xB5, 1,{0x3F}},

{0xB6, 1,{0x3F}},

{0xB7, 1,{0x3F}},

{0xB8, 1,{0x3F}},

{0xB9, 1,{0x0B}},

{0xBA, 1,{0x0D}},

{0xBB, 1,{0x09}},

{0xBC, 1,{0x3F}},

{0xBD, 1,{0x3F}},

{0xBE, 1,{0x07}},

{0xBF, 1,{0x03}},

{0xC0, 1,{0x3F}},

{0xC1, 1,{0x3F}},

{0xC2, 1,{0x3F}},

{0xC3, 1,{0x3F}},

{0xC4, 1,{0x02}},

{0xC5, 1,{0x06}},

{0xC6, 1,{0x3F}},

{0xC7, 1,{0x3F}},

{0xC8, 1,{0x08}},

{0xC9, 1,{0x0C}},

{0xCA, 1,{0x0A}},

{0xCB, 1,{0x3F}},

{0xCC, 1,{0x3F}},

{0xCD, 1,{0x3F}},

{0xCE, 1,{0x3F}},

{0xCF, 1,{0x3F}},

{0xD0, 1,{0x0E}},

{0xD1, 1,{0x10}},

{0xD2, 1,{0x14}},

{0xD3, 1,{0x12}},

{0xD4, 1,{0x16}},

{0xD5, 1,{0x00}},

{0xD6, 1,{0x04}},

{0xD7, 1,{0x3F}},

{0xDC, 1,{0x02}},

{0xDE, 1,{0x12}},

//{0xFE, 1,{0x0E}},//error report on


{0xFE, 1,{0x0E}},//error report off

{0x01, 1,{0x75}},

{0x54, 1,{0x01}},

{0x1B, 1,{0x00}},

{0x1C, 1,{0x00}},


{0xFE, 1,{0x04}},
//gamma2.2
{0x60, 1,{0x00}},


{0x61, 1,{0x0C}},

{0x62, 1,{0x12}},

{0x63, 1,{0x0E}},

{0x64, 1,{0x06}},

{0x65, 1,{0x12}},

{0x66, 1,{0x0E}},

{0x67, 1,{0x0B}},

{0x68, 1,{0x15}},

{0x69, 1,{0x0B}},

{0x6A, 1,{0x10}},

{0x6B, 1,{0x07}},

{0x6C, 1,{0x0F}},

{0x6D, 1,{0x12}},

{0x6E, 1,{0x0C}},

{0x6F, 1,{0x00}},

{0x70, 1,{0x00}},

{0x71, 1,{0x0C}},

{0x72, 1,{0x12}},

{0x73, 1,{0x0E}},

{0x74, 1,{0x06}},

{0x75, 1,{0x12}},

{0x76, 1,{0x0E}},

{0x77, 1,{0x0B}},

{0x78, 1,{0x15}},

{0x79, 1,{0x0B}},

{0x7A, 1,{0x10}},

{0x7B, 1,{0x07}},

{0x7C, 1,{0x0F}},

{0x7D, 1,{0x12}},

{0x7E, 1,{0x0C}},

{0x7F, 1,{0x00}},
//gamma2.2

/*//another gamma
{0x60, 1,{0x05}},

{0x61, 1,{0x0F}},

{0x62, 1,{0x16}},

{0x63, 1,{0x0D}},

{0x64, 1,{0x06}},

{0x65, 1,{0x11}},

{0x66, 1,{0x0E}},

{0x67, 1,{0x0A}},

{0x68, 1,{0x16}},

{0x69, 1,{0x0B}},

{0x6A, 1,{0x11}},

{0x6B, 1,{0x08}},

{0x6C, 1,{0x0F}},

{0x6D, 1,{0x10}},

{0x6E, 1,{0x0A}},

{0x6F, 1,{0x00}},

{0x72, 1,{0x16}},

{0x7D, 1,{0x10}},

{0x75, 1,{0x11}},

{0x7A, 1,{0x11}},

{0x71, 1,{0x0F}},

{0x73, 1,{0x0D}},

{0x74, 1,{0x06}},

{0x76, 1,{0x0E}},

{0x77, 1,{0x0A}},

{0x78, 1,{0x16}},

{0x79, 1,{0x0B}},

{0x7B, 1,{0x08}},

{0x7C, 1,{0x0F}},

{0x7E, 1,{0x0A}},

{0x70, 1,{0x05}},

{0x7F, 1,{0x00}},

*///another gamma
/*
//gamma2.5 twl
{0x62, 1,{0x1a}},
{0x72, 1,{0x1a}},

{0x6D, 1,{0x12}},
{0x7D, 1,{0x10}},

{0x75, 1,{0x13}},
{0x65, 1,{0x13}},

{0x6A, 1,{0x0e}},
{0x7A, 1,{0x0c}},

{0x61, 1,{0x13}},
{0x71, 1,{0x15}},

{0x63, 1,{0x0d}},
{0x73, 1,{0x0f}},

{0x64, 1,{0x05}},

{0x74, 1,{0x07}},


{0x66, 1,{0x0d}},
{0x76, 1,{0x0f}},

{0x67, 1,{0x0a}},
{0x77, 1,{0x0a}},

{0x68, 1,{0x18}},
{0x78, 1,{0x16}},


{0x69, 1,{0x0d}},
{0x79, 1,{0x0d}},


{0x6B, 1,{0x07}},
{0x7B, 1,{0x03}},

{0x6C, 1,{0x0e}},
{0x7C, 1,{0x0a}},


{0x6E, 1,{0x11}},
{0x7E, 1,{0x0b}},

{0x70, 1,{0x0d}},
{0x60, 1,{0x07}},

{0x6F, 1,{0x00}},
{0x7F, 1,{0x00}},*/

//gamma2.5 twl

{0xFE, 1,{0x00}},

//{0x36, 1,{0x00}},

{0x11, 0,{0x00}},

{REGFLAG_DELAY, 120, {}},

{0x29, 0,{0x00}},

{REGFLAG_DELAY, 50, {}},

{REGFLAG_END_OF_TABLE, 0x00, {}}
};

/*
static struct LCM_setting_table lcm_sleep_out_setting[] = {
	// Sleep Out
	{0x11, 1, {0x00}},
	{REGFLAG_DELAY, 120, {}},

	// Display ON
	{0x29, 1, {0x00}},
	{REGFLAG_DELAY, 10, {}},

	{REGFLAG_END_OF_TABLE, 0x00, {}}
};
*/
/*
static struct LCM_setting_table lcm_sleep_in_setting[] = {
	// Display off sequence
	{0x01, 1, {0x00}},
	{REGFLAG_DELAY, 50, {}},
	
	{0x28, 1, {0x00}},
	{REGFLAG_DELAY, 50, {}},

	// Sleep Mode On
	{0x10, 1, {0x00}},
	{REGFLAG_DELAY, 50, {}},

	{REGFLAG_END_OF_TABLE, 0x00, {}}
};
*/
static void push_table(struct LCM_setting_table *table, unsigned int count,
		unsigned char force_update)
{
	unsigned int i;

	for (i = 0; i < count; i++) {

		unsigned cmd;
		cmd = table[i].cmd;

		switch (cmd) {

		case REGFLAG_DELAY:
			MDELAY(table[i].count);
			break;

		case REGFLAG_END_OF_TABLE:
			break;

		default:
			dsi_set_cmdq_V2(cmd, table[i].count,
					table[i].para_list, force_update);
		}
	}

}


// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS * util)
{
	memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS * params)
{
	memset(params, 0, sizeof(LCM_PARAMS));

	params->type = LCM_TYPE_DSI;

	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;

	// enable tearing-free
	params->dbi.te_mode = LCM_DBI_TE_MODE_DISABLED;
	params->dbi.te_edge_polarity = LCM_POLARITY_RISING;

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode = CMD_MODE;
#else
	params->dsi.mode   = SYNC_PULSE_VDO_MODE;//SYNC_EVENT_VDO_MODE;//BURST_VDO_MODE;////
#endif

	// DSI
	/* Command mode setting */
		params->dsi.LANE_NUM				= LCM_FOUR_LANE;
	
	//The following defined the fomat for data coming from LCD engine.
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;
	
	
	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
	
#if (LCM_DSI_CMD_MODE)
	params->dsi.intermediat_buffer_num = 0;//because DSI/DPI HW design change, this parameters should be 0 when video mode in MT658X; or memory leakage
	params->dsi.word_count=FRAME_WIDTH*3;	//DSI CMD mode need set these two bellow params, different to 6577
#else
	params->dsi.intermediat_buffer_num = 0;	//because DSI/DPI HW design change, this parameters should be 0 when video mode in MT658X; or memory leakage
#endif

	// Video mode setting
	params->dsi.packet_size=256;

	params->dsi.vertical_sync_active				=  2;//4
	params->dsi.vertical_backporch					= 14;//50;
	params->dsi.vertical_frontporch					= 16;//50;
	params->dsi.vertical_active_line				= FRAME_HEIGHT; 

	params->dsi.horizontal_sync_active				= 8;//10
	params->dsi.horizontal_backporch				= 48;//34; 
	params->dsi.horizontal_frontporch				= 48;//24;
	params->dsi.horizontal_active_pixel				= FRAME_WIDTH;


	/*params->dsi.horizontal_sync_active				= 4;//10
	params->dsi.horizontal_backporch				= 16;//34; 
	params->dsi.horizontal_frontporch				= 16;//24;
	params->dsi.horizontal_active_pixel				= FRAME_WIDTH;*/

	// Bit rate calculation
	//1 Every lane speed
	params->dsi.PLL_CLOCK=230;//246;//228//225--63 230

	params->dsi.cont_clock = 1;
	params->dsi.clk_lp_per_line_enable = 1;
	params->dsi.ssc_disable = 1;	
	/*
	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 1;

        params->dsi.lcm_esd_check_table[0].cmd          = 0x0A;
        params->dsi.lcm_esd_check_table[0].count        = 1;
        params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;
*/
	
}

static void lcm_init(void)
{
#ifdef GPIO_LCM_LDO_1V8_EN_PIN
    lcm_util.set_gpio_mode(GPIO_LCM_LDO_1V8_EN_PIN, GPIO_MODE_00);
    lcm_util.set_gpio_dir(GPIO_LCM_LDO_1V8_EN_PIN, GPIO_DIR_OUT);
    lcm_util.set_gpio_out(GPIO_LCM_LDO_1V8_EN_PIN, GPIO_OUT_ONE);  //shm
#else   
    mt_dsi_pinctrl_set(LCM_POWER_DM_NO, 1);
#endif
    MDELAY(10);
	
#ifdef GPIO_LCM_LDO_2V8_EN_PIN
    lcm_util.set_gpio_mode(GPIO_LCM_LDO_2V8_EN_PIN, GPIO_MODE_00);
    lcm_util.set_gpio_dir(GPIO_LCM_LDO_2V8_EN_PIN, GPIO_DIR_OUT);
    lcm_util.set_gpio_out(GPIO_LCM_LDO_2V8_EN_PIN, GPIO_OUT_ONE);
#else   
    mt_dsi_pinctrl_set(LCM_POWER_DP_NO, 1);
#endif
    MDELAY(20);

	SET_RESET_PIN(1);
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(120);

	push_table(lcm_initialization_setting,sizeof(lcm_initialization_setting) /sizeof(struct LCM_setting_table), 1);
}

static void lcm_suspend(void)
{
	unsigned int array[5];
	array[0] = 0x00FE1500;
	dsi_set_cmdq(array, 1, 1);
	MDELAY(5);
	array[0] = 0x00280500;
	dsi_set_cmdq(array, 1, 1);
	MDELAY(50);
 	array[0] = 0x00100500;// read id return two byte,version and id
	dsi_set_cmdq(array, 1, 1);
	MDELAY(120);
#ifdef GPIO_LCM_RST
    lcm_util.set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_00);
    lcm_util.set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
    lcm_util.set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ZERO);  //shm
#else    
    SET_RESET_PIN(0);
#endif
    MDELAY(10);
#ifdef GPIO_LCM_LDO_2V8_EN_PIN
    lcm_util.set_gpio_mode(GPIO_LCM_LDO_2V8_EN_PIN, GPIO_MODE_00);
    lcm_util.set_gpio_dir(GPIO_LCM_LDO_2V8_EN_PIN, GPIO_DIR_OUT);
    lcm_util.set_gpio_out(GPIO_LCM_LDO_2V8_EN_PIN, GPIO_OUT_ZERO);
#else   
    mt_dsi_pinctrl_set(LCM_POWER_DP_NO, 0);
#endif
    MDELAY(5);
#ifdef GPIO_LCM_LDO_1V8_EN_PIN
    lcm_util.set_gpio_mode(GPIO_LCM_LDO_1V8_EN_PIN, GPIO_MODE_00);
    lcm_util.set_gpio_dir(GPIO_LCM_LDO_1V8_EN_PIN, GPIO_DIR_OUT);
    lcm_util.set_gpio_out(GPIO_LCM_LDO_1V8_EN_PIN, GPIO_OUT_ZERO);  //shm
#else   
    mt_dsi_pinctrl_set(LCM_POWER_DM_NO, 0);
#endif
    MDELAY(5);
}


static void lcm_resume(void)
{
    //unsigned int id;
	//unsigned char buffer[5];
	//unsigned int array[5];
	//lcm_compare_id();
//	push_table(lcm_sleep_out_setting,sizeof(lcm_sleep_out_setting) /sizeof(struct LCM_setting_table), 1);
#ifdef GPIO_LCM_LDO_2V8_EN_PIN
    lcm_util.set_gpio_mode(GPIO_LCM_LDO_2V8_EN_PIN, GPIO_MODE_00);
    lcm_util.set_gpio_dir(GPIO_LCM_LDO_2V8_EN_PIN, GPIO_DIR_OUT);
    lcm_util.set_gpio_out(GPIO_LCM_LDO_2V8_EN_PIN, GPIO_OUT_ZERO);
#else   
    mt_dsi_pinctrl_set(LCM_POWER_DP_NO, 0);    // 2V8 off
#endif
    MDELAY(5);
#ifdef GPIO_LCM_LDO_1V8_EN_PIN
    lcm_util.set_gpio_mode(GPIO_LCM_LDO_1V8_EN_PIN, GPIO_MODE_00);
    lcm_util.set_gpio_dir(GPIO_LCM_LDO_1V8_EN_PIN, GPIO_DIR_OUT);
    lcm_util.set_gpio_out(GPIO_LCM_LDO_1V8_EN_PIN, GPIO_OUT_ZERO);  //shm
#else   
    mt_dsi_pinctrl_set(LCM_POWER_DM_NO, 0);
#endif
    MDELAY(20);
	lcm_init();
			
/*array[0] = 0x00063902;// read id return two byte,version and id
	array[1] = 0x52AA55F0;
	array[2] = 0x00000108;
	dsi_set_cmdq(array, 3, 1);
	MDELAY(10);
	
	array[0] = 0x00023700;// read id return two byte,version and id
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0xc5, buffer, 2);
	id = ((buffer[0] << 8) | buffer[1]);
#if defined(BUILD_LK)
printf("%s, [rm68191_ctc50_jhzt]  buffer[0] = [0x%d] buffer[2] = [0x%d] ID = [0x%d]\n",__func__, buffer[0], buffer[1], id);
#else
printk("%s, [rm68191_ctc50_jhzt]  buffer[0] = [0x%d] buffer[2] = [0x%d] ID = [0x%d]\n",__func__, buffer[0], buffer[1], id);
#endif*/
}


static unsigned int lcm_compare_id(void)
{

 unsigned int array[4];
 char buffer[5];
 char id_high=0;
 char id_low=0;
 int id1=0;
 int id2=0;

#ifdef GPIO_LCM_LDO_1V8_EN_PIN
    lcm_util.set_gpio_mode(GPIO_LCM_LDO_1V8_EN_PIN, GPIO_MODE_00);
    lcm_util.set_gpio_dir(GPIO_LCM_LDO_1V8_EN_PIN, GPIO_DIR_OUT);
    lcm_util.set_gpio_out(GPIO_LCM_LDO_1V8_EN_PIN, GPIO_OUT_ONE);  //shm
#else   
    mt_dsi_pinctrl_set(LCM_POWER_DM_NO, 1);
#endif
    MDELAY(10);
#ifdef GPIO_LCM_LDO_2V8_EN_PIN
    lcm_util.set_gpio_mode(GPIO_LCM_LDO_2V8_EN_PIN, GPIO_MODE_00);
    lcm_util.set_gpio_dir(GPIO_LCM_LDO_2V8_EN_PIN, GPIO_DIR_OUT);
    lcm_util.set_gpio_out(GPIO_LCM_LDO_2V8_EN_PIN, GPIO_OUT_ONE);
#else   
    mt_dsi_pinctrl_set(LCM_POWER_DP_NO, 1);
#endif
    MDELAY(50);
	
 SET_RESET_PIN(1);
 MDELAY(10);
 SET_RESET_PIN(0);
 MDELAY(10);
 SET_RESET_PIN(1);
 MDELAY(200);
 array[0]=0x01FE1500;
 dsi_set_cmdq(array,1, 1);
 
 array[0] = 0x00013700;
 dsi_set_cmdq(array, 1, 1);
 read_reg_v2(0xde, buffer, 1);

 id_high = buffer[0];
 read_reg_v2(0xdf, buffer, 1);
 id_low = buffer[0];
 id1 = (id_high<<8) | id_low;

 #if defined(BUILD_LK)
  printf("rm68200a %s id1 = 0x%04x, id2 = 0x%04x\n", __func__, id1,id2);
 #else
  printk("rm68200a %s id1 = 0x%04x, id2 = 0x%04x\n", __func__, id1,id2);
 #endif
#ifdef GPIO_LCM_LDO_2V8_EN_PIN
    lcm_util.set_gpio_mode(GPIO_LCM_LDO_2V8_EN_PIN, GPIO_MODE_00);
    lcm_util.set_gpio_dir(GPIO_LCM_LDO_2V8_EN_PIN, GPIO_DIR_OUT);
    lcm_util.set_gpio_out(GPIO_LCM_LDO_2V8_EN_PIN, GPIO_OUT_ZERO);
#else   
    mt_dsi_pinctrl_set(LCM_POWER_DP_NO, 0);    // 2V8 off
#endif
    MDELAY(10);
#ifdef GPIO_LCM_LDO_1V8_EN_PIN
    lcm_util.set_gpio_mode(GPIO_LCM_LDO_1V8_EN_PIN, GPIO_MODE_00);
    lcm_util.set_gpio_dir(GPIO_LCM_LDO_1V8_EN_PIN, GPIO_DIR_OUT);
    lcm_util.set_gpio_out(GPIO_LCM_LDO_1V8_EN_PIN, GPIO_OUT_ZERO);  //shm
#else   
    mt_dsi_pinctrl_set(LCM_POWER_DM_NO, 0);
#endif
    MDELAY(50);
 return (LCM_RM68200_ID == id1)?1:0;

}
LCM_DRIVER hct_rm68200_dsi_vdo_hd_auo_50_ykl = 
{
	.name			= "hct_rm68200_dsi_vdo_hd_auo_50_ykl",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,	
	.compare_id     = lcm_compare_id,	

#if (LCM_DSI_CMD_MODE)
    //.update         = lcm_update,
#endif	//wqtao
};

