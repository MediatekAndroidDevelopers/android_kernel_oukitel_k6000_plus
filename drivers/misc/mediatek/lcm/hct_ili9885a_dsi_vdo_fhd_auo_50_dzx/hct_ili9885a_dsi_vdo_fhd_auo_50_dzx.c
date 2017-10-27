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
#define LCM_ID                       (0x9885)

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
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[124];
};
//============9885initialCode========
//注意最大分包长度等级设置为最高，最多123个参数
/////////////////////////////////////////////////////////////////

static struct LCM_setting_table lcm_initialization_setting[] = {

{0xB0,3,{0x98,0x85,0x0A}},

{0xC1,3,{0x00,0x00,0x00}},
{0xC2,5,{0x10,0xF7,0x80,0x08,0x0C}},
{0xC4,7,{0x70,0x19,0x23,0x00,0x0A,0x0A,0x00}},     //º¡®Ø
{0xCE,5,{0x11,0x22,0x34,0x00,0x0E}},
{0xD0,6,{0x55,0x05,0x32,0xE1,0x6C,0xC0}},     //single mode
{0xD1,3,{0x09,0x09,0xC2}},
{0xD2,3,{0x13,0x13,0xCA}},
{0xD3,11,{0x33,0x33,0x05,0x03,0x63,0x63,0x24,0x17,0x22,0x43,0x6E}},       //use VCOM1 for VCOMDC3

{0xD5,14,{0x8B,0x00,0x00,0x00,0x01,0x64,0x01,0x64,0x01,0x64,0x00,0x00,0x00,0x00}},    //set Vcom voltage 
{0xD6,01,{0x80}},

{0xDD,27,{0x10,0x31,0x00,0x3B,0x33,0x1D,0x00,0x3C,0x00,0x3C,0x00,0x49,0x00,0xFF,0xFF,0xF8,0xE1,0x80,0x33,0x33,0x15,0x08,0xA0,0x01,0x00,0x71,0x7F}},
{0xDE,20,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},

//GIP setting
{0xE5,71,{0x00,0x81,0x87,0x85,0x80,0x80,0x84,0x81,0x85,0x82,0x86,0x83,0x87,0x80,0x00,0x80,
          0x86,0x84,0x82,0x83,0x00,0x80,0x86,0x84,0x80,0x80,0x84,0x81,0x85,0x82,0x86,0x83,0x87,0x80,
          0x00,0x81,0x87,0x85,0x82,0x83,0x06,0x30,0x76,0x54,0x01,0xAC,0x3F,0xEF,0x01,0x01,0xAC,0x23,0x12,0x01,0xAC,
          0x23,0x05,0x26,0x01,0xAC,0x23,0x25,0x26,0x01,0xAC,0x00,0x00,0x00,0x00,0x00,0x00}},


{0xEA,15,{0x33,0x32,0x03,0x03,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x03}},
{0xEF,9,{0x3C,0x05,0x52,0x13,0xE1,0x33,0x5B,0x08,0x0C}},

//only for test purpose
{0xDF,11,{0x00,0x00,0x00,0x06,0x01,0x10,0x00,0x00,0x00,0x00,0xFE}},   //to measure GVDDP & GVDDN

//{0xE9,01,{0x01},                 //SC   QHD
//{0xB4,02,{0x04,0x16}},              //LANES CONTROL DSIINTERFACE CONTROL    
      
{0xEE,19,{0x22,0x10,0x02,0x02,0x0F,0x40,0x00,0x07,0x00,0x04,0x00,0x00,0x00,0xB9,0x77,0x00,0x55,0x05,0x00}},   //ILI4003

//gamma setting
{0xC7,122,{0x00,0x00,0x00,0x1A,0x00,0x4F,0x00,0x65,0x00,0x7D,0x00,0x96,
           0x00,0xA5,0x00,0xB5,0x00,0xC5,0x00,0xF3,0x01,0x1C,0x01,0x57,0x01,
           0x89,0x01,0xD5,0x02,0x13,0x02,0x15,0x02,0x4C,0x02,0x88,0x02,0xAD,0x02,
           0xE0,0x03,0x03,0x03,0x34,0x03,0x43,0x03,0x58,0x03,0x6C,0x03,0x7E,0x03,
           0xAA,0x03,0xC2,0x03,0xC7,0x03,0xE6,0x00,0x00,0x00,0x1A,0x00,0x4F,0x00,
           0x65,0x00,0x7D,0x00,0x96,0x00,0xA5,0x00,0xB5,0x00,0xC5,0x00,0xF3,0x01,
           0x1C,0x01,0x57,0x01,0x89,0x01,0xD5,0x02,0x13,0x02,0x15,0x02,0x4C,0x02,
           0x88,0x02,0xAD,0x02,0xE0,0x03,0x03,0x03,0x34,0x03,0x43,0x03,0x58,0x03,
           0x6C,0x03,0x7E,0x03,0xAA,0x03,0xC2,0x03,0xC7,0x03,0xE6,0x01,0x01}},

