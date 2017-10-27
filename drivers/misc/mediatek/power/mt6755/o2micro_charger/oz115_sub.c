/*****************************************************************************
* Copyright(c) O2Micro, 2013. All rights reserved.
*	
* O2Micro oz1c115 charger driver
* File: oz115_sub_charger.c

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

#include "bluewhale_charger.h"
//#include "charger_config.h"

#ifdef O2_GAUGE_SUPPORT
#include "../o2micro_battery/parameter.h"
#endif

#ifdef MTK_MACH_SUPPORT
#include <mt-plat/battery_common.h>
#include <mt-plat/battery_meter_hal.h>
#endif

#define VERSION		   "2016.5.07/6.00.01"
#define	RETRY_CNT	5

//#define BATT_TEMP_SIMULATION 
#define SUB_IR_COMPENSATION	

#define DBG_LEVEL KERN_ERR
#define oz115_sub_dbg(fmt, args...) printk(DBG_LEVEL"[oz115_sub]:"pr_fmt(fmt)"\n", ## args)
//#define ilimit_dbg(fmt, args...) printk(DBG_LEVEL"[sub_ilimit_work]:"pr_fmt(fmt)"\n", ## args)


static struct bluewhale_platform_data  oz115_sub_pdata_default = {
        .max_charger_currentmA = TARGET_NORMAL_CHG_CURRENT,
		.max_charger_voltagemV = O2_CONFIG_CV_VOLTAGE,
		.T34_charger_voltagemV = O2_CONFIG_CV_VOLTAGE,    //dangous
		.T45_charger_voltagemV = O2_CONFIG_CV_VOLTAGE,    //dangous	
		.termination_currentmA = O2_CHARGER_EOC,
		.wakeup_voltagemV = 3000,
		.wakeup_currentmA = 400,
		.recharge_voltagemV = 100,
		.min_vsys_voltagemV = 1800, //set the min one for sub charger
		.vbus_limit_currentmA = 1000,
		.max_cc_charge_time = 0,
		.max_wakeup_charge_time = 0,
		.rthm_10k = 0,			//1 for 10K, 0 for 100K thermal resistor
		.inpedance = O2_CHARGER_INPEDANCE,
};

#if defined(CONFIG_MTK_PUMP_EXPRESS_PLUS_SUPPORT)
extern bool mtk_pep_get_is_connect(void);
#endif

#if defined(CONFIG_MTK_PUMP_EXPRESS_PLUS_20_SUPPORT)
extern bool mtk_pep20_get_is_connect(void);
#endif

static struct bluewhale_charger_info *oz115_sub_info;
static DEFINE_MUTEX(oz115_sub_mutex);


static uint8_t start_adjust_ilimit = 0;
static int max_ilimit = 0;
static u8 find_max_ilimit = 0;


static uint8_t chg_adapter_type = CHG_ADAPTER_NULL;
#ifdef BATT_TEMP_SIMULATION 
static int batt_temp_simulation = 25;
#endif

#if !defined(O2_GAUGE_SUPPORT) && !defined(MTK_MACH_SUPPORT)
static struct power_supply *battery_psy = NULL;
#endif

static int oz115_sub_init(struct bluewhale_charger_info *charger);
static void o2_stop_charge(struct bluewhale_charger_info *charger, u8 cable_out);
static void o2_start_ac_charge(struct bluewhale_charger_info *charger);
static void o2_start_usb_charge(struct bluewhale_charger_info *charger);
static int get_battery_current(void);
static int get_battery_voltage(void);
static int32_t o2_read_vbus_voltage(void);
#ifdef SUB_IR_COMPENSATION	
static int cc_sub = 0;
static int sub_ir_compensation(struct bluewhale_charger_info *charger, int cv_val);
extern int ir_cc_dis;
#endif
extern int m_charger_full;
int charge_cur_sum = -1;
extern int cc_main;


static void print_all_register(struct bluewhale_charger_info *charger);
static void print_all_charger_info(struct bluewhale_charger_info *charger);
static int oz115_sub_check_vbus_status(struct bluewhale_charger_info *charger);
static void oz115_sub_get_setting_data(struct bluewhale_charger_info *charger);
static int oz115_sub_get_charging_status(struct bluewhale_charger_info *charger);
static int oz115_sub_get_charger_current(struct bluewhale_charger_info *charger);
static int oz115_sub_get_vbus_current(struct bluewhale_charger_info *charger);


/*****************************************************************************
 * Description:
 *		oz115_sub_read_byte 
 * Parameters:
 *		charger:	charger data
 *		index:	register index to be read
 *		*dat:	buffer to store data read back
 * Return:
 *      negative errno else a data byte received from the device.
 *****************************************************************************/
static int32_t oz115_sub_read_byte(struct bluewhale_charger_info *charger, uint8_t index, uint8_t *dat)
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
 *		oz115_sub_write_byte 
 * Parameters:
 *		charger:	charger data
 *		index:	register index to be write
 *		dat:		write data
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
static int32_t oz115_sub_write_byte(struct bluewhale_charger_info *charger, uint8_t index, uint8_t dat)
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

static int32_t oz115_sub_update_bits(struct bluewhale_charger_info *charger, uint8_t reg, uint8_t mask, uint8_t data)
{
	int32_t ret;
	uint8_t tmp;

	ret = oz115_sub_read_byte(charger, reg, &tmp);
	if (ret < 0)
		return ret;

	if ((tmp & mask) != data)
	{
		tmp &= ~mask;
		tmp |= data & mask;
		return oz115_sub_write_byte(charger, reg, tmp);

	}
	else
		return 0;
}

/*****************************************************************************/

