#include <linux/types.h>
#include <linux/delay.h>
#include <linux/reboot.h>
#include <mach/mt_pmic.h>
#include <linux/version.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,18,0))
#include <mt-plat/upmu_common.h>
#include <mt-plat/mt_boot.h>
#include <mt-plat/battery_common.h>
#include <mt-plat/charging.h>
#else
#include <mach/mt_boot.h>
#include <mach/battery_common.h>
#include <mach/charging.h>
#include <mach/upmu_common.h>
#include <mach/mt_charging.h>
#endif

#include "bluewhale_charger.h"
#include <mach/mt_sleep.h>
#include "../mtk_bif_intf.h"

static u32 charging_get_charger_det_status(void *data);

#if defined(CONFIG_MTK_PUMP_EXPRESS_PLUS_SUPPORT)
extern bool mtk_pep_get_is_connect(void);
#endif

#if defined(CONFIG_MTK_PUMP_EXPRESS_PLUS_20_SUPPORT)	
extern bool mtk_pep20_get_is_connect(void);
#endif

/* ============================================================ // */
/* Define */
/* ============================================================ // */
#define STATUS_OK	0
#define STATUS_FAIL	1
#define STATUS_UNSUPPORTED	-1
#define GETARRAYNUM(array) (sizeof(array)/sizeof(array[0]))


/* ============================================================ // */
/* Global variable */
/* ============================================================ // */

#if defined(MTK_WIRELESS_CHARGER_SUPPORT)
#define WIRELESS_CHARGER_EXIST_STATE 0
int wireless_charger_gpio_number = (168 | 0x80000000);
#endif

const u32 VBAT_CV_VTH[] = {
	BATTERY_VOLT_03_500000_V, BATTERY_VOLT_03_520000_V, BATTERY_VOLT_03_540000_V,
	    BATTERY_VOLT_03_560000_V,
	BATTERY_VOLT_03_580000_V, BATTERY_VOLT_03_600000_V, BATTERY_VOLT_03_620000_V,
	    BATTERY_VOLT_03_640000_V,
	BATTERY_VOLT_03_660000_V, BATTERY_VOLT_03_680000_V, BATTERY_VOLT_03_700000_V,
	    BATTERY_VOLT_03_720000_V,
	BATTERY_VOLT_03_740000_V, BATTERY_VOLT_03_760000_V, BATTERY_VOLT_03_780000_V,
	    BATTERY_VOLT_03_800000_V,
	BATTERY_VOLT_03_820000_V, BATTERY_VOLT_03_840000_V, BATTERY_VOLT_03_860000_V,
	    BATTERY_VOLT_03_880000_V,
	BATTERY_VOLT_03_900000_V, BATTERY_VOLT_03_920000_V, BATTERY_VOLT_03_940000_V,
	    BATTERY_VOLT_03_960000_V,
	BATTERY_VOLT_03_980000_V, BATTERY_VOLT_04_000000_V, BATTERY_VOLT_04_020000_V,
	    BATTERY_VOLT_04_040000_V,
	BATTERY_VOLT_04_060000_V, BATTERY_VOLT_04_080000_V, BATTERY_VOLT_04_100000_V,
	    BATTERY_VOLT_04_120000_V,
	BATTERY_VOLT_04_140000_V, BATTERY_VOLT_04_160000_V, BATTERY_VOLT_04_180000_V,
	    BATTERY_VOLT_04_200000_V,
	BATTERY_VOLT_04_220000_V, BATTERY_VOLT_04_240000_V, BATTERY_VOLT_04_260000_V,
	    BATTERY_VOLT_04_280000_V,
	BATTERY_VOLT_04_300000_V, BATTERY_VOLT_04_320000_V, BATTERY_VOLT_04_340000_V,
	    BATTERY_VOLT_04_360000_V,
	BATTERY_VOLT_04_380000_V, BATTERY_VOLT_04_400000_V, BATTERY_VOLT_04_420000_V,
	    BATTERY_VOLT_04_440000_V
};

const u32 CS_VTH[] = {
	CHARGE_CURRENT_550_00_MA, CHARGE_CURRENT_650_00_MA, CHARGE_CURRENT_750_00_MA,
	    CHARGE_CURRENT_850_00_MA,
	CHARGE_CURRENT_950_00_MA, CHARGE_CURRENT_1050_00_MA, CHARGE_CURRENT_1150_00_MA,
	    CHARGE_CURRENT_1250_00_MA
};

