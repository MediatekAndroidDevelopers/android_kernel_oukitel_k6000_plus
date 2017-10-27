#ifndef BUILD_LK
#include <linux/string.h>
#include <linux/kernel.h>
#endif
#include "lcm_drv.h"

#ifdef BUILD_LK
	#include <platform/mt_gpio.h>
	#include <string.h>
#else
#endif


#define LCM_ID_SH1386 (0x80)


// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  										(720)
#define FRAME_HEIGHT 										(1280)

#define REGFLAG_DELAY             							0XFFE
#define REGFLAG_END_OF_TABLE      							0xFFF   // END OF REGISTERS MARKER


#define LCM_DSI_CMD_MODE									0

#ifndef TRUE
    #define   TRUE     1
#endif
 
#ifndef FALSE
    #define   FALSE    0
#endif

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

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg											lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)


static void lcm_power_2v8_en(unsigned char enabled)
{
	if (enabled)
	{
		mt_dsi_pinctrl_set(LCM_POWER_DP_NO, 1);
	}	
	else
	{
		mt_dsi_pinctrl_set(LCM_POWER_DP_NO, 0);
	}
}

static void lcm_power_1v8_en(unsigned char enabled)
{
	if (enabled)
	{
		mt_dsi_pinctrl_set(LCM_POWER_DM_NO, 1);
	}	
	else
	{
		mt_dsi_pinctrl_set(LCM_POWER_DM_NO, 0);
	}
}

// +5v的gpio 用做 FPC上power ic 的 enable
static void lcm_power_ic_en(unsigned char enabled)
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

struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};


static struct LCM_setting_table lcm_initialization_setting[] = {   

	{0xF0,5,{0x55,0xAA,0x52,0x08,0x00}}, 
	{0xB0,4,{0x0F,0x0F,0x1E,0x14}}, 
	{0xB2,1,{0x00}}, 
	{0xBB,1,{0x77}}, 
	{0xB6,1,{0x03}},
	//0x00
#ifndef BUILD_LK
  {0xC0,20,{0x00,0x00,0x06,0x07,0x08,0x09,0x00,0x00,
            0x00,0x00,0x02,0x00,0x0A,0x0B,0x0C,0x0D,
            0x00,0x00,0x00,0x00}},
#else
	{0xC0,20,{0x03,0x00,0x06,0x07,0x08,0x09,0x00,0x00,
	          0x00,0x00,0x02,0x00,0x0A,0x0B,0x0C,0x0D,
	          0x00,0x00,0x00,0x00}}, 
#endif
	{0xC1,16,{0x08,0x24,0x24,0x01,0x18,0x24,0x9F,0x85,
	          0x08,0x24,0x24,0x01,0x18,0x24,0x9F,0x85}},
	{0xC2,24,{0x03,0x05,0x1B,0x24,0x13,0x31,0x01,0x05,
	          0x1B,0x24,0x13,0x31,0x03,0x05,0x1B,0x38,
	          0x00,0x11,0x02,0x05,0x1B,0x38,0x00,0x11}}, 
	 {0xC3,24,{0x02,0x05,0x1B,0x24,0x13,0x11,0x03,0x05,
	          0x1B,0x24,0x13,0x11,0x03,0x05,0x1B,0x38,
	          0x00,0x11,0x02,0x05,0x1B,0x38,0x00,0x11}},
			   		  
