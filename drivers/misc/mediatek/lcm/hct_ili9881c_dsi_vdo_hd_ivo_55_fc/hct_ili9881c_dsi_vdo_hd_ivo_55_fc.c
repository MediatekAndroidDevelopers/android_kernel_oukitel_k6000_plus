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
{0xFF,03,{0x98,0x81,0x03}},
//GIP_1
{0x01,01,{0x00}},
{0x02,01,{0x00}},
{0x03,01,{0x73}},
{0x04,01,{0x73}},
{0x05,01,{0x00}},
{0x06,01,{0x06}},
{0x07,01,{0x02}},
{0x08,01,{0x00}},
{0x09,01,{0x01}},
{0x0a,01,{0x01}},
{0x0b,01,{0x01}},
{0x0c,01,{0x01}},
{0x0d,01,{0x01}},
{0x0e,01,{0x01}},
{0x0f,01,{0x32}},
{0x10,01,{0x32}},
{0x11,01,{0x00}},
{0x12,01,{0x00}},
{0x13,01,{0x01}},
{0x14,01,{0x00}},
{0x15,01,{0x00}},
{0x16,01,{0x00}},
{0x17,01,{0x00}},
{0x18,01,{0x00}},
{0x19,01,{0x00}},
{0x1a,01,{0x00}},
{0x1b,01,{0x00}},
{0x1c,01,{0x00}},
{0x1d,01,{0x00}},
{0x1e,01,{0xC0}},
{0x1f,01,{0x80}},
{0x20,01,{0x03}},
{0x21,01,{0x04}},
{0x22,01,{0x00}},
{0x23,01,{0x00}},
{0x24,01,{0x00}},
{0x25,01,{0x00}},
{0x26,01,{0x00}},
{0x27,01,{0x00}},
{0x28,01,{0x33}},
{0x29,01,{0x02}},
{0x2a,01,{0x00}},
{0x2b,01,{0x00}},
{0x2c,01,{0x00}},
{0x2d,01,{0x00}},
{0x2e,01,{0x00}},
{0x2f,01,{0x00}},
{0x30,01,{0x00}},
{0x31,01,{0x00}},
{0x32,01,{0x00}},
{0x33,01,{0x00}},
{0x34,01,{0x03}},
{0x35,01,{0x00}},
{0x36,01,{0x03}},
{0x37,01,{0x00}},
{0x38,01,{0x00}},
{0x39,01,{0x00}},
{0x3a,01,{0x00}},
{0x3b,01,{0x00}},
{0x3c,01,{0x00}},
{0x3d,01,{0x00}},
{0x3e,01,{0x00}},
{0x3f,01,{0x00}},
{0x40,01,{0x00}},
{0x41,01,{0x00}},
{0x42,01,{0x00}},
{0x43,01,{0x00}},
{0x44,01,{0x00}},

//GIP_2
{0x50,01,{0x01}},
{0x51,01,{0x23}},
{0x52,01,{0x45}},
{0x53,01,{0x67}},
{0x54,01,{0x89}},
{0x55,01,{0xab}},
{0x56,01,{0x01}},
{0x57,01,{0x23}},
{0x58,01,{0x45}},
{0x59,01,{0x67}},
{0x5a,01,{0x89}},
{0x5b,01,{0xab}},
{0x5c,01,{0xcd}},
{0x5d,01,{0xef}},