const u32 INPUT_CS_VTH[] = {
	CHARGE_CURRENT_100_00_MA, CHARGE_CURRENT_500_00_MA, CHARGE_CURRENT_800_00_MA,
	    CHARGE_CURRENT_MAX
};

const u32 VCDT_HV_VTH[] = {
	BATTERY_VOLT_04_200000_V, BATTERY_VOLT_04_250000_V, BATTERY_VOLT_04_300000_V,
	    BATTERY_VOLT_04_350000_V,
	BATTERY_VOLT_04_400000_V, BATTERY_VOLT_04_450000_V, BATTERY_VOLT_04_500000_V,
	    BATTERY_VOLT_04_550000_V,
	BATTERY_VOLT_04_600000_V, BATTERY_VOLT_06_000000_V, BATTERY_VOLT_06_500000_V,
	    BATTERY_VOLT_07_000000_V,
	BATTERY_VOLT_07_500000_V, BATTERY_VOLT_08_500000_V, BATTERY_VOLT_09_500000_V,
	    BATTERY_VOLT_10_500000_V
};

u32 charging_value_to_parameter(const u32 *parameter, const u32 array_size, const u32 val)
{
	if (val < array_size)
		return parameter[val];
	battery_log(BAT_LOG_CRTI, "Can't find the parameter \r\n");
	return parameter[0];
}

u32 charging_parameter_to_value(const u32 *parameter, const u32 array_size, const u32 val)
{
	u32 i;

	for (i = 0; i < array_size; i++)
		if (val == *(parameter + i))
			return i;

	battery_log(BAT_LOG_CRTI, "NO register value match \r\n");

	return 0;
}


static u32 bmt_find_closest_level(const u32 *pList, u32 number, u32 level)
{
	u32 i;
	u32 max_value_in_last_element;

	if (pList[0] < pList[1])
		max_value_in_last_element = KAL_TRUE;
	else
		max_value_in_last_element = KAL_FALSE;

	if (max_value_in_last_element == KAL_TRUE) {
		for (i = (number - 1); i != 0; i--)	/* max value in the last element */
			if (pList[i] <= level)
				return pList[i];

		battery_log(BAT_LOG_CRTI, "Can't find closest level, small value first \r\n");
		return pList[0];
		/* return CHARGE_CURRENT_0_00_MA; */
	} else {
		for (i = 0; i < number; i++)	/* max value in the first element */
			if (pList[i] <= level)
				return pList[i];

		battery_log(BAT_LOG_CRTI, "Can't find closest level, large value first \r\n");
		return pList[number - 1];
		/* return CHARGE_CURRENT_0_00_MA; */
	}
}

static u32 charging_hw_init(void *data)
{
	u32 status = STATUS_OK;
//	static bool charging_init_flag = KAL_FALSE;

	
#if defined(MTK_WIRELESS_CHARGER_SUPPORT)
	mt_set_gpio_mode(wireless_charger_gpio_number, 0);	/* 0:GPIO mode */
	mt_set_gpio_dir(wireless_charger_gpio_number, 0);	/* 0: input, 1: output */
#endif

    battery_log(BAT_LOG_FULL,"[charging_hw_init]\n");
	bluewhale_init_extern();

	return status;
}


static u32 charging_dump_register(void *data)
{
	u32 status = STATUS_OK;
	//unsigned int reg_val = 0;
	//unsigned int i = 0;

    battery_log(BAT_LOG_FULL,"[charging_dump_register]\n");
    bluewhale_dump_register();

#if 0
    battery_log(BAT_LOG_FULL,"[charging_dump_register] dump PMIC registers\n");
	for (i = MT6328_CHR_CON0; i <= MT6328_CHR_CON40; i += 2) {
		reg_val = upmu_get_reg_value(i);
		battery_log(BAT_LOG_CRTI, "[0x%x]=0x%x,", i, reg_val);
	}

	battery_log(BAT_LOG_CRTI, "[PMIC_RG_VCDT_HV_VTH] = 0x%x\n",
			pmic_get_register_value(PMIC_RG_VCDT_HV_VTH));
	battery_log(BAT_LOG_CRTI, "[PMIC_RG_VCDT_LV_VTH] = 0x%x\n",
			pmic_get_register_value(PMIC_RG_VCDT_LV_VTH));
	battery_log(BAT_LOG_CRTI, "[PMIC_RG_LOW_ICH_DB] = 0x%x\n",
			pmic_get_register_value(PMIC_RG_LOW_ICH_DB));
	battery_log(BAT_LOG_CRTI, "[PMIC_RGS_VCDT_HV_DET] = 0x%x\n",
			pmic_get_register_value(PMIC_RGS_VCDT_HV_DET));
#endif

	return status;
}