	/*{0xC1,16,{0x08,0x00,0x00,0x01,0x18,0x00,0x68,0x85,
	          0x08,0x00,0x00,0x01,0x18,0x00,0x68,0x85}},
	{0xC2,24,{0x03,0x15,0x1B,0x00,0x22,0x30,0x01,0x15,
	          0x1B,0x00,0x22,0x30,0x03,0x15,0x1B,0x10,
	          0x01,0x10,0x02,0x15,0x1B,0x10,0x01,0x10}}, 
	 {0xC3,24,{0x02,0x15,0x1B,0x00,0x22,0x10,0x03,0x15,
	          0x1B,0x00,0x22,0x10,0x03,0x15,0x1B,0x10,
	          0x01,0x10,0x02,0x15,0x1B,0x10,0x01,0x10}}, 
	 {0xC4,24,{0x03,0x15,0x1B,0x00,0x22,0x30,0x01,0x15,
	          0x1B,0x00,0x22,0x30,0x03,0x15,0x1B,0x10,
	          0x01,0x10,0x02,0x15,0x1B,0x10,0x01,0x10}}, 		  
	 {0xC5,24,{0x02,0x15,0x1B,0x00,0x22,0x10,0x03,0x15,
	          0x1B,0x00,0x22,0x10,0x03,0x15,0x1B,0x10,
	          0x01,0x10,0x02,0x15,0x1B,0x10,0x01,0x10}}, 
	*/
	{0xF0,5,{0x55,0xAA,0x52,0x08,0x01}}, 
	{0xB5,1,{0x1E}}, 
	{0xB6,1,{0x2D}}, 
	{0xB7,1,{0x04}}, 
  {0xB8,1,{0x05}}, 
  {0xB9,1,{0x04}}, 
	{0xBA,1,{0x14}}, 
	//{0xBB,1,{0x2E}},   //VSP 6.4
	{0xBB,1,{0x2F}},   //VSP 6.1
	//{0xBB,1,{0x30}},     //VSP 5.8
	//{0xBB,1,{0x2D}},     //VSP 6.7
	{0xBE,1,{0x12}},//{0xBE,1,{0x1C}}, 
	{0xC2,3,{0x00,0x35,0x07}}, 
	{0xF0,5,{0x55,0xAA,0x52,0x08,0x02}}, 
  {0xC1,2,{0x03,0x00}}, //0x00 ,0x00
	{0xC9,1,{0x13}},//{0xC9,1,{0x1E}}
//	{0xD4,3,{0x02,0x04,0x35}}, //{0xD4,3,{0x02,0x04,0x2C}}
  {0xD4,3,{0x02,0x04,0x33}}, //{0xD4,3,{0x02,0x04,0x2C}}
	{0x8F,6,{0x5A,0x96,0x3C,0xC3,0xA5,0x69}},
	{0x81,1,{0x0B}}, 
	{0x89,1,{0x00}}, 
	{0x82,1,{0x04}}, 
	{0x8C,3,{0x55,0x49,0x53}}, 
	{0x9A,1,{0x5A}},
	{0xFF,4,{0xA5,0x5A,0x13,0x86}}, 
#ifndef BUILD_LK
  {0xFE,2,{0x01,0x84}}, //0x84
#else
  {0xFE,2,{0x01,0x54}},
#endif
	{0x35,1,{0x00}}, 
	{0x11,1,{0x00}}, 
	{REGFLAG_DELAY,140,{}},
		  
	{0x29,1,{0x00}}, 
  {REGFLAG_DELAY, 60, {}},

	{REGFLAG_END_OF_TABLE,0x00,{}}

};

/*static struct LCM_setting_table lcm_backlight_level_setting[] = {
	//{0x6F,1,{0x00}}, 
	//{0xC0,1,{0x03}},
	//{0xF0, 5,{0x55,0xAA,0x52,0x08,0x02}},
	{0xC1, 2,{0x02,0x00}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};
*/
#if 0
// unused-variable
static struct LCM_setting_table lcm_sleep_out_setting[] = {
	// Sleep Out
	{0x11, 1, {0x00}},
	{REGFLAG_DELAY, 120, {}},

	// Display ON
	{0x29, 1, {0x00}},
	{REGFLAG_DELAY, 20, {}},

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
		params->dbi.te_mode 				= LCM_DBI_TE_MODE_DISABLED;  //LCM_DBI_TE_MODE_VSYNC_ONLY;
		params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;

