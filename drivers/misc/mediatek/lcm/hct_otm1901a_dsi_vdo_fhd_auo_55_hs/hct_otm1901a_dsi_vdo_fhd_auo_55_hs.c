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

#ifdef BUILD_LK
#else
    #include <linux/string.h>
    #if defined(BUILD_UBOOT)
        #include <asm/arch/mt_gpio.h>
    #else
//	#include <mach/mt_gpio.h>
    #endif
#endif
#include "lcm_drv.h"

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  										(1080)
#define FRAME_HEIGHT 										(1920)

#define LCM_ID                      								(0x1901)

#define REGFLAG_DELAY             								(0XFE)
#define REGFLAG_END_OF_TABLE      								(0x100)	// END OF REGISTERS MARKER

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

#define SET_RESET_PIN(v)    									(lcm_util.set_reset_pin((v)))

#define UDELAY(n) 										(lcm_util.udelay(n))
#define MDELAY(n) 										(lcm_util.mdelay(n))

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


/*
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

*/
 struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};

static struct LCM_setting_table lcm_initialization_setting[] = {
{0x00,1,{0x00}},
{0xFF,4,{0x19,0x01,0x01,0x00}},//Enable command2

{0x00,1,{0x80}},
{0xFF,2,{0x19,0x01}},

{0x00,1,{0x00}},
{0x1C,1,{0x33}},//bypass mode
{0x00,1,{0xA0}},
{0xC1,1,{0xE8}},//Video mode sync
{0x00,1,{0xA7}},
{0xC1,1,{0x00}},
{0x00,1,{0x90}},
{0xC0,6,{0x00,0x2F,0x00,0x00,0x00,0x01}},

{0x00,1,{0xC0}},
{0xC0,6,{0x00,0x2F,0x00,0x00,0x00,0x01}},

{0x00,1,{0x9A}},
{0xC0,1,{0x1E}},
{0x00,1,{0xAC}},
{0xC0,1,{0x06}},
{0x00,1,{0xDC}},
{0xC0,1,{0x06}},
{0x00,1,{0xc2}},
{0xC5,1,{0x12}},
{0x00,1,{0x81}},
{0xA5,1,{0x06}},//Keep blue and PRC enable
{0x00,1,{0x82}},
{0xC4,1,{0xF0}},//Keep blue and PRC enable
{0x00,1,{0x92}},
{0xE9,1,{0x00}},
{0x00,1,{0x90}},
{0xF3,1,{0x01}},
{0x00,1,{0x93}},
{0xC5,1,{0x1E}},//VGH=9V
{0x00,1,{0x95}},
{0xC5,1,{0x32}},//VGL=-9V
{0x00,1,{0x97}},
{0xC5,1,{0x19}},//VGHO=8.5V
{0x00,1,{0x99}},
{0xC5,1,{0x2D}},//VGLO=-8.5V
{0x00,1,{0x9B}},
{0xC5,2,{0x44,0x40}},//charge pump 1 line (default)

{0x00,1,{0x00}},
{0xD9,2,{0x00,0xA4}},//VCOM

{0x00,1,{0x00}},
{0xD8,2,{0x1F,0x1F}},//GVDDP/GVDDN=4.4V

{0x00,1,{0xB3}},
{0xC0,1,{0xCC}},
{0x00,1,{0xBC}},
{0xC0,1,{0x00}},//Pixel base column inversion
{0x00,1,{0x84}},
{0xC4,1,{0x22}},
{0x00,1,{0x80}},
{0xC4,1,{0x38}},//Source=HiZ at Vblanking
{0x00,1,{0x94}},
{0xC1,1,{0x84}},
{0x00,1,{0x98}},
{0xC1,1,{0x74}},//SSC enabld
{0x00,1,{0xCD}},
{0xF5,1,{0x19}},
{0x00,1,{0xDB}},
{0xF5,1,{0x19}},
{0x00,1,{0xF5}},
{0xC1,1,{0x40}},
{0x00,1,{0xB9}},
{0xC0,1,{0x11}},
{0x00,1,{0x8D}},
{0xF5,1,{0x20}},//Power off setting
{0x00,1,{0x80}},//RTN
{0xC0,14,{0x00,0x86,0x00,0x0A,0x0A,0x00,0x86,0x0A,0x0A,0x00,0x86,0x00,0x0A,0x0A}},
{0x00,1,{0xF0}},
{0xC3,6,{0x00,0x00,0x00,0x00,0x00,0x80}},
{0x00,1,{0xA0}},// LTPS PCG & SELR/G/B Setting
{0xC0,7,{0x00,0x00,0x03,0x00,0x00,0x1E,0x06}},
{0x00,1,{0xD0}},//LTPS PCG & SELR/G/B Setting for Video Mode
{0xC0,7,{0x00,0x00,0x03,0x00,0x00,0x1E,0x06}},

{0x00,1,{0x90}},//LTPS VST Setting
{0xC2,4,{0x82,0x00,0x3D,0x40}},

{0x00,1,{0xB0}},//LTPS VEND Setting
{0xC2,8,{0x01,0x00,0x45,0x43,0x01,0x01,0x45,0x43}},

{0x00,1,{0x80}},// LTPS CKV
{0xC3,12,{0x84,0x04,0x01,0x00,0x02,0x87,0x83,0x04,0x01,0x00,0x02,0x87}},

{0x00,1,{0x80}},//PANEL_IF_SETTING: SIGIN_SEL
{0xCC,15,{0x09,0x0D,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x0E,0x28,0x28,0x28,0x28}},

{0x00,1,{0x90}},//PANEL_IF_SETTING: SIGIN_SEL
{0xCC,15,{0x0D,0x09,0x12,0x11,0x13,0x14,0x15,0x16,0x17,0x18,0x0E,0x28,0x28,0x28,0x28}},

{0x00,1,{0xA0}},//PANEL_IF_SETTING: SIGIN_SEL
{0xCC,15,{0x1D,0x1E,0x1F,0x19,0x1A,0x1B,0x1C,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27}},

{0x00,1,{0xB0}},//PANEL_IF_SETTING: SIGIN_SEL
{0xCC,8,{0x01,0x02,0x03,0x05,0x06,0x07,0x04,0x08}},

{0x00,1,{0xC0}},//PANEL_IF_SETTING: ENMODE
{0xCC,12,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x77}},

