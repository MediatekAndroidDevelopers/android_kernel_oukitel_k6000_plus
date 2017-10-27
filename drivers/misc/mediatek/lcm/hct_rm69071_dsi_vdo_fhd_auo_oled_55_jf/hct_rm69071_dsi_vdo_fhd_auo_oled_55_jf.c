#ifndef BUILD_LK
#include <linux/string.h>
#endif
#include "lcm_drv.h"

#ifdef BUILD_LK
//#include <platform/mt_gpio.h>
#include <string.h>
//#include <platform/mt_pmic.h>
#elif defined(BUILD_UBOOT)
//#include <asm/arch/mt_gpio.h>
#else
//#include <mach/mt_gpio.h>
//#include <linux/xlog.h>
//#include <mach/mt_pm_ldo.h>
#endif

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  (1080)
#define FRAME_HEIGHT (1920)



// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};
//static  int  VOL_2V9=20;
//static int currentBrightness;

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))

#define REGFLAG_DELAY             								0xFC
#define REGFLAG_UDELAY             								0xFB

#define REGFLAG_END_OF_TABLE      							    	0xFD  

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)    lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)       lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)                                      lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)                  lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)                                           lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)               lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

#define   LCM_DSI_CMD_MODE							(0)

struct LCM_setting_table {
    unsigned char cmd;
    unsigned char count;
    unsigned char para_list[64];
};

static struct LCM_setting_table lcm_initialization_setting[] = {

	{0xFE, 1,  {0x04}},
	{0x4D, 1,  {0x82}},	
	{0x4E, 1,  {0x01}},	
	{0x4F, 1,  {0x00}},	
	{0x44, 1,  {0x47}},	
	{0x5E, 1,  {0x00}},	
	
	{0x27, 1,  {0x60}},	
	{0x28, 1,  {0x60}},
	{0x2D, 1,  {0x60}},	
	{0x2F, 1,  {0x60}},
	{0x36, 1,  {0x0F}},	
	{0x37, 1,  {0x0F}},
	{0x3D, 1,  {0x0F}},	
	{0x3F, 1,  {0x0F}},
	
	{0xFE, 1,  {0x07}},
	{0x9E, 1,  {0x80}},	
	{0xA0, 1,  {0x00}},	
	{0xA3, 1,  {0x01}},	
	{0xA5, 1,  {0x1E}},	
	{0xA7, 1,  {0xB8}},	
	
	{0x00, 1,  {0xEC}},	
	{0x02, 1,  {0x03}},	
	{0x04, 1,  {0x00}},	
	{0x06, 1,  {0x0C}},	
	{0x08, 1,  {0x1E}},	
	{0x0A, 1,  {0xB8}},	
	
	{0x0B, 1,  {0xEC}},	
	{0x0D, 1,  {0x03}},	
	{0x0F, 1,  {0x00}},	
	{0x11, 1,  {0x0B}},	
	{0x13, 1,  {0x1E}},	
	{0x15, 1,  {0xB8}},
	
	{0x16, 1,  {0xEC}},	
	{0x18, 1,  {0x03}},	
	{0x1A, 1,  {0x00}},	
	{0x1C, 1,  {0x0A}},	
	{0x1E, 1,  {0x1E}},	
	{0x20, 1,  {0xB8}},
	
	{0x21, 1,  {0xEC}},	
	{0x23, 1,  {0x02}},	
	{0x25, 1,  {0x01}},	
	{0x27, 1,  {0x0B}},	
	{0x29, 1,  {0x7F}},	
	{0x2B, 1,  {0x2A}},	
	
	{0x2D, 1,  {0xEC}},	
	{0x30, 1,  {0x02}},	
	{0x32, 1,  {0x01}},	
	{0x34, 1,  {0x0C}},	
	{0x36, 1,  {0x7F}},	
	{0x38, 1,  {0x2A}},
	
