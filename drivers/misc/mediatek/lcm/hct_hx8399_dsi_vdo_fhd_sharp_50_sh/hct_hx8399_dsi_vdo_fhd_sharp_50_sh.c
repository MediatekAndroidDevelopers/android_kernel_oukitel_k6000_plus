
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
//	#include <mach/mt_gpio.h>
#endif
// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  										(1080)
#define FRAME_HEIGHT 										(1920)
#define LCM_ID                      								(0x8399)
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
struct LCM_setting_table {
	unsigned char cmd;
	unsigned char count;
	unsigned char para_list[64];
};


static struct LCM_setting_table lcm_initialization_setting[] = {
{0xB9,3,{0xFF,0x83,0x99}}, 
{0xB1,15,{0x02,0x04,0x6C,0x8C,0x01,0x32,0x33,0x11,0x11,0x53,0x48,0x56,0x73,0x02,0x02}},
{0xB2,15,{0x00,0x80,0x80,0xAE,0x05,0x07,0x5A,0x11,0x10,0x10,0x00,0x1F,0x70,0x03,0xC4}},   
{0xB4,44,{0x00,0xFF,0x00,0xAC,0x00,0x9C,0x00,0x00,0x08,0x00,0x00,0x02,0x00,0x24,0x02,0x07,0x09,0x21,0x03,0x00,0x00,0x04,0xA4,0x88,0x00,0xAC,0x00,0x9C,0x00,0x00,0x08,0x00,0x00,0x02,0x00,0x24,0x02,0x07,0x09,0x00,0x00,0x04,0xA4,0x81}},              
{0xD3,33,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x32,0x10,0x05,0x00,0x05,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x07,0x07,0x03,0x00,0x00,0x00,0x05,0x40}},          
{0xD5,32,{0x00,0x00,0x00,0x01,0x02,0x03,0x00,0x00,0x00,0x00,0x19,0x00,0x18,0x18,0x20,0x21,0x00,0x18,0x00,0x19,0x00,0x00,0x00,0x00,0x00,0x00,0x31,0x31,0x30,0x30,0x2F,0x2F}},
{0xD6,32,{0x40,0x40,0x03,0x02,0x01,0x00,0x40,0x40,0x40,0x40,0x18,0x40,0x19,0x18,0x21,0x20,0x40,0x18,0x40,0x19,0x40,0x40,0x40,0x40,0x40,0x40,0x31,0x31,0x30,0x30,0x2F,0x2F}},     
{0xD8,16,{0x28,0x2A,0x00,0x2A,0x28,0x0E,0xB0,0x2A,0x28,0x2A,0x00,0x2A,0x28,0x0A,0xA0,0x2A}},         
{0xBD,1,{0x01}},    
{0xD8,16,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x28,0x2A,0x00,0x2A,0x28,0x0A,0xE0,0x2A}},        
{0xBD,1,{0x02}},
{0xD8,8,{0x28,0x2A,0x00,0x2A,0x28,0x0A,0xE0,0x2A}}, 
{0xBD,1,{0x00}},
{0xE0,54,{0x01,0x2B,0x38,0x33,0x6B,0x71,0x7A,0x71,0x7C,0x88,0x8C,0x8E,0x92,0x93,0x9B,0x9E,0x9C,0xA8,0xA8,0xAE,0x9B,0xA7,0xAA,0x54,0x50,0x5D,0x69,0x01,0x2B,0x38,0x32,0x6A,0x71,0x7A,0x6D,0x7C,0x88,0x8C,0x8E,0x93,0x94,0x9B,0x9E,0x9E,0xA8,0xA7,0xAE,0x9E,0xA9,0xAB,0x56,0x52,0x5E,0x69}},
{0xD2,1,{0x22}},
{0xB6,3,{0x7D,0x7D,0x00}},
{0x11,1,{0x00}},  
{0x36,1,{0x02}},  
{REGFLAG_DELAY,200,{}},
{0x29,1,{0x00}},//Display ON 
{REGFLAG_DELAY,200,{}},

// Setting ending by predefined flag
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};


