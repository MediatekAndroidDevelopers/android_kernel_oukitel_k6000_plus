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

#define FRAME_WIDTH  									(720)
#define FRAME_HEIGHT 									(1280)

#define REGFLAG_DELAY             							0xAB
#define REGFLAG_END_OF_TABLE      						0xAA   // END OF REGISTERS MARKER


#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif
//static unsigned int lcm_esd_test = FALSE;      ///only for ESD test
// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))

#define R63350_LCM_ID       (0x3350)

#if defined(BUILD_UBOOT)
#define LCM_DEBUG(fmt, arg...) printf(fmt, ##arg)
#elif defined(BUILD_LK)
#define LCM_DEBUG(fmt, arg...) printf(fmt, ##arg)
#else
#define SENSORDB(fmt, arg...) xlog_printk(ANDROID_LOG_DEBUG, "[R63350]", fmt, ##arg)
#define LCM_DEBUG(fmt, arg...) printk(fmt, ##arg)
#endif

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)									lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)				lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg											lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

struct LCM_setting_table {
    unsigned char cmd;
    unsigned char count;
    unsigned char para_list[64];
};

#if 0 
static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
    unsigned int i;
    
    for(i = 0; i < count; i++) {
        unsigned cmd;
        cmd = table[i].cmd;
        
        switch (cmd) {
            case REGFLAG_DELAY :
                MDELAY(table[i].count);
                break;
            
            case REGFLAG_END_OF_TABLE :
                break;
            
            default:
                dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);				
            
        }
    }
}
#endif

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

	params->dbi.te_mode				 = LCM_DBI_TE_MODE_DISABLED;
	params->dbi.te_edge_polarity		 = LCM_POLARITY_RISING;

	params->dsi.mode   = SYNC_EVENT_VDO_MODE;//BURST_VDO_MODE;

	params->dsi.LANE_NUM			 = LCM_FOUR_LANE;

	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq	 = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding 	 = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format	 = LCM_DSI_FORMAT_RGB888;

	params->dsi.packet_size=256;

	params->dsi.intermediat_buffer_num =  0; //2;

	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
	params->dsi.word_count=720*3;

	params->dsi.vertical_sync_active		= 2;
	params->dsi.vertical_backporch			= 6;
	params->dsi.vertical_frontporch			= 8;	
	params->dsi.vertical_active_line			= FRAME_HEIGHT;

	params->dsi.horizontal_sync_active		= 10; //2
	params->dsi.horizontal_backporch		= 80; 
	params->dsi.horizontal_frontporch		= 80; //4
	params->dsi.horizontal_active_pixel		= FRAME_WIDTH;
	params->dsi.PLL_CLOCK = 215;//1

	params->dsi.ssc_disable = 1;
	params->dsi.noncont_clock = 1;
	params->dsi.noncont_clock_period = 2;

	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable      = 1;
	params->dsi.lcm_esd_check_table[0].cmd            = 0x0a;
	params->dsi.lcm_esd_check_table[0].count          = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x1C;
	
}