		params->dsi.mode   = BURST_VDO_MODE;

	
		// DSI
		/* Command mode setting */
		params->dsi.LANE_NUM				= LCM_FOUR_LANE;
		//The following defined the fomat for data coming from LCD engine.
		params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
		params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
		params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
		params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;




//		Video mode setting		
  params->dsi.intermediat_buffer_num = 0;	//because DSI/DPI HW design change, this parameters should be 0 when video mode in MT658X; or memory leakage
		params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
		params->dsi.word_count = 720*3;
		params->dsi.packet_size=512; //256

		params->dsi.vertical_sync_active				= 4;//2;
		params->dsi.vertical_backporch					= 30;   // from Q driver
		params->dsi.vertical_frontporch					= 30;  // rom Q driver
		params->dsi.vertical_active_line				= FRAME_HEIGHT;

		params->dsi.horizontal_sync_active				= 4;//10;
		params->dsi.horizontal_backporch				= 270;//270; // from Q driver
  		params->dsi.horizontal_frontporch				= 270;// 30// 0x18 ;
		params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

		params->dsi.PLL_CLOCK = 300;//240; //this value must be in MTK suggested table
 // params->dsi.ssc_disable = 1;  // ssc disable control (1: disable, 0: enable, default: 0)
		params->dsi.cont_clock=1;

		//ESD CHECK
		params->dsi.esd_check_enable = 0; //enable ESD check
		//params->dsi.cont_clock = 1;
		params->dsi.customization_esd_check_enable = 0; //0 TE ESD CHECK  1 LCD REG CHECK

		params->dsi.lcm_esd_check_table[0].cmd            = 0xac;//ac
		params->dsi.lcm_esd_check_table[0].count        = 1;
		params->dsi.lcm_esd_check_table[0].para_list[0] = 0x00;//00

//		params->dsi.noncont_clock=TRUE;
//		params->dsi.noncont_clock_period=2;

}

static void lcm_power_on(void){

    lcm_power_1v8_en(0);
    lcm_power_2v8_en(0);
    SET_RESET_PIN(0);
    MDELAY(10);

    lcm_power_1v8_en(1);
    MDELAY(15);

    lcm_power_2v8_en(1);
    MDELAY(15);

    SET_RESET_PIN(1);
	MDELAY(20);
    SET_RESET_PIN(0);
  MDELAY(20);
    SET_RESET_PIN(1);
    MDELAY(20);
    lcm_power_ic_en( 1);
 //   MDELAY(20);


}


static void lcm_init(void)
{
	lcm_power_on();


    push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);

#ifndef BUILD_LK
	printk("lcm_init sh1386 lcm init\n");
#endif 

}

static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
	// Sleep Out
	{0x28, 1, {0x00}},
	{REGFLAG_DELAY, 20, {}},

	// Display ON
	{0x10, 1, {0x00}},
	{REGFLAG_DELAY, 120, {}},

	{REGFLAG_END_OF_TABLE, 0x00, {}}
};


static void lcm_suspend(void)
{
	push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
 #if 1
    lcm_power_ic_en( 0);
    MDELAY(100);
    SET_RESET_PIN(0);
    MDELAY(10);
    lcm_power_2v8_en(0);
    MDELAY(10);

     lcm_power_1v8_en(0);

     MDELAY(10);
#endif
}

//static int is_lcm_resume=0;
static unsigned char goasetflag=0;
static void lcm_resume(void)
{
	   lcm_init();
	   goasetflag=0xeb;
}