static u32 charging_enable(void *data)
{
	u32 status = STATUS_OK;
	u32 enable = *(u32 *) (data);

	battery_log(BAT_LOG_CRTI, "get_boot_mode=%d\n", get_boot_mode());
	if (get_boot_mode() == META_BOOT || get_boot_mode() == ADVMETA_BOOT)
	{
		o2_suspend_charger_extern();
		return STATUS_OK;
	}

    battery_log(BAT_LOG_FULL,"[charging_enable] %d\n", enable);

    if(KAL_TRUE == enable)
		o2_start_charge_extern();
	else
	{
#if defined(CONFIG_USB_MTK_HDRC_HCD)
		if (mt_usb_is_device())
#endif
		{
			charging_get_charger_det_status(&enable);
			if (enable == 0) {
				battery_log(BAT_LOG_FULL,"[charging_enable] cable out\n");
				o2_stop_charge_extern(1);
			}
			else
				o2_stop_charge_extern(0);
		}
	}

	return status;
}


static u32 charging_set_cv_voltage(void *data)
{
	u32 status = STATUS_OK;
	unsigned int cv_value = *(unsigned int *) data / 1000; //mV

	battery_log(BAT_LOG_CRTI,"[charging_set_cv_voltage] set CV cv_value: %dmV\n", cv_value);

	bluewhale_set_chg_volt_extern(cv_value);

#if defined(CONFIG_OZ115_DUAL_SUPPORT)
	oz115_sub_set_chg_volt_extern(bluewhale_get_chg_volt_extern());
#endif
    return status;
}


static u32 charging_get_current(void *data)
{
	u32 status = STATUS_OK;
	unsigned int val = 0;

	val = 100 * bluewhale_get_charger_current_extern();
#if defined(CONFIG_OZ115_DUAL_SUPPORT)
	val += 100 * oz115_sub_get_charger_current_extern();
#endif

	*(u32 *)data = val;

    return status;
}



static u32 charging_set_current(void *data)
{
	u32 status = STATUS_OK;
	unsigned int current_value = *(unsigned int *) data / 100; //mA

#if defined(CONFIG_OZ115_DUAL_SUPPORT)
	unsigned int sub_current_value = 0;
	unsigned int current_value_sum  = current_value;

    oz115_set_charge_current_sum_extern(current_value_sum);

#if defined(CONFIG_MTK_PUMP_EXPRESS_PLUS_20_SUPPORT) || defined(CONFIG_MTK_PUMP_EXPRESS_PLUS_SUPPORT)
	if(
#if defined(CONFIG_MTK_PUMP_EXPRESS_PLUS_20_SUPPORT)	
		(mtk_pep20_get_is_connect() == true) || //3A
#endif
#if defined(CONFIG_MTK_PUMP_EXPRESS_PLUS_SUPPORT)	
	    (mtk_pep_get_is_connect() == true) ||
#endif
	    false) //not TA charger
#else //don't support PE+/PE20
	//if(current_value >= 2000)
	if(0)
#endif
	{
#if 1	
		if (current_value < 1200) {
			sub_current_value = current_value;
			current_value = 0;//the min value is 600mA
		}
		else {
#if 0
			//current_value >= sub_current_value;
			sub_current_value = current_value / 2;
			sub_current_value /= 200; //resgister step: 200mA
			sub_current_value *= 200;
			current_value = current_value - sub_current_value; 
			if (current_value % 200)
				current_value = (current_value / 200) * 200 + 200;
#else
			//sub_current_value >= current_value;
			current_value = current_value_sum / 2;
			current_value /= 200; //resgister step: 200mA
			current_value *= 200;
			sub_current_value = current_value_sum - current_value;
			if (sub_current_value % 200)
				sub_current_value = (sub_current_value / 200) * 200 + 200;
#endif
		}
#else
		//only for test
		sub_current_value = 2000;
		current_value =  2000;
#endif
	}

	battery_log(BAT_LOG_CRTI,"charging set charge current %d mA,sub=%d mA\n",current_value,sub_current_value);
#endif
	bluewhale_set_charger_current_extern(current_value);

#if defined(CONFIG_OZ115_DUAL_SUPPORT)
	if(!sub_current_value)
		oz115_sub_stop_charge_extern(0); //set SUS to disable charger

	oz115_sub_set_charger_current_extern(sub_current_value);

#endif

    return status;
}