/*****************************************************************************
 * Description:
 *		oz115_sub_set_min_vsys 
 * Parameters:
 *		charger:	charger data
 *		min_vsys_mv:	min sys voltage to be written
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
static int oz115_sub_set_min_vsys(struct bluewhale_charger_info *charger, int min_vsys_mv)
{
	int ret = 0;
	u8 vsys_val = 0; 

	if (min_vsys_mv < 1800)
		min_vsys_mv = 1800;		//limit to 1.8V
	else if (min_vsys_mv > 3600)
		min_vsys_mv = 3600;		//limit to 3.6V

	vsys_val = min_vsys_mv / VSYS_VOLT_STEP;	//step is 200mV

	ret = oz115_sub_update_bits(charger, REG_MIN_VSYS_VOLTAGE, 0x1f, vsys_val);

	return ret;
}

static int oz115_sub_get_min_vsys(struct bluewhale_charger_info *charger)
{
	uint8_t data;
	int ret = 0;

	ret = oz115_sub_read_byte(charger, REG_MIN_VSYS_VOLTAGE, &data);
	if (ret < 0)
		return ret;

	data &= 0x1f;
	return (data * VSYS_VOLT_STEP);	//step is 200mV
}

/*****************************************************************************
 * Description:
 *		oz115_sub_set_chg_volt 
 * Parameters:
 *		charger:	charger data
 *		chgvolt_mv:	charge voltage to be written
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
static int oz115_sub_set_chg_volt(struct bluewhale_charger_info *charger, int chgvolt_mv)
{
	int ret = 0;
	u8 chg_volt = 0; 

	if (chgvolt_mv < 4000)
		chgvolt_mv = 4000;		//limit to 4.0V
	else if (chgvolt_mv > 4600)
		chgvolt_mv = 4600;		//limit to 4.6V

	chg_volt = (chgvolt_mv - 4000) / CHG_VOLT_STEP;	//step is 25mV

	ret = oz115_sub_update_bits(charger, REG_CHARGER_VOLTAGE, 0x1f, chg_volt);

	return ret;
}

static int oz115_sub_get_chg_volt(struct bluewhale_charger_info *charger)
{
	uint8_t data;
	int ret = 0;

	ret = oz115_sub_read_byte(charger, REG_CHARGER_VOLTAGE, &data);
	if (ret < 0)
		return ret;
	return (data * CHG_VOLT_STEP);	//step is 25mA
}

/*****************************************************************************
 * Description:
 *		oz115_sub_set_chg_volt_extern 
 * Parameters:
 *		chgvolt_mv:	charge voltage to be written
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
int oz115_sub_set_chg_volt_extern(int chgvolt_mv)
{
	int ret = 0;

	mutex_lock(&oz115_sub_mutex);
	if (oz115_sub_info)
	{
		oz115_sub_dbg("***********try to set %d mV\n", chgvolt_mv);
		
		if (chgvolt_mv == 4400|| chgvolt_mv == 4350) {
			chgvolt_mv += 25;
			oz115_sub_dbg("***********modify to %d mV\n", chgvolt_mv);
		}
#if defined(SUB_IR_COMPENSATION)	
		if (o2_read_vbus_voltage() >= 6000)
			chgvolt_mv = sub_ir_compensation(oz115_sub_info, chgvolt_mv);
#endif
		ret = oz115_sub_set_chg_volt(oz115_sub_info, chgvolt_mv);

		oz115_sub_dbg("***********finale CV:%d mV\n", oz115_sub_get_chg_volt(oz115_sub_info));
	}
	else
		ret = -EINVAL;
	mutex_unlock(&oz115_sub_mutex);
	return ret;
}
EXPORT_SYMBOL(oz115_sub_set_chg_volt_extern);

/*****************************************************************************
 * Description:
 *		oz115_sub_set_t34_cv
 * Parameters:
 *		charger:	charger data
 *		chgvolt_mv:	charge voltage to be written at t34
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
static int oz115_sub_set_t34_cv(struct bluewhale_charger_info *charger, int chgvolt_mv)
{
	int ret = 0;
	u8 chg_volt = 0; 

	if (chgvolt_mv < 4000)
		chgvolt_mv = 4000;		//limit to 4.0V
	else if (chgvolt_mv > 4600)
		chgvolt_mv = 4600;		//limit to 4.6V

	chg_volt = (chgvolt_mv - 4000) / CHG_VOLT_STEP;	//step is 25mV

	ret = oz115_sub_update_bits(charger, REG_T34_CHG_VOLTAGE, 0x1f, chg_volt);

	return ret;
}

/*****************************************************************************
 * Description:
 *		oz115_sub_set_t45_cv 
 * Parameters:
 *		charger:	charger data
 *		chgvolt_mv:	charge voltage to be written at t45
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
static int oz115_sub_set_t45_cv(struct bluewhale_charger_info *charger, int chgvolt_mv)
{
	int ret = 0;
	u8 chg_volt = 0; 

	if (chgvolt_mv < 4000)
		chgvolt_mv = 4000;		//limit to 4.0V
	else if (chgvolt_mv > 4600)
		chgvolt_mv = 4600;		//limit to 4.6V

	chg_volt = (chgvolt_mv - 4000) / CHG_VOLT_STEP;	//step is 25mV

	ret = oz115_sub_update_bits(charger, REG_T45_CHG_VOLTAGE, 0x1f, chg_volt);

	return ret;
}

/*****************************************************************************
 * Description:
 *		oz115_sub_set_wakeup_volt 
 * Parameters:
 *		charger:	charger data
 *		wakeup_mv:	set wake up voltage
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
static int oz115_sub_set_wakeup_volt(struct bluewhale_charger_info *charger, int wakeup_mv)
{
	int ret = 0;
	u8 data = 0;
	u8 wak_volt = 0; 

	if (wakeup_mv < 1500)
		wakeup_mv = 1500;		//limit to 1.5V
	else if (wakeup_mv > 3000)
		wakeup_mv = 3000;		//limit to 3.0V

	wak_volt = wakeup_mv / WK_VOLT_STEP;	//step is 100mV

	ret = oz115_sub_read_byte(charger, REG_WAKEUP_VOLTAGE, &data);
	if (ret < 0)
		return ret;
	if (data != wak_volt) 
		ret = oz115_sub_write_byte(charger, REG_WAKEUP_VOLTAGE, wak_volt);	
	return ret;
}

/*****************************************************************************
 * Description:
 *		oz115_sub_set_eoc_current 
 * Parameters:
 *		charger:	charger data
 *		eoc_ma:	set end of charge current
 *		Only 0mA, 20mA, 40mA, ... , 320mA can be accurate
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
static int oz115_sub_set_eoc_current(struct bluewhale_charger_info *charger, int eoc_ma)
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

	ret = oz115_sub_read_byte(charger, REG_END_CHARGE_CURRENT, &data);
	if (ret < 0)
		return ret;
	if (data != eoc_curr) 
		ret = oz115_sub_write_byte(charger, REG_END_CHARGE_CURRENT, eoc_curr);	
	return ret;
}

static int oz115_sub_get_eoc_current(struct bluewhale_charger_info *charger)
{
	uint8_t data;
	int ret = 0;

	ret = oz115_sub_read_byte(charger, REG_END_CHARGE_CURRENT, &data);
	if (ret < 0)
		return ret;
	return (data * EOC_CURRT_STEP);	//step is 10mA
}

/*****************************************************************************
 * Description:
 *		oz115_sub_set_eoc_current_extern 
 * Parameters:
 *		eoc_ma:	set end of charge current
 *		Only 0mA, 20mA, 40mA, ... , 320mA can be accurate
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
int oz115_sub_set_eoc_current_extern(int eoc_ma)
{
	int ret = 0;

	mutex_lock(&oz115_sub_mutex);
	if (oz115_sub_info)
		ret = oz115_sub_set_eoc_current(oz115_sub_info, eoc_ma);
	else
		ret = -EINVAL;
	mutex_unlock(&oz115_sub_mutex);
	return ret;
}
EXPORT_SYMBOL(oz115_sub_set_eoc_current_extern);

/*****************************************************************************
 * Description:
 *		oz115_sub_set_vbus_current
 * Parameters:
 *		charger:	charger data
 *		ilmt_ma:	set input current limit
 *		Only 100mA, 500mA, 700mA, 900mA, 1000mA, 1200mA, 1400mA, 1500mA, 1700mA, 1900mA, 2000mA can be accurate
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
static int oz115_sub_set_vbus_current(struct bluewhale_charger_info *charger, int ilmt_ma)
{
	int ret = 0;
	u8 data = 0;
	u8 input_curr = 0;

	oz115_sub_dbg("%s: try to set %d mA", __func__, ilmt_ma);

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

	ret = oz115_sub_read_byte(charger, REG_VBUS_LIMIT_CURRENT, &data);
	if (ret < 0)
		return ret;
		
	if (data != input_curr) 
		ret = oz115_sub_write_byte(charger, REG_VBUS_LIMIT_CURRENT, input_curr);

	oz115_sub_dbg("%s: final set %d mA",
			__func__, oz115_sub_get_vbus_current(charger));
	return ret;
}

/*****************************************************************************
 * Description:
 *		oz115_sub_set_vbus_current_extern
 * Parameters:
 *		ilmt_ma:	set input current limit
 *		Only 100mA, 500mA, 700mA, 900mA, 1000mA, 1200mA, 1400mA, 1500mA, 1700mA, 1900mA, 2000mA can be accurate
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
int oz115_sub_set_vbus_current_extern(int ilmt_ma)
{
	int ret = 0;

	mutex_lock(&oz115_sub_mutex);
	if (oz115_sub_info)
		ret = oz115_sub_set_vbus_current(oz115_sub_info, ilmt_ma);
	else
		ret = -EINVAL;
	mutex_unlock(&oz115_sub_mutex);
	return ret;
}
EXPORT_SYMBOL(oz115_sub_set_vbus_current_extern);


/*****************************************************************************
 * Description:
 *		oz115_sub_get_vbus_current
 * Parameters:
 *		charger:	charger data
 * Return:
 *		Vbus input current in mA
 *****************************************************************************/