#if 1
static struct LCM_setting_table lcm_backlight_level_setting_F0={0xF0,5,{0x55,0xAA,0x52,0x08,0x02}};
static struct LCM_setting_table lcm_backlight_level_setting_C0={0xC0,20,{0x03,0x00,0x06,0x07,0x08,0x09,0x00,0x00,0x00,0x00,0x02,0x00,0x0A,0x0B,0x0C,0x0D,0x00,0x00,0x00,0x00}}; 
static struct LCM_setting_table lcm_backlight_level_setting_C1={0xc1,2,{0x00,0x00}};
static void lcm_setbacklight(unsigned int level)
{
	  if(level<50) level=50;
	  if(level>255) level=255;
	  level = level*(1023-50)/255+50;
#ifdef BUILD_LK
	printf("lcm_setbacklight level:%d\n",level);
#else
	printk("lcm_setbacklight level:%d\n",level);
#endif

	  lcm_backlight_level_setting_C1.para_list[0]=level>>8;
	  lcm_backlight_level_setting_C1.para_list[1]=level&0xff;
	  
	  dsi_set_cmdq_V2(0xC1, 2,lcm_backlight_level_setting_C1.para_list, 1);// backlight adjust
	  if(goasetflag==0xeb)  // execute only when resume 
	  {
		 goasetflag=0;	
		 MDELAY(20);
		 lcm_backlight_level_setting_F0.para_list[0]=0x55;
		 lcm_backlight_level_setting_F0.para_list[1]=0xAA;
		 lcm_backlight_level_setting_F0.para_list[2]=0x52;
		 lcm_backlight_level_setting_F0.para_list[3]=0x08;
		 lcm_backlight_level_setting_F0.para_list[4]=0x00;
		 dsi_set_cmdq_V2(0xF0, 5,lcm_backlight_level_setting_F0.para_list, 1);

		 MDELAY(20); 
		 lcm_backlight_level_setting_C0.para_list[0]=0x03;
		 dsi_set_cmdq_V2(0xC0, 11,lcm_backlight_level_setting_C0.para_list, 1);
		 MDELAY(20); 	
		 lcm_backlight_level_setting_F0.para_list[0]=0x55;
		 lcm_backlight_level_setting_F0.para_list[1]=0xAA;
		 lcm_backlight_level_setting_F0.para_list[2]=0x52;
		 lcm_backlight_level_setting_F0.para_list[3]=0x08;
		 lcm_backlight_level_setting_F0.para_list[4]=0x02;
		 dsi_set_cmdq_V2(0xF0, 5,lcm_backlight_level_setting_F0.para_list, 1);

	  }
}

#else

static struct LCM_setting_table lcm_backlight_level_setting_C1[]={
	{0xc1,2,{0x00,0x00}},
	{REGFLAG_END_OF_TABLE, 0x00, {}},	
};

static void lcm_setbacklight(unsigned int level)
{
	if(level<50) level=50;
	if(level>255) level=255;
    	level = level*235 / 255;
	level = level*(1023-50)/255+50;
#ifdef BUILD_LK
	printf("lcm_setbacklight level:%d\n",level);
#else
	printk("lcm_setbacklight level:%d\n",level);
#endif
	lcm_backlight_level_setting_C1[0].para_list[0]=level>>8;
	lcm_backlight_level_setting_C1[0].para_list[1]=level&0xff;
	
	dsi_set_cmdq_V2(0xC1, 2,lcm_backlight_level_setting_C1[0].para_list, 1);// backlight adjust
}

#endif
static unsigned int lcm_compare_id(void)
{
	unsigned int id = 0;
	unsigned char buffer[2];
	unsigned int array[16];

//	lcm_power_on();

	array[0] = 0x00023700;	
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0x04, buffer, 2);
	id = buffer[1];	

    
#ifdef BUILD_LK
	printf("%s, LK lcm_compare_id: sh1386 0x%08x\n", __func__, id);
#else
	printk("%s,kernel lcm_compare_id: sh1386 = 0x%08x\n", __func__, id);
#endif

	if (id == LCM_ID_SH1386)
		return 1;
	else
		return 0;
}
LCM_DRIVER hct_sh1386_dsi_vdo_hd_hx_55_hx = 
{
    	.name		= "hct_sh1386_dsi_vdo_hd_hx_55_hx",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
    .set_backlight = lcm_setbacklight,
	.compare_id    = lcm_compare_id,

};
