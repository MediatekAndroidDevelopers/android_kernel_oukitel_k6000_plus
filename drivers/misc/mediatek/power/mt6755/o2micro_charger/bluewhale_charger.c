/*****************************************************************************
* Copyright(c) O2Micro, 2013. All rights reserved.
*	
* O2Micro oz1c115 charger driver
* File: bluewhale_charger.c

* This program is free software and can be edistributed and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*	
* This Source Code Reference Design for O2MICRO oz1c115 charger access (\u201cReference Design\u201d) 
* is sole for the use of PRODUCT INTEGRATION REFERENCE ONLY, and contains confidential 
* and privileged information of O2Micro International Limited. O2Micro shall have no 
* liability to any PARTY FOR THE RELIABILITY, SERVICEABILITY FOR THE RESULT OF PRODUCT 
* INTEGRATION, or results from: (i) any modification or attempted modification of the 
* Reference Design by any party, or (ii) the combination, operation or use of the 
* Reference Design with non-O2Micro Reference Design.
*****************************************************************************/

#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/pm.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include "bluewhale_charger.h"

#ifdef O2_GAUGE_SUPPORT
#include "../o2micro_battery/parameter.h"
#endif

#ifdef MTK_MACH_SUPPORT
#include <linux/version.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,18,0))
#include <mt-plat/battery_common.h>
#include <mt-plat/battery_meter_hal.h>
#else
#include <mach/battery_common.h>
#include <mach/battery_meter_hal.h>
#endif

#endif

#define BW_ABS(a,b) ( ((a) > (b)) ? ((a) - (b)) : ((b) - (a)) )

#define	RETRY_CNT	5

//#define BATT_TEMP_SIMULATION 
//#define DYNAMIC_CV
#define IR_COMPENSATION	
#define O2_CONFIG_SOFTWARE_DPM_SUPPORT (1)

#define DBG_LEVEL KERN_ERR
#define bluewhale_dbg(fmt, args...) printk(DBG_LEVEL"[bluewhale]:"pr_fmt(fmt)"\n", ## args)
#define ilimit_dbg(fmt, args...) printk(DBG_LEVEL"[ilimit_work]:"pr_fmt(fmt)"\n", ## args)

#ifdef DYNAMIC_CV
static int dynamic_cv_enable = 100;
#endif

static struct bluewhale_platform_data  bluewhale_pdata_default = {
        .max_charger_currentmA = TARGET_NORMAL_CHG_CURRENT,
		.max_charger_voltagemV = O2_CONFIG_CV_VOLTAGE,
		.T34_charger_voltagemV = O2_CONFIG_CV_VOLTAGE,    //dangous
		.T45_charger_voltagemV = O2_CONFIG_CV_VOLTAGE,    //dangous	
		.termination_currentmA = O2_CHARGER_EOC,
		.wakeup_voltagemV = 3000,
		.wakeup_currentmA = 400,
		.recharge_voltagemV = 100,
		.min_vsys_voltagemV = 3400,
		.vbus_limit_currentmA = TARGET_NORMAL_CHG_ILIMIT,
		.max_cc_charge_time = 0,
		.max_wakeup_charge_time = 0,
		.rthm_10k = 0,			//1 for 10K, 0 for 100K thermal resistor
		.inpedance = O2_CHARGER_INPEDANCE,
};

#ifdef MTK_MACH_SUPPORT
kal_bool chargin_hw_init_done = KAL_FALSE;
#if defined(CONFIG_MTK_PUMP_EXPRESS_PLUS_SUPPORT)
extern bool mtk_pep_get_is_connect(void);
extern bool mtk_pep_get_to_check_chr_type(void);
#endif

#if defined(CONFIG_MTK_PUMP_EXPRESS_PLUS_20_SUPPORT)
extern bool mtk_pep20_get_is_connect(void);
extern bool mtk_pep20_get_to_check_chr_type(void);
#endif

#endif
static struct bluewhale_charger_info *charger_info;
static DEFINE_MUTEX(bluewhale_mutex);

static uint8_t adapter_type = CHG_ADAPTER_NULL;
#ifdef BATT_TEMP_SIMULATION 
static int batt_temp_simulation = 25;
#endif

#ifdef OTG_DELAY
static int try_enable_otg = 0;
#endif

#if !defined(O2_GAUGE_SUPPORT) && !defined(MTK_MACH_SUPPORT)
static struct power_supply *battery_psy = NULL;
#endif

static int bluewhale_init(struct bluewhale_charger_info *charger);
static void o2_stop_charge(struct bluewhale_charger_info *charger, u8 cable_out);
static void o2_start_ac_charge(struct bluewhale_charger_info *charger);
static void o2_start_usb_charge(struct bluewhale_charger_info *charger);
static int adji_get_bat_curr(void);
static int adji_get_bat_volt(void);
static int32_t adji_get_vbus_volt(void);
#ifdef IR_COMPENSATION	
int ir_cc_dis = -1;
#ifdef CONFIG_OZ115_DUAL_SUPPORT
extern int charge_cur_sum;
#endif
static int ir_compensation(struct bluewhale_charger_info *charger, int cv_val);
#endif

int m_charger_full = -1;
int cc_main = -1;

static void print_all_register(struct bluewhale_charger_info *charger);
static void print_all_charger_info(struct bluewhale_charger_info *charger);
static int bluewhale_check_vbus_status(struct bluewhale_charger_info *charger);
static void bluewhale_get_setting_data(struct bluewhale_charger_info *charger);
static int bluewhale_get_charging_status(struct bluewhale_charger_info *charger);
static inline int32_t o2_check_gauge_init_status(void);
static int bluewhale_get_vbus_current(struct bluewhale_charger_info *charger);
static int bluewhale_get_charger_current(struct bluewhale_charger_info *charger);


/*****************************************************************************
 * Description:
 *		bluewhale_read_byte 
 * Parameters:
 *		charger:	charger data
 *		index:	register index to be read
 *		*dat:	buffer to store data read back
 * Return:
 *      negative errno else a data byte received from the device.
 *****************************************************************************/
static int32_t bluewhale_read_byte(struct bluewhale_charger_info *charger, uint8_t index, uint8_t *dat)
{
	int32_t ret;
	uint8_t i;
	struct i2c_client *client = charger->client;

	for(i = 0; i < RETRY_CNT; i++){
		ret = i2c_smbus_read_byte_data(client, index);

		if(ret >= 0) break;
		else
			dev_err(&client->dev, "%s: err %d, %d times\n", __func__, ret, i);
	}
	if(i >= RETRY_CNT)
	{
		return ret;
	} 
	*dat = (uint8_t)ret;

	return ret;
}


/*****************************************************************************
 * Description:
 *		bluewhale_write_byte 
 * Parameters:
 *		charger:	charger data
 *		index:	register index to be write
 *		dat:		write data
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
static int32_t bluewhale_write_byte(struct bluewhale_charger_info *charger, uint8_t index, uint8_t dat)
{
	int32_t ret;
	uint8_t i;
	struct i2c_client *client = charger->client;
	
	for(i = 0; i < RETRY_CNT; i++){
		ret = i2c_smbus_write_byte_data(client, index, dat);
		if(ret >= 0) break;
		else
			dev_err(&client->dev, "%s: err %d, %d times\n", __func__, ret, i);
	}
	if(i >= RETRY_CNT)
	{
		return ret;
	}

	return ret;
}

static int32_t bluewhale_update_bits(struct bluewhale_charger_info *charger, uint8_t reg, uint8_t mask, uint8_t data)
{
	int32_t ret;
	uint8_t tmp;

	ret = bluewhale_read_byte(charger, reg, &tmp);
	if (ret < 0)
		return ret;

	if ((tmp & mask) != data)
	{
		tmp &= ~mask;
		tmp |= data & mask;
		return bluewhale_write_byte(charger, reg, tmp);
	}
	else
		return 0;
}

/*****************************************************************************
 * Description:
 *		bluewhale_set_min_vsys 
 * Parameters:
 *		charger:	charger data
 *		min_vsys_mv:	min sys voltage to be written
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
static int bluewhale_set_min_vsys(struct bluewhale_charger_info *charger, int min_vsys_mv)
{
	int ret = 0;
	u8 vsys_val = 0; 

	if (min_vsys_mv < 1800)
		min_vsys_mv = 1800;		//limit to 1.8V
	else if (min_vsys_mv > 3600)
		min_vsys_mv = 3600;		//limit to 3.6V

	vsys_val = min_vsys_mv / VSYS_VOLT_STEP;	//step is 200mV

	ret = bluewhale_update_bits(charger, REG_MIN_VSYS_VOLTAGE, 0x1f, vsys_val);

	return ret;
}

static int bluewhale_get_min_vsys(struct bluewhale_charger_info *charger)
{
	uint8_t data;
	int ret = 0;

	ret = bluewhale_read_byte(charger, REG_MIN_VSYS_VOLTAGE, &data);
	if (ret < 0)
		return ret;

	data &= 0x1f;
	return (data * VSYS_VOLT_STEP);	//step is 200mV
}

/*****************************************************************************
 * Description:
 *		bluewhale_set_chg_volt 
 * Parameters:
 *		charger:	charger data
 *		chgvolt_mv:	charge voltage to be written
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
static int bluewhale_set_chg_volt(struct bluewhale_charger_info *charger, int chgvolt_mv)
{
	int ret = 0;
	u8 chg_volt = 0; 

	if (chgvolt_mv < 4000)
		chgvolt_mv = 4000;		//limit to 4.0V
	else if (chgvolt_mv > 4600)
		chgvolt_mv = 4600;		//limit to 4.6V

	chg_volt = (chgvolt_mv - 4000) / CHG_VOLT_STEP;	//step is 25mV

	ret = bluewhale_update_bits(charger, REG_CHARGER_VOLTAGE, 0x1f, chg_volt);

	return ret;
}

static int bluewhale_get_chg_volt(struct bluewhale_charger_info *charger)
{
	uint8_t data;
	int ret = 0;

	ret = bluewhale_read_byte(charger, REG_CHARGER_VOLTAGE, &data);
	if (ret < 0)
		return ret;
	return (data * CHG_VOLT_STEP);	//step is 25mA
}

#ifdef DYNAMIC_CV
int select_cv(struct bluewhale_charger_info *charger, int chgvolt_mv)
{
	int vbat = 0;
	int vbus_current = 0;
	int cv = 0;
	int tmp1 = 0;
	int tmp2= 0;

	if(!bluewhale_check_vbus_status(charger) || !o2_check_gauge_init_status())
		return chgvolt_mv;

	vbat = adji_get_bat_volt();
	vbus_current = bluewhale_get_vbus_current(charger);

	vbat += dynamic_cv_enable;
	tmp1 = vbat / CHG_VOLT_STEP ;
	tmp2 = vbat % CHG_VOLT_STEP ;

	if(tmp2 > 0) tmp1 ++;

	cv = tmp1 * CHG_VOLT_STEP;

	if (chgvolt_mv < cv)
		cv = chgvolt_mv;

#if defined(IR_COMPENSATION)	
	if (adji_get_vbus_volt() >= 6000 && cv >= chgvolt_mv)
		cv = ir_compensation(charger_info, chgvolt_mv);
#endif

	if (cv < 4000)
		cv = 4000;
	else if (cv > 4600)
		cv = 4600;

	bluewhale_dbg("select CV: %d", cv);
	return cv;
}
#endif

/*****************************************************************************
 * Description:
 *		bluewhale_set_chg_volt_extern 
 * Parameters:
 *		chgvolt_mv:	charge voltage to be written
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
int bluewhale_set_chg_volt_extern(int chgvolt_mv)
{
	int ret = 0;

	mutex_lock(&bluewhale_mutex);
	if (charger_info)
	{
		bluewhale_dbg("***********try to set %d mV\n", chgvolt_mv);
		
		if (chgvolt_mv == 4350) {
			chgvolt_mv += 25;
			bluewhale_dbg("***********modify to %d mV\n", chgvolt_mv);
		}
		
#ifdef DYNAMIC_CV
		if (dynamic_cv_enable && adji_get_vbus_volt() < 6000)
			chgvolt_mv = select_cv(charger_info, chgvolt_mv);
#endif
#if defined(IR_COMPENSATION)	
		if (adji_get_vbus_volt() >= 6000)
			chgvolt_mv = ir_compensation(charger_info, chgvolt_mv);
		else
			ir_cc_dis = 0;
#endif
		ret = bluewhale_set_chg_volt(charger_info, chgvolt_mv);

		bluewhale_dbg("***********finale CV:%d mV\n", bluewhale_get_chg_volt(charger_info));
	}
	else
		ret = -EINVAL;
	mutex_unlock(&bluewhale_mutex);
	return ret;
}
EXPORT_SYMBOL(bluewhale_set_chg_volt_extern);

int bluewhale_get_chg_volt_extern(void)
{
	int ret = 0;

	mutex_lock(&bluewhale_mutex);
	if (charger_info)
	{
		ret = bluewhale_get_chg_volt(charger_info);
	}
	else
		ret = -EINVAL;
	mutex_unlock(&bluewhale_mutex);
	return ret;
}
EXPORT_SYMBOL(bluewhale_get_chg_volt_extern);

/*****************************************************************************
 * Description:
 *		bluewhale_set_t34_cv
 * Parameters:
 *		charger:	charger data
 *		chgvolt_mv:	charge voltage to be written at t34
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
static int bluewhale_set_t34_cv(struct bluewhale_charger_info *charger, int chgvolt_mv)
{
	int ret = 0;
	u8 chg_volt = 0; 

	if (chgvolt_mv < 4000)
		chgvolt_mv = 4000;		//limit to 4.0V
	else if (chgvolt_mv > 4600)
		chgvolt_mv = 4600;		//limit to 4.6V

	chg_volt = (chgvolt_mv - 4000) / CHG_VOLT_STEP;	//step is 25mV

	ret = bluewhale_update_bits(charger, REG_T34_CHG_VOLTAGE, 0x1f, chg_volt);

	return ret;
}

/*****************************************************************************
 * Description:
 *		bluewhale_set_t45_cv 
 * Parameters:
 *		charger:	charger data
 *		chgvolt_mv:	charge voltage to be written at t45
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
static int bluewhale_set_t45_cv(struct bluewhale_charger_info *charger, int chgvolt_mv)
{
	int ret = 0;
	u8 chg_volt = 0; 

	if (chgvolt_mv < 4000)
		chgvolt_mv = 4000;		//limit to 4.0V
	else if (chgvolt_mv > 4600)
		chgvolt_mv = 4600;		//limit to 4.6V

	chg_volt = (chgvolt_mv - 4000) / CHG_VOLT_STEP;	//step is 25mV

	ret = bluewhale_update_bits(charger, REG_T45_CHG_VOLTAGE, 0x1f, chg_volt);

	return ret;
}

/*****************************************************************************
 * Description:
 *		bluewhale_set_wakeup_volt 
 * Parameters:
 *		charger:	charger data
 *		wakeup_mv:	set wake up voltage
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
static int bluewhale_set_wakeup_volt(struct bluewhale_charger_info *charger, int wakeup_mv)
{
	int ret = 0;
	u8 data = 0;
	u8 wak_volt = 0; 

	if (wakeup_mv < 1500)
		wakeup_mv = 1500;		//limit to 1.5V
	else if (wakeup_mv > 3000)
		wakeup_mv = 3000;		//limit to 3.0V

	wak_volt = wakeup_mv / WK_VOLT_STEP;	//step is 100mV

	ret = bluewhale_read_byte(charger, REG_WAKEUP_VOLTAGE, &data);
	if (ret < 0)
		return ret;
	if (data != wak_volt) 
		ret = bluewhale_write_byte(charger, REG_WAKEUP_VOLTAGE, wak_volt);	
	return ret;
}

/*****************************************************************************
 * Description:
 *		bluewhale_set_eoc_current 
 * Parameters:
 *		charger:	charger data
 *		eoc_ma:	set end of charge current
 *		Only 0mA, 20mA, 40mA, ... , 320mA can be accurate
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
int bluewhale_set_eoc_current(struct bluewhale_charger_info *charger, int eoc_ma)
{
	int ret = 0;
	u8 data = 0;
	u8 eoc_curr = 0; 

	//Notice: with 00h, end of charge function function is disabled
	if (eoc_ma <= 0)
		eoc_ma = 0;		//min value is 0mA
	else if (eoc_ma > 320)
		eoc_ma = 320;		//limit to 320mA

	eoc_curr = eoc_ma / EOC_CURRT_STEP;	//step is 10mA

	ret = bluewhale_read_byte(charger, REG_END_CHARGE_CURRENT, &data);
	if (ret < 0)
		return ret;
	if (data != eoc_curr) 
		ret = bluewhale_write_byte(charger, REG_END_CHARGE_CURRENT, eoc_curr);	
	return ret;
}

static int bluewhale_get_eoc_current(struct bluewhale_charger_info *charger)
{
	uint8_t data;
	int ret = 0;

	ret = bluewhale_read_byte(charger, REG_END_CHARGE_CURRENT, &data);
	if (ret < 0)
		return ret;
	return (data * EOC_CURRT_STEP);	//step is 10mA
}

/*****************************************************************************
 * Description:
 *		bluewhale_set_eoc_current_extern 
 * Parameters:
 *		eoc_ma:	set end of charge current
 *		Only 0mA, 20mA, 40mA, ... , 320mA can be accurate
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
int bluewhale_set_eoc_current_extern(int eoc_ma)
{
	int ret = 0;

	mutex_lock(&bluewhale_mutex);
	if (charger_info)
		ret = bluewhale_set_eoc_current(charger_info, eoc_ma);
	else
		ret = -EINVAL;
	mutex_unlock(&bluewhale_mutex);
	return ret;
}
EXPORT_SYMBOL(bluewhale_set_eoc_current_extern);

/*****************************************************************************
 * Description:
 *		bluewhale_set_vbus_current
 * Parameters:
 *		charger:	charger data
 *		ilmt_ma:	set input current limit
 *		Only 100mA, 500mA, 700mA, 900mA, 1000mA, 1200mA, 1400mA, 1500mA, 1700mA, 1900mA, 2000mA can be accurate
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
static int bluewhale_set_vbus_current(struct bluewhale_charger_info *charger, int ilmt_ma)
{
	int ret = 0;
	u8 data = 0;
	u8 input_curr = 0;

	bluewhale_dbg("%s: try to set %d mA", __func__, ilmt_ma);
    
	if(ilmt_ma < 300)
		input_curr = 0x01;		//100mA
	else if(ilmt_ma >= 300 && ilmt_ma < 600)
		input_curr = 0x05;		//500mA
	else if(ilmt_ma >= 600 && ilmt_ma < 800)
		input_curr = 0x0c;	//700mA
	else if(ilmt_ma >= 800 && ilmt_ma < 950)
		input_curr = 0x09;	//900mA
	else if(ilmt_ma >= 950 && ilmt_ma < 1100)
		input_curr = 0x10;	//1000mA
	else if(ilmt_ma >= 1100 && ilmt_ma < 1300)
		input_curr = 0x12;	//1200mA
	else if(ilmt_ma >= 1300 && ilmt_ma < 1450)
		input_curr = 0x0e;	//1400mA
	else if(ilmt_ma >= 1450 && ilmt_ma < 1600)
		input_curr = 0x0f;	//1500mA
	else if(ilmt_ma >= 1600 && ilmt_ma < 1800)
		input_curr = 0x11;	//1700mA
	else if(ilmt_ma >= 1800 && ilmt_ma < 1950)
		input_curr = 0x13;	//1900mA
	else if(ilmt_ma >= 1950)
		input_curr = 0x14;	//2000mA

	ret = bluewhale_read_byte(charger, REG_VBUS_LIMIT_CURRENT, &data);
	if (ret < 0)
		return ret;
		
	if (data != input_curr) 
		ret = bluewhale_write_byte(charger, REG_VBUS_LIMIT_CURRENT, input_curr);

	bluewhale_dbg("%s: finale set %d mA",
			__func__, bluewhale_get_vbus_current(charger));
	return ret;
}



/*****************************************************************************
 * Description:
 *		bluewhale_get_vbus_current
 * Parameters:
 *		charger:	charger data
 * Return:
 *		Vbus input current in mA
 *****************************************************************************/