static int oz115_sub_get_vbus_current(struct bluewhale_charger_info *charger)
{
	int ret = 0;
	u8 data = 0;

	ret = oz115_sub_read_byte(charger, REG_VBUS_LIMIT_CURRENT, &data);
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
 *		oz115_sub_get_vbus_current_extern
 * Return:
 *		Vbus input current in mA
 *****************************************************************************/
int oz115_sub_get_vbus_current_extern(void)
{
	int ret = 0;

	mutex_lock(&oz115_sub_mutex);
	if (oz115_sub_info)
		ret = oz115_sub_get_vbus_current(oz115_sub_info);
	else
		ret = -EINVAL;
	mutex_unlock(&oz115_sub_mutex);
	return ret;
}
EXPORT_SYMBOL(oz115_sub_get_vbus_current_extern);

/*****************************************************************************
 * Description:
 *		oz115_sub_set_rechg_hystersis
 * Parameters:
 *		charger:	charger data
 *		hyst_mv:	set Recharge hysteresis Register
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
static int oz115_sub_set_rechg_hystersis(struct bluewhale_charger_info *charger, int hyst_mv)
{
	int ret = 0;
	u8 data = 0;
	u8 rechg = 0; 
		
	if (hyst_mv > 200) {
		hyst_mv = 200;			//limit to 200mV
	}
	//Notice: with 00h, recharge function is disabled

	rechg = hyst_mv / RECHG_VOLT_STEP;	//step is 50mV
	
	ret = oz115_sub_read_byte(charger, REG_RECHARGE_HYSTERESIS, &data);
	if (ret < 0)
		return ret;
	if (data != rechg) 
		ret = oz115_sub_write_byte(charger, REG_RECHARGE_HYSTERESIS, rechg);	
	return ret;
}

/*****************************************************************************
 * Description:
 *		oz115_sub_set_charger_current
 * Parameters:
 *		charger:	charger data
 *		chg_ma:	set charger current
 *		Only 600mA, 800mA, 1000mA, ... , 3800mA, 4000mA can be accurate
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
static int oz115_sub_set_charger_current(struct bluewhale_charger_info *charger, int chg_ma)
{
	int ret = 0;
	u8 data = 0;
	u8 chg_curr = 0; 

	oz115_sub_dbg("%s: try to set %d mA",
			__func__, chg_ma);

	if (chg_ma > 4000)
		chg_ma = 4000;		//limit to 4A		
	else if (chg_ma < 600 && chg_ma > 0)
		chg_ma = 600;		//limit to 600 mA		

	//notice: chg_curr value less than 06h, charger will be disabled.
	//charger can power system in this case.

	chg_curr = chg_ma / CHG_CURRT_STEP;	//step is 100mA

	oz115_sub_dbg("%s:%d", __func__, chg_curr);

	ret = oz115_sub_read_byte(charger, REG_CHARGE_CURRENT, &data);
	if (ret < 0)
		return ret;
	if (data != chg_curr) 
		ret = oz115_sub_write_byte(charger, REG_CHARGE_CURRENT, chg_curr);	

	oz115_sub_dbg("%s: final set %d mA",
			__func__, oz115_sub_get_charger_current(charger));
	return ret;
}

static int oz115_sub_get_charger_current(struct bluewhale_charger_info *charger)
{
	uint8_t data;
	int ret = 0;

	ret = oz115_sub_read_byte(charger, REG_CHARGE_CURRENT, &data);
	if (ret < 0)
		return ret;
	return (data * CHG_CURRT_STEP);	//step is 100mA
}

int oz115_sub_get_charger_current_extern(void)
{
	int ret = 0;

	mutex_lock(&oz115_sub_mutex);
	if (oz115_sub_info)
		ret = oz115_sub_get_charger_current(oz115_sub_info);
	else
		ret = -EINVAL;
	mutex_unlock(&oz115_sub_mutex);
	return ret;
}
EXPORT_SYMBOL(oz115_sub_get_charger_current_extern);

/*****************************************************************************
 * Description:
 *		oz115_sub_set_charger_current_extern
 * Parameters:
 *		chg_ma:	set charger current
 *		Only 600mA, 800mA, 1000mA, ... , 3800mA, 4000mA can be accurate
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
int oz115_sub_set_charger_current_extern(int chg_ma)
{
	int ret = 0;

	mutex_lock(&oz115_sub_mutex);

#ifdef SUB_IR_COMPENSATION	
	oz115_sub_dbg("%s: cc_sub:  %d, ir_cc_dis:%d\n", __func__, cc_sub, ir_cc_dis);
	if (ir_cc_dis > 0 && chg_ma != 0) chg_ma = charge_cur_sum - cc_main;
#endif

	if (oz115_sub_info)
		ret = oz115_sub_set_charger_current(oz115_sub_info, chg_ma);
	else
		ret = -EINVAL;
	mutex_unlock(&oz115_sub_mutex);
	return ret;
}
EXPORT_SYMBOL(oz115_sub_set_charger_current_extern);

int oz115_set_charge_current_sum_extern(int chg_ma_sum)
{
	mutex_lock(&oz115_sub_mutex);
	charge_cur_sum = chg_ma_sum;
	mutex_unlock(&oz115_sub_mutex);
	return 0;
}
EXPORT_SYMBOL(oz115_set_charge_current_sum_extern);
/*****************************************************************************
 * Description:
 *		oz115_sub_set_wakeup_current 
 * Parameters:
 *		charger:	charger data
 *		wak_ma:	set wakeup current
 *		Only 100mA, 120mA, ... , 400mA can be accurate
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
static int oz115_sub_set_wakeup_current(struct bluewhale_charger_info *charger, int wak_ma)
{
	int ret = 0;
	u8 data = 0;
	u8 wak_curr = 0; 
	if (wak_ma < 100)
		wak_ma = 100;		//limit to 100mA
	if (wak_ma > 400)
		wak_ma = 400;		//limit to 400mA
	wak_curr = wak_ma / WK_CURRT_STEP;	//step is 10mA

	ret = oz115_sub_read_byte(charger, REG_WAKEUP_CURRENT, &data);
	if (ret < 0)
		return ret;
	if (data != wak_curr) 
		ret = oz115_sub_write_byte(charger, REG_WAKEUP_CURRENT, wak_curr);
	return ret;
}

/*****************************************************************************
 * Description:
 *		oz115_sub_set_safety_cc_timer
 * Parameters:
 *		charger:	charger data
 *		tmin:	set safety cc charge time
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
static int oz115_sub_set_safety_cc_timer(struct bluewhale_charger_info *charger, int tmin)
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
		oz115_sub_dbg("%s: invalide value, set default value 180 minutes\n", __func__);
		tval = CC_TIMER_180MIN;	//default value
	}

	ret = oz115_sub_read_byte(charger, REG_SAFETY_TIMER, &data);
	if (ret < 0)
		return ret;

	if ((data & CC_TIMER_MASK) != tval) 
	{
		data &= WAKEUP_TIMER_MASK;	//keep wk timer
		data |= tval;				//update new cc timer
		ret = oz115_sub_write_byte(charger, REG_SAFETY_TIMER, data);	
	}
	return ret;
}

/*****************************************************************************
 * Description:
 *		oz115_sub_set_safety_wk_timer 
 * Parameters:
 *		charger:	charger data
 *		tmin:	set safety wakeup time
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
static int oz115_sub_set_safety_wk_timer(struct bluewhale_charger_info *charger, int tmin)
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
		oz115_sub_dbg("%s: invalide value, set default value 30 minutes\n", __func__);
		tval = WK_TIMER_30MIN;	//default value
	}

	ret = oz115_sub_read_byte(charger, REG_SAFETY_TIMER, &data);
	if (ret < 0)
		return ret;

	if ((data & WAKEUP_TIMER_MASK) != tval) 
	{
		data &= CC_TIMER_MASK;		//keep cc timer
		data |= tval;				//update new wk timer
		ret = oz115_sub_write_byte(charger, REG_SAFETY_TIMER, data);		
	} 
	return ret;
}

/*****************************************************************************
 * Description: 
 * 		oz115_sub_enable_otg
 * Parameters:
 *		charger:	charger data
 *		enable:	enable/disable OTG
 *		1, 0
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
static int oz115_sub_enable_otg(struct bluewhale_charger_info *charger, int enable)
{
	uint8_t data;
	uint8_t mask = 0;

    data = (enable == 0) ? 0 : 1;

	if (data) //enable otg, disable suspend
		mask = 0x06;
	else //disable otg
		mask = 0x04;

	return oz115_sub_update_bits(charger, REG_CHARGER_CONTROL, mask, data << 2);
}


/*****************************************************************************
 * Description:
 *		oz115_sub_enable_otg_extern
 * Parameters:
 *		enable:	enable/disable OTG
 *		1, 0
 * Return:
 *      negative errno, zero on success.
 *****************************************************************************/
int oz115_sub_enable_otg_extern(int enable)
{
	int ret = 0;

	mutex_lock(&oz115_sub_mutex);
	if (oz115_sub_info)
		ret = oz115_sub_enable_otg(oz115_sub_info, enable);
	else
		ret = -EINVAL;
	mutex_unlock(&oz115_sub_mutex);
	return ret;
}
EXPORT_SYMBOL(oz115_sub_enable_otg_extern);

/*****************************************************************************
 * Description:
 *		o2_read_vbus_voltage
 * Parameters:
 *		void
 * Return:
 *		vbus voltage, or negative errno
 *****************************************************************************/
static int32_t o2_read_vbus_voltage(void)
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
#endif
	
	oz115_sub_dbg("vbus voltage:%d", vbus_voltage);
	return vbus_voltage;
}

static inline int32_t o2_check_gauge_init_status(void)
{
#if defined O2_GAUGE_SUPPORT
	return oz8806_get_init_status();
#else
	return 1;
#endif
}

static int get_battery_voltage(void)
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

static int get_battery_current(void)
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


#ifdef ADJUST_ILIMIT

#define PMIC_UVLO (4350)
#define ILIMIT_2000 (2000)
#define ILIMIT_1500 (1500)
#define ILIMIT_1000 (1000)
#define IBAT_MIN_1 (500)
#define IBAT_MIN_2 (300)
#define IBAT_MIN_3 (250)
#define ILIMIT_STEP (9)