static void lcm_init(void)
{
	unsigned int data_array[16];

LCM_DEBUG("R63350: %s, %d", __func__, __LINE__);
	SET_RESET_PIN(1);
	MDELAY(20);
	SET_RESET_PIN(0);
	MDELAY(30);
	SET_RESET_PIN(1);
	MDELAY(50);

	data_array[0]=0x00022902;
	data_array[1]=0x000000B0;//B0
	dsi_set_cmdq(data_array,2, 1);

	data_array[0]=0x00022902;
	data_array[1]=0x000001D6;//D6
	dsi_set_cmdq(data_array,2, 1);

	data_array[0] = 0x00072902;
	data_array[1] = 0x000014B3;//B3
	data_array[2] = 0x00000000;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00032902;
	data_array[1] = 0x00000CB4;//B4 0C_4L 08_3L
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00042902;
	data_array[1] = 0x00BB49B6;//B6
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00242902;
	data_array[1] = 0x006604C1;//C1
	data_array[2] = 0x0A2F93FF;
	data_array[3] = 0x7FFFFFF8;
	data_array[4] = 0xA96C62D4;
	data_array[5] = 0x1FFFFFF3;
	data_array[6] = 0xF8FE1F94;
	data_array[7] = 0x00000000;
	data_array[8] = 0x02006200;
	data_array[9] = 0x11010022;
	dsi_set_cmdq(data_array, 10, 1);

	data_array[0] = 0x00092902;
	data_array[1] = 0x80F531C2;//C2
	data_array[2] = 0x00000808;
	data_array[3] = 0x00000008;
	dsi_set_cmdq(data_array, 4, 1);

	data_array[0] = 0x000C2902;
	data_array[1] = 0x000070C4;//C4
	data_array[2] = 0x00000000;
	data_array[3] = 0x03010000;
	dsi_set_cmdq(data_array, 4, 1);

	data_array[0] = 0x00162902;
	data_array[1] = 0x7501C8C6;//C6
	data_array[2] = 0x00006A05;
	data_array[3] = 0x00000000;
	data_array[4] = 0x00000000;
	data_array[5] = 0x1B0A0000;
	data_array[6] = 0x0000C807;
	dsi_set_cmdq(data_array, 7, 1);

	data_array[0] = 0x001F2902;
	data_array[1] = 0x2A2108C7;//C7
	data_array[2] = 0x5A504234;
	data_array[3] = 0x5F554D69;
	data_array[4] = 0x7D76716A;
	data_array[5] = 0x33292108;
	data_array[6] = 0x695A5042;
	data_array[7] = 0x6A60554D;
	data_array[8] = 0x007D7671;
	dsi_set_cmdq(data_array, 9, 1);

	data_array[0] = 0x00102902;
	data_array[1] = 0xFFFFFFCB;//CB
	data_array[2] = 0x000000FF;
	data_array[3] = 0x00000000;
	data_array[4] = 0x0000E000;
	dsi_set_cmdq(data_array, 5, 1);

	data_array[0]=0x00022902;
	data_array[1]=0x000008CC;//CC
	dsi_set_cmdq(data_array,2, 1);

	data_array[0] = 0x000B2902;
	data_array[1] = 0x000011D0;//D0
	data_array[2] = 0x1940D454;
	data_array[3] = 0x00000919;
	dsi_set_cmdq(data_array, 4, 1);

	data_array[0] = 0x00052902;
	data_array[1] = 0x164800D1;//D1
	data_array[2] = 0x0000000F;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x001B2902;
	data_array[1] = 0xBB331BD3;//D3
	data_array[2] = 0x3333B3BB;
	data_array[3] = 0x01003333;
	data_array[4] = 0xA0D80000;
	data_array[5] = 0x333F3F0B;
	data_array[6] = 0x8A127233;
	data_array[7] = 0x00BC3D57;
	dsi_set_cmdq(data_array, 8, 1);

	data_array[0] = 0x00082902;
	data_array[1] = 0x000006D5;//D5 VCOM
	data_array[2] = 0x19011901;
	dsi_set_cmdq(data_array, 3, 1);
	MDELAY(10);

	data_array[0] = 0x00290500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(30);

	data_array[0] = 0x00110500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(150);

}

static void lcm_suspend(void)
{
	unsigned int data_array[16];


	data_array[0]=0x00280500; 
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(30);

	data_array[0] = 0x00100500; 
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120);

	data_array[0] = 0x00022902;
	data_array[1] = 0x000000b0;
	dsi_set_cmdq(data_array, 2, 1);
	
	data_array[0] = 0x00022902;
	data_array[1] = 0x000001b1;
	dsi_set_cmdq(data_array, 2, 1);	
	MDELAY(30);
	
	SET_RESET_PIN(0);
	MDELAY(30);
	

}

static void lcm_resume(void)
{
	lcm_init();
	
}


static unsigned int lcm_compare_id(void)
{
	unsigned int id = 0;
	unsigned char buffer[5]; 
	unsigned int array[16];   
	//unsigned char id_high = 0;
	//unsigned char id_low = 0;
       // int i=0;
	
	LCM_DEBUG("R63350: %s, %d", __func__, __LINE__);
	
	SET_RESET_PIN(1);
	MDELAY(20);
	SET_RESET_PIN(0);
	MDELAY(30);
	SET_RESET_PIN(1);
	MDELAY(50);


	array[0] = 0x00022902;                          
	array[1] = 0x000000b0; 
	dsi_set_cmdq(array, 2, 1);//02 3C 33 50 XX

	array[0] = 0x00053700;// read id return two byte,version and id

	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0xbf, buffer, 5);

	id = buffer[2]<<8 | buffer[3]; //we only need ID

    #ifdef BUILD_LK
		LCM_DEBUG("%s, R63350-=-LCM-LK: id= x%08x,id0 = 0x%08x,id1 = 0x%08x,id2 = 0x%08x,id3 = 0x%08x,id4 = 0x%08x\n", __func__, id,buffer[0],buffer[1],buffer[2],buffer[3],buffer[4]);
    #else
		LCM_DEBUG("%s, R63350-=-LCM-KERNEL: id= x%08x,id0 = 0x%08x,id1 = 0x%08x,id2 = 0x%08x,id3 = 0x%08x,id4 = 0x%08x\n", __func__, id,buffer[0],buffer[1],buffer[2],buffer[3],buffer[4]);
    #endif


	return (R63350_LCM_ID == id)?1:0;
}



// ---------------------------------------------------------------------------
//  Get LCM Driver Hooks
// ---------------------------------------------------------------------------
LCM_DRIVER hct_r63350_dsi_vdo_hd_auo_55_rx = 
{
    .name			= "hct_r63350_dsi_vdo_hd_auo_55_rx",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.compare_id     = lcm_compare_id,
	//.esd_check 	= lcm_esd_check,
	//.esd_recover 	= lcm_esd_recover	
};