static int bluewhale_get_vbus_current(struct bluewhale_charger_info *charger)
{
	int ret = 0;
	u8 data = 0;

	ret = bluewhale_read_byte(charger, REG_VBUS_LIMIT_CURRENT, &data);
	if (ret < 0)
		return ret;

	switch(data)
	{
		case 0x00: return 0;
		case 0x01: ;
		case 0x02: ;
		case 0x03: return 100;
		case 0x04: ;
		case 0x05: ;
		case 0x06: ;
		case 0x07: return 500;
		case 0x08: ;
		case 0x09: ;
		case 0x0a: ;
		case 0x0b: return 900;
		case 0x0c: ;
		case 0x0d: return 700;
		case 0x0e: return 1400;
		case 0x0f: return 1500;
		case 0x10: return 1000;
		case 0x11: return 1700;
		case 0x12: return 1200;
		case 0x13: return 1900;
		case 0x14: return 2000;
		default:
				   return -1;
	}
}

/*****************************************************************************
 * Description:
 *		bluewhale_get_vbus_current_extern
 * Return:
 *		Vbus input current in mA
 *****************************************************************************/
int bluewhale_get_vbus_current_extern(void)
{
	int ret = 0;

	mutex_lock(&bluewhale_mutex);
	if (charger_info)
		ret = bluewhale_get_vbus_current(charger_info);
	else
		ret = -EINVAL;
	mutex_unlock(&bluewhale_mutex);
	return ret;
}
EXPORT_SYMBOL(bluewhale_get_vbus_current_extern);

/*****************************************************************************
 * Description:
 *		bluewhale_set_rechg_hystersis
 * Parameters:
 *		charger:	charger data
 *		hyst_mv:	set Recharge hysteresis Register
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
static int bluewhale_set_rechg_hystersis(struct bluewhale_charger_info *charger, int hyst_mv)
{
	int ret = 0;
	u8 data = 0;
	u8 rechg = 0; 
		
	if (hyst_mv > 200) {
		hyst_mv = 200;			//limit to 200mV
	}
	//Notice: with 00h, recharge function is disabled

	rechg = hyst_mv / RECHG_VOLT_STEP;	//step is 50mV
	
	ret = bluewhale_read_byte(charger, REG_RECHARGE_HYSTERESIS, &data);
	if (ret < 0)
		return ret;
	if (data != rechg) 
		ret = bluewhale_write_byte(charger, REG_RECHARGE_HYSTERESIS, rechg);	
	return ret;
}

/*****************************************************************************
 * Description:
 *		bluewhale_set_charger_current
 * Parameters:
 *		charger:	charger data
 *		chg_ma:	set charger current
 *		Only 600mA, 800mA, 1000mA, ... , 3800mA, 4000mA can be accurate
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
static int bluewhale_set_charger_current(struct bluewhale_charger_info *charger, int chg_ma)
{
	int ret = 0;
	u8 data = 0;
	u8 chg_curr = 0; 
	if (chg_ma > 4000)
		chg_ma = 4000;		//limit to 4A		
	else if (chg_ma < 600 && chg_ma > 0)
		chg_ma = 600;		//limit to 600 mA		

	//notice: chg_curr value less than 06h, charger will be disabled.
	//charger can power system in this case.

	chg_curr = chg_ma / CHG_CURRT_STEP;	//step is 100mA

	ret = bluewhale_read_byte(charger, REG_CHARGE_CURRENT, &data);
	if (ret < 0)
		return ret;
	if (data != chg_curr) 
		ret = bluewhale_write_byte(charger, REG_CHARGE_CURRENT, chg_curr);	

	return ret;
}

static int bluewhale_get_charger_current(struct bluewhale_charger_info *charger)
{
	uint8_t data;
	int ret = 0;

	ret = bluewhale_read_byte(charger, REG_CHARGE_CURRENT, &data);
	if (ret < 0)
		return ret;
	return (data * CHG_CURRT_STEP);	//step is 100mA
}
int bluewhale_get_charger_current_extern(void)
{
	int ret = 0;

	mutex_lock(&bluewhale_mutex);
	if (charger_info)
		ret = bluewhale_get_charger_current(charger_info);
	else
		ret = -EINVAL;
	mutex_unlock(&bluewhale_mutex);
	return ret;
}
EXPORT_SYMBOL(bluewhale_get_charger_current_extern);

/*****************************************************************************
 * Description:
 *		bluewhale_set_charger_current_extern
 * Parameters:
 *		chg_ma:	set charger current
 *		Only 600mA, 800mA, 1000mA, ... , 3800mA, 4000mA can be accurate
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
int bluewhale_set_charger_current_extern(int chg_ma)
{
	int ret = 0;

	mutex_lock(&bluewhale_mutex);
#ifdef IR_COMPENSATION	
	bluewhale_dbg("%s: cc_main: %d, ir_cc_dis:%d\n", __func__, cc_main, ir_cc_dis);
	if (chg_ma == 0) ir_cc_dis = 0;
	if (ir_cc_dis > 0 && cc_main < chg_ma) chg_ma = cc_main;
#endif
	cc_main = chg_ma;
	if (charger_info)
		ret = bluewhale_set_charger_current(charger_info, chg_ma);
	else
		ret = -EINVAL;
	mutex_unlock(&bluewhale_mutex);
	return ret;
}
EXPORT_SYMBOL(bluewhale_set_charger_current_extern);
/*****************************************************************************
 * Description:
 *		bluewhale_set_wakeup_current 
 * Parameters:
 *		charger:	charger data
 *		wak_ma:	set wakeup current
 *		Only 100mA, 120mA, ... , 400mA can be accurate
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
static int bluewhale_set_wakeup_current(struct bluewhale_charger_info *charger, int wak_ma)
{
	int ret = 0;
	u8 data = 0;
	u8 wak_curr = 0; 
	if (wak_ma < 100)
		wak_ma = 100;		//limit to 100mA
	if (wak_ma > 400)
		wak_ma = 400;		//limit to 400mA
	wak_curr = wak_ma / WK_CURRT_STEP;	//step is 10mA

	ret = bluewhale_read_byte(charger, REG_WAKEUP_CURRENT, &data);
	if (ret < 0)
		return ret;
	if (data != wak_curr) 
		ret = bluewhale_write_byte(charger, REG_WAKEUP_CURRENT, wak_curr);
	return ret;
}

/*****************************************************************************
 * Description:
 *		bluewhale_set_safety_cc_timer
 * Parameters:
 *		charger:	charger data
 *		tmin:	set safety cc charge time
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
static int bluewhale_set_safety_cc_timer(struct bluewhale_charger_info *charger, int tmin)
{
	int ret = 0;
	u8 data = 0;
	u8 tval = 0; 

	//Notice: with 0xh, saftety timer function is disabled
	if (tmin == 0) {	//disable
		tval = 0;
	}
	else if (tmin == 120) {	//120min
		tval = CC_TIMER_120MIN;
	}
	else if (tmin == 180) {	//180min
		tval = CC_TIMER_180MIN;
	}
	else if (tmin == 240) {	//240min
		tval = CC_TIMER_240MIN;
	}
	else if (tmin == 300) {	//300min
		tval = CC_TIMER_300MIN;
	}
	else if (tmin == 390) {	//300min
		tval = CC_TIMER_390MIN;
	}
	else if (tmin == 480) {	//300min
		tval = CC_TIMER_480MIN;
	}
	else if (tmin == 570) {	//300min
		tval = CC_TIMER_570MIN;
	}
	else {				//invalid values
		printk("%s: invalide value, set default value 180 minutes\n", __func__);
		tval = CC_TIMER_180MIN;	//default value
	}

	ret = bluewhale_read_byte(charger, REG_SAFETY_TIMER, &data);
	if (ret < 0)
		return ret;

	if ((data & CC_TIMER_MASK) != tval) 
	{
		data &= WAKEUP_TIMER_MASK;	//keep wk timer
		data |= tval;				//update new cc timer
		ret = bluewhale_write_byte(charger, REG_SAFETY_TIMER, data);	
	}
	return ret;
}

/*****************************************************************************
 * Description:
 *		bluewhale_set_safety_wk_timer 
 * Parameters:
 *		charger:	charger data
 *		tmin:	set safety wakeup time
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
static int bluewhale_set_safety_wk_timer(struct bluewhale_charger_info *charger, int tmin)
{
	int ret = 0;
	u8 data = 0;
	u8 tval = 0; 

	//Notice: with x0h, saftety timer function is disabled
	if (tmin == 0) {	//disable
		tval = 0;
	}
	else if (tmin == 15) {	//15min
		tval = WK_TIMER_15MIN;
	}
	else if (tmin == 30) {	//30min
		tval = WK_TIMER_30MIN;
	}
	else if (tmin == 45) {	//45min
		tval = WK_TIMER_45MIN;
	}
	else if (tmin == 60) {	//60min
		tval = WK_TIMER_60MIN;
	}
	else if (tmin == 75) {	//60min
		tval = WK_TIMER_75MIN;
	}
	else if (tmin == 90) {	//60min
		tval = WK_TIMER_90MIN;
	}
	else if (tmin == 105) {	//60min
		tval = WK_TIMER_105MIN;
	}
	else {				//invalid values
		printk("%s: invalide value, set default value 30 minutes\n", __func__);
		tval = WK_TIMER_30MIN;	//default value
	}

	ret = bluewhale_read_byte(charger, REG_SAFETY_TIMER, &data);
	if (ret < 0)
		return ret;

	if ((data & WAKEUP_TIMER_MASK) != tval) 
	{
		data &= CC_TIMER_MASK;		//keep cc timer
		data |= tval;				//update new wk timer
		ret = bluewhale_write_byte(charger, REG_SAFETY_TIMER, data);		
	} 
	return ret;
}

/*****************************************************************************
 * Description: 
 * 		bluewhale_enable_otg
 * Parameters:
 *		charger:	charger data
 *		enable:	enable/disable OTG
 *		1, 0
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
static int bluewhale_enable_otg(struct bluewhale_charger_info *charger, int enable)
{
	uint8_t data;
	uint8_t mask = 0;

    data = (enable == 0) ? 0 : 1;

	if (data) //enable otg, disable suspend
		mask = 0x06;
	else //disable otg
		mask = 0x04;

	return bluewhale_update_bits(charger, REG_CHARGER_CONTROL, mask, data << 2);
}


#ifdef OTG_DELAY

static void otg_delay_work_func(struct work_struct *work)
{
	int ret = 0;
	int vbus_volt = 0;
	struct bluewhale_charger_info *charger = container_of(work,
			struct bluewhale_charger_info, otg_delay_work.work);

	if (!try_enable_otg) {
		ret = bluewhale_enable_otg(charger, 0);
		//disable suspend mode
		bluewhale_update_bits(charger, REG_CHARGER_CONTROL, 0x02, 0 << 1);
#ifdef CONFIG_OZ115_DUAL_SUPPORT
		//suspend sub-oz115; disable otg of sub-oz115
		oz115_sub_stop_charge_extern(0);
		oz115_sub_enable_otg_extern(0);
#endif
		return; //otg cable is out, end  loop
	}

	//check vbus voltage to see if otg ocp
	vbus_volt = adji_get_vbus_volt();
	if (vbus_volt < 4500) {
		bluewhale_dbg("%s: OTG ocp, Vbus: %d", __func__, vbus_volt);
#if defined(CONFIG_OZ115_DUAL_SUPPORT) && !defined(OTG_EN_GPIO)
		//NA6: solution 4
		//enable sub-oz115 otg
		oz115_sub_enable_otg_extern(1);
		udelay(200);//usleep_range(200, 300);

		//disable main charger otg 
		bluewhale_enable_otg(charger, 0);
		//usleep_range(200, 300);
		if (try_enable_otg)
			udelay(200);
		//then if need bigger OTG current, enable main charger otg
#endif

#if defined(OTG_EN_GPIO)
		//NA6: solution 2
		if(charger->otg_en_gpio >= 0 && charger->otg_en_desc){
			//reset otg
			bluewhale_dbg("%s: otg_en_gpio:%d", __func__,
					gpio_get_value(charger->otg_en_gpio));

			gpiod_set_value(charger->otg_en_desc, 1);

			bluewhale_dbg("%s: otg_en_gpio:%d", __func__,
					gpio_get_value(charger->otg_en_gpio));

			udelay(40);
			gpiod_set_value(charger->otg_en_desc, 0);

			bluewhale_dbg("%s: otg_en_gpio:%d", __func__,
					gpio_get_value(charger->otg_en_gpio));

			udelay(450);

			//enable otg
			bluewhale_enable_otg(charger, 1);

			if (try_enable_otg)
				udelay(200);
		}//NA6: solution 2
#endif

		vbus_volt = adji_get_vbus_volt();

		if (adji_get_vbus_volt() < 4500) {
			bluewhale_dbg("%s: fail to fix OTG ocp, Vbus: %d", __func__, vbus_volt);
		}
		else
			bluewhale_dbg("%s: fix OTG ocp, Vbus: %d", __func__, vbus_volt);
	}

	if (try_enable_otg)
		schedule_delayed_work(&charger->otg_delay_work, msecs_to_jiffies(2000));
}
#endif
/*****************************************************************************
 * Description:
 *		bluewhale_enable_otg_extern
 * Parameters:
 *		enable:	enable/disable OTG
 *		1, 0
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
int bluewhale_enable_otg_extern(int enable)
{
	int ret = 0;

	mutex_lock(&bluewhale_mutex);
	if (charger_info) {
#ifdef OTG_DELAY
		try_enable_otg = enable;

		bluewhale_dbg("%s: enable:%d", __func__, enable);

#if defined(CONFIG_OZ115_DUAL_SUPPORT) && !defined(OTG_EN_GPIO)
		//suspend sub-oz115; disable otg of sub-oz115
		oz115_sub_stop_charge_extern(0);
		oz115_sub_enable_otg_extern(0);
#endif
		if (!enable) {
			cancel_delayed_work_sync(&charger_info->otg_delay_work);

#ifdef CONFIG_OZ115_DUAL_SUPPORT
			//suspend sub-oz115; disable otg of sub-oz115
			oz115_sub_stop_charge_extern(0);
			oz115_sub_enable_otg_extern(0);
#endif
			ret = bluewhale_enable_otg(charger_info, 0);
			//disable suspend mode
			bluewhale_update_bits(charger_info, REG_CHARGER_CONTROL, 0x02, 0 << 1);
		}
		else {

#if defined(CONFIG_OZ115_DUAL_SUPPORT) && !defined(OTG_EN_GPIO)
			//AN6: solution 4
			//enable suspend mode
			bluewhale_update_bits(charger_info, REG_CHARGER_CONTROL, 0x02, 1 << 1);
#endif

			ret = bluewhale_enable_otg(charger_info, 1);

			schedule_delayed_work(&charger_info->otg_delay_work, 0);
		}
#else
 		ret = bluewhale_enable_otg(charger_info, enable);
#endif
    }
 	else
 		ret = -EINVAL;

	mutex_unlock(&bluewhale_mutex);
	return ret;
}
EXPORT_SYMBOL(bluewhale_enable_otg_extern);

void bluewhale_set_ta_current_pattern_extern(unsigned int increase)
{
	mutex_lock(&bluewhale_mutex);

	if (!charger_info){
		goto end;
	}
	//bluewhale_set_chg_volt(charger_info, O2_CONFIG_CV_VOLTAGE);
	bluewhale_set_vbus_current(charger_info, 500);
	bluewhale_set_charger_current(charger_info, 2000);

    //disable suspend mode
	bluewhale_update_bits(charger_info, REG_CHARGER_CONTROL, 0x02, 0 << 1);

	msleep(100);

	if(increase)
	{
		bluewhale_set_vbus_current(charger_info, 100); /* 100mA */
		mdelay(85);
		pr_err("mtk_ta_increase() start\n");

		bluewhale_set_vbus_current(charger_info, 500); /* 500mA */
		pr_err("mtk_ta_increase() on 1");
		mdelay(85);

		bluewhale_set_vbus_current(charger_info, 100); /* 100mA */
		pr_err("mtk_ta_increase() off 1");
		mdelay(85);

		bluewhale_set_vbus_current(charger_info, 500); /* 500mA */
		pr_err("mtk_ta_increase() on 2");
		mdelay(85);

		bluewhale_set_vbus_current(charger_info, 100); /* 100mA */
		pr_err("mtk_ta_increase() off 2");
		mdelay(85);

		bluewhale_set_vbus_current(charger_info, 500); /* 500mA */
		pr_err("mtk_ta_increase() on 3");
		mdelay(281);

		bluewhale_set_vbus_current(charger_info, 100); /* 100mA */
		pr_err("mtk_ta_increase() off 3");
		mdelay(85);

		bluewhale_set_vbus_current(charger_info, 500); /* 500mA */
		pr_err("mtk_ta_increase() on 4");
		mdelay(281);

		bluewhale_set_vbus_current(charger_info, 100); /* 100mA */
		pr_err("mtk_ta_increase() off 4");
		mdelay(85);

		bluewhale_set_vbus_current(charger_info, 500); /* 500mA */
		pr_err("mtk_ta_increase() on 5");
		mdelay(281);

		bluewhale_set_vbus_current(charger_info, 100); /* 100mA */
		pr_err("mtk_ta_increase() off 5");
		mdelay(85);

		bluewhale_set_vbus_current(charger_info, 500); /* 500mA */
		pr_err("mtk_ta_increase() on 6");
		mdelay(440);

		bluewhale_set_vbus_current(charger_info, 100); /* 100mA */
		pr_err("mtk_ta_increase() off 6");
		mdelay(50);

		pr_notice("mtk_ta_increase() end \n");

		bluewhale_set_vbus_current(charger_info, 500); /* 500mA */
		mdelay(200);
	}
	else
	{
		bluewhale_set_vbus_current(charger_info, 100); /* 100mA */
		msleep(85);

		pr_err("mtk_ta_decrease() start\n");

		bluewhale_set_vbus_current(charger_info, 500); /* 500mA */
		pr_err("mtk_ta_decrease() on 1");
		mdelay(281);

		bluewhale_set_vbus_current(charger_info, 100); /* 100mA */
		pr_err("mtk_ta_decrease() off 1");
		mdelay(85);

		bluewhale_set_vbus_current(charger_info, 500); /* 500mA */
		pr_err("mtk_ta_decrease() on 2");
		mdelay(281);

		bluewhale_set_vbus_current(charger_info, 100); /* 100mA */
		pr_err("mtk_ta_decrease() off 2");
		mdelay(85);

		bluewhale_set_vbus_current(charger_info, 500); /* 500mA */
		pr_err("mtk_ta_decrease() on 3");
		mdelay(281);

		bluewhale_set_vbus_current(charger_info, 100); /* 100mA */
		pr_err("mtk_ta_decrease() off 3");
		mdelay(85);

		bluewhale_set_vbus_current(charger_info, 500); /* 500mA */
		pr_err("mtk_ta_decrease() on 4");
		mdelay(85);

		bluewhale_set_vbus_current(charger_info, 100); /* 100mA */
		pr_err("mtk_ta_decrease() off 4");
		mdelay(85);

		bluewhale_set_vbus_current(charger_info, 500); /* 500mA */
		pr_err("mtk_ta_decrease() on 5");
		mdelay(85);

		bluewhale_set_vbus_current(charger_info, 100); /* 100mA */
		pr_err("mtk_ta_decrease() off 5");
		mdelay(85);

		bluewhale_set_vbus_current(charger_info, 500); /* 500mA */
		pr_err("mtk_ta_decrease() on 6");
		mdelay(485);

		bluewhale_set_vbus_current(charger_info, 100); /* 100mA */
		pr_err("mtk_ta_decrease() off 6");
		mdelay(50);

		pr_notice("mtk_ta_decrease() end \n");

		bluewhale_set_vbus_current(charger_info, 500); /* 500mA */
	}

	print_all_register(charger_info);