//GIP_3
{0x5e,01,{0x10}},
{0x5f,01,{0x09}},
{0x60,01,{0x08}},
{0x61,01,{0x0F}},
{0x62,01,{0x0E}},
{0x63,01,{0x0D}},
{0x64,01,{0x0C}},
{0x65,01,{0x02}},
{0x66,01,{0x02}},
{0x67,01,{0x02}},
{0x68,01,{0x02}},
{0x69,01,{0x02}},
{0x6a,01,{0x02}},
{0x6b,01,{0x02}},
{0x6c,01,{0x02}},
{0x6d,01,{0x02}},
{0x6e,01,{0x02}},
{0x6f,01,{0x02}},
{0x70,01,{0x02}},
{0x71,01,{0x06}},
{0x72,01,{0x07}},
{0x73,01,{0x02}},
{0x74,01,{0x02}},
{0x75,01,{0x06}},
{0x76,01,{0x07}},
{0x77,01,{0x0E}},
{0x78,01,{0x0F}},
{0x79,01,{0x0C}},
{0x7a,01,{0x0D}},
{0x7b,01,{0x02}},
{0x7c,01,{0x02}},
{0x7d,01,{0x02}},
{0x7e,01,{0x02}},
{0x7f,01,{0x02}},
{0x80,01,{0x02}},
{0x81,01,{0x02}},
{0x82,01,{0x02}},
{0x83,01,{0x02}},
{0x84,01,{0x02}},
{0x85,01,{0x02}},
{0x86,01,{0x02}},
{0x87,01,{0x09}},
{0x88,01,{0x08}},
{0x89,01,{0x02}},
{0x8A,01,{0x02}},

//CMD_Page 4
{0xFF,03,{0x98,0x81,0x04}},
{0x6C,01,{0x15}},             //Set VCORE voltage =1.5V
{0x6E,01,{0x2b}},             //di_pwr_reg=0 for power mode 2A //VGH clamp 15V
{0x6F,01,{0x33}},             //reg vcl + pumping ratio VGH=3.0x VGL=-2.5x
{0x3A,01,{0xA4}},
{0x8D,01,{0x14}},             //VGL clamp -11V
{0x87,01,{0xBA}},
{0x26,01,{0x76}},
{0xB2,01,{0xD1}},

//CMD_Page 1
{0xFF,03,{0x98,0x81,0x01}},
{0x22,01,{0x0A}},             //BGR,0x SS
{0x31,01,{0x00}},	      //Cloumn inversion
{0x53,01,{0x40}},	      //VCOM1
{0x55,01,{0x40}},	      //VCOM2
{0x50,01,{0x9d}},	      //VREG1OUT=4.8V
{0x51,01,{0x98}},	      //VREG2OUT=-4.8V
{0x60,01,{0x14}},             //SDT

{0xA0,01,{0x08}},
{0xA1,01,{0x15}},
{0xA2,01,{0x20}},
{0xA3,01,{0x1f}},
{0xA4,01,{0x21}},
{0xA5,01,{0x31}},
{0xA6,01,{0x26}},
{0xA7,01,{0x24}},
{0xA8,01,{0x72}},
{0xA9,01,{0x1e}},
{0xAA,01,{0x2a}},
{0xAB,01,{0x5f}},
{0xAC,01,{0x1b}},
{0xAD,01,{0x19}},
{0xAE,01,{0x4d}},
{0xAF,01,{0x26}},
{0xB0,01,{0x27}},
{0xB1,01,{0x56}},
{0xB2,01,{0x69}},
{0xB3,01,{0x39}},

{0xC0,01,{0x08}},
{0xC1,01,{0x13}},
{0xC2,01,{0x1e}},
{0xC3,01,{0x03}},
{0xC4,01,{0x04}},
{0xC5,01,{0x1a}},
{0xC6,01,{0x10}},
{0xC7,01,{0x16}},
{0xC8,01,{0x6a}},
{0xC9,01,{0x18}},
{0xCA,01,{0x23}},
{0xCB,01,{0x60}},
{0xCC,01,{0x15}},
{0xCD,01,{0x14}},
{0xCE,01,{0x4a}},
{0xCF,01,{0x1c}},
{0xD0,01,{0x27}},
{0xD1,01,{0x59}},
{0xD2,01,{0x6b}},
{0xD3,01,{0x39}},