static u8 max_count = 0;
static int ilimit_increase[ILIMIT_STEP] = {500, 700, 900, 1000, 1200, 1500, 1700, 1900, 2000};
static int bat_cur[ILIMIT_STEP] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
#if 0
static int oz115_sub_get_system_loading(struct bluewhale_charger_info *charger, int ilimit)
{
	int loading_avg = 0;

//	if (find_max_ilimit)
	{
#if defined(O2_GAUGE_SUPPORT)
		loading_avg = oz8806_get_loading_avg();
#endif
	}
#if 0
	if (loading_avg == 0)
	{
		if(get_battery_voltage() < O2_CONFIG_EOD)
			return loading_avg;

		//enable suspend mode, shutdown charger.
		oz115_sub_update_bits(charger, REG_CHARGER_CONTROL, 0x02, 1 << 1);

		msleep(3000);//depend on time ADC measurement takes
		loading_avg = get_battery_current();

		//disable suspend mode, re-start charger
		oz115_sub_update_bits(charger, REG_CHARGER_CONTROL, 0x02, 0 << 1);

	}
#endif

	if (loading_avg < 0) loading_avg = 0 - loading_avg;

	return loading_avg;
}
#endif
static int check_ilimit_status(struct bluewhale_charger_info *charger, int ilimit)
{
	int vbus_voltage = 0;
	static int battery_current = 0;
	int vbus_current = 0;
	int cc_current = 0;

	int ret = 0;
	int err = 0;
	int8_t data_status = 0;
	//int loading_avg = 0;
	int bat_cur_diff = 0;
	int ilimit_diff = 0;
	int bat_cur_step = 0;
	int wait_count = 0;

	if (!start_adjust_ilimit && find_max_ilimit)
		return 1;
	else if (!start_adjust_ilimit && !find_max_ilimit)
		return 0;

	//check VDC_PR
	err = oz115_sub_read_byte(charger, REG_VBUS_STATUS, &data_status);

	charger->vdc_pr = (data_status & VDC_PR_FLAG) ? 1 : 0;

	if (charger->vdc_pr)
	{
		++ret;
		return ret;
	}

	msleep(5);

	if (!start_adjust_ilimit && find_max_ilimit)
		return 1;
	else if (!start_adjust_ilimit && !find_max_ilimit)
		return 0;
	//check ilimit
	vbus_current = oz115_sub_get_vbus_current(charger);
	ilimit_dbg("[check_ilimit_status] get vbus_current: %d", vbus_current);
	if (vbus_current < ilimit)
	{
		ilimit_dbg("[check_ilimit_status] ilimit:%d not ok, should lower ilimt", vbus_current);
		++ret;
		return ret;
	}

	while (ilimit >= ilimit_increase[bat_cur_step])
	{
		if (ilimit == ilimit_increase[bat_cur_step])
			break;

		bat_cur_step ++;
	}

	if (bat_cur_step < 0)
		bat_cur_step = 0;
	else if (bat_cur_step >= ILIMIT_STEP)
		bat_cur_step = ILIMIT_STEP - 1;
#if 0
	if (!find_max_ilimit && bat_cur_step > 0)
	{
		loading_avg = oz115_sub_get_system_loading(charger, ilimit);
		ilimit_dbg("[check_ilimit_status]loading_avg:%d", loading_avg);
	}
#endif

	if (find_max_ilimit)
	{
		msleep(1000); //over times ADC measurement takes
	}

	if (!start_adjust_ilimit && find_max_ilimit)
		return 1;
	else if (!start_adjust_ilimit && !find_max_ilimit)
		return 0;

	//check Vbus < PMIC UVLO
	vbus_voltage =  o2_read_vbus_voltage();
	ilimit_dbg("[check_ilimit_status] get vbus_voltage: %d", vbus_voltage);
	if (vbus_voltage < PMIC_UVLO)
	{
		++ret;
		return ret;
	}
#if 0
	else if (vbus_voltage > 5000 && !find_max_ilimit && vbus_current < 2000)
	{
		ilimit_dbg("[check_ilimit_status] Vbus > 5000mV, should try higher ilimit");
		++ret;
		return ret;
	}
#endif

	//check Icharging
	//NOTICE: here, should get the latest battery current from ADC

	while (find_max_ilimit && get_battery_current() == battery_current && wait_count < 6)
	{
		wait_count++;
		msleep(500); //over times ADC measurement takes
		ilimit_dbg("[check_ilimit_status] wait battery current updated: %d ms", wait_count * 500);
	}
	battery_current = get_battery_current();

	ilimit_dbg("[check_ilimit_status] get battery_current: %d", battery_current);
	cc_current = oz115_sub_get_charger_current(charger);
	ilimit_dbg("[check_ilimit_status] CC current: %d", cc_current);

	if(get_battery_voltage() >= (O2_CONFIG_CV_VOLTAGE - 50) || charger->chg_full) {

		ilimit_dbg("[check_ilimit_status] battery is almost full");
		if (charger->chg_full)
			return ret;
		else if (battery_current < -5  && vbus_current > 1000)
			return ++ret;
		else if (battery_current >= -5)
			return ret;
	}

	//if (find_max_ilimit)
	if (1)
	{
		bat_cur[bat_cur_step] = battery_current;

		if (bat_cur_step > 0)
		{
			bat_cur_diff = bat_cur[bat_cur_step] - bat_cur[bat_cur_step - 1];
			ilimit_diff = ilimit - ilimit_increase[bat_cur_step - 1];

			ilimit_dbg("[check_ilimit_status] bat_crr[%d](%d), bat_crr[%d](%d)",
					bat_cur_step, bat_cur[bat_cur_step], bat_cur_step - 1, bat_cur[bat_cur_step - 1]);

			ilimit_dbg("[check_ilimit_status] bat_cur_diff(%d), ilimit_diff(%d)",
						bat_cur_diff, ilimit_diff);

			ilimit_diff -= ilimit_diff/100 * 50;

			if (bat_cur[bat_cur_step] > 100)
				return ret;
			else if (bat_cur_diff < ilimit_diff)
			{
				++ret;
				return ret;
			}
		}
		else
		{
			ilimit_dbg("[check_ilimit_status] bat_crr[%d](%d)",
					bat_cur_step, bat_cur[bat_cur_step]);
		}
	}
	else if (bat_cur_step > 0)
	{
		if ((battery_current < 0) && (battery_current + 500 < bat_cur[bat_cur_step]))
		{
			++ret;
			return ret;
		}
	}
#if 0
	{
		if(get_battery_voltage() >= (O2_CONFIG_CV_VOLTAGE - 50) || charger->chg_full)
			return ret;

		if(((battery_current < IBAT_MIN_1) && (vbus_current >= ILIMIT_2000)) || 
				((battery_current < IBAT_MIN_2) && (vbus_current >= ILIMIT_1500)) ||
				((battery_current < IBAT_MIN_3) && (vbus_current >= ILIMIT_1000)))
		{
			if(((bat_cur[bat_cur_step] >= IBAT_MIN_1) && (vbus_current >= ILIMIT_2000)) || 
					((bat_cur[bat_cur_step] >= IBAT_MIN_2) && (vbus_current >= ILIMIT_1500)) ||
					((bat_cur[bat_cur_step] >= IBAT_MIN_3) && (vbus_current >= ILIMIT_1000)))
			{
				++ret;

				ilimit_dbg("[check_ilimit_status] charging current:%d too small, discrease ilimit", battery_current);
				return ret;
			}
		}
	}
#endif

	return ret;
}

static int charger_adjust_ilimit_increase_max(struct bluewhale_charger_info *charger, int ilimit_min, int ilimit_max)
{
	int vbus_current = 0;
	int ret = 0;
	int fail = 0;

	if (!ilimit_min)
		ilimit_min = ilimit_increase[0];

	if (!ilimit_max)
		ilimit_max = ilimit_increase[ARRAY_SIZE(ilimit_increase) - 1];

	ilimit_dbg("set vbus_current:%d", ilimit_min);
	ret = oz115_sub_set_vbus_current(charger, ilimit_min);

	if (ret < 0)
		oz115_sub_dbg(KERN_ERR"%s: fail to set vbus current, err %d\n", __func__, ret);

	if (check_ilimit_status(charger, ilimit_min))
		fail = 1;

	else if (ilimit_max > ilimit_min){

		ilimit_dbg("set vbus_current:%d", ilimit_max);
		ret = oz115_sub_set_vbus_current(charger, ilimit_max);

		if (check_ilimit_status(charger, ilimit_max))
			fail = 1;
	}

	if (fail)
	{
		//vbus_current = charger_adjust_ilimit_discrease(charger, ilimit_increase[i]);
		ret = oz115_sub_set_vbus_current(charger, ilimit_increase[0]);
		vbus_current = ilimit_increase[0];
	}
	else vbus_current = ilimit_max;

	ilimit_dbg("[charger_adjust_ilimit_increase_max] start at: %d, finale vbus_current:%d",
			ilimit_min, vbus_current);

	if (ret < 0)
		return ret;

	return vbus_current;

}
/*****************************************************************************
 * Description:
 *		try to set ilimit from lower to higher
 * Return:
 *		vbus input curent, or negative errno
 *****************************************************************************/
static int charger_adjust_ilimit_increase(struct bluewhale_charger_info *charger, int ilimit_min)
{
	int vbus_current = 0;
	int bat_vol = 0;
	int cc_current = 0;
	int i = 0;
	int ret = 0;
	int fail = 0;

	if (!ilimit_min)
		ilimit_min = ilimit_increase[0];

	bat_vol = get_battery_voltage();

	if(bat_vol < (4000))
	{
		ilimit_dbg("battery voltage[%d]", bat_vol);
		ret = oz115_sub_set_chg_volt(charger, 4200);
	}

	cc_current = oz115_sub_get_charger_current(charger);

	ret = oz115_sub_set_charger_current(charger, 3000);

	for (i = 0; i < ARRAY_SIZE(ilimit_increase); ++i)
	{
		if (!start_adjust_ilimit)
			break;

		if (ilimit_increase[i] < ilimit_min)
			continue;

		if (ilimit_increase[i] > max_ilimit)
		{
			ilimit_dbg("ilimit_increase[%d](%d) > max_ilimit(%d)", i, ilimit_increase[i], max_ilimit);
			break;
		}

		ilimit_dbg("set vbus_current:%d", ilimit_increase[i]);
		ret = oz115_sub_set_vbus_current(charger, ilimit_increase[i]);

		if (ret < 0)
			ilimit_work(KERN_ERR"%s: fail to set vbus current, err %d\n", __func__, ret);

		if (check_ilimit_status(charger, ilimit_increase[i]))
		{
			fail = 1;

			if (i > 0)
				--i;
			break;
		}
		else {
			vbus_current = ilimit_increase[i];
			continue;
		}
	}

	if (fail && start_adjust_ilimit)
	{
		vbus_current = ilimit_increase[i];
		ilimit_dbg("set vbus_current:%d", ilimit_increase[i]);

		if (get_battery_current() < 0 && vbus_current > ilimit_increase[0])
			vbus_current = charger_adjust_ilimit_increase_max(charger, 0, vbus_current);
		else
		{
			ret = oz115_sub_set_vbus_current(charger, ilimit_increase[i]);
			ret = check_ilimit_status(charger, ilimit_increase[i]);
		}
	}

	vbus_current = oz115_sub_get_vbus_current(charger);
	ilimit_dbg("[charger_adjust_ilimit_increase] start at: %d, finale vbus_current:%d",
			ilimit_min, vbus_current);

	ret = oz115_sub_set_charger_current(charger, cc_current);

	if (ret < 0)
		return ret;

	return vbus_current;
}