end:
	mutex_unlock(&bluewhale_mutex);
}
EXPORT_SYMBOL(bluewhale_set_ta_current_pattern_extern);

#ifdef CONFIG_MTK_PUMP_EXPRESS_PLUS_20_SUPPORT
unsigned int bluewhale_set_ta20_reset_extern(void)
{
	mutex_lock(&bluewhale_mutex);

	if (!charger_info){
		goto end;
	}

	bluewhale_set_charger_current(charger_info, 2000); //cc:512mA

	bluewhale_set_vbus_current(charger_info, 100);
	msleep(250);
	bluewhale_set_vbus_current(charger_info, 500);

end:
	mutex_unlock(&bluewhale_mutex);
	return 0;
}
EXPORT_SYMBOL(bluewhale_set_ta20_reset_extern);

struct timespec ptime[13];
static int cptime[13][2];

static int dtime(int i)
{
	struct timespec time;

	time = timespec_sub(ptime[i], ptime[i-1]);
	return time.tv_nsec/1000000;
}

#define PEOFFTIME 40
#define PEONTIME 90
#define STATUS_OK	0
#define STATUS_FAIL	1

unsigned int bluewhale_set_ta20_current_pattern_extern(int chr_vol)
{
	int value;
	int i, j = 0;
	int flag;
	unsigned int ret = STATUS_OK;

	mutex_lock(&bluewhale_mutex);

	if (!charger_info){
		goto end;
	}

	bluewhale_set_charger_current(charger_info, 2000); //cc:512mA
    //disable suspend mode
	bluewhale_update_bits(charger_info, REG_CHARGER_CONTROL, 0x02, 0 << 1);

	usleep_range(1000, 1200);
	value = ((CHR_VOLTAGE_ENUM)chr_vol - CHR_VOLT_05_500000_V) / CHR_VOLT_00_500000_V;

	bluewhale_set_vbus_current(charger_info, 100);
	msleep(70);

	get_monotonic_boottime(&ptime[j++]);
	for (i = 4; i >= 0; i--) {
		flag = value & (1 << i);

		if (flag == 0) {//bit[i] = 0
			bluewhale_set_vbus_current(charger_info, 500);
			mdelay(PEOFFTIME);
			get_monotonic_boottime(&ptime[j]);
			cptime[j][0] = PEOFFTIME;
			cptime[j][1] = dtime(j);
			if (cptime[j][1] < 30 || cptime[j][1] > 65) {
				battery_log(BAT_LOG_CRTI,
					"charging_set_ta20_current_pattern fail1: idx:%d target:%d actual:%d\n",
					i, PEOFFTIME, cptime[j][1]);
				ret = STATUS_FAIL;
		        goto end;
			}
			j++;
			bluewhale_set_vbus_current(charger_info, 100);
			mdelay(PEONTIME);
			get_monotonic_boottime(&ptime[j]);
			cptime[j][0] = PEONTIME;
			cptime[j][1] = dtime(j);
			if (cptime[j][1] < 90 || cptime[j][1] > 115) {
				battery_log(BAT_LOG_CRTI,
					"charging_set_ta20_current_pattern fail2: idx:%d target:%d actual:%d\n",
					i, PEOFFTIME, cptime[j][1]);
				ret = STATUS_FAIL;
		        goto end;
			}
			j++;

		} else { //bit[i] = 1
			bluewhale_set_vbus_current(charger_info, 500);
			mdelay(PEONTIME);
			get_monotonic_boottime(&ptime[j]);
			cptime[j][0] = PEONTIME;
			cptime[j][1] = dtime(j);
			if (cptime[j][1] < 90 || cptime[j][1] > 115) {
				battery_log(BAT_LOG_CRTI,
					"charging_set_ta20_current_pattern fail3: idx:%d target:%d actual:%d\n",
					i, PEOFFTIME, cptime[j][1]);
				ret = STATUS_FAIL;
		        goto end;
			}
			j++;
			bluewhale_set_vbus_current(charger_info, 100);
			mdelay(PEOFFTIME);
			get_monotonic_boottime(&ptime[j]);
			cptime[j][0] = PEOFFTIME;
			cptime[j][1] = dtime(j);
			if (cptime[j][1] < 30 || cptime[j][1] > 65) {
				battery_log(BAT_LOG_CRTI,
					"charging_set_ta20_current_pattern fail4: idx:%d target:%d actual:%d\n",
					i, PEOFFTIME, cptime[j][1]);
				ret = STATUS_FAIL;
		        goto end;
			}
			j++;
		}
	}

	bluewhale_set_vbus_current(charger_info, 500);
	msleep(160);
	get_monotonic_boottime(&ptime[j]);
	cptime[j][0] = 160;
	cptime[j][1] = dtime(j);
	if (cptime[j][1] < 150 || cptime[j][1] > 240) {
		battery_log(BAT_LOG_CRTI,
			"charging_set_ta20_current_pattern fail5: idx:%d target:%d actual:%d\n",
			i, PEOFFTIME, cptime[j][1]);
		ret = STATUS_FAIL;
		goto end;
	}
	j++;

	bluewhale_set_vbus_current(charger_info, 100);
	msleep(30);
	bluewhale_set_vbus_current(charger_info, 500);

	battery_log(BAT_LOG_CRTI,
	"[charging_set_ta20_current_pattern]:chr_vol:%d bit:%d time:%3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d!!\n",
	chr_vol, value,
	cptime[1][0], cptime[2][0], cptime[3][0], cptime[4][0], cptime[5][0],
	cptime[6][0], cptime[7][0], cptime[8][0], cptime[9][0], cptime[10][0], cptime[11][0]);

	battery_log(BAT_LOG_CRTI,
	"[charging_set_ta20_current_pattern2]:chr_vol:%d bit:%d time:%3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d!!\n",
	chr_vol, value,
	cptime[1][1], cptime[2][1], cptime[3][1], cptime[4][1], cptime[5][1],
	cptime[6][1], cptime[7][1], cptime[8][1], cptime[9][1], cptime[10][1], cptime[11][1]);


	bluewhale_set_vbus_current(charger_info, 500);

end:
	mutex_unlock(&bluewhale_mutex);
	return ret;
}
EXPORT_SYMBOL(bluewhale_set_ta20_current_pattern_extern);
#endif
/*****************************************************************************
 * Description:
 *		adji_get_vbus_volt
 * Parameters:
 *		void
 * Return:
 *		vbus voltage, or negative errno
 *****************************************************************************/
static int32_t adji_get_vbus_volt(void)
{

	int32_t vbus_voltage = 0;
	
#if defined(O2_GAUGE_SUPPORT) && defined(VBUS_COLLECT_BY_O2)

	if(oz8806_get_init_status())
	{
		vbus_voltage = oz8806_vbus_voltage();
	}
#elif defined(MTK_MACH_SUPPORT)
	int ret = 0;
	int val = 0;

	ret = bm_ctrl_cmd(BATTERY_METER_CMD_GET_ADC_V_CHARGER, &val);
	vbus_voltage = val;
#else
	vbus_voltage = -1;
	bluewhale_dbg("no ADC to measure vbus voltage");
#endif
	
	//bluewhale_dbg("vbus voltage:%d", vbus_voltage);
	return vbus_voltage;
}

int o2_read_vbus_voltage_extern(void)
{
	int ret = 0;

	mutex_lock(&bluewhale_mutex);
	if (charger_info)
		ret = adji_get_vbus_volt();
	else
		ret = -EINVAL;
	mutex_unlock(&bluewhale_mutex);
	return ret;
}
EXPORT_SYMBOL(o2_read_vbus_voltage_extern);


static inline int32_t o2_check_gauge_init_status(void)
{
#if defined O2_GAUGE_SUPPORT
	return oz8806_get_init_status();
#else
	return 1;
#endif
}

static int adji_get_bat_volt(void)
{
#if defined(O2_GAUGE_SUPPORT)
	return oz8806_get_battery_voltage(); //mV

#elif defined(MTK_MACH_SUPPORT)
	int val = 5;
	int ret = 0;
	ret = bm_ctrl_cmd(BATTERY_METER_CMD_GET_ADC_V_BAT_SENSE, &val);
	return val;

#else
	union power_supply_propval val;

	if (!battery_psy)
		battery_psy = power_supply_get_by_name ("battery"); 

	if (battery_psy 
		&& !battery_psy->get_property (battery_psy, POWER_SUPPLY_PROP_VOLTAGE_NOW, &val))
	{
		return (val.intval / 1000);
	}
	else
	{
		pr_err("can't get POWER_SUPPLY_PROP_VOLTAGE_NOW\n");
		return -1;
	}
#endif
}

static int adji_get_bat_curr(void)
{
#if defined(O2_GAUGE_SUPPORT)
	return oz8806_get_battry_current(); //mA
#elif defined(MTK_MACH_SUPPORT)
	int cur = 0;
	int val = 0;
	int ret = 0;

	ret = bm_ctrl_cmd(BATTERY_METER_CMD_GET_HW_FG_CURRENT, &cur);
	cur = cur/ 10;

	ret = bm_ctrl_cmd(BATTERY_METER_CMD_GET_HW_FG_CURRENT_SIGN, &val);
	if (val)
		return cur;
	else
		return 0 - cur;
#else
	union power_supply_propval val;

	if (!battery_psy)
		battery_psy = power_supply_get_by_name ("battery"); 

	if (battery_psy 
		&& !battery_psy->get_property (battery_psy, POWER_SUPPLY_PROP_CURRENT_NOW, &val))
	{
		return (val.intval / 1000);
	}
	else
	{
		pr_err("can't get POWER_SUPPLY_PROP_CURRENT_NOW\n");
		return -1;
	}
#endif
}



#if O2_CONFIG_SOFTWARE_DPM_SUPPORT

// SDPM internal optimizing mechanism
#define O2_CONFIG_ILIMIT_NTAG_SUPPORT		0
#define O2_CONFIG_ILIMIT_YTAG_SUPPORT		0
#define O2_CONFIG_TRUSTED_VBUS_SUPPORT		1	


// SDPM default parameter setting
#define ADJI_SDPM_ILIMIT_THRESHOLD 	(700)
#define ADJI_SDPM_ILIMIT_HIGH 		(1700)

#define ADJI_ILELS_NUM 			(10)
#define ADJI_CHRENV_RTNUM 		(2)

#define ADJI_JUDG_VBUSVOLT_LOWTH 	(4400)
#define ADJI_JUDG_SOPDISFUNC_IBATTH 	(-300)
#define ADJI_JUDG_IENOUGH_IBATTH 	(50)
#define ADJI_JUDG_AD_OSC_IBATTH		(400)
#define ADJI_JUDG_AD_OSC_VBUSTH		(300)


#ifdef O2_GAUGE_SUPPORT
#define ADJI_AVG_INFO_ARRAY_SIZE	(1)
#define ADJI_AVG_INFO_WAIT_TIME		(1360)
#define ADJI_AVG_INFO_CHECK_INTERVAL	(20)
#define ADJI_AVG_INFO_FIRST_DELAY	(1260)
#if O2_CONFIG_TRUSTED_VBUS_SUPPORT
#error, "DO NOT using trusted-vbus-mechanism when oz8806 is used!"
#endif
#else
#define ADJI_AVG_INFO_ARRAY_SIZE	(20)
#define ADJI_AVG_INFO_WAIT_TIME		(600)
#define ADJI_AVG_INFO_CHECK_INTERVAL	(30)
#define ADJI_AVG_INFO_FIRST_DELAY	(300)
#endif

#if O2_CONFIG_TRUSTED_VBUS_SUPPORT
#define ADJI_VBUS_OZ1C115C_THRESHOLD	(4570)
#define ADJI_VBUS_PER_STEP_DEVIATION	(120)
#endif

#define ADJI_SYSENV_CHANGE_IBATTH	(200)
#define ADJI_SYSENV_CHANGE_VBUSTH	(100)

#define ADJI_SYSENV_BETTER_IBATTH_IL	(200)
#define ADJI_SYSENV_BETTER_IBATTH_IM	(300)
#define ADJI_SYSENV_BETTER_IBATTH_IH	(500)

#define BAT_RES 0

enum {
	ADJI_RET_VBUSLOW,
	ADJI_RET_IENOUGH,
	ADJI_RET_SOPDISFUNC,
	ADJI_RET_HIGHEST
} adji_ret, adji_ret_buffer ;

enum {
	ADJI_CHECK_ILIMIT_RET_OK,
	ADJI_CHECK_ILIMIT_RET_SOP,
	ADJI_CHECK_ILIMIT_RET_LOWVOL,
	ADJI_CHECK_ILIMIT_RET_LOWCUR,
	ADJI_CHECK_ILIMIT_RET_EVB,
	ADJI_CHECK_ILIMIT_RET_EVW,
	ADJI_CHECK_ILIMIT_RET_HIGHVOL,
	ADJI_CHECK_ILIMIT_RET_ADDISFUNC
} adji_check_ilimit_ret ;

struct charger_sysenv {
	int32_t vbus_volt ;
	int32_t	vbat ;
	int32_t ibat ;
} ;

static int32_t prevcc = 0 ;
static int32_t dilimit = 0 ;
static struct charger_sysenv adji_chrenv_momt  ;
static struct charger_sysenv adji_chrenv_adjmomt  ;
static struct charger_sysenv adji_chrenv_rt[ADJI_CHRENV_RTNUM] ;
static uint8_t adji_chrenv_rtpos = 0 ;
static uint8_t adji_chrenv_rtpos_rewind = 0 ;

static int32_t adji_ilel_array[ADJI_ILELS_NUM] = {500,700,900,1000,1200,1400,1500,1700,1900,2000} ;
static int32_t adji_bat_curr[ADJI_ILELS_NUM] = {0,0,0,0,0,0,0,0,0,0} ;
static int32_t adji_ilimit_buffer = 0 ;

#if O2_CONFIG_TRUSTED_VBUS_SUPPORT
static int32_t adji_vbus[ADJI_ILELS_NUM] = {0,0,0,0,0,0,0,0,0,0} ;
#endif

#if O2_CONFIG_ILIMIT_NTAG_SUPPORT
#define ADJI_NTAG_CNT_NUM	(5)
static int32_t adji_ilel_ntag[ADJI_ILELS_NUM] = {0,0,0,0,0,0,0,0,0,0} ;
static int32_t adji_ilel_ntag_judge_cnt[ADJI_ILELS_NUM] = {0,0,0,0,0,0,0,0,0,0} ;

#endif

#if O2_CONFIG_ILIMIT_YTAG_SUPPORT
#define ADJI_YTAG_CNT_NUM	(5)
#define ADJI_YTAG_IBATTH	(200)
#define ADJI_YTAG_VBATTH	(80)
#define ADJI_YTAG_VBUSTH	(ADJI_JUDG_VBUSVOLT_LOWTH)
static int32_t adji_ilel_ytag[ADJI_ILELS_NUM] = {0,0,0,0,0,0,0,0,0,0} ;
static uint8_t adji_ytag_cnt = 0 ;
#endif

static uint8_t adji_started = 0 ;



