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
#define LCM_ID                       (0x9881)

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
    unsigned char cmd;
    unsigned char count;
    unsigned char para_list[64];
};


static struct LCM_setting_table lcm_initialization_setting[] = {
{0xFF,3,{0x98,0x81,0x03}},
{0x01,1,{0x00}},
{0x02,1,{0x00}},
{0x03,1,{0x72}},
{0x04,1,{0x00}},
{0x05,1,{0x00}},
{0x06,1,{0x09}},
{0x07,1,{0x00}},
{0x08,1,{0x00}},
{0x09,1,{0x00}},
{0x0A,1,{0x00}},
{0x0B,1,{0x00}},
{0x0C,1,{0x00}},
{0x0D,1,{0x00}},
{0x0E,1,{0x00}},
{0x0F,1,{0x00}}, 
{0x10,1,{0x00}},  
{0x11,1,{0x00}},
{0x12,1,{0x00}},
{0x13,1,{0x00}},
{0x14,1,{0x00}},
{0x15,1,{0x00}},
{0x16,1,{0x00}}, 
{0x17,1,{0x00}},
{0x18,1,{0x00}},
{0x19,1,{0x00}},
{0x1A,1,{0x00}},
{0x1B,1,{0x00}},
{0x1C,1,{0x00}},
{0x1D,1,{0x00}},
{0x1E,1,{0x40}},
{0x1F,1,{0x80}},
{0x20,1,{0x05}},
{0x21,1,{0x02}},
{0x22,1,{0x00}},
{0x23,1,{0x00}},
{0x24,1,{0x00}},
{0x25,1,{0x00}},
{0x26,1,{0x00}},
{0x27,1,{0x00}},
{0x28,1,{0x33}},
{0x29,1,{0x02}},
{0x2A,1,{0x00}},
{0x2B,1,{0x00}},
{0x2C,1,{0x00}},
{0x2D,1,{0x00}},
{0x2E,1,{0x00}},
{0x2F,1,{0x00}},
{0x30,1,{0x00}},
{0x31,1,{0x00}},
{0x32,1,{0x00}},
{0x33,1,{0x00}},
{0x34,1,{0x04}},
{0x35,1,{0x00}},
{0x36,1,{0x00}},
{0x37,1,{0x00}},
{0x38,1,{0x3C}},
{0x39,1,{0x00}},
{0x3A,1,{0x00}}, 
{0x3B,1,{0x00}},
{0x3C,1,{0x00}},
{0x3D,1,{0x00}},
{0x3E,1,{0x00}},
{0x3F,1,{0x00}},
{0x40,1,{0x00}},
{0x41,1,{0x00}},
{0x42,1,{0x00}},
{0x43,1,{0x01}},
{0x44,1,{0x00}},
//GIP_2
{0x50,1,{0x10}},
{0x51,1,{0x32}},
{0x52,1,{0x54}},
{0x53,1,{0x76}},
{0x54,1,{0x98}},
{0x55,1,{0xba}},
{0x56,1,{0x10}},
{0x57,1,{0x32}},
{0x58,1,{0x54}},
{0x59,1,{0x76}},
{0x5A,1,{0x98}},
{0x5B,1,{0xba}},
{0x5C,1,{0xdc}},
{0x5D,1,{0xfe}},
//GIP_3
{0x5E,1,{0x00}},
{0x5F,1,{0x0E}},
{0x60,1,{0x0F}},
{0x61,1,{0x0C}},
{0x62,1,{0x0D}},
{0x63,1,{0x06}},
{0x64,1,{0x07}},
{0x65,1,{0x02}},
{0x66,1,{0x02}},
{0x67,1,{0x02}},
{0x68,1,{0x02}},
{0x69,1,{0x01}},
{0x6A,1,{0x00}},
{0x6B,1,{0x02}},
{0x6C,1,{0x15}},
{0x6D,1,{0x14}},
{0x6E,1,{0x02}},
{0x6F,1,{0x02}},
{0x70,1,{0x02}},
{0x71,1,{0x02}},
{0x72,1,{0x02}},
{0x73,1,{0x02}},
{0x74,1,{0x02}},
{0x75,1,{0x0E}},
{0x76,1,{0x0F}},
{0x77,1,{0x0C}},
{0x78,1,{0x0D}},
{0x79,1,{0x06}},
{0x7A,1,{0x07}},
{0x7B,1,{0x02}},
{0x7C,1,{0x02}},
{0x7D,1,{0x02}},
{0x7E,1,{0x02}},
{0x7F,1,{0x01}},
{0x80,1,{0x00}},
{0x81,1,{0x02}},
{0x82,1,{0x14}},
{0x83,1,{0x15}},
{0x84,1,{0x02}},
{0x85,1,{0x02}},
{0x86,1,{0x02}},
{0x87,1,{0x02}},
{0x88,1,{0x02}},
{0x89,1,{0x02}}, 
{0x8A,1,{0x02}},
//CMD_Page 4
{0xFF,3,{0x98,0x81,0x04}},
{0x6C,1,{0x15}},        //Set VCORE voltage =1.5V
{0x6E,1,{0x2A}},        //di_pwr_reg=0 for power mode 2A //VGH clamp 15V
{0x6F,1,{0x33}},       // reg vcl + pumping ratio VGH=3x VGL=-2x
{0x3A,1,{0x94}},                //POWER SAVING
{0x8D,1,{0x14}},               //VGL clamp -10V
{0x87,1,{0xBA}},               //ESD    
{0x88,1,{0x0B}},
{0x3C,1,{0x81}},             
{0x26,1,{0x76}},            
{0xB2,1,{0xD1}},
{0xB5,1,{0x06}},
//-----------------------------------------------------------------------------------------S
//CMD_Page 
{0xFF,3,{0x98,0x81,0x01}},
{0x22,1,{0x0A}},		//BGR, SS
{0x31,1,{0x00}},		//column inversion
{0x40,1,{0x33}},
{0x53,1,{0x77}},		//VCOM1
{0x55,1,{0x7A}},		//VCOM2
{0x50,1,{0x95}},		//VREG1OUT=4.5V
{0x51,1,{0x90}},		//VREG2OUT=-4.48
{0x60,1,{0x21}},             //SDT

{0xA0,1,{0x08}},		   //VP255	Gamma P
{0xA1,1,{0x11}},               //VP251        
{0xA2,1,{0x1A}},               //VP247        
{0xA3,1,{0x0E}},               //VP243        
{0xA4,1,{0x0F}},               //VP239        
{0xA5,1,{0x21}},               //VP231         
{0xA6,1,{0x15}},               //VP219        
{0xA7,1,{0x18}},               //VP203        
{0xA8,1,{0x61}},               //VP175        
{0xA9,1,{0x1C}},               //VP144        
{0xAA,1,{0x28}},               //VP111        
{0xAB,1,{0x5B}},               //VP80         
{0xAC,1,{0x1E}},               //VP52         
{0xAD,1,{0x24}},               //VP36         
{0xAE,1,{0x42}},               //VP24         
{0xAF,1,{0x37}},               //VP16         
{0xB0,1,{0x3F}},               //VP12         
{0xB1,1,{0x46}},               //VP8          
{0xB2,1,{0x47}},               //VP4          
{0xB3,1,{0x39}},               //VP0          
                                               
{0xC0,1,{0x08}},		//VN255 GAMMA N
{0xC1,1,{0x11}},               //VN251        
{0xC2,1,{0x1A}},               //VN247        
{0xC3,1,{0x0E}},               //VN243        
{0xC4,1,{0x0F}},               //VN239        
{0xC5,1,{0x21}},               //VN231        
{0xC6,1,{0x15}},               //VN219        
{0xC7,1,{0x18}},               //VN203        
{0xC8,1,{0x61}},               //VN175        
{0xC9,1,{0x1C}},               //VN144        
{0xCA,1,{0x28}},               //VN111        
{0xCB,1,{0x5B}},               //VN80         
{0xCC,1,{0x1E}},               //VN52         
{0xCD,1,{0x24}},               //VN36         
{0xCE,1,{0x42}},               //VN24         
{0xCF,1,{0x37}},               //VN16         
{0xD0,1,{0x3F}},               //VN12         
{0xD1,1,{0x46}},               //VN8          
{0xD2,1,{0x47}},               //VN4          
{0xD3,1,{0x39}},               //VN0

//CMD_Page 0
{0xFF,3,{0x98,0x81,0x00}},
//{0x36,1,{0x03}},         
                         
{0x11,1,{0x00}},        
{REGFLAG_DELAY, 120, {}},
{0x29,1,{0x00}},	       
{REGFLAG_DELAY, 20, {}}, 
//{REGFLAG_END_OF_TABLE, 0x00, {}}



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
        params->physical_width = 62;
     	params->physical_height = 110;
	params->dsi.vertical_sync_active				=2;//8; //8;	//2;
	params->dsi.vertical_backporch					= 16;//24; //18;	//14;
	params->dsi.vertical_frontporch					= 18;//16; //20;	//16;
	params->dsi.vertical_active_line				= FRAME_HEIGHT; 