/*****************************************************************************
 * Description:
 *		charger_adjust_ilimit
 * Parameters:
 *		charger:	charger data
 * Return:
 *		vbus input curent, or negative errno
 *****************************************************************************/
static int charger_adjust_ilimit(struct bluewhale_charger_info *charger)
{
	int vbus_current = 0;
	int ret = 0;
	u8 try_max_count = 0;

	if (!start_adjust_ilimit)
		return ret;

	vbus_current = oz115_sub_get_vbus_current(charger);

	ilimit_dbg("[charger_adjust_ilimit] get vbus_current:%d", vbus_current);

#ifdef FIX_ADPATER_ILIMIT

	if (vbus_current != TARGET_NORMAL_CHG_ILIMIT){

		ret = oz115_sub_set_vbus_current(charger, TARGET_NORMAL_CHG_ILIMIT);
		vbus_current = oz115_sub_get_vbus_current(charger);
	}
	
	ilimit_dbg("[charger_adjust_ilimit] fix ilimit, finale vbus_current:%d", vbus_current);
	return vbus_current;

#endif 

	if (find_max_ilimit
	 || vbus_current == 500
	 || check_ilimit_status(charger, vbus_current)
	 || (max_ilimit >= 2000 && vbus_current < 2000 && max_count < 3))
	{
		find_max_ilimit = 1;
		//try to find MAX ilimit of this AC
		do {
			ilimit_dbg("[charger_adjust_ilimit] try to find MAX ilimit of this AC:%d", try_max_count);
			//vbus_current = charger_adjust_ilimit_discrease(charger,  0, vbus_current);
			vbus_current = charger_adjust_ilimit_increase(charger, 0);
			try_max_count ++;
		} while((vbus_current <= 500) && (try_max_count < 3));

		if (max_count < 3)
			max_count ++;

		find_max_ilimit = 0;
	}
	else if (max_ilimit < vbus_current)
		ret = oz115_sub_set_vbus_current(charger, max_ilimit);

	if (ret < 0)
		return ret;

	ilimit_dbg("[charger_adjust_ilimit] finale vbus_current:%d", vbus_current);

	return vbus_current;
}
#endif

#ifdef BATT_TEMP_SIMULATION 
static ssize_t batt_temp_store(struct device *dev, struct device_attribute *attr, 
               const char *buf, size_t count) 
{
	int val;

	if (kstrtoint(buf, 10, &val))
		return -EINVAL;

	batt_temp_simulation = val;

	oz115_sub_dbg("batt_temp_simulation: %d ", batt_temp_simulation);

	return count;
}

static ssize_t batt_temp_show(struct device *dev, struct device_attribute *attr,char *buf)
{
	return sprintf(buf, "%d\n", batt_temp_simulation);
}
#endif
#ifdef ADJUST_ILIMIT
static ssize_t stop_ilimit_work_store(struct device *dev, struct device_attribute *attr, 
               const char *buf, size_t count) 
{
	struct bluewhale_charger_info *charger;

	if (oz115_sub_info->dev == dev)
	    charger = dev_get_drvdata(dev);
	else //this device is the provate device of power supply 
	    charger = dev_get_drvdata(dev->parent);

	start_adjust_ilimit = 0;

	return count;
}
#endif

static int otg_enable = 0;
static ssize_t oz115_sub_otg_store(struct device *dev, struct device_attribute *attr, 
               const char *buf, size_t count) 
{
	int val;
	struct bluewhale_charger_info *charger;


	if (kstrtoint(buf, 10, &val))
		return -EINVAL;

	if (oz115_sub_info->dev == dev)
	    charger = dev_get_drvdata(dev);
	else //this device is the provate device of power supply 
	    charger = dev_get_drvdata(dev->parent);

	if (val == 1 || val == 0)
	{
		oz115_sub_enable_otg(charger, val);
		oz115_sub_dbg("%s: %s OTG", __func__, val?"enable":"disable");
		otg_enable = val;

		return count;
	}
	else
		oz115_sub_dbg("%s: wrong parameters", __func__);

	return -EINVAL;
}

static ssize_t oz115_sub_otg_show(struct device *dev, struct device_attribute *attr,char *buf)
{
	return sprintf(buf, "%d\n", otg_enable);
}

static ssize_t oz115_sub_show_vbus_volt(struct device *dev, struct device_attribute *attr,char *buf)
{
	return sprintf(buf, "%d\n", o2_read_vbus_voltage());
}

static int reg_enable = 1;

#define oz115_sub_PARAMS_ATTR(x)  \
static ssize_t oz115_sub_write_##x(struct device *dev, struct device_attribute *attr, \
		const char *buf, size_t count) \
{ \
	int data; \
	struct bluewhale_charger_info *charger;\
	if (oz115_sub_info->dev == dev)\
	    charger = dev_get_drvdata(dev);\
	else\
	    charger = dev_get_drvdata(dev->parent);\
	if( kstrtoint(buf, 10, &data) == 0) { \
		if(reg_enable ==1)\
		    oz115_sub_set_##x(charger, data);\
		return count; \
	}else { \
		pr_err("oz115_sub reg %s set failed\n",#x); \
		return 0; \
	} \
	return 0;\
}\
static ssize_t oz115_sub_read_##x(struct device *dev, struct device_attribute *attr, \
               char *buf) \
{ \
	struct bluewhale_charger_info *charger;\
	if (oz115_sub_info->dev == dev)\
	    charger = dev_get_drvdata(dev);\
	else\
	    charger = dev_get_drvdata(dev->parent);\
	charger->x = oz115_sub_get_##x(charger);\
    return sprintf(buf,"%d\n", charger->x);\
}

oz115_sub_PARAMS_ATTR(vbus_current) //input current limit
oz115_sub_PARAMS_ATTR(charger_current) //charge current
oz115_sub_PARAMS_ATTR(chg_volt) //charge voltage

static ssize_t oz115_sub_registers_show(struct device *dev, struct device_attribute *attr, char *buf)
{	
	int result = 0;
	u8 i = 0;
	u8 data = 0;
	struct bluewhale_charger_info *charger;

	if (oz115_sub_info->dev == dev)
	    charger = dev_get_drvdata(dev);
	else //this device is the provate device of power supply 
	    charger = dev_get_drvdata(dev->parent);

    for (i=REG_CHARGER_VOLTAGE; i<=REG_MIN_VSYS_VOLTAGE; i++)
    {
		oz115_sub_read_byte(charger, i, &data);
		result += sprintf(buf + result, "[0x%.2x] = 0x%.2x\n", i, data);
    }
    for (i=REG_CHARGE_CURRENT; i<=REG_VBUS_LIMIT_CURRENT; i++)
    {
		oz115_sub_read_byte(charger, i, &data);
		result += sprintf(buf + result, "[0x%.2x] = 0x%.2x\n", i, data);
	}

	oz115_sub_read_byte(charger, REG_SAFETY_TIMER, &data);
	result += sprintf(buf + result, "[0x%.2x] = 0x%.2x\n", REG_SAFETY_TIMER, data);

	oz115_sub_read_byte(charger, REG_CHARGER_CONTROL, &data);
	result += sprintf(buf + result, "[0x%.2x] = 0x%.2x\n", REG_CHARGER_CONTROL, data);

    for (i=REG_VBUS_STATUS; i<=REG_THM_STATUS; i++)
    {
		oz115_sub_read_byte(charger, i, &data);
		result += sprintf(buf + result, "[0x%.2x] = 0x%.2x\n", i, data);
    }
	
	return result;
}

#ifdef BATT_TEMP_SIMULATION 
static DEVICE_ATTR(batt_temp, S_IRUGO | (S_IWUSR|S_IWGRP), batt_temp_show, batt_temp_store);
#endif
#ifdef ADJUST_ILIMIT
static DEVICE_ATTR(stop_ilimit_work, S_IWUSR|S_IWGRP, NULL, stop_ilimit_work_store);
#endif
static DEVICE_ATTR(registers, S_IRUGO, oz115_sub_registers_show, NULL);

static DEVICE_ATTR(vbus_current, S_IRUGO|(S_IWUSR|S_IWGRP), oz115_sub_read_vbus_current, oz115_sub_write_vbus_current);
static DEVICE_ATTR(charger_current, S_IRUGO|(S_IWUSR|S_IWGRP), oz115_sub_read_charger_current, oz115_sub_write_charger_current);
static DEVICE_ATTR(chg_volt, S_IRUGO|(S_IWUSR|S_IWGRP), oz115_sub_read_chg_volt, oz115_sub_write_chg_volt);
static DEVICE_ATTR(vbus_volt, S_IRUGO, oz115_sub_show_vbus_volt, NULL);
static DEVICE_ATTR(enable_otg, S_IRUGO | (S_IWUSR|S_IWGRP), oz115_sub_otg_show, oz115_sub_otg_store);