// SDPM Debugging parameter
static int32_t o2_debug_sdpm_ilimit_threshold = ADJI_SDPM_ILIMIT_THRESHOLD  ;
static int32_t o2_debug_sdpm_ilimit_high = ADJI_SDPM_ILIMIT_HIGH ;
static int32_t o2_debug_sdpm_judg_ienough_ibatth = ADJI_JUDG_IENOUGH_IBATTH ; 
static int32_t o2_debug_sdpm_judg_sopdisfunc_ibatth = ADJI_JUDG_SOPDISFUNC_IBATTH ; 
static int32_t o2_debug_sdpm_judg_adosc_ibatth = ADJI_JUDG_AD_OSC_IBATTH ; 
static int32_t o2_debug_sdpm_judg_adosc_vbusth = ADJI_JUDG_AD_OSC_VBUSTH ; 
static int32_t o2_debug_sdpm_sysenv_change_ibatth = ADJI_SYSENV_CHANGE_IBATTH ; 
static int32_t o2_debug_sdpm_sysenv_change_vbusth = ADJI_SYSENV_CHANGE_VBUSTH ; 
static int32_t o2_debug_sdpm_sysenv_better_ibatth_il = ADJI_SYSENV_BETTER_IBATTH_IL ; 
static int32_t o2_debug_sdpm_sysenv_better_ibatth_im = ADJI_SYSENV_BETTER_IBATTH_IM ; 
static int32_t o2_debug_sdpm_sysenv_better_ibatth_ih = ADJI_SYSENV_BETTER_IBATTH_IH ; 
static int32_t o2_debug_sdpm_avg_info_check_interval = ADJI_AVG_INFO_CHECK_INTERVAL ; 
static int32_t o2_debug_sdpm_avg_info_first_delay = ADJI_AVG_INFO_FIRST_DELAY ; 
static int32_t o2_debug_sdpm_avg_info_wait_time = ADJI_AVG_INFO_WAIT_TIME ; 
#if O2_CONFIG_TRUSTED_VBUS_SUPPORT
static int32_t o2_debug_sdpm_vbus_oz115c_th = ADJI_VBUS_OZ1C115C_THRESHOLD ;
static int32_t o2_debug_sdpm_vbus_step_dev = ADJI_VBUS_PER_STEP_DEVIATION ;
#endif


#if O2_CONFIG_ILIMIT_NTAG_SUPPORT
static int32_t o2_ntag_support = 1 ;
static int32_t o2_ntag_debug_cnt_num = ADJI_NTAG_CNT_NUM ;
#endif

#if O2_CONFIG_ILIMIT_YTAG_SUPPORT
static int32_t o2_ytag_support = 1 ;
static int32_t o2_ytag_debug_cnt_num = ADJI_YTAG_CNT_NUM ;
static int32_t o2_ytag_debug_ibatth = ADJI_YTAG_IBATTH ;
static int32_t o2_ytag_debug_vbatth = ADJI_YTAG_VBATTH ;
#endif

/*** platform-specific-api ***/

static void adji_bubble_sorting(int32_t *vstart, int32_t size)
{
	int32_t i;
	int32_t j;
	int32_t tmp;

	for(i=0; i<(size-1); i++) {
		for(j=0; j<(size-1-i); j++) {
			if(vstart[j] > vstart[j+1]) {
				tmp = vstart[j+1] ;
				vstart[j+1] = vstart[j];
				vstart[j] = tmp ;
			}
		}
	}
}

static int32_t adji_cal_avg_info(int32_t *vstart, int32_t size)
{
	if(size == 1)
		return vstart[0] ;

	if(size == 2)
		return ( (vstart[0] + vstart[1])/2 ) ;

	if(size >= 3) {
		int32_t i ;
		int32_t tmp=0 ;
	
		for(i=1; i<(size -1);  i++)
			tmp += vstart[i] ;

		return ( tmp/(size-2) ) ;
	}

	return 0 ;
}

static int32_t ibat_array[ADJI_AVG_INFO_ARRAY_SIZE] ;
static int32_t vbat_array[ADJI_AVG_INFO_ARRAY_SIZE] ;
static int32_t vbus_array[ADJI_AVG_INFO_ARRAY_SIZE] ;
static int32_t nsamples = 0;

static int32_t adji_get_avg_info(int32_t * ibat, int32_t * vbat, int32_t * vbus, int32_t mode)
{
	int32_t ibat_tmp ;
	int32_t vbat_tmp ;
	int32_t vbus_tmp ;
	int32_t i ;
	int32_t j ;

	unsigned long stamp_start ; 
	unsigned long stamp_current ;	
	long stamp_diff ;
	long msec_diff ;

	stamp_start = jiffies ;

	if(mode == 0) {
		ibat_tmp = adji_get_bat_curr() ;
		vbat_tmp = adji_get_bat_volt() ;
		vbus_tmp = adji_get_vbus_volt() ;
	
		//in mode 0, we delay some time
		//to wait for info to get stable	
		msleep(o2_debug_sdpm_avg_info_first_delay) ;
	}

	i = 0 ;

	do {
		msleep(o2_debug_sdpm_avg_info_check_interval) ;

		ibat_array[i] = adji_get_bat_curr() ;
		vbat_array[i] = adji_get_bat_volt() ;
		vbus_array[i] = adji_get_vbus_volt() ;
			
		if(i == 0) {

			if(mode == 0) {
				//make sure we get the new value
				if( (ibat_array[i] != ibat_tmp) && (vbus_array[i] != vbus_tmp) && (vbus_array[i] > 4000) ) {
					i++ ;
				}	

			} else {
				i++ ;
			}

		} else {
			i++ ;
		}

		stamp_current = jiffies ;
		stamp_diff = (long)stamp_current - (long)stamp_start ;
		msec_diff = stamp_diff * 1000 /HZ ;
	} while ( (i<ADJI_AVG_INFO_ARRAY_SIZE) && (msec_diff <= o2_debug_sdpm_avg_info_wait_time) ) ;

	if (i == 0)
		i = 1;

	nsamples = i ; 
	adji_bubble_sorting(&(ibat_array[0]), i);	
	adji_bubble_sorting(&(vbat_array[0]), i);	
	adji_bubble_sorting(&(vbus_array[0]), i);	

	*ibat = adji_cal_avg_info(&(ibat_array[0]), i);
	*vbat = adji_cal_avg_info(&(vbat_array[0]), i);
	*vbus = adji_cal_avg_info(&(vbus_array[0]), i);
	
	//print all vol/curr infomation
	ilimit_dbg("[%s] all ibat,vbat,vbus (total num: %d) : ", __FUNCTION__, i);
	for(j=0; j<i; j++) {
		printk("%d	%d	%d\n", ibat_array[j], vbat_array[j], vbus_array[j] );
	}
	
	ilimit_dbg("[%s] average ibat(%d\t):%d,\tvbat:%d,\tvbus:%d ", __FUNCTION__, bluewhale_get_vbus_current(charger_info), *ibat, *vbat, *vbus);

	return 0 ;
}
#if 0
static int32_t vbus_off = 0 ;
static int32_t ibat_off = 0 ;
static int32_t vbat_off = 0 ;

static int check_off_status(void)
{
	if (adji_get_bat_volt() > 3600){
		//enable suspend mode
		bluewhale_update_bits(charger_info, REG_CHARGER_CONTROL, 0x02, 0x1 << 1);
		msleep(500);
		adji_get_avg_info(&ibat_off, &vbat_off, &vbus_off, 0) ;
		ilimit_dbg("[%s] average ibat_off:%d,\tvbat_off:%d,\tvbus)ff:%d ", __FUNCTION__, ibat_off, vbat_off, vbus_off);

		//disable suspend mode
		bluewhale_update_bits(charger_info, REG_CHARGER_CONTROL, 0x02, 0x0 << 1);
	}
	return 0;
}
#endif



#if O2_CONFIG_ILIMIT_NTAG_SUPPORT

static void adji_ntag_clear_all(void)
{
	int32_t i ;

	for(i=0; i<ADJI_ILELS_NUM; i++) {
		adji_ilel_ntag[i] = 0 ;		
		adji_ilel_ntag_judge_cnt[i] = 0 ;
	}
}

static int32_t adji_ntag_get_max_ilimit(void)
{
	int32_t i ;

	//from low to high
	for(i=0; i<ADJI_ILELS_NUM; i++) {
		if(adji_ilel_ntag[i] != 0)
			break;
	}

	if(i>0)	i-- ;

	return adji_ilel_array[i] ;
}

static void adji_ntag_set_ilimit(int32_t ilimit)
{
	int32_t i ;
	
	for(i=0; i<ADJI_ILELS_NUM; i++) {
		if(ilimit <= adji_ilel_array[i])
			break;
	}

	if(i < ADJI_ILELS_NUM)
		adji_ilel_ntag[i] = 1;

}

#endif	//O2_CONFIG_ILIMIT_NTAG_SUPPORT


#if O2_CONFIG_ILIMIT_YTAG_SUPPORT

static void adji_ytag_clear_all(void)
{
	int32_t i ;

	for(i=0; i<ADJI_ILELS_NUM; i++) {
		adji_ilel_ytag[i] = 0 ;		
	}

	adji_ytag_cnt = 0 ;
}

static void adji_ytag_clear_upon(int32_t ilimit)
{
	int32_t i ;
	
	for(i=(ADJI_ILELS_NUM-1); i>=0; i--) {
		if(ilimit < adji_ilel_array[i])
			adji_ilel_ytag[i] = 0 ;
	}

}

static int32_t adji_ytag_get_min_ilimit(void)
{
	int32_t i ;

	//from high to low
	for(i=(ADJI_ILELS_NUM-1); i>=0; i--) {
		if(adji_ilel_ytag[i] != 0)
			break;
	}

	if(i<0) 
		i = 0 ;

	return adji_ilel_array[i] ;
}

static void adji_ytag_set_ilimit(int32_t ilimit)
{
	int32_t i ;
	
	for(i=0; i<ADJI_ILELS_NUM; i++) {
		if(ilimit <= adji_ilel_array[i])
			break;
	}

	if(i < ADJI_ILELS_NUM)
		adji_ilel_ytag[i] = 1;

}


#endif	//O2_CONFIG_ILIMIT_YTAG_SUPPORT


/* Note:
 * This function is very important for SDMP
 * When combined with DYNAMIC_CV, you need to 
 * be very careful since CV-setting is very close
 * to the real battery voltage.
 */
static int32_t adji_check_ilimit(int32_t ilimit, int32_t cv_val, int32_t cc_val, int32_t vbus_volt, int32_t vbat, int32_t ibat)
{
	int32_t tmp ;
	int32_t ret ;

	/*
	 * Absolute ilimit-adjusting condition
	 * Need to adjust ilimit from 500 to 2000.
	 */
	
	//ilimit-500 is found
	if(ilimit == 500) {
		ret =  ADJI_CHECK_ILIMIT_RET_SOP ;
		ilimit_dbg("[%s] ilimit-level (%d) check aci_i500_fail.", __FUNCTION__, ilimit);
		goto aci_i500_fail ;
	}

	/*
	 * Absolute ilimit-readjusting condition.
	 * These conditions indicate some unstable or unsafe states.
	 * We decide to readjust ilimit if these conditions happen for conservative considering.
	 * Note: means that we should lower ilimit level (down-adjusting).
	 */

	//vbus voltage too low.
	if(vbus_volt < ADJI_JUDG_VBUSVOLT_LOWTH) {
		ret =  ADJI_CHECK_ILIMIT_RET_LOWVOL ;
		ilimit_dbg("[%s] ilimit-level (%d) check aci_absolute_fail: vbus_volt is %d.", 
				__FUNCTION__, ilimit, vbus_volt);
		goto aci_absolute_fail ; 
	}


	//possibly, adaptor's oscilating happens when ilimit is higher than 700
	//adaptor's oscilating condition is very strict
	//this determining condition is trying to catch the behavior of adaptor's disfunction
	if(ilimit > 700) {
		if( ibat < (adji_chrenv_momt.ibat - o2_debug_sdpm_judg_adosc_ibatth) && 
			vbus_volt > (adji_chrenv_momt.vbus_volt + o2_debug_sdpm_judg_adosc_vbusth) && 
				BW_ABS(cc_val,prevcc) < 200) {

#if O2_CONFIG_ILIMIT_NTAG_SUPPORT
if(o2_ntag_support) 
{

			adji_ntag_set_ilimit(ilimit) ;
			ilimit_dbg("[%s] [NTAG] ilimit-level (%d) oscilating, ntag this level.", __FUNCTION__, ilimit);

}
#endif
			
			ret =  ADJI_CHECK_ILIMIT_RET_ADDISFUNC;
			ilimit_dbg("[%s] ilimit-level (%d) check aci_absolute_fail: oscilating.", 
					__FUNCTION__, ilimit);
			goto aci_absolute_fail ; 

		}
	}


	//battery current too small.
	if( (vbat < cv_val - 80) && (cc_val >= 600) ) {
		if(ilimit >= 1700) {

			if(ibat < 200) {
				ret = ADJI_CHECK_ILIMIT_RET_LOWCUR ;
				ilimit_dbg("[%s] ilimit-level (%d) check aci_absolute_fail: ibat is %d.", 
						__FUNCTION__, ilimit, ibat);
				goto aci_absolute_fail ; 
			}

		} else if (ilimit >= 1200) {

			if(ibat < 50) {
				ret = ADJI_CHECK_ILIMIT_RET_LOWCUR ;
				ilimit_dbg("[%s] ilimit-level (%d) check aci_absolute_fail: ibat is %d.", 
						__FUNCTION__, ilimit, ibat);
				goto aci_absolute_fail ; 

			}

		}
	}

#if O2_CONFIG_TRUSTED_VBUS_SUPPORT
	//adapter's disfunction
	if (ibat < -300 && (vbus_array[nsamples-1] - vbus_array[0] > 150) ) {
		ret = ADJI_CHECK_ILIMIT_RET_LOWCUR ;
		ilimit_dbg("[%s] ilimit-level (%d) check aci_absolute_fail: ibat is %d.", 
				__FUNCTION__, ilimit, ibat);
		goto aci_absolute_fail ; 
	}
#endif

#if O2_CONFIG_ILIMIT_YTAG_SUPPORT
if(o2_ytag_support)
{

	/*
	 * To determine some specific ilimit level is 
	 * available for this specific charging-behavior,
	 * We need a very strict juding condition (to make sure that
	 * once the level is y-tagged, it is guaranteed to be available)
	 * or the result will be disastrous.
	 * Note: This is a dangerous mechanism!
	 */

	if( ilimit < adji_ytag_get_min_ilimit() ) {
		if( (adji_ret == ADJI_RET_VBUSLOW) || (adji_ret == ADJI_RET_SOPDISFUNC) ) {
			adji_ytag_clear_upon(ilimit);
			ilimit_dbg("[%s] [YTAG] ilimit-level (%d), clear above levels.", __FUNCTION__, ilimit);

		}
			
	}
	
	if( (vbat < (cv_val - o2_ytag_debug_vbatth)) && (vbus_volt > ADJI_YTAG_VBUSTH) ) {
	
		//assume: ilimit lower 100, system comsumes 600, 
		//when ilimit is higher, make it harder to achieve ytag-setting
		//this makes sense.
		if(ibat > (ilimit - 100 - 600 + o2_ytag_debug_ibatth) ) {

			if(adji_ytag_cnt < o2_ytag_debug_cnt_num) {
				adji_ytag_cnt++ ;
				ilimit_dbg("[%s] [YTAG] ilimit-level (%d) keep stable, count: %d.", __FUNCTION__, ilimit, adji_ytag_cnt);
			}

			if(adji_ytag_cnt == o2_ytag_debug_cnt_num) {
				adji_ytag_set_ilimit(ilimit) ;
				ilimit_dbg("[%s] [YTAG] ilimit-level (%d) is available, reach count: %d.", __FUNCTION__, ilimit, adji_ytag_cnt);
			}

		} else {
			adji_ytag_cnt = 0 ;
		} 	

	} else {
		adji_ytag_cnt = 0 ;
	}

}
#endif

	/* 
	 * When ilimit has been high enough, DO NOT DO the optimization 
	 * to aviod unnecessary readjusting. Because DPM-optimizing may 
	 * be too risky and damage the algorithm's stability-level, we do 
	 * not want that trouble if the ilimit is relatively high.
	 * Just Let It Go!
	 */

	if(ilimit >= o2_debug_sdpm_ilimit_high) {
		ret =  ADJI_CHECK_ILIMIT_RET_OK;
		goto aci_succeed ; 
	}

	// if ilimit is relatively high, just 
	// one level lower than the desired one
	if(ilimit >= dilimit-200) {
		ret =  ADJI_CHECK_ILIMIT_RET_OK;
		goto aci_succeed ; 
	}
	
	/*
	 * DPM-optimizing ilimit-readjusting condition.
	 * These conditions indicate some states which 
	 * ilimit could be raised to optimize charging efficiecy.
	 * Note: Means that we could raise ilimit level (up-adjusting).
	 * Note: These optimizing-operation may be risky and very useful at the mean time.
	 */

	//the big condition which up-readjusting is allowed
	if ( vbus_volt > (ADJI_JUDG_VBUSVOLT_LOWTH + 100) ) {
		
		switch(adji_ret) {
			case ADJI_RET_IENOUGH:
				tmp = adji_chrenv_momt.vbus_volt ;

				if(BW_ABS(vbus_volt,tmp) < o2_debug_sdpm_sysenv_change_vbusth
					&& BW_ABS(cc_val,prevcc) < 200) {

					//when system consume less &&  more AD-power is need
					//note: the charger's system environment is better
					if (ibat > (adji_chrenv_momt.ibat + o2_debug_sdpm_sysenv_change_ibatth) ) {
						ret = ADJI_CHECK_ILIMIT_RET_EVB;
						ilimit_dbg("[%s] ilimit-level (%d) check aci_relative_fail: ibat is %d %d.", 
								__FUNCTION__, ilimit, ibat, adji_chrenv_momt.ibat);
						goto aci_relative_fail ; 
					}

					//when system consume more && more AD-power is need
					//note: the charger's system environment is worse
					if (ibat < (adji_chrenv_momt.ibat - o2_debug_sdpm_sysenv_change_ibatth) ) {
						ret = ADJI_CHECK_ILIMIT_RET_EVW;
						ilimit_dbg("[%s] ilimit-level (%d) check aci_relative_fail: ibat is %d %d.", 
								__FUNCTION__, ilimit, ibat, adji_chrenv_momt.ibat);
						goto aci_relative_fail ; 
					}

				}

				break ;

			case ADJI_RET_VBUSLOW:

				//I don't know why I add this. Just think maybe it will help.
				//Because when this happens, it definitely means something wrong.
				if(vbus_volt > (adji_chrenv_momt.vbus_volt + 300)) {
					ret = ADJI_CHECK_ILIMIT_RET_HIGHVOL ;
					ilimit_dbg("[%s] ilimit-level (%d) check aci_relative_fail: vbus_volt is %d %d.", 
							__FUNCTION__, ilimit, vbus_volt, adji_chrenv_momt.vbus_volt);
					goto aci_relative_fail ; 
				}
				break ;

			case ADJI_RET_SOPDISFUNC:
			case ADJI_RET_HIGHEST:
				//do not try in these cases
				//do not do any thing
				break;

			default:
				break;
		}

	}

	ret = ADJI_CHECK_ILIMIT_RET_OK;

aci_succeed:
	return ret ;

aci_i500_fail:
	return ret ;

aci_absolute_fail:

#if O2_CONFIG_ILIMIT_YTAG_SUPPORT
if(o2_ytag_support)
{

	if( ilimit < adji_ytag_get_min_ilimit() ) {
		if( (adji_ret == ADJI_RET_VBUSLOW) ||
			(adji_ret == ADJI_RET_SOPDISFUNC) ) {
			adji_ytag_clear_upon(ilimit);
			ilimit_dbg("[%s] [YTAG] ilimit-level (%d), clear above levels.", __FUNCTION__, ilimit);
		}
	}

}
#endif
	return ret ;

aci_relative_fail:
	return ret ;		
}


