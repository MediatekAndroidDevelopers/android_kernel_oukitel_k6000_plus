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
#define LCM_ID                       (0x023C62)

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
/*
{0xB0,1,{0x04}},

{0xE9,1,{0x01}},

{0xD6,1,{0x01}},

{0xB3,3,{0x31,0x00,0x06}},

{0xB4,1,{0x00}},

{0xB6,5,{0x33,0xD3,0x80,0xFF,0xFF}},

{0xC0,1,{0x00}},

{0xC1,45,{0x84,0x08,0x00,0xEF,0xFF,0x4F,0xA7,0x8C,0xF7,0x98, 0xA5,0xFE,0x67,0xCD,0xB9,0xC6,0xFE,0x57,0xCB,0x9C,0x2B,0x06,0x29,0xFF,0x7F,0x00,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x40,0x02,0x62,0x01,0x06,0x05,0x84,0x00,0x01,0x00,0x01}},

{0xCB,17,{0xF1,0x9F,0x9F,0xFF,0x08,0x00,0x10,0x80,0x00,0x00,0x00,0x98,0x9F,0x01,0x00,0x00,0x40}},

{0xC2,16,{0x01,0xF7,0x80,0x0D,0x69,0x08,0x0C,0x10,0x00,0x08,0x60,0x00,0x00,0x00,0x00,0x00}},

{0xC3,62,{0x78,0x77,0x87,0x78,0x70,0x00,0x00,0x00,0x00,0x00,0x3B,0x63,0xB6,0x3B,0x60,0x3F,0xC3,0xFC,0x3F,0xC0,0x01,0x01,0x03,0x28,0x00,0x01,0x00,0x01,0x11,0x01,0x13,0x00,0x13,0x00,0x13,0x00,0x13,0x00,0x13,0x00,0x13,0x00,0x13,0x00,0x13,0x00,0x13,0x00,0x2B,0x00,0x2B,0x00,0x2B,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x20}},

{0xC4,16,{0x70,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x02,0x25,0x01,0x00,0x00,0x00}},

{0xC5,7,{0x10,0x00,0x00,0x00,0x00,0x70,0x00}},

{0xC6,22,{0x62,0x05,0x54,0x05,0x54,0x01,0x0E,0x01,0x02,0x01,0x02,0x01,0x02,0x01,0x02,0x01,0x02,0x08,0x15,0x06,0x7F,0xFF}},

{0xC7,30,{0x00,0x10,0x1A,0x24,0x33,0x40,0x49,0x56,0x3A,0x42,0x4F,0x5C,0x65,0x6E,0x7A,0x00,0x10,0x1A,0x24,0x33,0x40,0x49,0x57,0x3B,0x43,0x4F,0x5C,0x65,0x6E,0x7A}},

{0xC8,19,{0x00,0x00,0x00,0x00,0x00,0xFC,0x00,0x00,0x00,0x00,0x00,0xFC,0x00,0x00,0x00,0x00,0x00,0xFC,0x00}},

{0xC9,19,{0x00,0x00,0x00,0x00,0x00,0xFC,0x00,0x00,0x00,0x00,0x00,0xFC,0x00,0x00,0x00,0x00,0x00,0xFC,0x00}},

{0xCC,1,{0x1E}},

{0xCD,19,{0x00,0x00,0x22,0x00,0x22,0x00,0x22,0x00,0xAC,0xAC,0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0x15,0x00,0x00}},

{0xCF,2,{0x48,0x10}},

{0xD0,17,{0x33,0x59,0xD9,0x31,0x01,0x10,0x10,0x10,0x19,0x19,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},

{0xD1,1,{0x00}},

{0xD3,20,{0x1B,0x3B,0xBB,0x77,0x77,0x77,0xBB,0xB3,0x33,0x00,0x00,0x6D,0x6B,0xD3,0xCB,0x33,0xBB,0xF2,0xFD,0xC6}},

{0xD5,7,{0x03,0x00,0x00,0x00,0x2E,0x00,0x2E}},

{0xD9,2,{0x00,0x00}},

{0xD6,1,{0x01}},

{0xB0,1,{0x03}},

{0x30,4,{0x00,0x00,0x02,0xA7}},

{0x35,1,{0x00}},
*/
{0x11,1,{0x00}},  
{REGFLAG_DELAY, 120, {}},
{0x29,1,{0x00}},  
{REGFLAG_DELAY, 50, {}},
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
	params->dsi.vertical_sync_active				= 4; //10; //8;	//2;
	params->dsi.vertical_backporch					= 12;// 20; //18;	//14;
	params->dsi.vertical_frontporch					= 15;//10; //20;	//16;
	params->dsi.vertical_active_line				= FRAME_HEIGHT; 

	params->dsi.horizontal_sync_active				= 4;//40;	//2;
	params->dsi.horizontal_backporch				= 60;//100;//120;	//60;	//42;
	params->dsi.horizontal_frontporch				= 60;//80;//100;	//60;	//44;
	params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