static struct attribute *oz115_sub_attributes[] = {
	&dev_attr_registers.attr,
	&dev_attr_vbus_current.attr, //input current limit
	&dev_attr_charger_current.attr, //charge current
	&dev_attr_chg_volt.attr, //charge voltage
	&dev_attr_vbus_volt.attr,     //vbus voltage
	&dev_attr_enable_otg.attr, //enable/disable OTG(boost)
#ifdef BATT_TEMP_SIMULATION 
	&dev_attr_batt_temp.attr,
#endif
#ifdef ADJUST_ILIMIT
	&dev_attr_stop_ilimit_work.attr,
#endif
	NULL,
};

static struct attribute_group oz115_sub_attribute_group = {
	.attrs = oz115_sub_attributes,
};

static int oz115_sub_create_sys(struct device *dev)
{
	int err;

	oz115_sub_dbg("oz115_sub_create_sysfs");
	
	if(NULL == dev){
		oz115_sub_dbg(KERN_ERR"creat oz115_sub sysfs fail: NULL dev\n");
		return -EINVAL;
	}

	err = sysfs_create_group(&(dev->kobj), &oz115_sub_attribute_group);

	if (err) 
	{
		oz115_sub_dbg(KERN_ERR"creat oz115_sub sysfs group fail\n");
		return -EIO;
	}

	oz115_sub_dbg(KERN_CRIT"creat oz115_sub sysfs group ok\n");	
	return err;
}

/*****************************************************************************
 * Description:
 *      oz115_sub_get_setting_data	
 * Parameters:
 *		charger:	charger data
 *****************************************************************************/
static void oz115_sub_get_setting_data(struct bluewhale_charger_info *charger)
{
	if (!charger)
		return;

	charger->chg_volt = oz115_sub_get_chg_volt(charger);

	charger->charger_current = oz115_sub_get_charger_current(charger);

	charger->vbus_current = oz115_sub_get_vbus_current(charger);

	charger->eoc_current = oz115_sub_get_eoc_current(charger);

	//oz115_sub_dbg("chg_volt:%d, vbus_current: %d, charger_current: %d, eoc_current: %d", 
			//charger->chg_volt, charger->vbus_current, charger->charger_current, charger->eoc_current);

}