static void adji_chrenv_reset_all(void)
{
	uint8_t i ;

	adji_chrenv_momt.vbus_volt = 0 ;
	adji_chrenv_momt.vbat = 0 ;
	adji_chrenv_momt.ibat = 0 ;

	adji_chrenv_adjmomt.vbus_volt = 0 ;
	adji_chrenv_adjmomt.vbat = 0 ;
	adji_chrenv_adjmomt.ibat = 0 ;

	for(i=0; i< ADJI_CHRENV_RTNUM; i++) {
		adji_chrenv_rt[i].vbus_volt = 0;
		adji_chrenv_rt[i].vbat = 0;
		adji_chrenv_rt[i].ibat = 0;
	}

	adji_chrenv_rtpos = 0;
	adji_chrenv_rtpos_rewind = 0;
		
}

static void adji_chrenv_update_momt(int32_t vbus, int32_t vbat, int32_t ibat)
{
	adji_chrenv_momt.vbus_volt = vbus ;
	adji_chrenv_momt.vbat = vbat ;
	adji_chrenv_momt.ibat = ibat ;
}

static void adji_chrenv_update_adjmomt(int32_t vbus, int32_t vbat, int32_t ibat)
{
	adji_chrenv_adjmomt.vbus_volt = vbus ;
	adji_chrenv_adjmomt.vbat = vbat ;
	adji_chrenv_adjmomt.ibat = ibat ;
}

/*
 * adji_adjust_ilimit_up -- raise ilimit up between @ilimit_min and @ilimit_max
 * @ptr to charger struct
 * @ilimit_min: the minimal value of ilimit for adjusting
 * @ilimit_max: the maximal value of ilimit for adjusting
 * @retval: -1 if adji_dpm is stopped, the successfully adjusted ilimit value otherwise
 */
static int32_t adji_adjust_ilimit_up(struct bluewhale_charger_info *charger, int32_t ilimit_min, int32_t ilimit_max)
{
	int32_t ibat = 0 ;
	int32_t vbat = 0 ;
	int32_t vbus_volt = 0 ;
	int32_t bat_cur_diff = 0 ;
	int32_t ilimit_diff = 0 ;
	int32_t temp_cc = 0;
	int32_t ret ;
	int8_t fail = 0 ;
	int8_t i = 0 ;


#if O2_CONFIG_ILIMIT_NTAG_SUPPORT
	int32_t ilimit_ntag = 0 ;
#endif

#if O2_CONFIG_ILIMIT_YTAG_SUPPORT
	int32_t ilimit_ytag = 0 ;
#endif


	if(ilimit_min <= 0)
		ilimit_min = 500 ;

	if(ilimit_max >= 2000)
		ilimit_max = 2000 ;


#if O2_CONFIG_ILIMIT_NTAG_SUPPORT
if(o2_ntag_support) 
{

	ilimit_ntag = adji_ntag_get_max_ilimit();

	if(ilimit_max > ilimit_ntag)
		ilimit_max = ilimit_ntag ;

}
#endif

#if O2_CONFIG_ILIMIT_YTAG_SUPPORT
if(o2_ytag_support)
{

	adji_ytag_cnt = 0 ;
	
	ilimit_ytag = adji_ytag_get_min_ilimit();

}
#endif

	//remember the desired ilimit value
	dilimit = ilimit_max ;

	//force cc to 2200 for ilimit-adjusting
	ret = bluewhale_get_charger_current(charger) ;
	if(ret < 0) {
		ilimit_dbg("[%s] get charger current error: %d", __FUNCTION__, ret);
		return -1 ;
	} else {
		temp_cc = ret ;
		if(temp_cc != 2200)
			bluewhale_set_charger_current(charger, 2200) ;
	}


	//
	for(i=0; i < (ADJI_ILELS_NUM); i++) {

		//skip the ilimit-level needless to adjust
		if( ilimit_min > adji_ilel_array[i] )
			continue;

		//stop at the ilimit-level whose value larger than ilimit_max
		if( ilimit_max < adji_ilel_array[i] )
			break;
		
		if( adji_ilimit_buffer != 0 && adji_ilimit_buffer < adji_ilel_array[i]) {
			adji_ret = adji_ret_buffer ;
			adji_ilimit_buffer = 0;
			if(i>0) i-- ;
			goto adji_end ;
		}	

		ilimit_dbg("[%s] try ilimit level %d(%d)", __FUNCTION__, adji_ilel_array[i], i);


		ret = bluewhale_set_vbus_current(charger, adji_ilel_array[i]);
		
		//get average ibat/vbat/vbus, and then check
		adji_get_avg_info(&ibat, &vbat, &vbus_volt, 0) ;
			
		if(ibat < 200 && vbus_volt < ADJI_JUDG_VBUSVOLT_LOWTH) {
			ilimit_dbg("[%s] try ilimit level %d(%d) Vbus-Checking Fail",
					__FUNCTION__, adji_ilel_array[i], i);

			adji_ret = ADJI_RET_VBUSLOW ;
			fail = 1 ;
			break;
		}
#if 0
		if (ibat <= ibat_off * 2/3 && vbus_volt > vbus_off){
			ilimit_dbg("[%s] try ilimit level %d(%d) Vbus-Checking Fail",
					__FUNCTION__, adji_ilel_array[i], i);
			adji_ret = ADJI_RET_VBUSLOW ;
			fail = 1 ;
			break;
		}
#endif

		ilimit_dbg("[%s] try ilimit level %d(%d) Vbus-Checking OK",
				__FUNCTION__, adji_ilel_array[i], i);
		
		adji_bat_curr[i] = ibat ;
#if O2_CONFIG_TRUSTED_VBUS_SUPPORT
		adji_vbus[i] = vbus_volt ;
#endif

		if(i>0) {
			bat_cur_diff = adji_bat_curr[i] - adji_bat_curr[i-1] ;
			ilimit_diff = adji_ilel_array[i] - adji_ilel_array[i-1] ;

			ilimit_dbg("[%s] adji_bat_curr[%d](%d), adji_bat_curr[%d](%d)",
					__FUNCTION__, i, adji_bat_curr[i], i-1, adji_bat_curr[i-1]);

			ilimit_dbg("[%s] bat_cur_diff(%d), ilimit_diff(%d)",
					__FUNCTION__, bat_cur_diff, ilimit_diff);

			if(bat_cur_diff < o2_debug_sdpm_judg_sopdisfunc_ibatth) { //?
				ilimit_dbg("[%s] try ilimit level %d(%d) IBAT-ABS-Checking Fail: %d",
						__FUNCTION__, adji_ilel_array[i], i, bat_cur_diff);

#if O2_CONFIG_TRUSTED_VBUS_SUPPORT
				//add vbus judgement to resist system noise
				if ( (i>=2) && (adji_vbus[i] > o2_debug_sdpm_vbus_oz115c_th)
					&& (BW_ABS(adji_vbus[i],adji_vbus[i-1]) < o2_debug_sdpm_vbus_step_dev) ) { //at least 1000

					ilimit_dbg("[%s] try ilimit level %d(%d) vbus check ok: %d",
						__FUNCTION__, adji_ilel_array[i], i, adji_vbus[i]);
				} else 
#endif
				{
					adji_ret = ADJI_RET_SOPDISFUNC ;
					fail = 1 ;
					break ;
				}

#if O2_CONFIG_TRUSTED_VBUS_SUPPORT
				// to catch adapter's disfunction
				if (vbus_array[nsamples-1] - vbus_array[0] > 150) {
					adji_ret = ADJI_RET_SOPDISFUNC ;
					fail = 1 ;
					break ;
				}
#endif
			}			
			ilimit_dbg("[%s] try ilimit level %d(%d) IBAT-ABS-Checking OK: %d",
					__FUNCTION__, adji_ilel_array[i], i, bat_cur_diff);

			if(i > 1 && bat_cur_diff < 100) {
				bat_cur_diff = adji_bat_curr[i] - adji_bat_curr[i-2] ;

				//if the current does not increase obviously	
				if(bat_cur_diff < o2_debug_sdpm_judg_ienough_ibatth) {
					ilimit_dbg("[%s] try ilimit level %d(%d) IBAT-ENG-Checking Fail: %d",
							__FUNCTION__, adji_ilel_array[i], i, bat_cur_diff);

#if O2_CONFIG_TRUSTED_VBUS_SUPPORT
					//add vbus judgement to resist system noise
					if ( (i>=2) && (adji_vbus[i] > o2_debug_sdpm_vbus_oz115c_th)
						&& (BW_ABS(adji_vbus[i],adji_vbus[i-1]) < o2_debug_sdpm_vbus_step_dev) ) { //at least 1000

						ilimit_dbg("[%s] try ilimit level %d(%d) vbus check ok: %d",
							__FUNCTION__, adji_ilel_array[i], i, adji_vbus[i]);
					} else 
#endif
					{
						adji_ret = ADJI_RET_IENOUGH ;
						fail =1 ;
						break ;
					}
#if O2_CONFIG_TRUSTED_VBUS_SUPPORT
					// to catch vbus's voltage drop
					if (vbus_array[0] <= ADJI_JUDG_VBUSVOLT_LOWTH) {
						adji_ret = ADJI_RET_IENOUGH ;
						fail = 1 ;
						break ;
					}
#endif
				}

				ilimit_dbg("[%s] try ilimit level %d(%d) IBAT-ENG-Checking OK: %d",
						__FUNCTION__, adji_ilel_array[i], i, bat_cur_diff);
	
			}

		} else {
			ilimit_dbg("[%s] bat_crr[%d](%d)", __FUNCTION__, i, adji_bat_curr[i]);
		}
	}

	if(fail == 1) {

#if O2_CONFIG_ILIMIT_NTAG_SUPPORT
if(o2_ntag_support) 
{

		if(i>1) {
			if((adji_ret == ADJI_RET_VBUSLOW) || (adji_ret == ADJI_RET_SOPDISFUNC)) {

				if(adji_ilel_ntag_judge_cnt[i] < o2_ntag_debug_cnt_num) {
					adji_ilel_ntag_judge_cnt[i] += 1 ;
					ilimit_dbg("[%s] [NTAG] ilimit-level (%d) fails, ntag count num: %d.",
							__FUNCTION__, adji_ilel_array[i], adji_ilel_ntag_judge_cnt[i]);
				}

				
				if(adji_ilel_ntag_judge_cnt[i] == o2_ntag_debug_cnt_num) {

#if O2_CONFIG_ILIMIT_YTAG_SUPPORT
if(o2_ytag_support)
{

					ilimit_ytag = adji_ytag_get_min_ilimit() ;
					
					if(adji_ilel_array[i] > ilimit_ytag) {
						adji_ntag_set_ilimit(adji_ilel_array[i]);

						ilimit_dbg("[%s] [NTAG] ilimit-level (%d) reach max count (%d), not YTAG, ntag it.",
								__FUNCTION__, adji_ilel_array[i], adji_ilel_ntag_judge_cnt[i]);
					} else {

						ilimit_dbg("[%s] [NTAG] ilimit-level (%d) reach max count (%d), but YTAG, ignore it.",
								__FUNCTION__, adji_ilel_array[i], adji_ilel_ntag_judge_cnt[i]);

					}

}
else
#endif 
{
					adji_ntag_set_ilimit(adji_ilel_array[i]);
					ilimit_dbg("[%s] [NTAG] ilimit-level (%d) reach max count (%d), ntag it.",
							__FUNCTION__, adji_ilel_array[i], adji_ilel_ntag_judge_cnt[i]);

}
				}
			}
		}

}
#endif
			
		//i-- ;
		if( (adji_ret == ADJI_RET_IENOUGH) && (i>=4) ) //4--1200
			i -= 2;
		else {
			i -= 1;
		}
		if(i<0) i=0;
		adji_ilimit_buffer = adji_ilel_array[i] ;
		adji_ret_buffer = adji_ret ;
		i = 0;
		ret = bluewhale_set_vbus_current(charger, adji_ilel_array[i]);
		goto adji_end ;
	}

	
	adji_ilimit_buffer = 0;
	adji_ret = ADJI_RET_HIGHEST ;
	i-- ; //?
	
	ilimit_dbg("[%s] succeed at ADJI_RET_HIGHEST", __FUNCTION__);
	goto adji_end ;

adji_end:
	prevcc = 2200 ;
	ilimit_dbg("[%s]: succeed at %d", __FUNCTION__, adji_ilel_array[i] ) ;
	adji_chrenv_update_momt(vbus_volt, vbat, ibat) ;
	adji_chrenv_update_adjmomt(vbus_volt, vbat, ibat) ;
	if(temp_cc != 2200)
		bluewhale_set_charger_current(charger, temp_cc) ;
	return adji_ilel_array[i] ;
}


static int32_t  adji_adjust_ilimit_wrapper(struct bluewhale_charger_info *charger, int32_t ilimit)
{
	int32_t vbus_volt = 0 ;
	int32_t bat_volt = 0 ;
	int32_t bat_curr = 0 ;

	int32_t tempcv = 0 ;
	int32_t tempcc = 0 ;
	int32_t vbus_ilimit_lel = 0 ;
	int32_t check_ret = 0 ;
	int32_t ret = 0 ;

	//get all vol/curr information	
	//bat_curr = adji_get_bat_curr() ;
	//bat_volt = adji_get_bat_volt() ;
	//vbus_volt = adji_get_vbus_volt() ;
	adji_get_avg_info(&bat_curr, &bat_volt, &vbus_volt, 1) ;
	
	ilimit_dbg("[%s]: vbus_volt: %d, bat_volt: %d, bat_curr: %d", __FUNCTION__, vbus_volt, bat_volt, bat_curr);

	//get ilimit-level and CV Setting, and then check ilimit
	ret = bluewhale_get_vbus_current(charger) ;
	if(ret < 0) {
		return ret ;
	} else {
		vbus_ilimit_lel = ret ;
	}
	ilimit_dbg("[%s]: vbus_ilimit_lel: %d", __FUNCTION__, vbus_ilimit_lel);
	
	//get CV for ilimit-checking	
	ret = bluewhale_get_chg_volt(charger);
	if(ret < 0) {
		return ret;
	} else {
		tempcv =  ret ;
	}

	//get CC for ilimit-checking
	ret = bluewhale_get_charger_current(charger);
	if(ret < 0) {
		return ret;
	} else {
		tempcc =  ret ;
	}

	check_ret = adji_check_ilimit(vbus_ilimit_lel, tempcv, tempcc, vbus_volt, bat_volt, bat_curr) ;

	ilimit_dbg("[%s] adji_check_ilimit ret: %d", __FUNCTION__, check_ret);
	if(check_ret != ADJI_CHECK_ILIMIT_RET_OK) {

		ret = adji_adjust_ilimit_up(charger, 0, ilimit) ; 
		
		/** not distinguish these specific conditions for now
		//reserved for future inprovement

		if( check_ret == ADJI_CHECK_ILIMIT_RET_SOP || check_ret == ADJI_CHECK_ILIMIT_RET_LOWVOL || check_ret == ADJI_CHECK_ILIMIT_RET_LOWCUR ) {
			ret = adji_adjust_ilimit_up(charger, 0, ilimit) ; 
		} else if( check_ret == ADJI_CHECK_ILIMIT_RET_EVB || check_ret == ADJI_CHECK_ILIMIT_RET_EVW ) {
			ret = adji_adjust_ilimit_up(charger, vbus_ilimit_lel, ilimit) ; 
		}
		*/

	} else {
		adji_chrenv_update_momt(vbus_volt, bat_volt, bat_curr) ;
		prevcc = tempcc ;
	}

	return ret ;
}


static void bluewhale_dis_sdpm(void)
{
	adji_started = 0 ;
	adji_chrenv_reset_all();
	adji_ilimit_buffer = 0 ;

#if O2_CONFIG_ILIMIT_NTAG_SUPPORT
if(o2_ntag_support)
{
 
	adji_ntag_clear_all();

}
#endif

#if O2_CONFIG_ILIMIT_YTAG_SUPPORT
if(o2_ytag_support)
{

	adji_ytag_clear_all();

}
#endif

}

static int find_vbus_current_step(int ilmt_ma)
{
	u8 i = 0;
	for (i = 0; i < ADJI_ILELS_NUM; i++){
		if (ilmt_ma <= adji_ilel_array[i])
			return adji_ilel_array[i];
	}
	return adji_ilel_array[ADJI_ILELS_NUM-1];
}

#endif	//O2_CONFIG_SOFTWARE_DPM_SUPPORT