static u32 charging_set_input_current(void *data)
{
	u32 status = STATUS_OK;
	unsigned int current_value = *(unsigned int *) data / 100; //mA

#if defined(CONFIG_OZ115_DUAL_SUPPORT)
	unsigned int sub_current_value = 0;

#if defined(CONFIG_MTK_PUMP_EXPRESS_PLUS_20_SUPPORT) || defined(CONFIG_MTK_PUMP_EXPRESS_PLUS_SUPPORT)
	if(
#if defined(CONFIG_MTK_PUMP_EXPRESS_PLUS_20_SUPPORT)	
		(mtk_pep20_get_is_connect() == true && (current_value >= 200)) || //2A
#endif
#if defined(CONFIG_MTK_PUMP_EXPRESS_PLUS_SUPPORT)	
	    (mtk_pep_get_is_connect() == true && (current_value >= 200)) ||
#endif
	    false) //not TA charger
#else //don't support PE+/PE20
	//if(current_value >= 2000)
	if(0)
#endif
	{
		sub_current_value = current_value/2;
		current_value = current_value - sub_current_value; //1000  改9V2A
		if (sub_current_value == 1000) sub_current_value += 200; //1200  改9V2A 
	}

	battery_log(BAT_LOG_CRTI,"charging set_input_current %d mA,sub=%d mA\n",current_value,sub_current_value);

#endif
	bluewhale_set_vbus_current_extern(current_value);

#if defined(CONFIG_OZ115_DUAL_SUPPORT)
	if(!sub_current_value)
		oz115_sub_stop_charge_extern(0); //set SUS to disable charger

	oz115_sub_set_vbus_current_extern(sub_current_value);
#endif

    return status;
}

static u32 charging_get_input_current(void *data)
{
	int ret = 0;
	unsigned int current_value = 0;

	current_value = bluewhale_get_vbus_current_extern();

#if defined(CONFIG_OZ115_DUAL_SUPPORT)
	if (oz115_sub_is_enabled())
		current_value += oz115_sub_get_vbus_current_extern();
#endif
	*((u32 *)data) = current_value * 100;

	return ret;
}


static u32 charging_get_charging_status(void *data)
{
	u32 status = STATUS_OK;
	if (bluewhale_get_charging_status_extern() == 1)
		*(u32 *)data = KAL_TRUE;
	else
		*(u32 *)data = KAL_FALSE;

#if defined(CONFIG_OZ115_DUAL_SUPPORT)
	oz115_sub_get_charging_status_extern();
#endif

    return status;
}

static u32 charging_reset_watch_dog_timer(void *data)
{
	u32 status = STATUS_OK;

	//set nothing temp  3-21

	return status;
}