// Bit rate calculation
//1 Every lane speed
//params->dsi.pll_div1=0;		// div1=0,1,2,3;div1_real=1,2,4,4 ----0: 546Mbps  1:273Mbps
//params->dsi.pll_div2=0;		// div2=0,1,2,3;div1_real=1,2,4,4	
//params->dsi.fbk_div =0x12;    // fref=26MHz, fvco=fref*(fbk_div+1)*2/(div1_real*div2_real)	

// zhangxiaofei add for test
params->dsi.PLL_CLOCK = 450;//208;	
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
    MDELAY(120);
    SET_RESET_PIN(0);
    MDELAY(20);

#ifdef GPIO_LCM_LDO_2V8_EN_PIN
    lcm_util.set_gpio_mode(GPIO_LCM_LDO_2V8_EN_PIN, GPIO_MODE_00);
    lcm_util.set_gpio_dir(GPIO_LCM_LDO_2V8_EN_PIN, GPIO_DIR_OUT);
    lcm_util.set_gpio_out(GPIO_LCM_LDO_2V8_EN_PIN, GPIO_OUT_ZERO);
#else   
    /* 2V8 off*/
//    mt_dsi_pinctrl_set(LCM_POWER_DP_NO, 0);     //lsm modify for t698 tp power as lcm ldo 20170327
#endif
    MDELAY(5);
#ifdef GPIO_LCM_LDO_1V8_EN_PIN
    lcm_util.set_gpio_mode(GPIO_LCM_LDO_1V8_EN_PIN, GPIO_MODE_00);
    lcm_util.set_gpio_dir(GPIO_LCM_LDO_1V8_EN_PIN, GPIO_DIR_OUT);
    lcm_util.set_gpio_out(GPIO_LCM_LDO_1V8_EN_PIN, GPIO_OUT_ZERO);  //shm
#else   
//    mt_dsi_pinctrl_set(LCM_POWER_DM_NO, 0);    //lsm modify for t698 tp power as lcm ldo 20170327
#endif
    MDELAY(5);
}


    //for LGE backlight IC mapping table


    // Refresh value of backlight level.

//static unsigned int lcm_compare_id(void);

static void lcm_resume(void)
{
#ifdef GPIO_LCM_LDO_2V8_EN_PIN
    lcm_util.set_gpio_mode(GPIO_LCM_LDO_2V8_EN_PIN, GPIO_MODE_00);
    lcm_util.set_gpio_dir(GPIO_LCM_LDO_2V8_EN_PIN, GPIO_DIR_OUT);
    lcm_util.set_gpio_out(GPIO_LCM_LDO_2V8_EN_PIN, GPIO_OUT_ZERO);
#else   
//    mt_dsi_pinctrl_set(LCM_POWER_DP_NO, 0);    // 2V8 off    //lsm modify for t698 tp power as lcm ldo 20170327
#endif
    MDELAY(5);
#ifdef GPIO_LCM_LDO_1V8_EN_PIN
    lcm_util.set_gpio_mode(GPIO_LCM_LDO_1V8_EN_PIN, GPIO_MODE_00);
    lcm_util.set_gpio_dir(GPIO_LCM_LDO_1V8_EN_PIN, GPIO_DIR_OUT);
    lcm_util.set_gpio_out(GPIO_LCM_LDO_1V8_EN_PIN, GPIO_OUT_ZERO);  //shm
#else   
//    mt_dsi_pinctrl_set(LCM_POWER_DM_NO, 0);    //lsm modify for t698 tp power as lcm ldo 20170327
#endif
    MDELAY(20);
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
	

	unsigned int id = 0;
    unsigned char buffer[4];
    unsigned int array[16];

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
        MDELAY(50);  

		
		
	     array[0] = 0x00022902;
    array[1] = 0x000000b0;
    dsi_set_cmdq(array, 2, 1);

    array[0] = 0x00043700;// read id return two byte,version and id
    dsi_set_cmdq(array, 1, 1);

    read_reg_v2(0xbf, buffer, 4);
    id = buffer[0]<<16 | buffer[1]<<8 | buffer[2]; //we only need ID
	
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

LCM_DRIVER hct_td4300_dsi_vdo_fhd_lg_55_rx = 
{
    .name           = "hct_td4300_dsi_vdo_fhd_lg_55_rx",
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