/*****************************************************************************
 * Description:
 *		bluewhale_set_vbus_current_extern
 * Parameters:
 *		ilmt_ma:	set input current limit
 *		Only 100mA, 500mA, 700mA, 900mA, 1000mA, 1200mA, 1400mA, 1500mA, 1700mA, 1900mA, 2000mA can be accurate
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
int bluewhale_set_vbus_current_extern(int ilmt_ma)
{
	int ret = 0;
#if O2_CONFIG_SOFTWARE_DPM_SUPPORT
	static int32_t adji_ilimit_pre = 0 ;
#endif

	mutex_lock(&bluewhale_mutex);
	if (!charger_info) {
		ret = -EINVAL; 
		goto end;
	}

#if O2_CONFIG_SOFTWARE_DPM_SUPPORT
	if (
#if defined(MTK_MACH_SUPPORT)
		BMT_status.charger_type != STANDARD_CHARGER ||
		KAL_TRUE != BMT_status.charger_exist ||
#if defined(CONFIG_MTK_PUMP_EXPRESS_PLUS_20_SUPPORT)
	    mtk_pep20_get_is_connect() == true ||
        //mtk_pep20_get_to_check_chr_type() == true  ||
#endif
#if defined(CONFIG_MTK_PUMP_EXPRESS_PLUS_SUPPORT)
	    mtk_pep_get_is_connect() == true ||
        //mtk_pep_get_to_check_chr_type() == true  ||
#endif

#endif
	  adapter_type > CHG_ADAPTER_NORMAL ||
	  //adji_get_bat_volt() < 3500 ||
	  ilmt_ma <= 700
	  ) 
	{

		ret = bluewhale_set_vbus_current(charger_info, ilmt_ma);
		bluewhale_dis_sdpm();
		goto end;
	}

	//find responding vbus current value
	ilmt_ma = find_vbus_current_step(ilmt_ma);

	if(adji_started == 0) {
		ilimit_dbg("[%s] adji just started for the first time.", __FUNCTION__);
		adji_ilimit_buffer = 0;
		ret = adji_adjust_ilimit_up(charger_info, 0, ilmt_ma) ; 
		adji_ilimit_pre = ilmt_ma ;
		adji_started = 1 ;
	}
	else {
		if(adji_ilimit_pre != ilmt_ma) {
			ilimit_dbg("[%s] adji stared, ilimit changed: old %d, new %d.",
					__FUNCTION__, adji_ilimit_pre, ilmt_ma);
			adji_ilimit_buffer = 0;
			ret = adji_adjust_ilimit_up(charger_info, 0, ilmt_ma) ; 
			adji_ilimit_pre = ilmt_ma ;
		} else {
			ilimit_dbg("[%s] adji stared, ilimit not changed: %d.",
					__FUNCTION__, ilmt_ma);
			ret = adji_adjust_ilimit_wrapper(charger_info, ilmt_ma) ;
		}	
	}
#else
	ret = bluewhale_set_vbus_current(charger_info, ilmt_ma);
#endif
end:
	mutex_unlock(&bluewhale_mutex);
	return ret;
}
EXPORT_SYMBOL(bluewhale_set_vbus_current_extern);

#ifdef BATT_TEMP_SIMULATION 
static ssize_t batt_temp_store(struct device *dev, struct device_attribute *attr, 
               const char *buf, size_t count) 
{
	int val;

	if (kstrtoint(buf, 10, &val))
		return -EINVAL;

	batt_temp_simulation = val;

	bluewhale_dbg("batt_temp_simulation: %d ", batt_temp_simulation);

	return count;
}

static ssize_t batt_temp_show(struct device *dev, struct device_attribute *attr,char *buf)
{
	return sprintf(buf, "%d\n", batt_temp_simulation);
}
#endif

#if O2_CONFIG_SOFTWARE_DPM_SUPPORT

static ssize_t bluewhale_sdpm_debug_ilimit_th_store(struct device *dev, struct device_attribute *attr, 
               const char *buf, size_t count) 
{
	int32_t val;

        if (sscanf(buf, "%d", &val) == 1) { 
		o2_debug_sdpm_ilimit_threshold = val ;
		return count;
	} else {
		return -EINVAL ;
	}

}

ssize_t bluewhale_sdpm_debug_ilimit_th_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "o2_debug_sdpm_ilimit_threshold:%d\n", o2_debug_sdpm_ilimit_threshold);
}



static ssize_t bluewhale_sdpm_debug_ilimit_high_store(struct device *dev, struct device_attribute *attr, 
               const char *buf, size_t count) 
{
	int32_t val;

        if (sscanf(buf, "%d", &val) == 1) { 
		o2_debug_sdpm_ilimit_high = val ;
		return count;
	} else {
		return -EINVAL ;
	}

}

ssize_t bluewhale_sdpm_debug_ilimit_high_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "o2_debug_sdpm_ilimit_high:%d\n", o2_debug_sdpm_ilimit_high);
}



static ssize_t bluewhale_sdpm_debug_judg_ienough_ibatth_store(struct device *dev, struct device_attribute *attr, 
               const char *buf, size_t count) 
{
	int32_t val;

        if (sscanf(buf, "%d", &val) == 1) { 
		o2_debug_sdpm_judg_ienough_ibatth = val ;
		return count;
	} else {
		return -EINVAL ;
	}

}

ssize_t bluewhale_sdpm_debug_judg_ienough_ibatth_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "o2_debug_sdpm_judg_ienough_ibatth:%d\n", o2_debug_sdpm_judg_ienough_ibatth);
}





static ssize_t bluewhale_sdpm_debug_judg_sopdisfunc_ibatth_store(struct device *dev, struct device_attribute *attr, 
               const char *buf, size_t count) 
{
	int32_t val;

        if (sscanf(buf, "%d", &val) == 1) { 
		o2_debug_sdpm_judg_sopdisfunc_ibatth = val ;
		return count;
	} else {
		return -EINVAL ;
	}

}

ssize_t bluewhale_sdpm_debug_judg_sopdisfunc_ibatth_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "o2_debug_sdpm_judg_sopdisfunc_ibatth:%d\n", o2_debug_sdpm_judg_sopdisfunc_ibatth);
}




static ssize_t bluewhale_sdpm_debug_judg_adosc_ibatth_store(struct device *dev, struct device_attribute *attr, 
               const char *buf, size_t count) 
{
	int32_t val;

        if (sscanf(buf, "%d", &val) == 1) { 
		o2_debug_sdpm_judg_adosc_ibatth = val ;
		return count;
	} else {
		return -EINVAL ;
	}

}

ssize_t bluewhale_sdpm_debug_judg_adosc_ibatth_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "o2_debug_sdpm_judg_adosc_ibatth:%d\n", o2_debug_sdpm_judg_adosc_ibatth);
}



static ssize_t bluewhale_sdpm_debug_judg_adosc_vbusth_store(struct device *dev, struct device_attribute *attr, 
               const char *buf, size_t count) 
{
	int32_t val;

        if (sscanf(buf, "%d", &val) == 1) { 
		o2_debug_sdpm_judg_adosc_vbusth = val ;
		return count;
	} else {
		return -EINVAL ;
	}

}

ssize_t bluewhale_sdpm_debug_judg_adosc_vbusth_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "o2_debug_sdpm_judg_adosc_vbusth:%d\n", o2_debug_sdpm_judg_adosc_vbusth);
}








static ssize_t bluewhale_sdpm_debug_sysenv_change_ibatth_store(struct device *dev, struct device_attribute *attr, 
               const char *buf, size_t count) 
{
	int32_t val;

        if (sscanf(buf, "%d", &val) == 1) { 
		o2_debug_sdpm_sysenv_change_ibatth = val ;
		return count;
	} else {
		return -EINVAL ;
	}

}

ssize_t bluewhale_sdpm_debug_sysenv_change_ibatth_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "o2_debug_sdpm_sysenv_change_ibatth:%d\n", o2_debug_sdpm_sysenv_change_ibatth);
}




static ssize_t bluewhale_sdpm_debug_sysenv_change_vbusth_store(struct device *dev, struct device_attribute *attr, 
               const char *buf, size_t count) 
{
	int32_t val;

        if (sscanf(buf, "%d", &val) == 1) { 
		o2_debug_sdpm_sysenv_change_vbusth = val ;
		return count;
	} else {
		return -EINVAL ;
	}

}

ssize_t bluewhale_sdpm_debug_sysenv_change_vbusth_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "o2_debug_sdpm_sysenv_change_vbusth:%d\n", o2_debug_sdpm_sysenv_change_vbusth);
}





static ssize_t bluewhale_sdpm_debug_sysenv_better_ibatth_store(struct device *dev, struct device_attribute *attr, 
               const char *buf, size_t count) 
{
	int32_t vall, valm, valh;

        if (sscanf(buf, "%d %d %d", &vall, &valm, &valh) == 3) { 
		o2_debug_sdpm_sysenv_better_ibatth_il = vall ;
		o2_debug_sdpm_sysenv_better_ibatth_im = valm ;
		o2_debug_sdpm_sysenv_better_ibatth_ih = valh ;
		return count;
	} else {
		return -EINVAL ;
	}

}

ssize_t bluewhale_sdpm_debug_sysenv_better_ibatth_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "o2_debug_sdpm_sysenv_better: %d(il) %d(im) %d(ih)\n", 
		o2_debug_sdpm_sysenv_better_ibatth_il, 
		o2_debug_sdpm_sysenv_better_ibatth_im, 
		o2_debug_sdpm_sysenv_better_ibatth_ih);
}





static ssize_t bluewhale_sdpm_debug_avg_info_store(struct device *dev, struct device_attribute *attr, 
               const char *buf, size_t count) 
{
	int32_t val1, val2, val3;

        if (sscanf(buf, "%d %d %d", &val1, &val2, &val3) == 3) { 
		o2_debug_sdpm_avg_info_check_interval = val1 ; 
		o2_debug_sdpm_avg_info_first_delay = val2 ; 
		o2_debug_sdpm_avg_info_wait_time = val3 ; 
		return count;
	} else {
		return -EINVAL ;
	}

}

ssize_t bluewhale_sdpm_debug_avg_info_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "o2_debug_sdpm_avg_info: %d(check interval) %d(first delay) %d(wait time)\n", 
		o2_debug_sdpm_avg_info_check_interval,
		o2_debug_sdpm_avg_info_first_delay,
		o2_debug_sdpm_avg_info_wait_time);
}

#if O2_CONFIG_TRUSTED_VBUS_SUPPORT
static ssize_t bluewhale_sdpm_debug_tvbus_store(struct device *dev, struct device_attribute *attr, 
               const char *buf, size_t count) 
{
	int32_t val1, val2;

        if (sscanf(buf, "%d %d", &val1, &val2) == 2) { 
		o2_debug_sdpm_vbus_oz115c_th = val1 ; 
		o2_debug_sdpm_vbus_step_dev = val2 ; 
		return count;
	} else {
		return -EINVAL ;
	}

}

ssize_t bluewhale_sdpm_debug_tvbus_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "o2_debug_sdpm_trusted_vbus: %d(trusted threshold) %d(step deviation)\n", 
		o2_debug_sdpm_vbus_oz115c_th, o2_debug_sdpm_vbus_step_dev);
}

#endif












#if O2_CONFIG_ILIMIT_NTAG_SUPPORT

static ssize_t bluewhale_sdpm_ntag_debug_cnt_num_store(struct device *dev, struct device_attribute *attr, 
               const char *buf, size_t count) 
{
	int32_t val;

        if (sscanf(buf, "%d", &val) == 1) { 
		o2_ntag_debug_cnt_num = val ;
		return count;
	} else {
		return -EINVAL ;
	}

}

ssize_t bluewhale_sdpm_ntag_debug_cnt_num_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "o2_ntag_debug_cnt_num:%d\n", o2_ntag_debug_cnt_num);
}


static ssize_t bluewhale_sdpm_conf_ntag_store(struct device *dev, struct device_attribute *attr, 
               const char *buf, size_t count) 
{
	int32_t val;

        if (sscanf(buf, "%d", &val) == 1) { 
		o2_ntag_support = val ;
		return count;
	} else {
		return -EINVAL ;
	}

}

ssize_t bluewhale_sdpm_conf_ntag_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "ntag:%d\n", o2_ntag_support);
}
#endif


#if O2_CONFIG_ILIMIT_YTAG_SUPPORT

static ssize_t bluewhale_sdpm_ytag_debug_cnt_num_store(struct device *dev, struct device_attribute *attr, 
               const char *buf, size_t count) 
{
	int32_t val;

        if (sscanf(buf, "%d", &val) == 1) { 
		o2_ytag_debug_cnt_num = val ;
		return count;
	} else {
		return -EINVAL ;
	}

}

ssize_t bluewhale_sdpm_ytag_debug_cnt_num_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "o2_ytag_debug_cnt_num:%d\n", o2_ytag_debug_cnt_num);
}

static ssize_t bluewhale_sdpm_ytag_debug_ibatth_store(struct device *dev, struct device_attribute *attr, 
               const char *buf, size_t count) 
{
	int32_t val;

        if (sscanf(buf, "%d", &val) == 1) { 
		o2_ytag_debug_ibatth = val ;
		return count;
	} else {
		return -EINVAL ;
	}

}

ssize_t bluewhale_sdpm_ytag_debug_ibatth_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "o2_ytag_debug_ibatth:%d\n", o2_ytag_debug_ibatth);
}


static ssize_t bluewhale_sdpm_ytag_debug_vbatth_store(struct device *dev, struct device_attribute *attr, 
               const char *buf, size_t count) 
{
	int32_t val;

        if (sscanf(buf, "%d", &val) == 1) { 
		o2_ytag_debug_vbatth = val ;
		return count;
	} else {
		return -EINVAL ;
	}

}

ssize_t bluewhale_sdpm_ytag_debug_vbatth_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "o2_ytag_debug_vbatth:%d\n", o2_ytag_debug_vbatth);
}


static ssize_t bluewhale_sdpm_conf_ytag_store(struct device *dev, struct device_attribute *attr, 
               const char *buf, size_t count) 
{
	int32_t val;

        if (sscanf(buf, "%d", &val) == 1) { 
		o2_ytag_support = val ;
		return count;
	} else {
		return -EINVAL ;
	}

}

ssize_t bluewhale_sdpm_conf_ytag_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "ytag:%d\n", o2_ytag_support);
}

#endif

#endif //O2_CONFIG_SOFTWARE_DPM_SUPPORT

static int otg_enable = 0;
static ssize_t bluewhale_otg_store(struct device *dev, struct device_attribute *attr, 
               const char *buf, size_t count) 
{
	int val;
	struct bluewhale_charger_info *charger;


	if (kstrtoint(buf, 10, &val))
		return -EINVAL;

	if (charger_info->dev == dev)
	    charger = dev_get_drvdata(dev);
	else //this device is the provate device of power supply 
	    charger = dev_get_drvdata(dev->parent);

	if (val == 1 || val == 0)
	{
		bluewhale_enable_otg(charger, val);
		bluewhale_dbg("%s: %s OTG", __func__, val?"enable":"disable");
		otg_enable = val;

		return count;
	}
	else
		bluewhale_dbg("%s: wrong parameters", __func__);

	return -EINVAL;
}

static ssize_t bluewhale_otg_show(struct device *dev, struct device_attribute *attr,char *buf)
{
	return sprintf(buf, "%d\n", otg_enable);
}

static ssize_t bluewhale_show_vbus_volt(struct device *dev, struct device_attribute *attr,char *buf)
{
	return sprintf(buf, "%d\n", adji_get_vbus_volt());
}
#ifdef DYNAMIC_CV
static ssize_t bluewhale_dynamic_cv_store(struct device *dev, struct device_attribute *attr, 
               const char *buf, size_t count) 
{
	int val;
	struct bluewhale_charger_info *charger;


	if (kstrtoint(buf, 10, &val))
		return -EINVAL;

	if (charger_info->dev == dev)
	    charger = dev_get_drvdata(dev);
	else //this device is the provate device of power supply 
	    charger = dev_get_drvdata(dev->parent);

	if (val >= 0)
	{
		bluewhale_dbg("%s: %s dynamic cv : %d ", __func__, val > 0?"enable":"disable", val);
		dynamic_cv_enable = val;
		return count;
	}
	else
		bluewhale_dbg("%s: wrong parameters", __func__);

	return -EINVAL;
}

static ssize_t bluewhale_dynamic_cv_show(struct device *dev, struct device_attribute *attr,char *buf)
{
	return sprintf(buf, "%d\n", dynamic_cv_enable);
}
#endif

static int reg_enable = 1;

#define BLUEWHALE_PARAMS_ATTR(x)  \
static ssize_t bluewhale_write_##x(struct device *dev, struct device_attribute *attr, \
		const char *buf, size_t count) \
{ \
	int data; \
	struct bluewhale_charger_info *charger;\
	if (charger_info->dev == dev)\
	    charger = dev_get_drvdata(dev);\
	else\
	    charger = dev_get_drvdata(dev->parent);\
	if( kstrtoint(buf, 10, &data) == 0) { \
		if(reg_enable ==1)\
		    bluewhale_set_##x(charger, data);\
		return count; \
	}else { \
		pr_err("bluewhale reg %s set failed\n",#x); \
		return 0; \
	} \
	return 0;\
}\
static ssize_t bluewhale_read_##x(struct device *dev, struct device_attribute *attr, \
               char *buf) \
{ \
	struct bluewhale_charger_info *charger;\
	if (charger_info->dev == dev)\
	    charger = dev_get_drvdata(dev);\
	else\
	    charger = dev_get_drvdata(dev->parent);\
	charger->x = bluewhale_get_##x(charger);\
    return sprintf(buf,"%d\n", charger->x);\
}

BLUEWHALE_PARAMS_ATTR(vbus_current) //input current limit
BLUEWHALE_PARAMS_ATTR(charger_current) //charge current
BLUEWHALE_PARAMS_ATTR(chg_volt) //charge voltage

static ssize_t bluewhale_registers_show(struct device *dev, struct device_attribute *attr, char *buf)
{	
	int result = 0;
	u8 i = 0;
	u8 data = 0;
	struct bluewhale_charger_info *charger;

	if (charger_info->dev == dev)
	    charger = dev_get_drvdata(dev);
	else //this device is the provate device of power supply 
	    charger = dev_get_drvdata(dev->parent);

    for (i=REG_CHARGER_VOLTAGE; i<=REG_MIN_VSYS_VOLTAGE; i++)
    {
		bluewhale_read_byte(charger, i, &data);
		result += sprintf(buf + result, "[0x%.2x] = 0x%.2x\n", i, data);
    }
    for (i=REG_CHARGE_CURRENT; i<=REG_VBUS_LIMIT_CURRENT; i++)
    {
		bluewhale_read_byte(charger, i, &data);
		result += sprintf(buf + result, "[0x%.2x] = 0x%.2x\n", i, data);
	}

	bluewhale_read_byte(charger, REG_SAFETY_TIMER, &data);
	result += sprintf(buf + result, "[0x%.2x] = 0x%.2x\n", REG_SAFETY_TIMER, data);

	bluewhale_read_byte(charger, REG_CHARGER_CONTROL, &data);
	result += sprintf(buf + result, "[0x%.2x] = 0x%.2x\n", REG_CHARGER_CONTROL, data);

    for (i=REG_VBUS_STATUS; i<=REG_THM_STATUS; i++)
    {
		bluewhale_read_byte(charger, i, &data);
		result += sprintf(buf + result, "[0x%.2x] = 0x%.2x\n", i, data);
    }
	
	return result;
}

#ifdef BATT_TEMP_SIMULATION 
static DEVICE_ATTR(batt_temp, S_IRUGO | (S_IWUSR|S_IWGRP), batt_temp_show, batt_temp_store);
#endif
static DEVICE_ATTR(registers, S_IRUGO, bluewhale_registers_show, NULL);

static DEVICE_ATTR(vbus_current, S_IRUGO|(S_IWUSR|S_IWGRP), bluewhale_read_vbus_current, bluewhale_write_vbus_current);
static DEVICE_ATTR(charger_current, S_IRUGO|(S_IWUSR|S_IWGRP), bluewhale_read_charger_current, bluewhale_write_charger_current);
static DEVICE_ATTR(chg_volt, S_IRUGO|(S_IWUSR|S_IWGRP), bluewhale_read_chg_volt, bluewhale_write_chg_volt);
static DEVICE_ATTR(vbus_volt, S_IRUGO, bluewhale_show_vbus_volt, NULL);
static DEVICE_ATTR(enable_otg, S_IRUGO | (S_IWUSR|S_IWGRP), bluewhale_otg_show, bluewhale_otg_store);
#ifdef DYNAMIC_CV
static DEVICE_ATTR(dynamic_cv, S_IRUGO | (S_IWUSR|S_IWGRP), bluewhale_dynamic_cv_show, bluewhale_dynamic_cv_store);
#endif

#if O2_CONFIG_SOFTWARE_DPM_SUPPORT


#if O2_CONFIG_ILIMIT_NTAG_SUPPORT
static DEVICE_ATTR(config_ntag, S_IRUGO | S_IWUSR, bluewhale_sdpm_conf_ntag_show, bluewhale_sdpm_conf_ntag_store);
static DEVICE_ATTR(ntag_debug_cntnum, S_IRUGO | S_IWUSR, bluewhale_sdpm_ntag_debug_cnt_num_show, bluewhale_sdpm_ntag_debug_cnt_num_store);
#endif

#if O2_CONFIG_ILIMIT_YTAG_SUPPORT
static DEVICE_ATTR(config_ytag, S_IRUGO | S_IWUSR, bluewhale_sdpm_conf_ytag_show, bluewhale_sdpm_conf_ytag_store);
static DEVICE_ATTR(ytag_debug_cntnum, S_IRUGO | S_IWUSR, bluewhale_sdpm_ytag_debug_cnt_num_show, bluewhale_sdpm_ytag_debug_cnt_num_store);
static DEVICE_ATTR(ytag_debug_ibatth, S_IRUGO | S_IWUSR, bluewhale_sdpm_ytag_debug_ibatth_show, bluewhale_sdpm_ytag_debug_ibatth_store);
static DEVICE_ATTR(ytag_debug_vbatth, S_IRUGO | S_IWUSR, bluewhale_sdpm_ytag_debug_vbatth_show, bluewhale_sdpm_ytag_debug_vbatth_store);
#endif

#if O2_CONFIG_TRUSTED_VBUS_SUPPORT
static DEVICE_ATTR(debug_trusted_vbus, S_IRUGO | S_IWUSR, bluewhale_sdpm_debug_tvbus_show, bluewhale_sdpm_debug_tvbus_store);
#endif

static DEVICE_ATTR(debug_ilimit_th, S_IRUGO | S_IWUSR, bluewhale_sdpm_debug_ilimit_th_show, bluewhale_sdpm_debug_ilimit_th_store);
static DEVICE_ATTR(debug_ilimit_high, S_IRUGO | S_IWUSR, bluewhale_sdpm_debug_ilimit_high_show, bluewhale_sdpm_debug_ilimit_high_store);
static DEVICE_ATTR(debug_judg_ienough_ibatth, S_IRUGO | S_IWUSR, bluewhale_sdpm_debug_judg_ienough_ibatth_show, bluewhale_sdpm_debug_judg_ienough_ibatth_store);
static DEVICE_ATTR(debug_judg_sopdisfunc_ibatth, S_IRUGO | S_IWUSR, bluewhale_sdpm_debug_judg_sopdisfunc_ibatth_show, bluewhale_sdpm_debug_judg_sopdisfunc_ibatth_store);
static DEVICE_ATTR(debug_judg_adosc_ibatth, S_IRUGO | S_IWUSR, bluewhale_sdpm_debug_judg_adosc_ibatth_show, bluewhale_sdpm_debug_judg_adosc_ibatth_store);
static DEVICE_ATTR(debug_judg_adosc_vbusth, S_IRUGO | S_IWUSR, bluewhale_sdpm_debug_judg_adosc_vbusth_show, bluewhale_sdpm_debug_judg_adosc_vbusth_store);
static DEVICE_ATTR(debug_sysenv_change_ibatth, S_IRUGO | S_IWUSR, bluewhale_sdpm_debug_sysenv_change_ibatth_show, bluewhale_sdpm_debug_sysenv_change_ibatth_store);
static DEVICE_ATTR(debug_sysenv_change_vbusth, S_IRUGO | S_IWUSR, bluewhale_sdpm_debug_sysenv_change_vbusth_show, bluewhale_sdpm_debug_sysenv_change_vbusth_store);
static DEVICE_ATTR(debug_sysenv_better_ibatth, S_IRUGO | S_IWUSR, bluewhale_sdpm_debug_sysenv_better_ibatth_show, bluewhale_sdpm_debug_sysenv_better_ibatth_store);
static DEVICE_ATTR(debug_avg_info, S_IRUGO | S_IWUSR, bluewhale_sdpm_debug_avg_info_show, bluewhale_sdpm_debug_avg_info_store);


#endif	//O2_CONFIG_SOFTWARE_DPM_SUPPORT
static struct attribute *bluewhale_attributes[] = {
	&dev_attr_registers.attr,
	&dev_attr_vbus_current.attr, //input current limit
	&dev_attr_charger_current.attr, //charge current
	&dev_attr_chg_volt.attr, //charge voltage
	&dev_attr_vbus_volt.attr,     //vbus voltage
	&dev_attr_enable_otg.attr, //enable/disable OTG(boost)
#ifdef DYNAMIC_CV
	&dev_attr_dynamic_cv.attr, //dynamic cv
#endif
#ifdef BATT_TEMP_SIMULATION 
	&dev_attr_batt_temp.attr,
#endif
#if O2_CONFIG_SOFTWARE_DPM_SUPPORT

	//for debugging sdpm

#if O2_CONFIG_ILIMIT_NTAG_SUPPORT
	&dev_attr_config_ntag.attr,
	&dev_attr_ntag_debug_cntnum.attr,
#endif

#if O2_CONFIG_ILIMIT_YTAG_SUPPORT
	&dev_attr_config_ytag.attr,
	&dev_attr_ytag_debug_cntnum.attr,
	&dev_attr_ytag_debug_ibatth.attr,
	&dev_attr_ytag_debug_vbatth.attr,
#endif
#if O2_CONFIG_TRUSTED_VBUS_SUPPORT
	&dev_attr_debug_trusted_vbus.attr,
#endif

	&dev_attr_debug_ilimit_th.attr,
	&dev_attr_debug_ilimit_high.attr,
	&dev_attr_debug_judg_ienough_ibatth.attr,
	&dev_attr_debug_judg_sopdisfunc_ibatth.attr,
	&dev_attr_debug_judg_adosc_ibatth.attr,
	&dev_attr_debug_judg_adosc_vbusth.attr,
	&dev_attr_debug_sysenv_change_ibatth.attr,
	&dev_attr_debug_sysenv_change_vbusth.attr,
	&dev_attr_debug_sysenv_better_ibatth.attr,
	&dev_attr_debug_avg_info.attr,


#endif	//O2_CONFIG_SOFTWARE_DPM_SUPPORT
	NULL,
};

static struct attribute_group bluewhale_attribute_group = {
	.attrs = bluewhale_attributes,
};

static int bluewhale_create_sys(struct device *dev)
{
	int err;

	bluewhale_dbg("bluewhale_create_sysfs");
	
	if(NULL == dev){
		printk(KERN_ERR"creat bluewhale sysfs fail: NULL dev\n");
		return -EINVAL;
	}

	err = sysfs_create_group(&(dev->kobj), &bluewhale_attribute_group);

	if (err) 
	{
		printk(KERN_ERR"creat bluewhale sysfs group fail\n");
		return -EIO;
	}

	printk(KERN_CRIT"creat bluewhale sysfs group ok\n");	
	return err;
}

/*****************************************************************************
 * Description:
 *      bluewhale_get_setting_data	
 * Parameters:
 *		charger:	charger data
 *****************************************************************************/