static u32 charging_set_hv_threshold(void *data)
{
	u32 status = STATUS_OK;

	u32 set_hv_voltage;
	u32 array_size;
	u16 register_value;
	u32 voltage = *(u32 *) (data);

	array_size = GETARRAYNUM(VCDT_HV_VTH);
	set_hv_voltage = bmt_find_closest_level(VCDT_HV_VTH, array_size, voltage);
	register_value = charging_parameter_to_value(VCDT_HV_VTH, array_size, set_hv_voltage);
	pmic_set_register_value(PMIC_RG_VCDT_HV_VTH, register_value);

#if 0
	register_value = 0x20; //plug out HW detection debounce time: 512 ms
	pmic_set_register_value(PMIC_RG_LOW_ICH_DB, register_value);

	battery_log(BAT_LOG_CRTI, "[charging_set_hv_threshold] [PMIC_RG_LOW_ICH_DB] = 0x%x\n",
			pmic_get_register_value(PMIC_RG_LOW_ICH_DB));
	battery_log(BAT_LOG_CRTI, "[charging_set_hv_threshold] [PMIC_RG_VCDT_HV_VTH] = 0x%x\n",
			pmic_get_register_value(PMIC_RG_VCDT_HV_VTH));
	battery_log(BAT_LOG_CRTI, "[charging_set_hv_threshold] [PMIC_RG_VCDT_LV_VTH] = 0x%x\n",
			pmic_get_register_value(PMIC_RG_VCDT_LV_VTH));
	battery_log(BAT_LOG_CRTI, "[charging_set_hv_threshold] [PMIC_RGS_VCDT_HV_DET] = 0x%x\n",
			pmic_get_register_value(PMIC_RGS_VCDT_HV_DET));
	battery_log(BAT_LOG_CRTI, "[charging_set_hv_threshold] [PMIC_RG_LOW_ICH_DB] = 0x%x\n",
			pmic_get_register_value(PMIC_RG_LOW_ICH_DB));
#endif

	return status;
}

static unsigned int charging_set_vindpm_voltage(void *data)
{
	unsigned int status = STATUS_OK;

#ifdef CONFIG_OZ105T_SUPPORT
	unsigned int voltage = *(unsigned int *) data ; //mV
	oz105t_set_vlimit_extern(voltage);
#endif
	return status;
}

static unsigned int charging_set_vbus_ovp_en(void *data)
{

	unsigned int status = STATUS_OK;
	unsigned int e = *(unsigned int *) data;
	
	//disable chargerIN HV detection, cause max HV is 10.5 V
	pmic_set_register_value(PMIC_RG_VCDT_HV_EN, e);
	//pmic_set_register_value(MT6351_PMIC_RG_VCDT_HV_EN, e);                              

	return status;
}

static unsigned int charging_get_bif_vbat(void *data)
{
	int ret = 0;
	u32 vbat = 0;

	ret = mtk_bif_get_vbat(&vbat);
	if (ret < 0)
		return STATUS_UNSUPPORTED;

	*((u32 *)data) = vbat;

	return STATUS_OK;
}

static unsigned int charging_get_bif_tbat(void *data)
{
	int ret = 0, tbat = 0;

	ret = mtk_bif_get_tbat(&tbat);
	if (ret < 0)
		return STATUS_UNSUPPORTED;

	*((int *)data) = tbat;

	return STATUS_OK;
}

static unsigned int charging_get_bif_is_exist(void *data)
{
	int bif_exist = 0;

	bif_exist = mtk_bif_is_hw_exist();
	*(bool *)data = bif_exist;

	return 0;
}

static unsigned int charging_set_chrind_ck_pdn(void *data)
{
	int status = STATUS_OK;
	unsigned int pwr_dn;

	pwr_dn = *(unsigned int *) data;

#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
	pmic_set_register_value(PMIC_CLK_DRV_CHRIND_CK_PDN, pwr_dn);
#else
	pmic_set_register_value(PMIC_RG_DRV_CHRIND_CK_PDN, pwr_dn);
#endif

	return status;
}

static unsigned int charging_sw_init(void *data)
{
	u32 status = STATUS_OK;
    battery_log(BAT_LOG_FULL,"[charging_sw_init]\n");
	bluewhale_init_extern();

	return status;
}

static unsigned int charging_enable_safetytimer(void *data)
{
	int status = STATUS_OK;
	return status;
}

static unsigned int charging_get_is_safetytimer_enable(void *data)
{
	int ret = 0;
	return ret;
}

static unsigned int charging_set_dp(void *data)
{
	unsigned int status = STATUS_OK;

#ifndef CONFIG_POWER_EXT
	unsigned int en;

	en = *(int *) data;
	hw_charging_enable_dp_voltage(en);
#endif

	return status;
}

static unsigned int charging_get_charger_temperature(void *data)
{
	return STATUS_UNSUPPORTED;
}

static unsigned int charging_set_boost_current_limit(void *data)
{
	int ret = 0;

#ifdef CONFIG_OZ115_SUPPORT                                                              
	return ret;
#endif

#ifdef CONFIG_OZ105T_SUPPORT
	current_limit = *((unsigned int *)data);
	//oz105t_set_otg_ocp_extern(current_limit);
	//oz105t_set_otg_voltage_extern(5250);     
#endif

	return ret;
}