	{0xA9, 1,  {0x6A}},	
	{0xAB, 1,  {0x05}},	
	{0xAD, 1,  {0x7F}},	
	{0xAF, 1,  {0x35}},	
	{0xBB, 1,  {0x80}},	
	{0xBC, 1,  {0x1C}},
	
	{0xFE, 1,  {0x08}},
	{0x5F, 1,  {0x57}},	
	{0x03, 1,  {0x40}},	
	{0x07, 1,  {0x1A}},	
	{0x00, 1,  {0x00}},	
	{0x2A, 1,  {0x00}},	
	{0x2F, 1,  {0x04}},	
	{0x5D, 1,  {0x89}},	
	{0x5F, 1,  {0x00}},
	
	{0x8A, 1,  {0x07}},	
	{0x8B, 1,  {0x70}},
	
	{0xA4, 1,  {0x00}},	
	{0xAA, 1,  {0xC0}},	
	{0xD2, 1,  {0x01}},	
	
	{0xFE, 1,  {0x09}},	
	{0xB8, 1,  {0x01}},
	{0xB9, 1,  {0x54}},
	
	{0xFE, 1,  {0x0A}},	
	{0x14, 1,  {0x52}},
	
	{0xFE, 1,  {0x0D}},	
	{0xC0, 1,  {0x01}},
	{0x53, 1,  {0xFE}},
	{0x02, 1,  {0x65}},
	
	{0xFE, 1,  {0x00}},	
	{0x35, 1,  {0x00}},
	{0xC2, 1,  {0x03}},
//	{0x21, 1,  {0x00}},	
	{0x55, 1,  {0x00}},
//	{0x51, 1,  {0x00}},	
//	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_backlight_level_setting[] = {
	{0x51, 1,  {0x00}},
//	{REGFLAG_END_OF_TABLE, 0x00, {}}	
};

static struct LCM_setting_table lcm_poweron_setting[] = {

	{0xFE, 1,  {0x07}},
	{0xA9, 1,  {0xFA}},
	//{REGFLAG_UDELAY, 16667, {}},
	{REGFLAG_DELAY, 17, {}},	
	{0xFE, 1,  {0x00}},
//	{REGFLAG_END_OF_TABLE, 0x00, {}}	
};
static struct LCM_setting_table lcm_poweroff_setting[] = {

	{0xFE, 1,  {0x07}},
	{0xA9, 1,  {0x6A}},
	//{REGFLAG_UDELAY, 16667, {}},
	{REGFLAG_DELAY, 17, {}},
	{0xFE, 1,  {0x00}},
//	{REGFLAG_END_OF_TABLE, 0x00, {}}	
};
static struct LCM_setting_table lcm_sleepout[] = {

	{0x11, 1,  {0x00}},
//	{REGFLAG_END_OF_TABLE, 0x00, {}}	
};
static struct LCM_setting_table lcm_sleepin[] = {

	{0x10, 1,  {0x00}},
//	{REGFLAG_END_OF_TABLE, 0x00, {}}	
};
static struct LCM_setting_table lcm_displayon[] = {

	{0x29, 1,  {0x00}},
//	{REGFLAG_END_OF_TABLE, 0x00, {}}	
};
static struct LCM_setting_table lcm_displayoff[] = {