static void bluewhale_get_setting_data(struct bluewhale_charger_info *charger)
{
	if (!charger)
		return;

	charger->chg_volt = bluewhale_get_chg_volt(charger);

	charger->charger_current = bluewhale_get_charger_current(charger);

	charger->vbus_current = bluewhale_get_vbus_current(charger);

	charger->eoc_current = bluewhale_get_eoc_current(charger);

	//bluewhale_dbg("chg_volt:%d, vbus_current: %d, charger_current: %d, eoc_current: %d", 
			//charger->chg_volt, charger->vbus_current, charger->charger_current, charger->eoc_current);

}

static int bluewhale_check_charger_status(struct bluewhale_charger_info *charger)
{
	u8 data_status;
	u8 data_thm = 0;
	u8 i = 0;
	int ret;
	
	/* Check charger status */
	ret = bluewhale_read_byte(charger, REG_CHARGER_STATUS, &data_status);
	if (ret < 0) {
		printk(KERN_ERR"%s: fail to get Charger status\n", __func__);
		return ret; 
	}

	charger->initial_state = (data_status & CHARGER_INIT) ? 1 : 0;
	charger->in_wakeup_state = (data_status & IN_WAKEUP_STATE) ? 1 : 0;
	charger->in_cc_state = (data_status & IN_CC_STATE) ? 1 : 0;
	charger->in_cv_state = (data_status & IN_CV_STATE) ? 1 : 0;
	charger->chg_full = (data_status & CHARGE_FULL_STATE) ? 1 : 0;
	charger->cc_timeout = (data_status & CC_TIMER_FLAG) ? 1 : 0;
	charger->wak_timeout = (data_status & WK_TIMER_FLAG) ? 1 : 0;
 
	m_charger_full = charger->chg_full;

	/* Check thermal status*/
	ret = bluewhale_read_byte(charger, REG_THM_STATUS, &data_thm);
	if (ret < 0) {
		printk(KERN_ERR"%s: fail to get Thermal status\n", __func__);
		//mutex_unlock(&charger->msg_lock);
		return ret; 
	}
	if (!data_thm)
		charger->thermal_status = THM_DISABLE;
	else {
		for (i = 0; i < 7; i ++) {
			if (data_thm & (1 << i))
				charger->thermal_status = i + 1;
		}
	}

	return ret;
}

static int bluewhale_check_vbus_status(struct bluewhale_charger_info *charger)
{
	int ret = 0;
	u8 data_status = 0;

	/* Check VBUS and VSYS */
	ret = bluewhale_read_byte(charger, REG_VBUS_STATUS, &data_status);

	if(ret < 0) {
		printk(KERN_ERR"fail to get Vbus status\n");
	}

	charger->vdc_pr = (data_status & VDC_PR_FLAG) ? 1 : 0;
	charger->vsys_ovp = (data_status & VSYS_OVP_FLAG) ? 1 : 0;
	charger->vbus_ovp = (data_status & VBUS_OVP_FLAG) ? 1 : 0; //it's reserved for oz1c115
	charger->vbus_uvp = (data_status & VBUS_UVP_FLAG) ? 1 : 0;
	charger->vbus_ok = (data_status & VBUS_OK_FLAG) ? 1 : 0;

	if (!charger->vbus_ok)
		printk(KERN_ERR"invalid adapter or no adapter\n");

	if (charger->vdc_pr)
		bluewhale_dbg("vdc < vdc threshold of system priority");

	return ret > 0 ? charger->vbus_ok : ret;
}

/*****************************************************************************
 * Description:
 *		bluewhale_get_charging_status 
 * Parameters:
 *		charger:	charger data
 * Return:
 *		negative errno if error happens
 *****************************************************************************/
static int bluewhale_get_charging_status(struct bluewhale_charger_info *charger)
{
	int ret = 0;

	bool last_vbus_ok;

	if (!charger)
		return -EINVAL;

	last_vbus_ok = charger->vbus_ok;

	ret = bluewhale_check_vbus_status(charger);

	ret = bluewhale_check_charger_status(charger);

	bluewhale_get_setting_data(charger);

	print_all_register(charger);
	print_all_charger_info(charger);

	return ret;
}

static void print_all_charger_info(struct bluewhale_charger_info *charger)
{
	printk("## bluewhale charger information ##\n"
		"chg_volt: %d, vbus_current: %d, charger_current: %d, eoc_current: %d\n"
		"vdc_pr: %d, vbus_ovp: %d, vbus_ok: %d, vbus_uvp: %d, vsys_ovp: %d,\n"
		"cc_timeout: %d, wak_timeout: %d, chg_full: %d, in_cc_state: %d, in_cv_state: %d\n"
		"initial_state: %d, in_wakeup_state: %d, thermal_status: %d, adapter_type: %d\n",
		charger->chg_volt, charger->vbus_current,
		charger->charger_current,charger->eoc_current,
		charger->vdc_pr, charger->vbus_ovp, charger->vbus_ok,
		charger->vbus_uvp, charger->vsys_ovp,
		charger->cc_timeout, charger->wak_timeout, charger->chg_full,
		charger->in_cc_state, charger->in_cv_state,
		charger->initial_state,charger->in_wakeup_state,
		charger->thermal_status, adapter_type);
}

static void print_all_register(struct bluewhale_charger_info *charger)
{
	u8 i = 0;
	u8 data = 0;

    printk("[bluewhale charger]:\n");

    for (i=REG_CHARGER_VOLTAGE; i<=REG_MIN_VSYS_VOLTAGE; i++)
    {
		bluewhale_read_byte(charger, i, &data);
		printk("[0x%02x]=0x%02x ", i, data);        
    }
    for (i=REG_CHARGE_CURRENT; i<=REG_VBUS_LIMIT_CURRENT; i++)
    {
		bluewhale_read_byte(charger, i, &data);
		printk("[0x%02x]=0x%02x ", i, data);        
	}

	bluewhale_read_byte(charger, REG_SAFETY_TIMER, &data);
	printk("[0x%02x]=0x%02x ", REG_SAFETY_TIMER, data);        

	bluewhale_read_byte(charger, REG_CHARGER_CONTROL, &data);
	printk("[0x%02x]=0x%02x ", REG_CHARGER_CONTROL, data);        

    for (i=REG_VBUS_STATUS; i<=REG_THM_STATUS; i++)
    {
		bluewhale_read_byte(charger, i, &data);
		printk("[0x%02x]=0x%02x ", i, data);        
    }
    printk("\n");
}


#ifdef IR_COMPENSATION	

static int ir_compensation(struct bluewhale_charger_info *charger, int cv_val)
{
	int32_t charge_volt = cv_val;
	int Ibat = 0;
	//int Ibat_main = 0;
#if 0
	int cv_max = 0;
    int cc_sub = 0;
	int32_t ir;
#endif
	int vbat = 0;

	if(!bluewhale_check_vbus_status(charger) || !o2_check_gauge_init_status())
		return cv_val;

	/*********************************************/
	cc_main = bluewhale_get_charger_current(charger);
	bluewhale_dbg("%s: cc_main: %d\n", __func__, cc_main);
#ifdef CONFIG_OZ115_DUAL_SUPPORT
	bluewhale_check_charger_status(charger);
	if ((charger->in_cv_state || charger->chg_full) && cc_main > 1600) {
		bluewhale_dbg("%s: in_cv_state: %d\n",
				__func__, charger->in_cv_state);
		cc_main -=200; //discrease CC of main charger
		ir_cc_dis = 1;
		bluewhale_set_charger_current(charger, cc_main);
		msleep(200);
	}
#endif

	/*********************************************/
	Ibat = adji_get_bat_curr();
    
//#ifdef CONFIG_OZ115_DUAL_SUPPORT
#if  0
	if (Ibat < 2000 && (charger->in_cv_state || charger->chg_full) && charge_cur_sum > 2000 && cc_main <= 1600)
	{
		ir_cc_dis = 1;
		bluewhale_set_charger_current(charger, 0);
		msleep(200);
	}
	else if (charge_cur_sum != -1 && charge_cur_sum <= 2000)
	{
		ir_cc_dis = 0;
	}
#endif

	cc_main = bluewhale_get_charger_current(charger);

	bluewhale_dbg("%s: cc_main: %d, ir_cc_dis:%d\n", __func__, cc_main, ir_cc_dis);

#if 0

	ir = Ibat * charger->pdata->inpedance /1000;
	if (cv_val)
		charge_volt = cv_val + ir;
	else
		charge_volt = charger->pdata->max_charger_voltagemV + ir;

	charge_volt = charge_volt / 25 * 25;

	bluewhale_dbg("%s:Ibat: %d mA, inpedance:%d mohm, CV:%d mV\n",
			      __func__, Ibat, charger->pdata->inpedance, charge_volt);

	if (cv_val && charge_volt < cv_val)
		charge_volt = cv_val;
	else if(charge_volt < charger->pdata->max_charger_voltagemV)
		charge_volt = charger->pdata->max_charger_voltagemV;
#if 0
	if(charge_volt > IR_COMPENSATION_MAX_VOLTAGE) //CV <= 4400 ?
		charge_volt = IR_COMPENSATION_MAX_VOLTAGE;
#endif

/*********************************************/
	//find max Ibat of main charger
	cc_main = bluewhale_get_charger_current(charger);
	cc_sub = charge_cur_sum - cc_main;

	cv_max = (VSYS_MAX_VOLTAGE - 20) - (Ibat - cc_sub)* charger->pdata->inpedance / 1000;
	if(charge_volt > cv_max)
		charge_volt = cv_max;

	bluewhale_dbg("%s: cc_main: %d, cc_sub:%d, cv_max: %d\n", __func__, cc_main, cc_sub, cv_max);

#endif	
/*********************************************/
	vbat = adji_get_bat_volt();
	if ((cv_val && vbat >= cv_val) && Ibat < 1000)
		charge_volt = cv_val;	

#if 1
	if(charge_volt > IR_COMPENSATION_MAX_VOLTAGE) //CV <= 4400
		charge_volt = IR_COMPENSATION_MAX_VOLTAGE;
#endif

	bluewhale_dbg("Vbat:%d mV, Ibat:%d\n", vbat, Ibat);
	bluewhale_dbg("***********calculate CV:%d mV\n", charge_volt);
	return charge_volt;
}
#endif

/*****************************************************************************
 * Description:
 *		bluewhale_get_charging_status_extern 
 * Return:
 *		negative errno if error happens
 *****************************************************************************/
int bluewhale_get_charging_status_extern(void)
{
	int ret = 0;

	mutex_lock(&bluewhale_mutex);
	if (charger_info)
		ret = bluewhale_get_charging_status(charger_info);
	else
		ret = -EINVAL;
	mutex_unlock(&bluewhale_mutex);

	if (ret < 0 || charger_info->vbus_ok == 0)
		return -1;

	if (charger_info->vbus_ok && charger_info->chg_full)
		return 1;
	else
		return 0;
}
EXPORT_SYMBOL(bluewhale_get_charging_status_extern);