static int oz115_sub_check_charger_status(struct bluewhale_charger_info *charger)
{
	u8 data_status;
	u8 data_thm = 0;
	u8 i = 0;
	int ret;
	
	/* Check charger status */
	ret = oz115_sub_read_byte(charger, REG_CHARGER_STATUS, &data_status);
	if (ret < 0) {
		oz115_sub_dbg(KERN_ERR"%s: fail to get Charger status\n", __func__);
		return ret; 
	}

	charger->initial_state = (data_status & CHARGER_INIT) ? 1 : 0;
	charger->in_wakeup_state = (data_status & IN_WAKEUP_STATE) ? 1 : 0;
	charger->in_cc_state = (data_status & IN_CC_STATE) ? 1 : 0;
	charger->in_cv_state = (data_status & IN_CV_STATE) ? 1 : 0;
	charger->chg_full = (data_status & CHARGE_FULL_STATE) ? 1 : 0;
	charger->cc_timeout = (data_status & CC_TIMER_FLAG) ? 1 : 0;
	charger->wak_timeout = (data_status & WK_TIMER_FLAG) ? 1 : 0;
 
	/* Check thermal status*/
	ret = oz115_sub_read_byte(charger, REG_THM_STATUS, &data_thm);
	if (ret < 0) {
		oz115_sub_dbg(KERN_ERR"%s: fail to get Thermal status\n", __func__);
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

static int oz115_sub_check_vbus_status(struct bluewhale_charger_info *charger)
{
	int ret = 0;
	u8 data_status = 0;

	/* Check VBUS and VSYS */
	ret = oz115_sub_read_byte(charger, REG_VBUS_STATUS, &data_status);

	if(ret < 0) {
		oz115_sub_dbg(KERN_ERR"fail to get Vbus status\n");
	}

	charger->vdc_pr = (data_status & VDC_PR_FLAG) ? 1 : 0;
	charger->vsys_ovp = (data_status & VSYS_OVP_FLAG) ? 1 : 0;
	charger->vbus_ovp = (data_status & VBUS_OVP_FLAG) ? 1 : 0; //it's reserved for oz1c115
	charger->vbus_uvp = (data_status & VBUS_UVP_FLAG) ? 1 : 0;
	charger->vbus_ok = (data_status & VBUS_OK_FLAG) ? 1 : 0;

	if (!charger->vbus_ok)
		oz115_sub_dbg(KERN_ERR"invalid adapter or no adapter\n");

	if (charger->vdc_pr)
		oz115_sub_dbg("vdc < vdc threshold of system priority");

	return ret > 0 ? charger->vbus_ok : ret;
}

/*****************************************************************************
 * Description:
 *		oz115_sub_get_charging_status 
 * Parameters:
 *		charger:	charger data
 * Return:
 *		negative errno if error happens
 *****************************************************************************/
static int oz115_sub_get_charging_status(struct bluewhale_charger_info *charger)
{
	int ret = 0;

	bool last_vbus_ok;

	if (!charger)
		return -EINVAL;

	last_vbus_ok = charger->vbus_ok;

	ret = oz115_sub_check_vbus_status(charger);


	ret = oz115_sub_check_charger_status(charger);

	oz115_sub_get_setting_data(charger);

	print_all_register(charger);
	print_all_charger_info(charger);

	return ret;
}

static void print_all_charger_info(struct bluewhale_charger_info *charger)
{
	oz115_sub_dbg("## oz115_sub charger information ##\n"
		"chg_volt: %d, vbus_current: %d, charger_current: %d, eoc_current: %d\n"
		"vdc_pr: %d, vbus_ovp: %d, vbus_ok: %d, vbus_uvp: %d, vsys_ovp: %d,\n"
		"cc_timeout: %d, wak_timeout: %d, chg_full: %d, in_cc_state: %d, in_cv_state: %d\n"
		"initial_state: %d, in_wakeup_state: %d, thermal_status: %d, chg_adapter_type: %d\n",
		charger->chg_volt, charger->vbus_current,
		charger->charger_current,charger->eoc_current,
		charger->vdc_pr, charger->vbus_ovp, charger->vbus_ok,
		charger->vbus_uvp, charger->vsys_ovp,
		charger->cc_timeout, charger->wak_timeout, charger->chg_full,
		charger->in_cc_state, charger->in_cv_state,
		charger->initial_state,charger->in_wakeup_state,
		charger->thermal_status, chg_adapter_type);
}

static void print_all_register(struct bluewhale_charger_info *charger)
{
	u8 i = 0;
	u8 data = 0;

    printk("[oz115_sub charger]:\n");

    for (i=REG_CHARGER_VOLTAGE; i<=REG_MIN_VSYS_VOLTAGE; i++)
    {
		oz115_sub_read_byte(charger, i, &data);
		printk("[0x%02x]=0x%02x ", i, data);        
    }
    for (i=REG_CHARGE_CURRENT; i<=REG_VBUS_LIMIT_CURRENT; i++)
    {
		oz115_sub_read_byte(charger, i, &data);
		printk("[0x%02x]=0x%02x ", i, data);        
	}

	oz115_sub_read_byte(charger, REG_SAFETY_TIMER, &data);
	printk("[0x%02x]=0x%02x ", REG_SAFETY_TIMER, data);        

	oz115_sub_read_byte(charger, REG_CHARGER_CONTROL, &data);
	printk("[0x%02x]=0x%02x ", REG_CHARGER_CONTROL, data);        

    for (i=REG_VBUS_STATUS; i<=REG_THM_STATUS; i++)
    {
		oz115_sub_read_byte(charger, i, &data);
		printk("[0x%02x]=0x%02x ", i, data);        
    }
    printk("\n");
}


#ifdef SUB_IR_COMPENSATION	

static int sub_ir_compensation(struct bluewhale_charger_info *charger, int cv_val)
{
	int32_t ir;
	int32_t charge_volt;
	int Ibat = 0;
	int vbat = 0;

	if(!oz115_sub_check_vbus_status(charger) || !o2_check_gauge_init_status())
		return cv_val;

	/*********************************************/
	cc_sub = oz115_sub_get_charger_current(charger);
	oz115_sub_dbg("%s: cc_sub: %d\n", __func__, cc_sub);

	if (ir_cc_dis) {

		oz115_sub_set_charger_current(charger, charge_cur_sum - cc_main);
		msleep(200);
		cc_sub = oz115_sub_get_charger_current(charger);
	}

	oz115_sub_dbg("%s: cc_main: %d, cc_sub: %d, ir_cc_dis:%d\n",
			__func__, cc_main, cc_sub, ir_cc_dis);
	/*********************************************/

	Ibat = get_battery_current();
	ir = Ibat * charger->pdata->inpedance /1000;
	if (cv_val)
		charge_volt = cv_val + ir;
	else
		charge_volt = charger->pdata->max_charger_voltagemV + ir;

	charge_volt = charge_volt / 25 * 25;

	oz115_sub_dbg("%s:Ibat: %d mA, inpedance:%d mohm, CV:%d mV\n",
			      __func__, Ibat, charger->pdata->inpedance, charge_volt);

	if (cv_val && charge_volt < cv_val)
		charge_volt = cv_val;
	else if(charge_volt < charger->pdata->max_charger_voltagemV)
		charge_volt = charger->pdata->max_charger_voltagemV;

	if(charge_volt > 4600)
		charge_volt = 4600;

/*********************************************/
	//find max Ibat of main charger
#if 0
	cc_main_tmp = bluewhale_get_charger_current_extern();
	oz115_sub_dbg("%s: cc_main_tmp: %d\n", __func__, cc_main_tmp);

	cv_max_main = 4500 - cc_main_tmp * charger->pdata->inpedance / 1000;
	if(charge_volt > cv_max_main)
		charge_volt = bluewhale_get_chg_volt_extern() + 25;

	//when cc_main = cc_sub = 2000, CVm = CVs
	if (!ir_cc_dis) charge_volt = bluewhale_get_chg_volt_extern();
	oz115_sub_dbg("***********cv_max_main: %d, calculate CV:%d mV\n", cv_max_main, charge_volt);

#endif
/*********************************************/
	vbat = get_battery_voltage();
	if ((cv_val && vbat >= (cv_val+50)) && Ibat < 500)
		charge_volt = cv_val;
	oz115_sub_dbg("Vbat:%d mV\n", vbat);

	oz115_sub_dbg("***********calculate CV:%d mV\n", charge_volt);
	return charge_volt;
}
#endif

/*****************************************************************************
 * Description:
 *		oz115_sub_get_charging_status_extern 
 * Return:
 *		negative errno if error happens
 *****************************************************************************/
int oz115_sub_get_charging_status_extern(void)
{
	int ret = 0;

	mutex_lock(&oz115_sub_mutex);
	if (oz115_sub_info)
		ret = oz115_sub_get_charging_status(oz115_sub_info);
	else
		ret = -EINVAL;
	mutex_unlock(&oz115_sub_mutex);

	if (ret < 0 || oz115_sub_info->vbus_ok == 0)
		return -1;

	if (oz115_sub_info->vbus_ok && oz115_sub_info->chg_full)
		return 1;
	else
		return 0;
}
EXPORT_SYMBOL(oz115_sub_get_charging_status_extern);

void oz115_sub_dump_register(void)
{
	mutex_lock(&oz115_sub_mutex);

	if (!oz115_sub_info) goto end;
#if 0
	print_all_register(oz115_sub_info);
#else
	oz115_sub_get_charging_status(oz115_sub_info);
#endif
 
end:
	mutex_unlock(&oz115_sub_mutex);
}
EXPORT_SYMBOL(oz115_sub_dump_register);


/*****************************************************************************
 * Description:
 *		oz115_sub_init
 * Parameters:
 *		charger:	charger data
 * Return:
 *		negative errno if error happens
 *****************************************************************************/
static int oz115_sub_init(struct bluewhale_charger_info *charger)
{
	struct bluewhale_platform_data *pdata = NULL;

	int ret = 0;

  	oz115_sub_dbg(KERN_CRIT"O2Micro oz115_sub Driver oz115_sub_init\n");

	if (!charger) return -EINVAL;
	
	pdata = charger->pdata;

	//*************************************************************************************
	//note: you must test usb type and set vbus limit current.
	//for wall charger you can set current more than 500mA
	// but for pc charger you may set current not more than 500 mA for protect pc usb port
	//************************************************************************************/

    //enable system over-power protection
	//oz115_sub_update_bits(charger, REG_CHARGER_CONTROL, 0x80, 0x1 << 7);

	// write rthm  100k
	ret = oz115_sub_update_bits(charger, REG_CHARGER_CONTROL, 0x1, pdata->rthm_10k);

	if (
#if defined(CONFIG_MTK_PUMP_EXPRESS_PLUS_20_SUPPORT)
			mtk_pep20_get_is_connect() == true ||
#endif
#if defined(CONFIG_MTK_PUMP_EXPRESS_PLUS_SUPPORT)
			mtk_pep_get_is_connect() == true ||
#endif
			false )
		oz115_sub_update_bits(charger, REG_CHARGER_CONTROL, 0x18, 0x3 << 3);
	else //set chager PWM TON time
		oz115_sub_update_bits(charger, REG_CHARGER_CONTROL, 0x18, 0x0 << 3);


	/* Min VSYS:3.6V */
    ret = oz115_sub_get_min_vsys(charger);
    ret = oz115_sub_set_min_vsys(charger, pdata->min_vsys_voltagemV);	
	
	/* EOC CHARGE:80 mA */
	if (m_charger_full || !cc_main)
		oz115_sub_set_eoc_current(charger, pdata->termination_currentmA / 2);
	else
		oz115_sub_set_eoc_current(charger, pdata->termination_currentmA / 2);

	/* MAX CC CHARGE TIME:disabled*/
	oz115_sub_set_safety_cc_timer(charger, pdata->max_cc_charge_time);
	/* MAX WAKEUP CHARGE TIME:disabled*/
	oz115_sub_set_safety_wk_timer(charger, pdata->max_wakeup_charge_time);

	/* RECHG HYSTERESIS:0.1V */
	oz115_sub_set_rechg_hystersis(charger, pdata->recharge_voltagemV);
	/* T34 CHARGE VOLTAGE:4.35V */
	oz115_sub_set_t34_cv(charger, pdata->T34_charger_voltagemV);
	/* T45 CHARGE VOLTAGE:4.35V */
	oz115_sub_set_t45_cv(charger, pdata->T45_charger_voltagemV);

	/* WAKEUP VOLTAGE:default 2.5V */
	oz115_sub_set_wakeup_volt(charger, pdata->wakeup_voltagemV);
	/* WAKEUP CURRENT:default 0.1A */
	oz115_sub_set_wakeup_current(charger, pdata->wakeup_currentmA);

	get_battery_current();
	get_battery_voltage();

	return ret;
}

/*****************************************************************************
 * Description:
 *		oz115_sub_init_extern
 * Parameters:
 *		None
 * Return:
 *		negative errno if error happens
 *****************************************************************************/
int oz115_sub_init_extern(void)
{
	int ret = 0;
	mutex_lock(&oz115_sub_mutex);
	if (oz115_sub_info)
		ret = oz115_sub_init(oz115_sub_info);
	else
		ret = -EINVAL;
	mutex_unlock(&oz115_sub_mutex);
	return ret;
}
EXPORT_SYMBOL(oz115_sub_init_extern);

/*****************************************************************************
 * Description:
 *		o2_start_ac_charge
 *****************************************************************************/
static void o2_start_ac_charge(struct bluewhale_charger_info *charger)
{
	if (!charger) return;

	oz115_sub_dbg(KERN_CRIT"start ac charge\n");

    //disable suspend mode
	oz115_sub_update_bits(charger, REG_CHARGER_CONTROL, 0x02, 0 << 1);
    //enable system over-power protection
	//oz115_sub_update_bits(charger, REG_CHARGER_CONTROL, 0x80, 0x1 << 7);

	chg_adapter_type = CHG_ADAPTER_NORMAL;
	if (
#if defined(CONFIG_MTK_PUMP_EXPRESS_PLUS_20_SUPPORT)
			mtk_pep20_get_is_connect() == true ||
#endif
#if defined(CONFIG_MTK_PUMP_EXPRESS_PLUS_SUPPORT)
			mtk_pep_get_is_connect() == true ||
#endif
			false )
	{
		oz115_sub_update_bits(charger, REG_CHARGER_CONTROL, 0x18, 0x3 << 3);
		chg_adapter_type = CHG_ADAPTER_MTK;
	}

	start_adjust_ilimit = 1;
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
	oz115_sub_dbg(KERN_CRIT"start usb charge\n");

	if (!charger) return;

    //enable suspend mode
	oz115_sub_update_bits(charger, REG_CHARGER_CONTROL, 0x02, 1 << 1);

    //enable system over-power protection
	//oz115_sub_update_bits(charger, REG_CHARGER_CONTROL, 0x80, 0x1 << 7);

	//disable ki2c
	oz115_sub_update_bits(charger, REG_CHARGER_CONTROL, 0x18, 0x0 << 3);
	chg_adapter_type = CHG_ADAPTER_PC;

	start_adjust_ilimit = 0;
}

/*****************************************************************************
 * Description:
 *		oz115_sub_start_charge_extern
 * Parameters:
 *		None
 * Return:
 *      negative errno if any error
 *****************************************************************************/
int oz115_sub_start_charge_extern(void)
{
	int ret = 0;

	mutex_lock(&oz115_sub_mutex);
	if (oz115_sub_info)
	{
#ifdef MTK_MACH_SUPPORT
		if (BMT_status.charger_type == STANDARD_CHARGER)
			o2_start_ac_charge(oz115_sub_info);
		else
			o2_start_usb_charge(oz115_sub_info);
#endif

	}
	else
		ret = -EINVAL;
	mutex_unlock(&oz115_sub_mutex);
	return ret;
}
EXPORT_SYMBOL(oz115_sub_start_charge_extern);

/*****************************************************************************
 * Description:
 *		o2_stop_charge, this is used when plug out adapter cable
 * Parameters:
 *		None
 *****************************************************************************/
static void o2_stop_charge(struct bluewhale_charger_info *charger, u8 cable_out)
{
	int ret = 0;

	oz115_sub_dbg(KERN_CRIT"%s !!\n", __func__);

	start_adjust_ilimit = 0;
	max_ilimit = 0;
	find_max_ilimit = 1;

    //enable suspend mode
	oz115_sub_update_bits(charger, REG_CHARGER_CONTROL, 0x02, 0x1 << 1);

	//disable system over-power protection
	oz115_sub_update_bits(charger, REG_CHARGER_CONTROL, 0x80, 0x0 << 7);
	
	ret = oz115_sub_set_charger_current(charger, 0);
	ret = oz115_sub_set_vbus_current(charger, 0);

	//disable ki2c
	oz115_sub_update_bits(charger, REG_CHARGER_CONTROL, 0x18, 0x0 << 3);

	chg_adapter_type = CHG_ADAPTER_NULL;
}

/*****************************************************************************
 * Description:
 *		oz115_sub_stop_charge_extern
 * Parameters:
 *		None
 * Return:
 *      negative errno if any error
 *****************************************************************************/
int oz115_sub_stop_charge_extern(u8 cable_out)
{
	int ret = 0;

	mutex_lock(&oz115_sub_mutex);
	if (oz115_sub_info)
		o2_stop_charge(oz115_sub_info, cable_out);
	else
		ret = -EINVAL;
	mutex_unlock(&oz115_sub_mutex);
	return ret;
}
EXPORT_SYMBOL(oz115_sub_stop_charge_extern);

int oz115_sub_is_enabled(void)
{
	int ret = 0;
	uint8_t tmp = 0;

	if (oz115_sub_info) {
		ret = oz115_sub_read_byte(oz115_sub_info, REG_CHARGER_CONTROL, &tmp);
		if ((tmp & 0x02))
			ret = 0;
		else
			ret = 1;
	}
	else
		ret = -EINVAL;

	return ret;
}
EXPORT_SYMBOL(oz115_sub_is_enabled);

/*****************************************************************************
 * Description:
 *		oz115_sub_charger_probe
 * Parameters:
 *		
 * Return:
 *      negative errno if any error
 *****************************************************************************/
static int  oz115_sub_charger_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
	struct bluewhale_charger_info *charger;
	int ret = 0;

	oz115_sub_dbg("O2Micro oz115_sub Driver Loading\n");

	charger = devm_kzalloc(&client->dev, sizeof(*charger), GFP_KERNEL);
	if (!charger) {
		dev_err(&client->dev, "Can't alloc charger struct\n");
		return -ENOMEM;
	}
	
	charger->pdata = &oz115_sub_pdata_default;
	charger->client = client;	
	charger->dev = &client->dev;
	
	i2c_set_clientdata(client, charger);

	//for public use
	oz115_sub_info = charger;

	oz115_sub_dbg("O2Micro oz115_sub Driver Loading 1");	

	//sys/class/i2c-dev/i2c-2/device/2-0010/
	ret = oz115_sub_create_sys(&(client->dev));
	if(ret){
		oz115_sub_dbg(KERN_ERR"Err failed to creat charger attributes\n");
		goto sys_failed;
	}

	//enable suspend mode, shutdown charger.
	oz115_sub_update_bits(charger, REG_CHARGER_CONTROL, 0x02, 1 << 1);
	oz115_sub_init(charger);
	//set ilmit
	oz115_sub_set_vbus_current(charger, 0);
	//set charging current, CC
	oz115_sub_set_charger_current(charger, 0);
	oz115_sub_set_chg_volt(charger, charger->pdata->max_charger_voltagemV);

	oz115_sub_dbg("O2Micro oz115_sub Driver Loading 2");

	return 0;

sys_failed:

	return ret;
}


/*****************************************************************************
 * Description:
 *		oz115_sub_charger_remove
 * Parameters:
 *		
 * Return:
 *      negative errno if any error
 *****************************************************************************/
static int  oz115_sub_charger_remove(struct i2c_client *client)
{
	struct bluewhale_charger_info *charger = i2c_get_clientdata(client);

	start_adjust_ilimit = 0;
	oz115_sub_dbg(KERN_CRIT"%s: oz115_sub Charger Driver removed\n", __func__);
	//suspend charger
	oz115_sub_update_bits(charger, REG_CHARGER_CONTROL, 0x02, 0x1 << 1);
	return 0;
}
/*****************************************************************************
 * Description:
 *		oz115_sub_charger_shutdown
 * Parameters:
 *****************************************************************************/
static void  oz115_sub_charger_shutdown(struct i2c_client *client)
{
	struct bluewhale_charger_info *charger = i2c_get_clientdata(client);

	oz115_sub_dbg(KERN_CRIT"%s\n", __func__);

	start_adjust_ilimit = 0;

	//suspend charger
	oz115_sub_update_bits(charger, REG_CHARGER_CONTROL, 0x02, 0x1 << 1);

	//disable system over-power protection
	oz115_sub_update_bits(charger, REG_CHARGER_CONTROL, 0x80, 0x0 << 7);

	oz115_sub_set_charger_current(charger, 0);
	oz115_sub_set_vbus_current(charger, 0);

	//disable ki2c
	oz115_sub_update_bits(charger, REG_CHARGER_CONTROL, 0x18, 0x0 << 3);
}


/*****************************************************************************
 * Description:
 *		oz115_sub_charger_suspend
 * Parameters:
 *		
 * Return:
 *      negative errno if any error
 *****************************************************************************/
static int oz115_sub_charger_suspend(struct device *dev)
{
	//struct platform_device * const pdev = to_platform_device(dev);
	//struct bluewhale_charger_info * const charger = platform_get_drvdata(pdev);
	return 0;
}


/*****************************************************************************
 * Description:
 *		oz115_sub_charger_resume
 * Parameters:
 *		
 * Return:
 *      negative errno if any error
 *****************************************************************************/
static int oz115_sub_charger_resume(struct device *dev)
{
	//struct platform_device * const pdev = to_platform_device(dev);
	//struct bluewhale_charger_info * const charger = platform_get_drvdata(pdev);
	return 0;
}


static const struct i2c_device_id oz115_sub_i2c_ids[] = {
		{"oz115_sub-charger", 0},
		{}
}; 

MODULE_DEVICE_TABLE(i2c, oz115_sub_i2c_ids);

static const struct dev_pm_ops pm_ops = {
        .suspend        = oz115_sub_charger_suspend,
        .resume			= oz115_sub_charger_resume,
};

#ifdef CONFIG_OF
static const struct of_device_id oz115_sub_of_match[] = {
	{.compatible = "o2micro,oz115_sub",},
	{},
};

MODULE_DEVICE_TABLE(of, oz115_sub_of_match);
#else
static struct i2c_board_info __initdata i2c_oz115_sub={ I2C_BOARD_INFO("oz115_sub-charger", 0x10)}; // 7-bit address of 0x20
#endif


static struct i2c_driver oz115_sub_charger_driver = {
		.driver		= {
			.name 		= "oz115_sub-charger",
//			.pm			= &pm_ops,
#ifdef CONFIG_OF
			.of_match_table = oz115_sub_of_match,
#endif
		},
		.probe		= oz115_sub_charger_probe,
		.shutdown	= oz115_sub_charger_shutdown,
		.remove     = oz115_sub_charger_remove,
		.id_table	= oz115_sub_i2c_ids,
};


static int __init oz115_sub_charger_init(void)
{
	int ret = 0;

#ifndef CONFIG_OF
	static struct i2c_client *charger_client; //Notice: this is static pointer
	struct i2c_adapter *i2c_adap;

	i2c_adap = i2c_get_adapter(CHARGER_I2C_NUM);
	if (i2c_adap)
	{
		charger_client = i2c_new_device(i2c_adap, &i2c_oz115_sub);
		i2c_put_adapter(i2c_adap);
	}
	else {
		pr_err("failed to get adapter %d.\n", CHARGER_I2C_NUM);
		pr_err("statically declare I2C devices\n");

		ret = i2c_register_board_info(CHARGER_I2C_NUM, &i2c_oz115_sub, 1);

		if(ret != 0)
			pr_err("failed to register i2c_board_info.\n");
	}
#endif
	pr_err("%s\n", __func__);

	ret = i2c_add_driver(&oz115_sub_charger_driver);

    if(ret != 0)
        pr_err("failed to register oz115_sub_charger i2c driver.\n");
    else
        pr_err("Success to register oz115_sub_charger i2c driver.\n");

	return ret;
}

static void __exit oz115_sub_charger_exit(void)
{
	oz115_sub_dbg(KERN_CRIT"oz115_sub driver exit\n");
	
	i2c_del_driver(&oz115_sub_charger_driver);
}

module_init(oz115_sub_charger_init);
module_exit(oz115_sub_charger_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("O2Micro");
MODULE_DESCRIPTION("O2Micro oz115_sub Charger Driver");