	{0x28, 1,  {0x00}},
//	{REGFLAG_END_OF_TABLE, 0x00, {}}	
};

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
    unsigned int i;

    for(i = 0; i < count; i++)
    {
        unsigned cmd;
        cmd = table[i].cmd;

        switch (cmd) {

            	case REGFLAG_DELAY :
              		if(table[i].count <= 10)
                    		MDELAY(table[i].count);
                	else
                    		MDELAY(table[i].count);
                	break;
				
		//case REGFLAG_UDELAY :
			//UDELAY(table[i].count);
			//break;

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

	params->dbi.te_mode = LCM_DBI_TE_MODE_VSYNC_ONLY;
	params->dbi.te_edge_polarity = LCM_POLARITY_RISING;

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode = CMD_MODE;
#else
	params->dsi.mode = SYNC_PULSE_VDO_MODE;//BURST_VDO_MODE;
#endif

	params->dsi.noncont_clock = 1;
	params->dsi.noncont_clock_period = 1;
//    	params->dsi.cont_clock=0; //1;
	// DSI
	/* Command mode setting */
	//1 Three lane or Four lane
	params->dsi.LANE_NUM = LCM_FOUR_LANE;
	//The following defined the fomat for data coming from LCD engine.
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;

	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;
	
	params->dsi.vertical_sync_active = 5;
	params->dsi.vertical_backporch = 7;
	params->dsi.vertical_frontporch = 12;
	params->dsi.vertical_active_line = FRAME_HEIGHT;


	params->dsi.horizontal_sync_active = 5;
	params->dsi.horizontal_backporch = 120;
	params->dsi.horizontal_frontporch = 8;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	//params->dsi.pll_select=1;     //0: MIPI_PLL; 1: LVDS_PLL
	// Bit rate calculation
	//1 Every lane speed
	params->dsi.PLL_CLOCK = 425; //425
	params->dsi.ssc_disable = 1;

}

static void lcm_init(void)
{	

	SET_RESET_PIN(0);
	MDELAY(10);
	mt_dsi_pinctrl_set(LCM_POWER_DP_NO, 1);
	MDELAY(10);
	mt_dsi_pinctrl_set(LCM_POWER_DM_NO, 1);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(20);
	#if 1
		push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);	
	    	push_table(lcm_backlight_level_setting, sizeof(lcm_backlight_level_setting) / sizeof(struct LCM_setting_table), 1);
		MDELAY(20);
	
		push_table(lcm_poweron_setting, sizeof(lcm_poweron_setting) / sizeof(struct LCM_setting_table), 1);	
		MDELAY(50);
	
		push_table(lcm_sleepout, sizeof(lcm_sleepout) / sizeof(struct LCM_setting_table), 1);	
		//MDELAY(50);// time must equal to 50

		MDELAY(120);

		push_table(lcm_displayon, sizeof(lcm_displayon) / sizeof(struct LCM_setting_table), 1);			
	
		MDELAY(100);		//20
		//set VENG to -2.9V
	
		//push_table(lcm_poweron_setting, sizeof(lcm_poweron_setting) / sizeof(struct LCM_setting_table), 1);	
		//MDELAY(50);
        #else 
		push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
	    	push_table(lcm_backlight_level_setting, sizeof(lcm_backlight_level_setting) / sizeof(struct LCM_setting_table), 1);	
		MDELAY(100);
	
		push_table(lcm_sleepout, sizeof(lcm_sleepout) / sizeof(struct LCM_setting_table), 1);	
		MDELAY(50);// time must equal to 50

		MDELAY(200);

		push_table(lcm_displayon, sizeof(lcm_displayon) / sizeof(struct LCM_setting_table), 1);	
	       MDELAY(50);		
	
		MDELAY(100);		//20
		//set VENG to -2.9V
	
		push_table(lcm_poweron_setting, sizeof(lcm_poweron_setting) / sizeof(struct LCM_setting_table), 1);
	#endif
}

static void lcm_suspend(void)
{

	push_table(lcm_displayoff, sizeof(lcm_displayoff) / sizeof(struct LCM_setting_table), 1);	
	MDELAY(100);	
	

	push_table(lcm_poweroff_setting, sizeof(lcm_poweroff_setting) / sizeof(struct LCM_setting_table), 1);	
	MDELAY(100);	
	
	push_table(lcm_sleepin, sizeof(lcm_sleepin) / sizeof(struct LCM_setting_table), 1);	
	MDELAY(100);	

	mt_dsi_pinctrl_set(LCM_POWER_DP_NO, 0);
	MDELAY(10);
	mt_dsi_pinctrl_set(LCM_POWER_DM_NO, 0);
	MDELAY(10);
	SET_RESET_PIN(0);

}


static void lcm_resume(void)
{

	lcm_init();

}