	{0x11,1,{0x00}},
	{REGFLAG_DELAY,120,{}},	
	{0x29,1,{0x00}},
	{REGFLAG_DELAY,20,{}},	
          {0x35,1, {0x00} },
           
{REGFLAG_END_OF_TABLE, 0x00, {}}
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
    params->dsi.lcm_ext_te_enable			= 1;
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
	params->dsi.vertical_backporch					= 24; //18;	//14;
	params->dsi.vertical_frontporch					= 32; //20;	//16;
	params->dsi.vertical_active_line				= FRAME_HEIGHT; 

	params->dsi.horizontal_sync_active				= 20;	//2;
	params->dsi.horizontal_backporch				= 60;//120;	//60;	//42;
	params->dsi.horizontal_frontporch				= 80;//100;	//60;	//44;
	params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

// Bit rate calculation
//1 Every lane speed
//params->dsi.pll_div1=0;		// div1=0,1,2,3;div1_real=1,2,4,4 ----0: 546Mbps  1:273Mbps
//params->dsi.pll_div2=0;		// div2=0,1,2,3;div1_real=1,2,4,4	
//params->dsi.fbk_div =0x12;    // fref=26MHz, fvco=fref*(fbk_div+1)*2/(div1_real*div2_real)	

// zhangxiaofei add for test
	
	params->dsi.PLL_CLOCK = 440;//208;
	params->dsi.cont_clock=1;
	params->dsi.ssc_disable			= 1;
	params->dsi.ssc_range			= 4;
	
	params->dsi.clk_lp_per_line_enable = 0;
#if 0
	
	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 0;
	
	params->dsi.lcm_esd_check_table[0].cmd          = 0x0a;
	params->dsi.lcm_esd_check_table[0].count        = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9c;
        params->dsi.lcm_esd_check_table[1].cmd          = 0xe5;
	params->dsi.lcm_esd_check_table[1].count        = 4;
	params->dsi.lcm_esd_check_table[1].para_list[0] = 0x36;
	params->dsi.lcm_esd_check_table[1].para_list[1] = 0x36;
	params->dsi.lcm_esd_check_table[1].para_list[2] = 0xa1;
	params->dsi.lcm_esd_check_table[1].para_list[3] = 0xf6;
	params->dsi.lcm_esd_check_table[2].cmd          = 0x0b;
	params->dsi.lcm_esd_check_table[2].count        = 1;
	params->dsi.lcm_esd_check_table[2].para_list[0] = 0x00;  
#endif 
}

static void lcm_init(void)
{
    SET_RESET_PIN(1);
    MDELAY(10);
    SET_RESET_PIN(0);
    MDELAY(10);
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

/*
static unsigned int lcm_compare_id(void)
{
unsigned int id=0,id1=0;
	unsigned char buffer[2];
    unsigned int array[16]; 

        //Do reset here
        SET_RESET_PIN(1);
        MDELAY(10);
        SET_RESET_PIN(0);
    MDELAY(50);
        SET_RESET_PIN(1);
    MDELAY(120);
       
        array[0]=0x00043902;
        array[1]=0x018198FF;
        dsi_set_cmdq(array, 3, 1);
        MDELAY(10);
        array[0]=0x00023700;
        dsi_set_cmdq(array, 1, 1);
	MDELAY(10);
    
        read_reg_v2(0x00, buffer,1);
	id  =  buffer[0];
        read_reg_v2(0x01, buffer,1);
	id1 =  buffer[1];
        read_reg_v2(0x02, buffer,1);
	id1 =  buffer[0];
       // id = (id_midd &lt;&lt; 8) | id_low;

#ifdef BUILD_LK
	//printk("FL11281 id = 0x%08x id1=%x \n",  id,id1);
#else
	//printk("FL11281 id = 0x%08x  id1=%x \n",id,id1);
#endif
	//return 1;	
	return (LCM_ID == id1)?1:0;//id1

}
*/

static unsigned int lcm_compare_id(void)
{
 
     char id_high=0;
       unsigned int id = 0; 
        char id_low=0;
	unsigned char buffer[2];
	unsigned int array[16];

 
        SET_RESET_PIN(1);
        MDELAY(10);
        SET_RESET_PIN(0);
        MDELAY(10);       
        SET_RESET_PIN(1);
        MDELAY(50);  

	array[0] = 0x00023700;	// read id return two byte,version and id
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0xBF, buffer, 2);   //get the manufacturer id,get in spec
	id_high = buffer[0];		//we only need ID
	id_low = buffer[1];
	id =(id_high << 8) | id_low;  
	
#ifdef BUILD_LK
	printf("%s, LK ili9885 txd auo debug: ili9885 0x%08x\n", __func__, id);
#else
	printk("%s, kernel debug:id = 0x%08x\n", __func__, id);
#endif
    if(id == LCM_ID)
    	return 1;
    else
        return 0;

}

LCM_DRIVER hct_ili9885a_dsi_vdo_fhd_auo_50_dzx = 
{
    .name           = "hct_ili9885a_dsi_vdo_fhd_auo_50_dzx",
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