//CMD_Page 0
{0xFF,03,{0x98,0x81,0x00}},
{0x11,01,{0x00}},	//sleep out
{REGFLAG_DELAY, 150, {0x00}},
{0x29,01,{0x00}},	//display on
{0x35,01,{0x00}},	//TE on
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
                //MDELAY(1);
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
        params->physical_width = 68;
     	params->physical_height = 121;
	params->dsi.vertical_sync_active				= 8; //8;	//2;
	params->dsi.vertical_backporch					= 20; //18;	//14;
	params->dsi.vertical_frontporch					= 16; //20;	//16;
	params->dsi.vertical_active_line				= FRAME_HEIGHT; 

	params->dsi.horizontal_sync_active				= 60;	//2;
	params->dsi.horizontal_backporch				= 90;//120;	//60;	//42;
	params->dsi.horizontal_frontporch				= 76;//100;	//60;	//44;
	params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

// Bit rate calculation
//1 Every lane speed
//params->dsi.pll_div1=0;		// div1=0,1,2,3;div1_real=1,2,4,4 ----0: 546Mbps  1:273Mbps
//params->dsi.pll_div2=0;		// div2=0,1,2,3;div1_real=1,2,4,4	
//params->dsi.fbk_div =0x12;    // fref=26MHz, fvco=fref*(fbk_div+1)*2/(div1_real*div2_real)	

// zhangxiaofei add for test
params->dsi.PLL_CLOCK = 238;//233;//230;//208;	
    params->dsi.HS_TRAIL = 20;
    params->dsi.ssc_disable = 1;

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
}


    //for LGE backlight IC mapping table


    // Refresh value of backlight level.


static void lcm_resume(void)
{
   //  lcm_init();
  unsigned int data_array[16];


    data_array[0]=0x00110500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(120);
    data_array[0]=0x00290500;

    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(20);
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

extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int* rawdata);
  #define AUX_IN0_LCD_ID  12
  #define ADC_MIN_VALUE   0x800
  //0                             
//47 / 147 * 4096 = 0x51D               0.575V
  //100/ 147 * 4096 = 0xAE2               1.22V
static unsigned int lcm_compare_id(void)
{
	int   array[4];
	char  buffer[5];
	unsigned int id_high;
	unsigned int id_low;
	unsigned int id=0;
	         int adcdata[4] = {0};
         int rawdata = 0;
          int ret = 0;

          ret = IMM_GetOneChannelValue(AUX_IN0_LCD_ID, adcdata, &rawdata);
 #if defined(BUILD_LK)
         printf("hct_ili9881c_dsi_vdo_hd_ivo_55_fc adc = %x adcdata= %x %x, ret=%d, ADC_MIN_VALUE=%x\r\n",rawdata, adcdata[0], adcdata[1],ret, ADC_MIN_VALUE);
  #else
          printk("hct_ili9881c_dsi_vdo_hd_ivo_55_fc adc = %x adcdata= %x %x, ret=%d, ADC_MIN_VALUE=%x\r\n",rawdata, adcdata[0], adcdata[1],ret, ADC_MIN_VALUE);
  #endif

	if(rawdata>0x630 ) //0x63d
	{


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
	MDELAY(10);
    
        read_reg_v2(0x00, buffer,1);
	id_high = buffer[0]; ///////////////////////0x98
        read_reg_v2(0x01, buffer,1);
	id_low = buffer[0]; ///////////////////////0x06
       // id = (id_midd &lt;&lt; 8) | id_low;
	id = (id_high << 8) | id_low;

		#if defined(BUILD_LK)
		printf("ILI9881 %s id_high = 0x%04x, id_low = 0x%04x\n,id=0x%x\n", __func__, id_high, id_low,id);
#else
		printk("ILI9881 %s id_high = 0x%04x, id_low = 0x%04x\n,id=0x%x\n", __func__, id_high, id_low,id);
#endif
	//return 1;	
	 return (LCM_ID == id)?1:0;
	 }
	 else
	 return 0;

}

// ---------------------------------------------------------------------------
//  Get LCM Driver Hooks
// ---------------------------------------------------------------------------
LCM_DRIVER hct_ili9881c_dsi_vdo_hd_ivo_55_fc = 
{
    .name           = "hct_ili9881c_dsi_vdo_hd_ivo_55_fc",
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