#if (LCM_DSI_CMD_MODE)
static void lcm_update(unsigned int x, unsigned int y,
		       unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0 >> 8) & 0xFF);
	unsigned char x0_LSB = (x0 & 0xFF);
	unsigned char x1_MSB = ((x1 >> 8) & 0xFF);
	unsigned char x1_LSB = (x1 & 0xFF);
	unsigned char y0_MSB = ((y0 >> 8) & 0xFF);
	unsigned char y0_LSB = (y0 & 0xFF);
	unsigned char y1_MSB = ((y1 >> 8) & 0xFF);
	unsigned char y1_LSB = (y1 & 0xFF);

	unsigned int data_array[16];

	data_array[0] = 0x00053902;
	data_array[1] =
	    (x1_MSB << 24) | (x0_LSB << 16) | (x0_MSB << 8) | 0x2a;
	data_array[2] = (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00053902;
	data_array[1] =
	    (y1_MSB << 24) | (y0_LSB << 16) | (y0_MSB << 8) | 0x2b;
	data_array[2] = (y1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x002c3909;
	dsi_set_cmdq(data_array, 1, 0);
}
#endif

#if 0
static void lcm_setbacklight_cmdq(void* handle,unsigned int level)
{
	unsigned int cmd = 0x51;
	unsigned int count =1;
#ifdef BUILD_LK
 	printf("%s,lk RM69071 backlight: level = %d\n", __func__, level);
#else
 	printk("%s, kernel RM69071 backlight: level = %d\n", __func__, level);
#endif
 // Refresh value of backlight level.

 	currentBrightness = level;
 	dsi_set_cmd_by_cmdq_dual(handle, cmd, count, &currentBrightness, 1);
} 
#endif


static void lcm_setbacklight(unsigned int level)
{
	unsigned int mapped_level = 0;
	if(level > 255)
		level = 255;

	mapped_level=level;
#ifdef BUILD_LK
 	printf("%s,lk RM69071 backlight: level = %d,mapped_level\n", __func__, level, mapped_level);
#else
 	printk("%s, kernel RM69071 backlight: level = %d,mapped_level = %d\n", __func__, level, mapped_level);
#endif	
 //	currentBrightness = mapped_level;	
	lcm_backlight_level_setting[0].para_list[0] = mapped_level;
	push_table(lcm_backlight_level_setting, sizeof(lcm_backlight_level_setting) / sizeof(struct LCM_setting_table), 1);

}
 
static unsigned int lcm_compare_id(void)
{
	
	char reg;
	int id;
	SET_RESET_PIN(0);
	MDELAY(10);
	mt_dsi_pinctrl_set(LCM_POWER_DP_NO, 1);
	MDELAY(10);
	mt_dsi_pinctrl_set(LCM_POWER_DM_NO, 1);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(20);

	reg = 0x4;
        dsi_set_cmdq_V2(0xfe, 1, &reg, 1);
	read_reg_v2(0xde,&reg,1);
	id = reg << 8;
	read_reg_v2(0xdf,&reg,1);
	id |= reg;
 #if defined(BUILD_LK)
  printf("rm68200a %s id:6907,read id = 0x%x\n", __func__, id);
 #else
  printk("rm68200a %s id:6907,read id = 0x%x\n", __func__, id);
 #endif
 	mt_dsi_pinctrl_set(LCM_POWER_DP_NO, 0);
	MDELAY(10);
	mt_dsi_pinctrl_set(LCM_POWER_DM_NO, 0);
	MDELAY(10);
	SET_RESET_PIN(0);
 return (0x6907 == id)?1:0;
}


LCM_DRIVER hct_rm69071_dsi_vdo_fhd_auo_oled_55_jf = {
	.name = "hct_rm69071_dsi_vdo_fhd_auo_oled_55_jf",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
	.compare_id     = lcm_compare_id,
#if (LCM_DSI_CMD_MODE)
	.update = lcm_update,
#endif
//	.set_backlight_cmdq = lcm_setbacklight_cmdq,
	.set_backlight = lcm_setbacklight,

};
