
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

#define FRAME_WIDTH                                          (1080)
#define FRAME_HEIGHT                                         (1920)

#define REGFLAG_DELAY                                         0XFE
#define REGFLAG_END_OF_TABLE                                  0xFF   // END OF REGISTERS MARKER

#define LCM_ID  0x5521
#define LCM_DSI_CMD_MODE                                    0

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)    lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)        lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)                                    lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)                lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg                                            lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)                   lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size) 

struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};

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
        params->dbi.te_mode 				= LCM_DBI_TE_MODE_DISABLED;
        //params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;

        params->dsi.mode   = BURST_VDO_MODE; //SYNC_PULSE_VDO_MODE; //SYNC_PULSE_VDO_MODE;//BURST_VDO_MODE;

        // DSI
        /* Command mode setting */
        //1 Three lane or Four lane
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
        params->dsi.intermediat_buffer_num = 0;//because DSI/DPI HW design change, this parameters should be 0 when video mode in MT658X; or memory leakage

        params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
//        params->dsi.word_count=720*3;
//	params->physical_width = 69;
//     	params->physical_height = 122;

//VS=2
//VBP=6
//VFP=8
//HS=8
//HBP=80
//HFP=80
	params->dsi.vertical_sync_active			= 2;
	params->dsi.vertical_backporch				= 6;
	params->dsi.vertical_frontporch				= 8;	
	params->dsi.vertical_active_line				= FRAME_HEIGHT;

	params->dsi.horizontal_sync_active			= 8; //2
	params->dsi.horizontal_backporch			= 80; //4
	params->dsi.horizontal_frontporch			= 80; //4
	params->dsi.horizontal_active_pixel			= FRAME_WIDTH;
        // clk_lp_per_line_enable need set to 0, otherwise shall blink 
        params->dsi.esd_check_enable = 0; 
        params->dsi.customization_esd_check_enable = 0;   
        params->dsi.lcm_esd_check_table[0].cmd          = 0x0A;    
        params->dsi.lcm_esd_check_table[0].count        = 1;     
        params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9c;

	params->dsi.PLL_CLOCK=410;

}

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


static struct LCM_setting_table lcm_initialization_setting[] = {
{0xB0,1,{0x04}},
{0xD6,1,{0x01}},
{0xB3,6,{0x14,0x00,0x00,0x00,0x00,0x00}},
{0xB4,2,{0x0C,0x00}},
{0xB6,3,{0x4B,0xCB,0x00}},
{0xC1,35,{0x04,0x60,0x00,0x20,0xA9,0x30,0x22,0xFB,0xF0,0xFF,0xFF,0x9B,0x7B,0xCF,0xB5,0xFF,0xFF,0x87,0x9F,0x45,0x22,0x54,0x02,0x00,0x00,0x00,0x00,0x00,0x22,0x33,0x03,0x22,0x00,0xFF,0x11}},
{0xC2,8,{0x31,0xF7,0x80,0x08,0x08,0x00,0x00,0x08}},
{0xC4,8,{0x70,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03}},
{0xC6,21,{0xC8,0x3C,0x3C,0x07,0x01,0x07,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0A,0x1E,0x07,0xC8}},
{0xC7,30,{0x02,0x19,0x22,0x2C,0x3A,0x47,0x50,0x5E,0x40,0x47,0x52,0x5E,0x67,0x6C,0x76,0x02,0x19,0x22,0x2C,0x3A,0x47,0x50,0x5E,0x40,0x47,0x52,0x5E,0x67,0x6C,0x76}},
{0xCB,15,{0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xE0,0x00,0x00}},
{0xCC,1,{0x34}},
{0xD0,10,{0x11,0x00,0x00,0x54,0xD2,0x40,0x19,0x19,0x09,0x00}},

//ANX6585 or NT50198
//{0xD1,4,{0x04,0x48,0x06,0x0F}},

//ANX6586 or HX5186 OT2005
{0xD1,4,{0x04,0x40,0x08,0x0F}},

{0xD3,26,{0x1B,0x33,0xBB,0xBB,0xB3,0x33,0x33,0x33,0x33,0x00,0x01,0x00,0x00,0xF8,0xA0,0x08,0x2F,0x2F,0x33,0x33,0x72,0x12,0x8A,0x57,0x3D,0xBC}},
{0xD5,7,{0x06,0x00,0x00,0x01,0x27,0x01,0x27}},

{REGFLAG_DELAY, 10, {}},
{0x29,1,{0x00}},
{REGFLAG_DELAY, 30, {}},

{0x11,1,{0x00}},
{REGFLAG_DELAY, 150, {}},


{REGFLAG_END_OF_TABLE, 0x00, {}}
	// Note
	// Strongly recommend not to set Sleep out / Display On here. That will cause messed frame to be shown as later the backlight is on.


	// Setting ending by predefined flag
};

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
    MDELAY(20);
    data_array[0]=0x00100500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(120);
    SET_RESET_PIN(0);
    MDELAY(20);
}


static void lcm_resume(void)
{
     lcm_init();
/*
    unsigned int data_array[16];
    data_array[0]=0x00110500;
    dsi_set_cmdq(data_array,1,1);
    MDELAY(100);
    data_array[0]=0x00290500;
    dsi_set_cmdq(data_array,1,1);
    MDELAY(10);
*/
}


static unsigned int lcm_compare_id(void)
{
/*
    unsigned int id = 0;
    unsigned char buffer[5];
    unsigned int array[16];

    SET_RESET_PIN(1);  //shm
    MDELAY(10);
    SET_RESET_PIN(0);
    MDELAY(10);
    SET_RESET_PIN(1);
    MDELAY(20);

    array[0] = 0x00053700;  // read id return two byte,version and id
    dsi_set_cmdq(array, 1, 1);
    read_reg_v2(0xbf, buffer, 5);
     MDELAY(20);
    id = ((buffer[2] << 8) | buffer[3]);    //we only need ID

#ifdef BUILD_LK
	printf("%s,  otm1287a+tm id = 0x%08x\n", __func__, id);
#else
	printk("%s,  otm1287a+tm id = 0x%08x\n", __func__, id);
#endif
    return (LCM_ID == id)?1:0;
*/
        return 1;
}









// ---------------------------------------------------------------------------
//  Get LCM Driver Hooks
// ---------------------------------------------------------------------------
LCM_DRIVER hct_r63350a_dsi_vdo_fhd_auo_55_hz= 
{
    .name			= "hct_r63350a_dsi_vdo_fhd_auo_55_hz",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    .compare_id    = lcm_compare_id,
#if 0//defined(LCM_DSI_CMD_MODE)
//    .set_backlight	= lcm_setbacklight,
    //.set_pwm        = lcm_setpwm,
    //.get_pwm        = lcm_getpwm,
    .update         = lcm_update
#endif
};