static unsigned int charging_enable_otg(void *data)
{
	int ret = 0;
	unsigned int enable = 0;

	enable = *((unsigned int *)data);
	bluewhale_enable_otg_extern(enable);

	return ret;
}

static unsigned int charging_set_pwrstat_led_en(void *data)
{
	int ret = 0;
	unsigned int led_en;

	led_en = *(unsigned int *) data;

	pmic_set_register_value(PMIC_CHRIND_MODE, 0x2); /* register mode */
	pmic_set_register_value(PMIC_CHRIND_EN_SEL, !led_en); /* 0: Auto, 1: SW */
	pmic_set_register_value(PMIC_CHRIND_EN, led_en);

	return ret;
}
#ifdef CONFIG_MTK_PUMP_EXPRESS_PLUS_20_SUPPORT
static unsigned int charging_set_ta20_reset(void *data)
{
#ifdef CONFIG_OZ105T_SUPPORT
	oz105t_set_vlimit_extern(4500); //mV
#endif
	bluewhale_set_ta20_reset_extern();

	return STATUS_OK;
}

static unsigned int charging_set_ta20_current_pattern(void *data)
{
	CHR_VOLTAGE_ENUM chr_vol = *(CHR_VOLTAGE_ENUM *) data;

	battery_log(BAT_LOG_CRTI, "charging_set_ta20_current_pattern\n");
#ifdef CONFIG_OZ115_DUAL_SUPPORT
	oz115_sub_stop_charge_extern(0);
	oz115_sub_dump_register();
#endif

#ifdef CONFIG_OZ105T_SUPPORT
	oz105t_set_vlimit_extern(4500); //mV
#endif
	return bluewhale_set_ta20_current_pattern_extern(chr_vol);
}
#endif

static unsigned int charging_enable_direct_charge(void *data)
{
	return -ENOTSUPP;
}

static unsigned int charging_get_ibus(void *data)
{
	return -ENOTSUPP;
}

static unsigned int charging_get_vbus(void *data)
{
	return -ENOTSUPP;
}

static unsigned int charging_reset_dc_watch_dog_timer(void *data)
{
	return -ENOTSUPP;
}

static unsigned int charging_run_aicl(void *data)
{
	return -ENOTSUPP;
}


static unsigned int charging_get_hv_status(void *data)
{
	u32 status = STATUS_OK;

#if defined(CONFIG_POWER_EXT) || defined(CONFIG_MTK_FPGA)
	*(kal_bool *) (data) = 0;
#else
	*(kal_bool *) (data) = pmic_get_register_value(PMIC_RGS_VCDT_HV_DET);
#endif
	return status;
}


static unsigned int charging_get_battery_status(void *data)
{
	unsigned int status = STATUS_OK;

#if 1 //defined(CONFIG_POWER_EXT) || defined(CONFIG_MTK_FPGA)
	*(kal_bool *) (data) = 0;	/* battery exist */
	battery_log(BAT_LOG_CRTI, "[charging_get_battery_status] battery exist for bring up.\n");
#else
	unsigned int val = 0;

	val = pmic_get_register_value(PMIC_BATON_TDET_EN);
	battery_log(BAT_LOG_FULL, "[charging_get_battery_status] BATON_TDET_EN = %d\n", val);
	if (val) {
		pmic_set_register_value(PMIC_BATON_TDET_EN, 1);
		pmic_set_register_value(PMIC_RG_BATON_EN, 1);
		*(kal_bool *) (data) = pmic_get_register_value(PMIC_RGS_BATON_UNDET);
	} else {
		*(kal_bool *) (data) = KAL_FALSE;
	}
#endif

	return status;
}


static u32 charging_get_charger_det_status(void *data)
{
	unsigned int status = STATUS_OK;
	unsigned int val = 0;

#if defined(CONFIG_POWER_EXT) || defined(CONFIG_MTK_FPGA)
	val = 1;
	battery_log(BAT_LOG_CRTI, "[charging_get_charger_det_status] chr exist for fpga.\n");
#else
	val = pmic_get_register_value(PMIC_RGS_CHRDET);
#endif

	*(kal_bool *) (data) = val;

	return status;
}