	params->dsi.horizontal_sync_active				= 8;//40;	//2;
	params->dsi.horizontal_backporch				= 60;//100;//120;	//60;	//42;
	params->dsi.horizontal_frontporch				= 60;//70;//100;	//60;	//44;
	params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

// Bit rate calculation
//1 Every lane speed
//params->dsi.pll_div1=0;		// div1=0,1,2,3;div1_real=1,2,4,4 ----0: 546Mbps  1:273Mbps
//params->dsi.pll_div2=0;		// div2=0,1,2,3;div1_real=1,2,4,4	
//params->dsi.fbk_div =0x12;    // fref=26MHz, fvco=fref*(fbk_div+1)*2/(div1_real*div2_real)	

// zhangxiaofei add for test
params->dsi.PLL_CLOCK = 212;//208;	

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
    MDELAY(10);
    SET_RESET_PIN(1);
    MDELAY(120);

    push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
/*
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
    lcm_util.set_gpio_out(GPIO_LCM_LDO_2V8_EN_PIN, GPIO_OUT_ONE);
#else   
    mt_dsi_pinctrl_set(LCM_POWER_DP_NO, 1);
#endif
    MDELAY(10);
#ifdef GPIO_LCM_LDO_1V8_EN_PIN

    lcm_util.set_gpio_mode(GPIO_LCM_LDO_1V8_EN_PIN, GPIO_MODE_00);
    lcm_util.set_gpio_dir(GPIO_LCM_LDO_1V8_EN_PIN, GPIO_DIR_OUT);

    lcm_util.set_gpio_out(GPIO_LCM_LDO_1V8_EN_PIN, GPIO_OUT_ONE);  //shm
#else   

    mt_dsi_pinctrl_set(LCM_POWER_DM_NO, 1);
#endif
    MDELAY(10);
   
#ifdef GPIO_LCM_RST
    lcm_util.set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ONE);  //shm
#else    
    SET_RESET_PIN(1);
#endif
   
    MDELAY(20);



    push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
*/
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
/*
 	unsigned int data_array[16];

	data_array[0] = 0x00280500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(50);
	data_array[0] = 0x00100500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(50);
	

#ifdef GPIO_LCM_RST
    lcm_util.set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_00);
    lcm_util.set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
    lcm_util.set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ZERO);  //shm
#else    
    SET_RESET_PIN(0);
#endif
    MDELAY(10);

#ifdef GPIO_LCM_LDO_1V8_EN_PIN
//    lcm_util.set_gpio_mode(GPIO_LCM_LDO_1V8_EN_PIN, GPIO_MODE_00);
//   lcm_util.set_gpio_dir(GPIO_LCM_LDO_1V8_EN_PIN, GPIO_DIR_OUT);
//    lcm_util.set_gpio_out(GPIO_LCM_LDO_1V8_EN_PIN, GPIO_OUT_ZERO);  //shm
#else   
    mt_dsi_pinctrl_set(LCM_POWER_DM_NO, 0);
#endif
    MDELAY(5);
#ifdef GPIO_LCM_LDO_2V8_EN_PIN
    lcm_util.set_gpio_mode(GPIO_LCM_LDO_2V8_EN_PIN, GPIO_MODE_00);
    lcm_util.set_gpio_dir(GPIO_LCM_LDO_2V8_EN_PIN, GPIO_DIR_OUT);
    lcm_util.set_gpio_out(GPIO_LCM_LDO_2V8_EN_PIN, GPIO_OUT_ZERO);
#else   
    mt_dsi_pinctrl_set(LCM_POWER_DP_NO, 0);
#endif
    MDELAY(5);
*/
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


//extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int* rawdata);
//#define AUX_IN0_LCD_ID	12
//#define ADC_MIN_VALUE	0x471


static unsigned int lcm_compare_id(void)
{
	int   array[4];
		char  buffer[5];
		unsigned int id_high;
		unsigned int id_low;
		unsigned int id=0;

//int adcdata[4] = {0};
//	int rawdata = 0;
//	int ret = 0;
//	ret = IMM_GetOneChannelValue(AUX_IN0_LCD_ID, adcdata, &rawdata);

//if(rawdata < ADC_MIN_VALUE && (ret==0)){
       // Do reset here
        SET_RESET_PIN(1);
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
    
        read_reg_v2(0x00, buffer,1);
	id_high = buffer[0]; ///////////////////////0x98
        read_reg_v2(0x01, buffer,1);
	id_low = buffer[0]; ///////////////////////0x06
       // id = (id_midd &lt;&lt; 8) | id_low;
	id = (id_high << 8) | id_low;

	//printk("FL11281 id = 0x%08x  id1=%x \n",id,id1);

id = (id_high << 8) | id_low;	
	return (LCM_ID == id)?1:0;//id1
//}else
//	return 0;
}

// ---------------------------------------------------------------------------
//  Get LCM Driver Hooks
// ---------------------------------------------------------------------------
LCM_DRIVER hct_ili9881c_dsi_vdo_hd_hsd_60_hx = 
{
    .name           = "hct_ili9881c_dsi_vdo_hd_hsd_60_hx",
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