void bluewhale_dump_register(void)
{
	mutex_lock(&bluewhale_mutex);

	if (!charger_info) goto end;
	print_all_register(charger_info);
	bluewhale_dbg("Vbat:%d, Ibat: %d, Vbus: %d", 
	adji_get_bat_volt(),         
	adji_get_bat_curr(),         
	adji_get_vbus_volt()); 

end:
	mutex_unlock(&bluewhale_mutex);
}
EXPORT_SYMBOL(bluewhale_dump_register);


/*****************************************************************************
 * Description:
 *		bluewhale_init
 * Parameters:
 *		charger:	charger data
 * Return:
 *		negative errno if error happens
 *****************************************************************************/
static int bluewhale_init(struct bluewhale_charger_info *charger)
{
	struct bluewhale_platform_data *pdata = NULL;

	int ret = 0;

  	printk(KERN_CRIT"O2Micro bluewhale Driver bluewhale_init\n");

	if (!charger) return -EINVAL;
	
	pdata = charger->pdata;

	//*************************************************************************************
	//note: you must test usb type and set vbus limit current.
	//for wall charger you can set current more than 500mA
	// but for pc charger you may set current not more than 500 mA for protect pc usb port
	//************************************************************************************/

	// write rthm  100k
	ret = bluewhale_update_bits(charger, REG_CHARGER_CONTROL, 0x1, pdata->rthm_10k);

	if (
#if defined(CONFIG_MTK_PUMP_EXPRESS_PLUS_20_SUPPORT)
			mtk_pep20_get_is_connect() == true ||
#endif
#if defined(CONFIG_MTK_PUMP_EXPRESS_PLUS_SUPPORT)
			mtk_pep_get_is_connect() == true ||
#endif
			false )
		bluewhale_update_bits(charger, REG_CHARGER_CONTROL, 0x18, 0x3 << 3);
	else //set chager PWM TON time
		bluewhale_update_bits(charger, REG_CHARGER_CONTROL, 0x18, 0x0 << 3);

	/* Min VSYS:3.4V */
    ret = bluewhale_get_min_vsys(charger);
    ret = bluewhale_set_min_vsys(charger, pdata->min_vsys_voltagemV);	
	
//#ifdef CONFIG_ARCH_S5PV210
#if 0
	//CV voltage
	ret = bluewhale_set_chg_volt(charger, pdata->max_charger_voltagemV);

	// useless,we will make this later
	//set ilmit
	bluewhale_set_vbus_current(charger, pdata->vbus_limit_currentmA);
	//set charging current, CC
	bluewhale_set_charger_current(charger, pdata->max_charger_currentmA);
#endif

	/* EOC CHARGE:80 mA */
#ifdef CONFIG_OZ115_DUAL_SUPPORT
	if(oz115_sub_is_enabled() == 1)
		bluewhale_set_eoc_current(charger, pdata->termination_currentmA/2);
	else
		bluewhale_set_eoc_current(charger, pdata->termination_currentmA/2);
#else
	bluewhale_set_eoc_current(charger, pdata->termination_currentmA);
#endif

	/* MAX CC CHARGE TIME:disabled*/
	bluewhale_set_safety_cc_timer(charger, pdata->max_cc_charge_time);
	/* MAX WAKEUP CHARGE TIME:disabled*/
	bluewhale_set_safety_wk_timer(charger, pdata->max_wakeup_charge_time);

	/* RECHG HYSTERESIS:0.1V */
	bluewhale_set_rechg_hystersis(charger, pdata->recharge_voltagemV);
	/* T34 CHARGE VOLTAGE:4.35V */
	bluewhale_set_t34_cv(charger, pdata->T34_charger_voltagemV);
	/* T45 CHARGE VOLTAGE:4.35V */
	bluewhale_set_t45_cv(charger, pdata->T45_charger_voltagemV);

	/* WAKEUP VOLTAGE:default 2.5V */
	bluewhale_set_wakeup_volt(charger, pdata->wakeup_voltagemV);
	/* WAKEUP CURRENT:default 0.1A */
	bluewhale_set_wakeup_current(charger, pdata->wakeup_currentmA);

	adji_get_bat_curr();
	adji_get_bat_volt();

	return ret;
}

/*****************************************************************************
 * Description:
 *		bluewhale_init_extern
 * Parameters:
 *		None
 * Return:
 *		negative errno if error happens
 *****************************************************************************/
int bluewhale_init_extern(void)
{
	int ret = 0;
	mutex_lock(&bluewhale_mutex);
	if (charger_info)
		ret = bluewhale_init(charger_info);
	else
		ret = -EINVAL;
	mutex_unlock(&bluewhale_mutex);
	return ret;
}
EXPORT_SYMBOL(bluewhale_init_extern);

/*****************************************************************************
 * Description:
 *		o2_start_ac_charge
 *****************************************************************************/
static void o2_start_ac_charge(struct bluewhale_charger_info *charger)
{
	if (!charger) return;

	printk(KERN_CRIT"start ac charge\n");

    //disable suspend mode
	bluewhale_update_bits(charger, REG_CHARGER_CONTROL, 0x02, 0 << 1);

	printk(KERN_CRIT"normal charge adpter\n");

	adapter_type = CHG_ADAPTER_NORMAL;

	if (
#if defined(CONFIG_MTK_PUMP_EXPRESS_PLUS_20_SUPPORT)
			mtk_pep20_get_is_connect() == true ||
#endif
#if defined(CONFIG_MTK_PUMP_EXPRESS_PLUS_SUPPORT)
			mtk_pep_get_is_connect() == true ||
#endif
			false )
	{
		adapter_type = CHG_ADAPTER_MTK;
		bluewhale_update_bits(charger, REG_CHARGER_CONTROL, 0x18, 0x3 << 3);
	}

#if defined(CONFIG_ARCH_S5PV210)
	adapter_type = get_chg_adapter_type();
	if (adapter_type > CHG_ADAPTER_NORMAL)
		bluewhale_update_bits(charger, REG_CHARGER_CONTROL, 0x18, 0x3 << 3);
#endif
}


/*****************************************************************************
 * Description:
 *		o2_start_usb_charge
 * Parameters:
 *		None
 *      negative errno if any error
 *****************************************************************************/
static void o2_start_usb_charge(struct bluewhale_charger_info *charger)
{
	printk(KERN_CRIT"start usb charge\n");

	if (!charger) return;

    //disable suspend mode
	bluewhale_update_bits(charger, REG_CHARGER_CONTROL, 0x02, 0 << 1);

	adapter_type = CHG_ADAPTER_PC;
}

/*****************************************************************************
 * Description:
 *		o2_start_charge_extern
 * Parameters:
 *		None
 * Return:
 *      negative errno if any error
 *****************************************************************************/
#ifdef MTK_MACH_SUPPORT
int o2_start_charge_extern(void)
#else
int o2_start_charge_extern(int chg_type)
#endif
{
	int ret = 0;

	mutex_lock(&bluewhale_mutex);
	if (charger_info)
	{
#ifdef MTK_MACH_SUPPORT
		if (BMT_status.charger_type == STANDARD_CHARGER)
#else
		if (O2_CHARGER_AC == chg_type)
#endif
			o2_start_ac_charge(charger_info);
		else
			o2_start_usb_charge(charger_info);
	}
	else
		ret = -EINVAL;
	mutex_unlock(&bluewhale_mutex);
	return ret;
}
EXPORT_SYMBOL(o2_start_charge_extern);

/*****************************************************************************
 * Description:
 *		o2_stop_charge, this is used when plug out adapter cable
 * Parameters:
 *		None
 *****************************************************************************/
static void o2_stop_charge(struct bluewhale_charger_info *charger, u8 cable_out)
{
	int ret = 0;

	printk(KERN_CRIT"%s !!\n", __func__);

	//disable system over-power protection
	bluewhale_update_bits(charger, REG_CHARGER_CONTROL, 0x80, 0x0 << 7);
	
#ifdef IR_COMPENSATION	
	ir_cc_dis = 0;
#endif
	/* stop charging when adapter is in
	 * disable charging, and charger only powers system
     */
	if (!cable_out)
	{
		ret = bluewhale_set_charger_current(charger, 0);
		printk(KERN_CRIT"%s return, cable is not out !!\n", __func__);
		return;
	}

#if O2_CONFIG_SOFTWARE_DPM_SUPPORT
	bluewhale_dis_sdpm() ;
#endif


	ret = bluewhale_set_charger_current(charger, STOP_CHG_CURRENT);
	ret = bluewhale_set_vbus_current(charger, STOP_CHG_ILIMIT);


    //enable suspend mode
	//bluewhale_update_bits(charger, REG_CHARGER_CONTROL, 0x02, 0x1 << 1);

	//disable ki2c
	bluewhale_update_bits(charger, REG_CHARGER_CONTROL, 0x18, 0x0 << 3);

	adapter_type = CHG_ADAPTER_NULL;
}

/*****************************************************************************
 * Description:
 *		o2_stop_charge_extern
 * Parameters:
 *		None
 * Return:
 *      negative errno if any error
 *****************************************************************************/
int o2_stop_charge_extern(u8 cable_out)
{
	int ret = 0;

	mutex_lock(&bluewhale_mutex);
	if (charger_info)
		o2_stop_charge(charger_info, cable_out);
	else
		ret = -EINVAL;
	mutex_unlock(&bluewhale_mutex);
	return ret;
}
EXPORT_SYMBOL(o2_stop_charge_extern);

int o2_suspend_charger_extern(void)
{
	int ret = 0;
	u8 data = 0;

	mutex_lock(&bluewhale_mutex);
	//enable suspend mode
	if (charger_info)
		ret = bluewhale_update_bits(charger_info, REG_CHARGER_CONTROL, 0x02, 1 << 1);
	else
		ret = -EINVAL;

	if (charger_info)
	{
		bluewhale_dbg("%s:", __func__);
		bluewhale_read_byte(charger_info, REG_CHARGER_CONTROL, &data);
		printk("[0x%02x]=0x%02x ", REG_CHARGER_CONTROL, data);        
	}

	mutex_unlock(&bluewhale_mutex);

	return ret;
}
EXPORT_SYMBOL(o2_suspend_charger_extern);

/*****************************************************************************
 * Description:
 *		bluewhale_charger_probe
 * Parameters:
 *		
 * Return:
 *      negative errno if any error
 *****************************************************************************/
static int  bluewhale_charger_probe(struct i2c_client *client,
									const struct i2c_device_id *id)
{
	struct bluewhale_charger_info *charger;
	int ret = 0;
	static int otg_gpio;
	struct gpio_desc *otg_en_gpiod = NULL;

	printk("O2Micro bluewhale Driver Loading\n");

	charger = devm_kzalloc(&client->dev, sizeof(*charger), GFP_KERNEL);
	if (!charger) {
		dev_err(&client->dev, "Can't alloc charger struct\n");
		return -ENOMEM;
	}
	
	charger->pdata = &bluewhale_pdata_default;
	charger->client = client;	
	charger->dev = &client->dev;

	i2c_set_clientdata(client, charger);

	//for public use
	charger_info = charger;

	bluewhale_dbg("O2Micro bluewhale Driver Loading 1");	

#if defined(CONFIG_OF) && defined(OTG_EN_GPIO)
	if(client->dev.of_node){
		of_property_read_u32(client->dev.of_node, "chg_en_gpio", &otg_gpio);
		charger->otg_en_gpio = otg_gpio;
		if (charger->otg_en_gpio < 0){
			printk("%s fail to get otg_en_gpio in dts\n", __func__);
		}
		else{
			printk("%s get otg_en_gpio:%d in dts\n", __func__, charger->otg_en_gpio);
//			ret = gpio_request(charger->otg_en_gpio, "otg_en_gpio");        
			otg_en_gpiod= gpio_to_desc(otg_gpio);
			charger->otg_en_desc = otg_en_gpiod;

			if(!otg_en_gpiod){
				printk("%s %d gpio request failed\n", __func__, charger->otg_en_gpio);
			}
			else
			{
				gpiod_direction_output(otg_en_gpiod, 1);

#if 0

				msleep(500);
				bluewhale_dbg("%s: otg_en_gpio:%d", __func__,
						gpio_get_value(charger->otg_en_gpio));

#endif
				gpiod_set_value(otg_en_gpiod, 0);
				msleep(500);

				bluewhale_dbg("%s: otg_en_gpio:%d", __func__,
						gpio_get_value(charger->otg_en_gpio));
			}
		}
	}
	else {
		printk("dev.of_node is NULL");
		ret = -ENODEV;
		goto sys_failed;
	}

#endif

	//sys/class/i2c-dev/i2c-2/device/2-0010/
	ret = bluewhale_create_sys(&(client->dev));
	if(ret){
		printk(KERN_ERR"Err failed to creat charger attributes\n");
		goto sys_failed;
	}

	bluewhale_init(charger);

	print_all_register(charger);
	print_all_charger_info(charger);

	//set ilmit
	bluewhale_set_vbus_current(charger, charger->pdata->vbus_limit_currentmA);
	//set charging current, CC
	bluewhale_set_charger_current(charger, charger->pdata->max_charger_currentmA);

    //disable suspend mode
	bluewhale_update_bits(charger, REG_CHARGER_CONTROL, 0x02, 0 << 1);

#ifdef OTG_DELAY
	INIT_DELAYED_WORK(&charger->otg_delay_work, otg_delay_work_func);
#endif

	bluewhale_dbg("O2Micro bluewhale Driver Loading 2");
#ifdef MTK_MACH_SUPPORT
    chargin_hw_init_done = KAL_TRUE;
#endif
	return 0;

sys_failed:

	return ret;
}


/*****************************************************************************
 * Description:
 *		bluewhale_charger_remove
 * Parameters:
 *		
 * Return:
 *      negative errno if any error
 *****************************************************************************/
static int  bluewhale_charger_remove(struct i2c_client *client)
{
	struct bluewhale_charger_info *charger = i2c_get_clientdata(client);
	printk(KERN_CRIT"%s: bluewhale Charger Driver removed\n", __func__);
#ifdef OTG_DELAY
	cancel_delayed_work_sync(&charger->otg_delay_work);
#endif
	return 0;
}
/*****************************************************************************
 * Description:
 *		bluewhale_charger_shutdown
 * Parameters:
 *****************************************************************************/
static void  bluewhale_charger_shutdown(struct i2c_client *client)
{
	struct bluewhale_charger_info *charger = i2c_get_clientdata(client);

	printk(KERN_CRIT"%s\n", __func__);
#ifdef OTG_DELAY
	cancel_delayed_work_sync(&charger->otg_delay_work);
#endif

	//configure charger here in case that can't reboot
	if (bluewhale_get_charger_current(charger) == 0)
	{
		bluewhale_set_charger_current(charger, STOP_CHG_CURRENT);
		bluewhale_set_vbus_current(charger, STOP_CHG_ILIMIT);
	}
    //disable system over-power protection
	bluewhale_update_bits(charger, REG_CHARGER_CONTROL, 0x80, 0x0 << 7);
	//disable otg
	bluewhale_enable_otg(charger, 0);
}


/*****************************************************************************
 * Description:
 *		bluewhale_charger_suspend
 * Parameters:
 *		
 * Return:
 *      negative errno if any error
 *****************************************************************************/
static int bluewhale_charger_suspend(struct device *dev)
{
	//struct platform_device * const pdev = to_platform_device(dev);
	//struct bluewhale_charger_info * const charger = platform_get_drvdata(pdev);
	return 0;
}


/*****************************************************************************
 * Description:
 *		bluewhale_charger_resume
 * Parameters:
 *		
 * Return:
 *      negative errno if any error
 *****************************************************************************/
static int bluewhale_charger_resume(struct device *dev)
{
	//struct platform_device * const pdev = to_platform_device(dev);
	//struct bluewhale_charger_info * const charger = platform_get_drvdata(pdev);
	return 0;
}


static const struct i2c_device_id bluewhale_i2c_ids[] = {
		{"bluewhale-charger", 0},
		{}
}; 

MODULE_DEVICE_TABLE(i2c, bluewhale_i2c_ids);

static const struct dev_pm_ops pm_ops = {
        .suspend        = bluewhale_charger_suspend,
        .resume			= bluewhale_charger_resume,
};

#ifdef CONFIG_OF
static const struct of_device_id bluewhale_of_match[] = {
	{.compatible = "o2micro,oz115",},
	{},
};

MODULE_DEVICE_TABLE(of, bluewhale_of_match);
#else
static struct i2c_board_info __initdata i2c_bluewhale={ I2C_BOARD_INFO("bluewhale-charger", 0x10)}; // 7-bit address of 0x20
#endif


static struct i2c_driver bluewhale_charger_driver = {
		.driver		= {
			.name 		= "bluewhale-charger",
			.pm			= &pm_ops,
#ifdef CONFIG_OF
			.of_match_table = bluewhale_of_match,
#endif
		},
		.probe		= bluewhale_charger_probe,
		.shutdown	= bluewhale_charger_shutdown,
		.remove     = bluewhale_charger_remove,
		.id_table	= bluewhale_i2c_ids,
};


static int __init bluewhale_charger_init(void)
{
	int ret = 0;

#ifndef CONFIG_OF
	static struct i2c_client *charger_client; //Notice: this is static pointer
	struct i2c_adapter *i2c_adap;

	i2c_adap = i2c_get_adapter(CHARGER_I2C_NUM);
	if (i2c_adap)
	{
		charger_client = i2c_new_device(i2c_adap, &i2c_bluewhale);
		i2c_put_adapter(i2c_adap);
	}
	else {
		pr_err("failed to get adapter %d.\n", CHARGER_I2C_NUM);
		pr_err("statically declare I2C devices\n");

		ret = i2c_register_board_info(CHARGER_I2C_NUM, &i2c_bluewhale, 1);

		if(ret != 0)
			pr_err("failed to register i2c_board_info.\n");
	}
#endif
	pr_err("%s\n", __func__);

	ret = i2c_add_driver(&bluewhale_charger_driver);

    if(ret != 0)
        pr_err("failed to register bluewhale_charger i2c driver.\n");
    else
        pr_err("Success to register bluewhale_charger i2c driver.\n");

	return ret;
}

static void __exit bluewhale_charger_exit(void)
{
	printk(KERN_CRIT"bluewhale driver exit\n");
	
	i2c_del_driver(&bluewhale_charger_driver);
}

module_init(bluewhale_charger_init);
module_exit(bluewhale_charger_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("O2Micro");
MODULE_DESCRIPTION("O2Micro BlueWhale Charger Driver");