static u32 charging_get_charger_type(void *data)
{
	u32 status = STATUS_OK;

#if defined(CONFIG_POWER_EXT) || defined(CONFIG_MTK_FPGA)
	*(CHARGER_TYPE *) (data) = STANDARD_HOST;
#else
	*(CHARGER_TYPE *) (data) = hw_charging_get_charger_type();
#endif

	return status;
}

static u32 charging_get_is_pcm_timer_trigger(void *data)
{
	u32 status = STATUS_OK;
/* M migration
	if (slp_get_wake_reason() == WR_PCM_TIMER)
		*(kal_bool *) (data) = KAL_TRUE;
	else
		*(kal_bool *) (data) = KAL_FALSE;
	battery_log(BAT_LOG_CRTI, "slp_get_wake_reason=%d\n", slp_get_wake_reason());
*/
	*(kal_bool *)(data) = KAL_FALSE;
	return status;
}

static u32 charging_set_platform_reset(void *data)
{
	unsigned int status = STATUS_OK;

#if defined(CONFIG_POWER_EXT) || defined(CONFIG_MTK_FPGA)
#else
	battery_log(BAT_LOG_CRTI, "charging_set_platform_reset\n");

	kernel_restart("battery service reboot system");
	/* arch_reset(0,NULL); */
#endif

	return status;
}

static u32 charging_get_platform_boot_mode(void *data)
{
	unsigned int status = STATUS_OK;

#if defined(CONFIG_POWER_EXT) || defined(CONFIG_MTK_FPGA)
#else
	*(unsigned int *) (data) = get_boot_mode();

	battery_log(BAT_LOG_CRTI, "get_boot_mode=%d\n", get_boot_mode());
#endif

	return status;
}

static u32 charging_set_power_off(void *data)
{
	unsigned int status = STATUS_OK;

#if defined(CONFIG_POWER_EXT) || defined(CONFIG_MTK_FPGA)
#else
	battery_log(BAT_LOG_CRTI, "charging_set_power_off\n");
	kernel_power_off();
#endif

	return status;
}

static u32 charging_get_power_source(void *data)
{
	u32 status = STATUS_UNSUPPORTED;

	return status;
}

static u32 charging_get_csdac_full_flag(void *data)
{
	return STATUS_UNSUPPORTED;
}

static u32 charging_set_ta_current_pattern(void *data)
{
#if	!defined(CONFIG_MTK_PUMP_EXPRESS_PLUS_SUPPORT)
	return STATUS_UNSUPPORTED;
#else
	unsigned int status = STATUS_OK;
	unsigned int increase = *(unsigned int*)(data);

	battery_log(BAT_LOG_CRTI, "charging_set_ta_current_pattern\n");
#ifdef CONFIG_OZ115_DUAL_SUPPORT
	oz115_sub_stop_charge_extern(0);
	oz115_sub_dump_register();
#endif
	//bluewhale_dump_register();
	bluewhale_set_ta_current_pattern_extern(increase);


	return status;	
#endif
}

static u32 charging_set_error_state(void *data)
{
	return STATUS_UNSUPPORTED;
}

static unsigned int charging_diso_init(void *data)
{
	return STATUS_UNSUPPORTED;
}

static unsigned int charging_get_diso_state(void *data)
{
	return STATUS_UNSUPPORTED;
}

#if defined(CONFIG_OZ115_DUAL_SUPPORT)
static unsigned int charging_hw_init_dual(void *data)
{
	charging_hw_init(data);
	oz115_sub_init_extern();
	
	return 1;
}
static unsigned int charging_dump_register_dual(void *data)
{
	charging_dump_register(data);
    oz115_sub_dump_register();
	return 1;
}