static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
	// Display off sequence
	{0x28, 1, {0x00}},
    {REGFLAG_DELAY, 150, {}},

	// Sleep Mode On
	{0x10, 1, {0x00}},
	{REGFLAG_DELAY, 150, {}},

	{REGFLAG_END_OF_TABLE, 0x00, {}}
};
static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
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
			dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
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
#if (LCM_DSI_CMD_MODE)
	params->dbi.te_mode 				= LCM_DBI_TE_MODE_VSYNC_ONLY;
	params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;
#endif

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode   = CMD_MODE;
#else
    params->dsi.mode = BURST_VDO_MODE;//SYNC_EVENT_VDO_MODE; SYNC_EVENT_VDO_MODE;
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
		
	params->dsi.vertical_sync_active				= 5;
	params->dsi.vertical_backporch					= 16;
	params->dsi.vertical_frontporch					= 16;
	params->dsi.vertical_active_line				= FRAME_HEIGHT; 

	params->dsi.horizontal_sync_active				= 8;
	params->dsi.horizontal_backporch				= 40;
	params->dsi.horizontal_frontporch				= 40;
	params->dsi.horizontal_active_pixel				= FRAME_WIDTH;
   // params->dsi.TA_GO =5;
	//params->dsi.compatibility_for_nvk = 1;

	// Bit rate calculation

	params->dsi.PLL_CLOCK=470;
#if 0
//        params->dsi.ssc_disable=1;
    	params->dsi.cont_clock=0; //1;
params->dsi.clk_lp_per_line_enable=1;

params->dsi.esd_check_enable=1;
params->dsi.customization_esd_check_enable=1;
//params->dsi.noncont_clock=1;
params->dsi.lcm_esd_check_table[0].cmd=0x0a;
params->dsi.lcm_esd_check_table[0].count=1;
params->dsi.lcm_esd_check_table[0].para_list[0]=0x9c;
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

    
	push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);	//wqtao. enable
	SET_RESET_PIN(0);
	MDELAY(30);

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
extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int* rawdata);
  #define AUX_IN0_LCD_ID  12
  #define ADC_MIN_VALUE   0x800
static unsigned int lcm_compare_id(void)
{
	unsigned int id=0;
	unsigned char buffer[2];
	char id0=0;
	char id1=0;
	unsigned int array[16];

         int adcdata[4] = {0};
         int rawdata = 0;
         int ret = 0;

          ret = IMM_GetOneChannelValue(AUX_IN0_LCD_ID, adcdata, &rawdata);
 #if defined(BUILD_LK)
         printf("hct_8399-sh adc = %x adcdata= %x %x, ret=%d, ADC_MIN_VALUE=%x\r\n",rawdata, adcdata[0], adcdata[1],ret, ADC_MIN_VALUE);
  #else
          printk("hct_8399-sh adc = %x adcdata= %x %x, ret=%d, ADC_MIN_VALUE=%x\r\n",rawdata, adcdata[0], adcdata[1],ret, ADC_MIN_VALUE);
  #endif
    rawdata=0;
	if(1)//((rawdata<0x100)) //0x63d
	{

		SET_RESET_PIN(1);
		MDELAY(1);
		SET_RESET_PIN(0);
		MDELAY(1);
		SET_RESET_PIN(1);
		MDELAY(120);//Must over 6 ms

		array[0]=0x00043902;
		array[1]=0x9983FFB9;// page enable
		dsi_set_cmdq(array, 2, 1);
		MDELAY(10);


		array[0] = 0x00013700;// return byte number
		dsi_set_cmdq(array, 1, 1);
		MDELAY(10);
		read_reg_v2(0xDA, buffer, 1);
		id0 = buffer[0]; 

		array[0] = 0x00013700;// return byte number
		dsi_set_cmdq(array, 1, 1);
		MDELAY(10);
		read_reg_v2(0xDB, buffer, 1);
		id1 = buffer[0]; 
	
		id = id0<<8 | id1;
	#ifdef BUILD_LK
		printf("[hx8399]%s,  hsd id = 0x%x,0x%x,0x%x\n", __func__, id,id0,id1);
	#else
		printk("[hx8399]%s,  hsd id = 0x%x,0x%x,0x%x\n", __func__, id,id0,id1);
	#endif

	    return (LCM_ID == id)?1:0;
	}
	else
		return 0;
}




LCM_DRIVER hct_hx8399_dsi_vdo_fhd_sharp_50_sh= 
{
    .name			= "hct_hx8399_dsi_vdo_fhd_sharp_50_sh",
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