{0x00,1,{0xD0}},//PANEL_IF_SETTING: ENMODE
{0xCC,12,{0xFF,0x00,0x30,0xC0,0x0F,0x30,0x00,0x00,0x33,0x03,0x00,0x77}},

{0x00,1,{0xDE}},//PANEL_IF_SETTING: ENMODE
{0xCC,1,{0x00}},
{0x00,1,{0x80}},//PANEL_IF_SETTING: ENMODE
{0xCB,15,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x30,0x00,0x00,0x00,0x00}},

{0x00,1,{0x90}},//PANEL_IF_SETTING: ENMODE
{0xCB,15,{0x30,0x00,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},

{0x00,1,{0xA0}},//PANEL_IF_SETTING: ENMODE
{0xCB,15,{0x15,0x15,0x05,0xF5,0x01,0x01,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00}},

{0x00,1,{0xB0}},//PANEL_IF_SETTING: ENMODE
{0xCB,15,{0x00,0x01,0xFD,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},

{0x00,1,{0xC0}},//PANEL_IF_SETTING: ENMODE
{0xCB,8,{0x00,0x00,0x00,0x00,0x00,0x00,0x77,0x77}},

{0x00,1,{0xD0}},//PANEL_IF_SETTING: ENMODE
{0xCB,8,{0x00,0x00,0x00,0x00,0x00,0x00,0x77,0x77}},

{0x00,1,{0xE0}},//PANEL_IF_SETTING: ENMODE
{0xCB,8,{0x00,0x00,0x00,0x01,0x01,0x01,0x77,0x77}},

{0x00,1,{0xF0}},//PANEL_IF_SETTING: ENMODE
{0xCB,8,{0x11,0x11,0x11,0x11,0x11,0x11,0x77,0x77}},

{0x00,1,{0x80}},//PANEL_IF_SETTING: CGOUT_SEL
{0xCD,15,{0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x01,0x12,0x11,0x03,0x04,0x0B,0x17}},

{0x00,1,{0x90}},//PANEL_IF_SETTING: CGOUT_SEL
{0xCD,11,{0x3D,0x02,0x3D,0x25,0x25,0x25,0x1F,0x20,0x21,0x25,0x25}},

{0x00,1,{0xA0}},//PANEL_IF_SETTING: CGOUT_SEL
{0xCD,15,{0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x01,0x12,0x11,0x03,0x04,0x0B,0x17}},

{0x00,1,{0xB0}},//PANEL_IF_SETTING: CGOUT_SEL
{0xCD,11,{0x17,0x02,0x3D,0x25,0x25,0x25,0x1F,0x20,0x21,0x25,0x25}},

{0x00,1,{0x00}},
{0xE1,24,{0x1F,0x2E,0x33,0x3C,0x42,0x49,0x54,0x64,0x6C,0x7D,0x88,0x90,0x6B,0x67,0x63,0x54,0x43,0x33,0x27,0x21,0x18,0x00,0x00,0x00}},
{0x00,1,{0x00}},
{0xE2,24,{0x1F,0x2E,0x33,0x3C,0x42,0x49,0x54,0x64,0x6C,0x7D,0x88,0x90,0x6B,0x67,0x63,0x54,0x43,0x33,0x27,0x21,0x18,0x00,0x00,0x00}},
{0x00,1,{0x00}},
{0xE3,24,{0x37,0x3E,0x41,0x47,0x4D,0x52,0x5B,0x68,0x70,0x81,0x8A,0x91,0x6A,0x66,0x62,0x54,0x43,0x33,0x29,0x23,0x1B,0x13,0x0C,0x00}},
{0x00,1,{0x00}},
{0xE4,24,{0x37,0x3E,0x41,0x47,0x4D,0x52,0x5B,0x68,0x70,0x81,0x8A,0x91,0x6A,0x66,0x62,0x54,0x43,0x33,0x29,0x23,0x1B,0x13,0x0C,0x00}},
{0x00,1,{0x00}},
{0xE5,24,{0x1F,0x2A,0x2F,0x38,0x40,0x46,0x51,0x62,0x6B,0x7E,0x88,0x90,0x6B,0x67,0x64,0x56,0x45,0x35,0x2B,0x25,0x1E,0x0F,0x00,0x00}},
{0x00,1,{0x00}},
{0xE6,24,{0x1F,0x2A,0x2F,0x38,0x40,0x46,0x51,0x62,0x6B,0x7E,0x88,0x90,0x6B,0x67,0x64,0x56,0x45,0x35,0x2B,0x25,0x1E,0x0F,0x00,0x00}},

{0x00,1,{0xD4}},//LTPS Ground Period for Gate Precharge
{0xC3,4,{0x01,0x01,0x01,0x01}},

{0x00,1,{0xF2}},//Gate slew rate
{0xC1,3,{0x80,0x0F,0x0F}},

{0x00,1,{0xF7}},//LTPS CKH EQ Setting 
{0xC3,4,{0x03,0x1B,0x00,0x00}},

{0x00,1,{0x00}},//Disable command2
{0xFF,3,{0xFF,0xFF,0xFF}},
	{0x11,1,{0x00}},  
	{REGFLAG_DELAY,200,{}},
	{0x29,1,{0x00}},//Display ON 
	{REGFLAG_DELAY,200,{}},


// Setting ending by predefined flag
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

#if 0
static struct LCM_setting_table lcm_sleep_out_setting[] = {
    // Sleep Out
	{0x11, 1, {0x00}},
	{REGFLAG_DELAY, 150, {}},
	// Display ON
	//{0x2C, 1, {0x00}},
	//{0x13, 1, {0x00}},
	{0x29, 1, {0x00}},
	{REGFLAG_DELAY, 200, {}},
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

static struct LCM_setting_table lcm_compare_id_setting[] = {
	// Display off sequence
    {0xFF,4,{0x19,0x01,0x01,0x00}},//Enable command2
    {REGFLAG_DELAY, 10, {}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}

};

static struct LCM_setting_table lcm_backlight_level_setting[] = {
	{0x51, 1, {0xFF}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};
#endif

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
	params->dbi.te_mode 			= LCM_DBI_TE_MODE_DISABLED;
	params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;

	params->dsi.mode   = SYNC_EVENT_VDO_MODE;	//SYNC_PULSE_VDO_MODE;

	// DSI
	/* Command mode setting */
	params->dsi.LANE_NUM			= LCM_FOUR_LANE;
	//The following defined the fomat for data coming from LCD engine. 
	params->dsi.data_format.color_order 	= LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq   	= LCM_DSI_TRANS_SEQ_MSB_FIRST; 
	params->dsi.data_format.padding     	= LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format      	= LCM_DSI_FORMAT_RGB888;
	// Highly depends on LCD driver capability.
	// Not support in MT6573
	params->dsi.packet_size=256;
	// Video mode setting		
	params->dsi.intermediat_buffer_num 	= 2;
	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
	params->dsi.vertical_sync_active				= 10;
	params->dsi.vertical_backporch					= 4;
	params->dsi.vertical_frontporch					= 8;
	params->dsi.vertical_active_line				= FRAME_HEIGHT; 
	params->dsi.horizontal_sync_active				= 36;
	params->dsi.horizontal_backporch				= 80;
	params->dsi.horizontal_frontporch				= 80;
	//params->dsi.horizontal_blanking_pixel		       		= 60;
	params->dsi.horizontal_active_pixel		       		= FRAME_WIDTH;
	// Bit rate calculation

	params->dsi.PLL_CLOCK=410;
}


static void lcm_init(void)
{

/*
    SET_RESET_PIN(1);
    MDELAY(10);
*/
    SET_RESET_PIN(0);
    MDELAY(10);
/*	lcm_power_5v_en(1);
    MDELAY(10);
	lcm_power_n5v_en(1);
    MDELAY(10);
    */
    SET_RESET_PIN(1);
    MDELAY(120);

    push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_suspend(void)
{
	unsigned int data_array[16];

	data_array[0] = 0x00280500; // Display Off
	dsi_set_cmdq(data_array, 1, 1); 
	MDELAY(150);
	data_array[0] = 0x00100500; ; // Sleep In
	dsi_set_cmdq(data_array, 1, 1);

	SET_RESET_PIN(1);

	MDELAY(10);
/*	lcm_power_n5v_en(0);
    MDELAY(10);
    	lcm_power_5v_en(0);
    MDELAY(10);
    */
	SET_RESET_PIN(0);
//	MDELAY(30);
//	SET_RESET_PIN(1);
//	MDELAY(120);
//	push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);	//wqtao. enable

}


static void lcm_resume(void)
{
	lcm_init();
}

static unsigned int lcm_compare_id(void)
{
	unsigned int id=0, id0=0, id1=0;
	unsigned char buffer[2];
	unsigned int array[16];  

	SET_RESET_PIN(1);
	MDELAY(1);
	SET_RESET_PIN(0);
	MDELAY(1);
	/*
	lcm_power_5v_en(1);
    MDELAY(10);
	lcm_power_n5v_en(1);
    MDELAY(10);	
    */
	SET_RESET_PIN(1);
	MDELAY(120);//Must over 6 ms
	
    array[0] = 0x00053700;  // read id return two byte,version and id
    dsi_set_cmdq(array, 1, 1);
    read_reg_v2(0xa1, buffer, 5);
    id = ((buffer[2] << 8) | buffer[3]);    //we only need ID
    
#ifdef BUILD_LK
	printf("[otm1901a]%s,  hsd id = 0x%x,0x%x,0x%x\n", __func__, id,id0,id1);
#else
	printk("[otm1901a]%s,  hsd id = 0x%x,0x%x,0x%x\n", __func__, id,id0,id1);
#endif

/*
   MDELAY(10);
	lcm_power_n5v_en(0);
    MDELAY(10);
    	lcm_power_5v_en(0);
    	*/
    return (LCM_ID == id)?1:0;
    //  return 1;
}

// ---------------------------------------------------------------------------
//  Get LCM Driver Hooks
// ---------------------------------------------------------------------------
LCM_DRIVER hct_otm1901a_dsi_vdo_fhd_auo_55_hs = 
{
	.name			  = "hct_otm1901a_dsi_vdo_fhd_auo_55_hs",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,	
	.compare_id     = lcm_compare_id,	
};