static unsigned int charging_enable_dual(void *data)
{
	u32 enable = *(u32 *) (data);

	battery_log(BAT_LOG_CRTI, "get_boot_mode=%d\n", get_boot_mode());
	if (get_boot_mode() == META_BOOT || get_boot_mode() == ADVMETA_BOOT)
	{
		o2_suspend_charger_extern();
#ifdef CONFIG_OZ115_DUAL_SUPPORT
		oz115_sub_stop_charge_extern(0); //suspend sub-oz115 charger
#endif
		return 1;
	}

	charging_enable(data);

#if defined(CONFIG_MTK_PUMP_EXPRESS_PLUS_20_SUPPORT) || defined(CONFIG_MTK_PUMP_EXPRESS_PLUS_SUPPORT)
	if ( 
#if defined(CONFIG_MTK_PUMP_EXPRESS_PLUS_20_SUPPORT)	
			mtk_pep20_get_is_connect() == false &&
#endif
#if defined(CONFIG_MTK_PUMP_EXPRESS_PLUS_SUPPORT)	
			 mtk_pep_get_is_connect() == false &&
#endif
		     KAL_TRUE == enable )
	{
		oz115_sub_stop_charge_extern(0); //set SUS to disable charger
		return 1;
	}
#endif
	if(!oz115_sub_get_charger_current_extern() && KAL_TRUE == enable) {
		oz115_sub_stop_charge_extern(0); //set SUS to disable charger
		return 1;
	}

    if(KAL_TRUE == enable)
		oz115_sub_start_charge_extern();
	else
	{
#if defined(CONFIG_USB_MTK_HDRC_HCD)
		if (mt_usb_is_device())
#endif
		{
			charging_get_charger_det_status(&enable);
			if (enable == 0) {
				battery_log(BAT_LOG_FULL,"[charging_enable] cable out\n");
				oz115_sub_stop_charge_extern(1);
			}
			else
				oz115_sub_stop_charge_extern(0);
		}
	}

	return 1;
}
#endif

static unsigned int (* const charging_func[CHARGING_CMD_NUMBER])(void *data)=
{
#if defined(CONFIG_OZ115_DUAL_SUPPORT)
	charging_hw_init_dual,
	charging_dump_register_dual,
	charging_enable_dual,
#else
	charging_hw_init,
	charging_dump_register,
	charging_enable,
#endif
	charging_set_cv_voltage,
	charging_get_current,
	charging_set_current,
	charging_set_input_current,
	charging_get_charging_status,
	charging_reset_watch_dog_timer,
	charging_set_hv_threshold,
	charging_get_hv_status,
	charging_get_battery_status,
	charging_get_charger_det_status,
	charging_get_charger_type,
	charging_get_is_pcm_timer_trigger,
	charging_set_platform_reset,
	charging_get_platform_boot_mode,
	charging_set_power_off,
	charging_get_power_source,
	charging_get_csdac_full_flag,
	charging_set_ta_current_pattern,
	charging_set_error_state,
	charging_diso_init,
	charging_get_diso_state,   
	charging_set_vindpm_voltage, //oz115 does nothing
	charging_set_vbus_ovp_en,//25
	charging_get_bif_vbat,
	charging_set_chrind_ck_pdn,
	charging_sw_init, //28
	charging_enable_safetytimer,
	NULL, //charging_set_hiz_swchr,
	charging_get_bif_tbat,
#ifdef CONFIG_MTK_PUMP_EXPRESS_PLUS_20_SUPPORT
	charging_set_ta20_reset,
	charging_set_ta20_current_pattern,
#else
	NULL,
	NULL,
#endif
	charging_set_dp,
	charging_get_charger_temperature,
	charging_set_boost_current_limit, //oz115 des nothing
	charging_enable_otg,
	NULL, //charging_enable_power_path,
	charging_get_bif_is_exist,
	charging_get_input_current, //40
	charging_enable_direct_charge,
	NULL, //charging_get_is_power_path_enable,
	charging_get_is_safetytimer_enable,
	charging_set_pwrstat_led_en,//44
	charging_get_ibus,
	charging_get_vbus,
	charging_reset_dc_watch_dog_timer,
	charging_run_aicl,//48
	NULL, //charging_set_ircmp_resistor,
	NULL //charging_set_ircmp_volt_clamp,
};

s32 chr_control_interface(CHARGING_CTRL_CMD cmd, void *data)
{
	s32 status;

	if (cmd < CHARGING_CMD_NUMBER)
		if (charging_func[cmd] ){
		    status = charging_func[cmd] (data);		
		}else{
			printk("charging_func[%d] is NULL \n",cmd );
			return STATUS_UNSUPPORTED;
		}
	else
		return STATUS_UNSUPPORTED;

	return status;
}
