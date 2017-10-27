/*
 * leadingui_lu2x3x.c
 *
 * Copyright (c) 2015 LeadingUI Co.,Ltd.
 * Author: jaehan@leadingui.com
 * Copyright (c) 2011-2015, The Linux Foundation. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 *
 */

/*------------------------------------------------------------------------------
v2.02
	1. reset-gpio is false
		call leadingui_ts_host_request_comm
		call set normal mode command
v2.03
	1. organisation codes
	2. remove PLATFORMDATA_BOARD_FILE preprocessor
v2.04
	1. F/W Upgrade modified
v2.05
    1. Max Keycode error fixed
v2.06
	1. add MTK support
v2.07
	1. source organize
v2.08
	1. Suspend/Resume -> Always regulator on,
	2. Suspend: command
	3. Resume : Reset GPIO
v2.09
	1. add g_ts_data
	2. remove fw_info_mutex
	3. change i2c_access to g_ts_mutex
v2.10
	1. change resume sequence
    2. change power on delay and reset delay time
v2.11
	1. resume function bug fixed
v2.12
	1. add delay and change positon of lock call in resume function
	2. bug fixed
v2.13
	1. i2c_write fixed
	2. remove function definition code
	3. error fixed wd_timer call routines
v2.14
	1. remove call ts_reset in leadingui_ts_power_ctrl
v2.15
	1. remove ts_data->work_lock and semaphore
v2.16
	1. add error recovy in leadingui_ts_resume
	2. Whel resume state, must call device reset.
	3. change position of mutex unlock in leadingui_fw_upgrade_store
	4. Delete unnecessary code
v2.18
	1. Modified Multi-touch Device Driver Requirements
v2.19
	1. modified directives of TYPE_B_PROTOCOL
	2. add Config_Touch_GPIO function
	3. change timer start position in leadnigui_ts_probe
v2.20
	1. add i2c clock speed in i2c write functions.
	2. modified MTK_EINT_PIN_MODE(), leadingui_ts_host_request_comm().
	3. change function parameter of the leadingui_ts_suspend and leadingui_ts_resume.
v2.21
	1. remove unused variables.
v2.22
	1. etc...

 ------------------------------------------------------------------------------*/

#include "tpd.h"
#include <linux/string.h>
#include <linux/wait.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of_gpio.h>
#include <asm-generic/gpio.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <mt-plat/mt_gpio.h>
#include <mach/gpio_const.h>
#ifdef TPD_PROXIMITY
#include <linux/hwmsensor.h>
#include <linux/hwmsen_dev.h>
#include <linux/sensors_io.h>
#endif

#include "mt_boot_common.h"
#include "upmu_common.h"
#include <linux/of_irq.h>
#include <linux/interrupt.h>
#ifdef GTP_SUPPORT_I2C_DMA
#include <linux/dma-mapping.h>
#endif
#include <linux/proc_fs.h>  /*proc*/

#include <linux/wakelock.h>//wujian
#ifdef CONFIG_OF
#include <linux/regulator/consumer.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#endif

#include "tpd_custom_lu2x3x.h"
#include "leadingui_lu2x3x.h"
#include <linux/gpio.h>
#include <linux/of_gpio.h>

#include <linux/input/mt.h>

#if BUILT_IN_UPGRADE
//#include "leadingui_lu2010_fw.h"
//#include "leadingui_lu3010_fw.h"
#include "leadingui_lu310a_fw.h"
#endif

//#include <cust_gpio_usage.h>
#include <mt-plat/mt_boot.h>
#include <mt-plat/mt_gpio.h>
#include <mach/gpio_const.h>
extern int mt_get_gpio_in(unsigned long pin);

static u8 boot_mode;

static int tpd_flag = 0;
static int tpd_halt = 0;

extern struct tpd_device *tpd;
static struct task_struct *thread = NULL;
struct leadingui_ts_data *g_ts_data = NULL;
static unsigned int touch_irq = 0;

static DECLARE_WAIT_QUEUE_HEAD(waiter);

static DEFINE_MUTEX(g_ts_mutex);

#ifdef TPD_HAVE_BUTTON
static int tpd_keys_local[TPD_KEY_COUNT] = TPD_KEYS;
static int tpd_keys_dim_local[TPD_KEY_COUNT][4] = TPD_KEYS_DIM;
#endif

#define DRIVER_VERSION				   "2.22"

//#define TYPE_B_PROTOCOL

#define LITTLE_ENDIAN				   0x0
#define BIG_ENDIAN					   0x1

#define LUI_SUSPEND_LEVEL			   1

#define CHIP_RESET_IO_UDELAY		   30
#define CHIP_RESET_IO_MDELAY		   1
#define BOOTING_DELAY				   30

#define CHIP_RESET_MDELAY_5			   5
#define CHIP_RESET_MDELAY_10		   10
#define CHIP_RESET_MDELAY_20		   20

#define LU2010_RESET_MDELAY			   100
#define LU3010_RESET_MDELAY			   100
#define LU3100_RESET_MDELAY			   100
#define LU310A_RESET_MDELAY			   100

#define POWER_ON_DELAY				   30
#define SUSPEND_CMD_DELAY			   1
#define RESUME_SW_WAKEUP_DELAY		   20

#define I2C_ADDRESS_LENGTH			   2
#define I2C_MAX_TRANSFER_SIZE		   8
#define I2C_MAX_RETRY_COUNT			   10

#define DEVIE_MAX_RECOVERY_COUNT	   3

/* communication for device */
#define LU2010_CTRL_CHIP_ID_REG		   0x10FD
#define DEVICE_CTRL_CHIP_ID_REG		   0x10FC
#define DEVICE_CTRL_COMMAND_REG		   0x10FF

#define DEVICE_CTRL_RESET			   0x02
#define DEVICE_CTRL_DONE			   0x01

#define I2C_WAIT_DELAY				   100		/* ms */
#define I2C_INTN_SWITCH_UDELAY		   100		/* us, typical is 50ns */
#define I2C_INTN_SWITCH_MDELAY		   1		/* 1 ms */


/* communication for firmware */
#define DEVICE_COMMAND_REG			   0x00E0
#define DEVICE_REPLY_REG			   0x00ED
#define DEVICE_DATA_REG				   0x0100
#define DEVICE_EVENT_REG			   0x0000
#define DEVICE_EVENT_DATA_REG		   0x0004
#define LU2010_EVENT_DATA_REG		   0x0005

#define DEVICE_CMD_OP_MODE			   0xA3
#define DEVICE_CMD_DEV_INFO			   0xAA
#define DEVICE_CMD_SIMPLE			   0xD0

#define DEVICE_OP_MA_NORMAL			   0x00
#define DEVICE_OP_MA_PWR_DN			   0x01

#if LUI_GESTURE_EN
#define DEVICE_OP_MA_GESTURE_WAKEUP    0x04
#endif

#define DEVICE_OP_MN_NONE			   0x00

#define DEVICE_CMD_LENGTH              5
#define DEVICE_REPLY_LENGTH            3

#define DEVICE_SIMPLE_DATA_SIZE        0x10

#if LUI_GESTURE_EN
#define NO_GESTURE_WAKEUP              0
#define GESTURE_C_WAKEUP_ENABLE        0x01
#define GESTURE_M_WAKEUP_ENABLE        0x02
#define GESTURE_V_WAKEUP_ENABLE        0x04
#define GESTURE_W_WAKEUP_ENABLE        0x08
#define GESTURE_Z_WAKEUP_ENABLE        0x10
#define GESTURE_KINOCK_WAKEUP_ENABLE   0x20

#define ALL_GESTURE_WAKEUP_ENABLE      GESTURE_C_WAKEUP_ENABLE|GESTURE_M_WAKEUP_ENABLE\
	                                   |GESTURE_V_WAKEUP_ENABLE|GESTURE_W_WAKEUP_ENABLE\
                                       |GESTURE_Z_WAKEUP_ENABLE|GESTURE_KINOCK_WAKEUP_ENABLE
#endif




#define LU2_EVENT_LENGTH			   sizeof(struct lu2_touch_event)
#define LU3_EVENT_LENGTH			   sizeof(struct lu3_touch_event)

#define MAX_DEVICE_EVENT_LENGTH		   (LU2_EVENT_LENGTH > LU3_EVENT_LENGTH) ? LU2_EVENT_LENGTH : LU3_EVENT_LENGTH

static struct workqueue_struct *leadingui_wq;

static struct {
	struct lu2_touch_event lu2_tevent;
	struct lu2_touch_data  lu2_tdata[LUI_MAX_FINGER];

	struct lu3_touch_event lu3_tevent;
	struct lu3_touch_data  lu3_tdata[LUI_MAX_FINGER];
}g_fdata;

#if LUI_GESTURE_EN
static int gesture_wakeup_mode = 0;
u16 gesture_mask = 0;
static int gesture_wakeup_switch = 0;
/*
short point_num = 0;
unsigned short lu_coordinate_x[150] = {0};
unsigned short lu_coordinate_y[150] = {0};
*/
static char tpgesture_value[10]={};
static char tpgesture_status_value[5] = {};

#endif

//unsigned int tpd_rst_gpio_number = 10;
//unsigned int tpd_int_gpio_number = 1;
//#define GPIO_CTP_EINT_PIN          (GPIO1 | 0x80000000)

#define LUI_MAX_KEYCODES		       4

#if (LUI_MAX_KEYCODES)
static int leadingui_keymap[LUI_MAX_KEYCODES] = { KEY_MENU, KEY_HOMEPAGE, KEY_BACK ,0};
#else
static int leadingui_keymap[LUI_MAX_KEYCODES] = { };
#endif

#if 0
static struct leadingui_platform_data leadingui_touch_pdata = {
	.allow_device_id  = DEVICE_LU310A,
	.max_x            = 720,
	.max_y            = 1280,
	.max_keys         = LUI_MAX_KEYCODES,
	.key_map          = leadingui_keymap,
	.support_pressure = true,
	.max_pressure     = 100,
	.max_touch_major  = 10,
	.max_touch_minor  = 10,

	.use_regulator    = true,
	.use_reset_gpio   = true,
	.support_suspend  = true,
};
#else
static struct leadingui_platform_data leadingui_touch_pdata = {
	.reset_gpio       = 0,	             /* reset gpio number */
	.irq_gpio         = 1,	             /* irq gpio number */
	.allow_device_id  = DEVICE_LU310A,
	.max_x            = 1080,		     //Venzo X-Resolution
	.max_y            = 1920,            //Venzo Y -Resolution
	.max_keys         = LUI_MAX_KEYCODES,
	.key_map          = leadingui_keymap,
	.support_pressure = true,
	.max_pressure     = 100,
	.max_touch_major  = 10,
	.max_touch_minor  = 10,
	.use_regulator    = true,
	.use_reset_gpio   = true,
	.support_suspend  = true,
};
#endif

/* extern functions */
extern void mt_eint_unmask(unsigned int line);
extern void mt_eint_mask(unsigned int line);
extern void mt_eint_set_hw_debounce(unsigned int eintno, unsigned int ms);
extern unsigned int mt_eint_set_sens(unsigned int eintno, unsigned int sens);
extern void mt_eint_registration(unsigned int eint_num, unsigned int flag, void (EINT_FUNC_PTR)(void), unsigned int is_auto_umask);

static void tpd_eint_handler(void);
static int touch_event_handler(void *data);

static int leadingui_ts_suspend(struct device *h);
static int leadingui_ts_resume(struct device *h);


/* function declaration */
static int leadingui_ts_dev_irq_init (struct leadingui_ts_data *ts_data);
static void release_all_ts_event(struct leadingui_ts_data *ts_data);

/* mediatek functions */
enum {
	MODE_GPIO = 0,
	MODE_EINT = 1
};

enum {
	DIR_IN  = 0,
	DIR_OUT = 1
};


#if (LUI_WATCHDOG_TIMER)
static struct delayed_work leadingui_wdt_check_work;
static struct workqueue_struct *leadingui_wdt_check_workqueue = NULL;
static void leadingui_wd_timer_work(struct work_struct *work)
#endif

static void MTK_RST_PIN_INIT(void)
{
	//tpd_gpio_output(tpd_rst_gpio_number, 1);
	tpd_gpio_output(GTP_RST_PORT, 1);
}

static void MTK_RST_PIN_TOGGLE(void)
{
	//tpd_gpio_output(tpd_rst_gpio_number, 0);
	tpd_gpio_output(GTP_RST_PORT, 0);
	udelay(CHIP_RESET_IO_UDELAY);
	//tpd_gpio_output(tpd_rst_gpio_number, 1);
	tpd_gpio_output(GTP_RST_PORT, 1);
}

#if 1
static void MTK_EINT_PIN_MODE(int mode, unsigned long dir)
{
	if (mode == MODE_EINT) {
		//tpd_gpio_as_int(tpd_int_gpio_number);
		tpd_gpio_as_int(GTP_INT_PORT);
	} else {

		if (dir == DIR_IN)
			//tpd_gpio_as_int(tpd_int_gpio_number);
			tpd_gpio_as_int(GTP_INT_PORT);
		else
			//tpd_gpio_output(tpd_int_gpio_number, 0);
			tpd_gpio_output(GTP_INT_PORT, 0);
	}

}
#endif
static int leadingui_ts_irq_enable(struct leadingui_ts_data *ts_data, bool enable)
{
	int ret = 0;

	if (enable) {
		LUI_TRACE("irq enabled\n");

		if (!ts_data->irq_enabled) {
			enable_irq(touch_irq);
			ts_data->irq_enabled = true;
		}

	} else {
		LUI_TRACE("irq disabled\n");

		if (ts_data->irq_enabled) {
			disable_irq_nosync(touch_irq);
			ts_data->irq_enabled = false;
		}
	}

	return ret;
}

static int touch_i2c_read(struct i2c_client *client, u16 addr, int len, u8 *buf)
{
	int retry = 0;
	u8 addr_buff[I2C_ADDRESS_LENGTH];

	u16 remain_bytes = len;
	u16 offset = 0;

	struct i2c_msg msgs[] = {
		{
			.addr   = client->addr,
			.flags  = 0,
			.len    = I2C_ADDRESS_LENGTH,
			.buf    = addr_buff,
			.timing = 400
		},
		{
			.addr   = client->addr,
			.flags  = I2C_M_RD,
			.timing = 400
		},
	};

    while (remain_bytes > 0)
    {
	    addr_buff[0] = ((addr + offset) >> 8) & 0xFF;
		addr_buff[1] = ( addr + offset) & 0xFF;

		msgs[1].buf = &buf[offset];

		if ( remain_bytes > I2C_MAX_TRANSFER_SIZE ) {
			 msgs[1].len   = I2C_MAX_TRANSFER_SIZE;
			 remain_bytes -= I2C_MAX_TRANSFER_SIZE;
			 offset       += I2C_MAX_TRANSFER_SIZE;
		} else {
			 msgs[1].len = remain_bytes;
			 remain_bytes = 0;
		}

		retry = 0;

		while (i2c_transfer(client->adapter, &msgs[0], 2) != 2) {

			retry++;

			if (retry == 3) {
				LUI_ERROR("I2C read 0x%X length=%d failed\n", addr + offset, len);
				return -EIO;
			}
		}
    }

	return 0;
}

static int touch_i2c_write(struct i2c_client *client, u16 addr, int len, u8 * buf)
{
	u8  send_buf[I2C_MAX_TRANSFER_SIZE];
	u16 trans_size   = 0;
	u16 remain_bytes = 0;
	u16 offset       = 0;
	u16 max_transaction_size = I2C_MAX_TRANSFER_SIZE - I2C_ADDRESS_LENGTH;

	struct i2c_msg msg = {
		.addr   = client->addr,
		.flags  = 0,
		.buf    = send_buf,
		.timing = 400,
	};

	if (len <= 0) {
		LUI_ERROR("invalid length \n");
		return -EINVAL;
	}

	remain_bytes = len;

	while (remain_bytes > 0) {

		send_buf[0] = ((addr + offset) >> 8) & 0xFF;
		send_buf[1] = (addr + offset) & 0xFF;

		if (remain_bytes >= max_transaction_size)
			trans_size = max_transaction_size;
		else
			trans_size = remain_bytes;

		msg.len = trans_size + I2C_ADDRESS_LENGTH;

		memcpy(&send_buf[I2C_ADDRESS_LENGTH], &buf[offset], trans_size);

		if (i2c_transfer(client->adapter, &msg, 1) != 1) {
			LUI_ERROR("i2c_transfer error\n");
    		return -EIO;
		}

		remain_bytes -= trans_size;
		offset       += trans_size;
	}

	return 0;
}

static int touch_i2c_write_byte(struct i2c_client *client, u16 addr, u8 data)
{
	unsigned char send_buf[I2C_ADDRESS_LENGTH + 1];
	struct i2c_msg msg = {
		.addr   = client->addr,
		.flags  = client->flags,
		.len    = I2C_ADDRESS_LENGTH + 1,
		.buf    = send_buf,
		.timing = 400,
	};

	send_buf[0] = (addr >> 8) & 0xFF;
	send_buf[1] = addr & 0xFF;
	send_buf[2] = (unsigned char)data;

	if (i2c_transfer(client->adapter, &msg, 1) < 0) {
		LUI_ERROR("i2c_transfer error\n");
   		return -EIO;
	}

	return 0;
}

static int touch_i2c_done(struct i2c_client *client)
{
	int ret;
	ret = touch_i2c_write_byte(client, DEVICE_CTRL_COMMAND_REG, DEVICE_CTRL_DONE);

	if (ret) {
		LUI_ERROR("touch_i2c_done error\n");
	}

	return ret;
}

/* wait device interrupt */
static int touch_i2c_wait_device_ready(struct leadingui_ts_data *ts_data)
{
	int delay_cnt = 10000;
	
	/* wait INTn low */
	while(delay_cnt) {
		udelay(500);

		//if (tpd_int_gpio_number)
		//if (!mt_get_gpio_in(GPIO_CTP_EINT_PIN))
		//	return 0;
		if (!gpio_get_value(ts_data->pdata->irq_gpio))
			return 0;

		delay_cnt--;
	}

	LUI_ERROR("wait device interrupt timeout\n");
	return -EINTR;
}

static int leadingui_ts_check_chip_id(struct leadingui_ts_data *ts_data)
{
	int ret;
	u8 buff[2];
	u16 device_id;

	/* read chip id */
	if (ts_data->pdata->allow_device_id == DEVICE_LU2010)
		ret = touch_i2c_read(ts_data->client, LU2010_CTRL_CHIP_ID_REG, 2, buff);
	else
		ret = touch_i2c_read(ts_data->client, DEVICE_CTRL_CHIP_ID_REG, 2, buff);

	if (ret) {
		LUI_ERROR("get device id error\n");
	} else {
		device_id = ((buff[1] << 8) & 0xFF00) | (buff[0] & 0x00FF);

		LUI_INFO("connected device id: 0x%04x\n", device_id);

		/* check supported device */
		if (device_id != ts_data->pdata->allow_device_id) {
			LUI_ERROR("this driver only allow 0x%04X device!\n", ts_data->pdata->allow_device_id);
			return -ENODEV;
		}
	}

	return ret;
}

/*
*
* Host to device communication request
*
*/
static int leadingui_ts_host_request_comm(struct i2c_client *client)
{
	struct  leadingui_ts_data *ts_data = i2c_get_clientdata(client);
	
	int ret = 0;

	/* is interrupt enable ? */
	//gpio_direction_input(ts_data->pdata->irq_gpio); 
	MTK_EINT_PIN_MODE(MODE_EINT, DIR_IN);
	udelay( 30 );
    //if (!mt_get_gpio_in(GPIO_CTP_EINT_PIN)) {
	//if (!tpd_int_gpio_number) {
	if (!gpio_get_value(ts_data->pdata->irq_gpio)) {

		LUI_WARNING("irq_gpio is LOW.\n");

		ret = touch_i2c_done(client);
		if (ret)
			return ret;
	}
    //MTK_EINT_PIN_MODE (MODE_EINT, DIR_OUT);
	//tpd_gpio_output(tpd_int_gpio_number, 0);
	tpd_gpio_output(GTP_INT_PORT, 0);
	mdelay(I2C_INTN_SWITCH_MDELAY);
	//tpd_gpio_output(tpd_int_gpio_number, 1);
	tpd_gpio_output(GTP_INT_PORT, 1);
	MTK_EINT_PIN_MODE (MODE_EINT, DIR_IN);

	ret = touch_i2c_wait_device_ready(ts_data);

	return ret;
}

/*
 * command :
 * length : data length
 * data : data buffer
 */
static int leadingui_ts_command(struct i2c_client *client,
								u8 command, u16 param1, u16 param2,
								int length, u8* data)
{
	struct  leadingui_ts_data *ts_data = i2c_get_clientdata(client);
	u8 i2c_cmd[DEVICE_CMD_LENGTH];
	u8 i2c_reply[DEVICE_REPLY_LENGTH];
	int ret = 0;
	u16 reply_size;

	if (!ts_data) {
		LUI_ERROR("ts_data is null\n");
		ret = -EINVAL;
		goto err_done_proc;
	}

	/* make command data */
	i2c_cmd[0] = command;
	i2c_cmd[1] = (param1     ) & 0xff;
	i2c_cmd[2] = (param1 >> 8) & 0xff;
	i2c_cmd[3] = (param2     ) & 0xff;
	i2c_cmd[4] = (param2 >> 8) & 0xff;

	/* Host to device communication request */
	ret = leadingui_ts_host_request_comm(client);

	if (ret) goto err_done_proc;

	/* write command */
	ret = touch_i2c_write(client, DEVICE_COMMAND_REG, DEVICE_CMD_LENGTH, i2c_cmd);

	if (ret) goto err_done_proc;

	/* write i2c done */
	ret = touch_i2c_done(client);

	if (ret) goto err_done_proc;

	/* wait device interrupt */
	ret = touch_i2c_wait_device_ready(ts_data);

	if (ret) goto err_done_proc;

	/* read reply data */
	ret = touch_i2c_read(client, DEVICE_REPLY_REG, DEVICE_REPLY_LENGTH, i2c_reply);

	if (ret) goto err_done_proc;

	reply_size = ((i2c_reply[1] << 8) & 0xFF00) | (i2c_reply[0] & 0x00FF);

	/* check command */
	if (command != i2c_reply[2]) {
        #if 0 /* Who 2016-09-07*/
    	printk("i2c_reply0: 0x%02x,i2c_reply1: 0x%02x,i2c_reply2: 0x%02x\n",i2c_reply[0],i2c_reply[1],i2c_reply[2]);
    	printk("command: 0x%02x\n",command);
        #endif /* Who 2016-09-07 */	
		LUI_ERROR("Invalid reply type\n");
		goto err_done_proc;
	}

	if (length > 0) {
		/* read reply data */
		if (reply_size < length) {
			LUI_ERROR("reply_size(%d) < length(%d)", reply_size, length);
			ret = -EIO;
		} else {

			if (reply_size > length)
				reply_size = length;

			ret = touch_i2c_read(client, DEVICE_DATA_REG, length, data);
		}
	}

err_done_proc:

	/* write i2c done */
	touch_i2c_done(client);

	return ret;
}

static void leadingui_ts_reset_delay(struct leadingui_ts_data *ts_data)
{
	switch(ts_data->device_id) {
	case DEVICE_LU2010: msleep(LU2010_RESET_MDELAY); break;
	case DEVICE_LU3010: msleep(LU3010_RESET_MDELAY); break;
	case DEVICE_LU3100: msleep(LU3100_RESET_MDELAY); break;
	case DEVICE_LU310A: msleep(LU310A_RESET_MDELAY); break;
	default:            msleep(CHIP_RESET_MDELAY_20);break;
	}
}

static void leadingui_wakeup_signal(struct  leadingui_ts_data *ts_data)
{
	//tpd_gpio_output(tpd_int_gpio_number, 0);
	tpd_gpio_output(GTP_INT_PORT, 0);
	msleep(I2C_INTN_SWITCH_MDELAY);
	//tpd_gpio_output(tpd_int_gpio_number, 1);
	tpd_gpio_output(GTP_INT_PORT, 1);
	MTK_EINT_PIN_MODE (MODE_EINT, DIR_IN);
	msleep(10);
}

static int leadingui_ts_reset(struct leadingui_ts_data *ts_data)
{
	int ret = 0;
	int retry = 0;

	MTK_RST_PIN_TOGGLE();
	msleep(1);
	leadingui_ts_reset_delay(ts_data);
	return 0;
	if (ts_data->pdata->use_reset_gpio) {
		//tpd_gpio_output(tpd_rst_gpio_number, 0);
		tpd_gpio_output(GTP_RST_PORT, 0);
		udelay(CHIP_RESET_IO_UDELAY);
		//msleep(CHIP_RESET_IO_MDELAY);
		//tpd_gpio_output(tpd_int_gpio_number, 1);
		tpd_gpio_output(GTP_INT_PORT, 1);
	} else {

retry_reset:
		/* s/w reset is no wait. */
		ret = touch_i2c_write_byte( ts_data->client,
		                            DEVICE_CTRL_COMMAND_REG,
		                            DEVICE_CTRL_RESET);
		if (ret) {

			if ((ts_data->pdata->allow_device_id == DEVICE_LU2010) ||
				(ts_data->pdata->allow_device_id == DEVICE_LU3010)) {

   				LUI_TRACE("LU%04X device reset value is always false. \n", ts_data->device_id);
   				ret = 0;

			} else {
				LUI_ERROR("chip reset fail.\n");

			    /* wake-up device */
				leadingui_wakeup_signal(ts_data);

				touch_i2c_done(ts_data->client);

				msleep(CHIP_RESET_MDELAY_20);

				if (retry < 3) {
					LUI_ERROR("retry=%d.\n", retry);
					retry++;

					goto retry_reset;
				}
			}
		}
	}

	/* delay */
	if (ret >= 0) {
		leadingui_ts_reset_delay(ts_data);
	}

	return ret;
}

static int leadingui_ts_simple_info(struct leadingui_ts_data *ts_data, u16 *device_id, u16 *fw_version, u32 *fw_chksum, u16 *param_chksum)
{
	int ret;
	u8  data_buff[DEVICE_SIMPLE_DATA_SIZE];

	ret = leadingui_ts_command(ts_data->client,
	                           DEVICE_CMD_SIMPLE, 0, 0,
	                           DEVICE_SIMPLE_DATA_SIZE, data_buff);

	if (ret >= 0) {
		*device_id    = ((data_buff[1]<<8) | data_buff[0]);
		*fw_version   = ((data_buff[2]<<8) | data_buff[3]);
		*fw_chksum    = ((data_buff[7]<<24) |(data_buff[6]<<16) |(data_buff[5]<<8) | data_buff[4]);
		*param_chksum = ((data_buff[9]<<8) | data_buff[8]);
		/*
		LUI_TRACE("device_id = 0x%04x\n", (data_buff[1]<<8) | data_buff[0]);
		LUI_TRACE("f/w Ver = %d.%d\n", data_buff[2], data_buff[3]);
        */
	}

	return ret;
}

static int leadingui_ts_model_info(struct leadingui_ts_data *ts_data, u8 *model, u16 *device_id, u16 *fw_version)
{
	int ret = 0;

	u8  data_buff[23];

	if (model == NULL){
		LUI_ERROR("model argument is null.\n");
		return -EINVAL;
	}

	ret = leadingui_ts_command(ts_data->client,
	                           DEVICE_CMD_DEV_INFO, 0, 0,
	                           23, data_buff);
	if (ret >= 0){
		memcpy(model, data_buff + 3, 16);
		*fw_version = ((data_buff[19]<<8) | data_buff[20]);
		*device_id  = ((data_buff[22]<<8) | data_buff[21]);
		/*
		LUI_INFO("Model = %s", 			ts_data->model);
		LUI_INFO("f/w Ver.  = %d.%d\n", (ts_data->fw_version >> 8) & 0xff, ts_data->fw_version & 0xff);
		LUI_INFO("device_id = 0x%04X\n",(data_buff[22]<<8) | data_buff[21]);
		*/
	}else{
		LUI_ERROR("get device info error.\n");
	}

	return ret;
}

#if (LUI_WATCHDOG_TIMER)

static void leadingui_wd_timer_start(struct leadingui_ts_data *ts_data)
{
 
    #if 0
	if (ts_data == NULL) {
		LUI_ERROR("ts_data == NULL\n");
		return;
	}

	if (!LUI_WATCHDOG_TIMEOUT) {
		LUI_ERROR("Watchdog timer value is zero.\n");
		return;
	}

	mod_timer(&ts_data->wd_timer, jiffies + msecs_to_jiffies(LUI_WATCHDOG_TIMEOUT));
    #endif

	queue_delayed_work(leadingui_wdt_check_workqueue, &leadingui_wdt_check_work, LUI_WATCHDOG_TIMEOUT);

}

static void leadingui_wd_timer_stop(struct leadingui_ts_data *ts_data)
{
    #if 0
	if (ts_data == NULL){
		LUI_ERROR("ts_data == NULL\n");
		return;
	}

	del_timer(&ts_data->wd_timer);
	cancel_work_sync(&ts_data->wd_timer_work);
	del_timer(&ts_data->wd_timer);
    #endif 	
	cancel_delayed_work_sync(&leadingui_wdt_check_work);
}

#if 0
static void leadingui_wd_timeout_handler(unsigned long data)
{
	struct leadingui_ts_data *ts_data = (struct leadingui_ts_data *)data;

	if (ts_data == NULL) {
		LUI_ERROR("ts_data is NULL.\n");
		return;
	}

	if (!work_pending(&ts_data->wd_timer_work))
		schedule_work(&ts_data->wd_timer_work);
}
#endif

#if 0
static void leadingui_wd_timer_init(struct leadingui_ts_data *ts_data)
{

	if (ts_data == NULL){
		LUI_ERROR("ts_data == NULL\n");
		return;
	}

	setup_timer(&ts_data->wd_timer,
				leadingui_wd_timeout_handler,
				(unsigned long)ts_data);

	LUI_TRACE_WD_TIMER("Watchdog timer init.\n");
	
}
#endif

static void leadingui_wd_timer_work(struct work_struct *work)
{
	struct leadingui_ts_data *ts_data =
		           container_of(work, struct leadingui_ts_data, wd_timer_work);
	int ret = 0;
	u8  data_buff[23];
	u16 fw_ver = 0;
	u16 dev_id = 0;
	u32 fw_chksum=0;
	u16 param_chksum=0;
	//int retry_cnt = 0;

    /*	
    if (ts_data == NULL) {
		LUI_ERROR("ts_data is NULL.\n");
		leadingui_wd_timer_start(ts_data);
		return;
	}
    */
	/* check device state */
	if (ts_data->power_state != POWER_WAKE) {
		LUI_ERROR("device not wake, power state=%d\n", ts_data->power_state);
		return;
	}
    #if LUI_GESTURE_EN
	else
	{
        if(1 == gesture_wakeup_mode)
        {
			LUI_WARNING("device in gesture wakeup mode...\n");
			return;
        }
	}
    #endif	

	/* check f/w upgrade state */
	if (ts_data->fw_info.is_updating) {
		LUI_WARNING("device f/w updating...\n");
		return;
	}

	mutex_lock(&g_ts_mutex);

	/* check touch reported flag */
	if (ts_data->touch_reported) {

		ts_data->touch_reported = false;
		leadingui_wd_timer_start(ts_data);

		mutex_unlock(&g_ts_mutex);
		return;
	}

	/* disable interrupt */
	leadingui_ts_irq_enable(ts_data, false);

	if (atomic_read(&ts_data->device_init) != 1) {
		LUI_ERROR("device does not initialize\n");
		ret = -EPERM;
		goto fail_device_init_check;
	}

	/* check device */
	if (ts_data->device_id == DEVICE_LU2010) {

		ret = leadingui_ts_command(ts_data->client, DEVICE_CMD_DEV_INFO, 0, 0, 23, data_buff);

		if (ret >= 0) {
			fw_ver = (data_buff[19]<<8) | data_buff[20];
			dev_id = (data_buff[22]<<8) | data_buff[21];
		}
	} else {
		ret = leadingui_ts_simple_info(ts_data, &dev_id, &fw_ver, &fw_chksum, &param_chksum);
	}

    //dev_id=0x310a;
    //fw_ver=0xeccf;
	if (ret >= 0) {

		if ((ts_data->fw_version == fw_ver) && (ts_data->device_id == dev_id)) {

			LUI_TRACE_WD_TIMER("Watchdog timer - check device OK!\n");

			/* set interrupt mode */
			MTK_EINT_PIN_MODE (MODE_EINT, DIR_IN);

			/* enable irq */
			leadingui_ts_irq_enable(ts_data, true);

			/* restart timer */
			leadingui_wd_timer_start(ts_data);

			mutex_unlock(&g_ts_mutex);

			return;
		}

		LUI_ERROR("Watchdog timer device id check error.\n");
   		LUI_ERROR("f/w Ver.  = %d.%d\n", (fw_ver >> 8) & 0xff, fw_ver & 0xff);
   		LUI_ERROR("device_id = 0x%04X\n", dev_id);
	}

	LUI_ERROR("error. timeout occured. maybe ts device dead. so reset & reinit.\r\n");

fail_device_init_check:

	release_all_ts_event(ts_data);

	/* device reset */
	leadingui_ts_dev_irq_init(ts_data);

	leadingui_wd_timer_start(ts_data);

	mutex_unlock(&g_ts_mutex);
}

#endif

#define SwapUInt16(v) (ushort)(((v & 0xff) << 8) | ((v >> 8) & 0xff))
#define SwapUInt32(v) (u32)(((SwapUInt16((ushort)v) & 0xffff) << 0x10) | (SwapUInt16((ushort)(v >> 0x10)) & 0xffff))

static u32 make_checksum(const u8 *data, u16 data_length, int dataBytes, int endianType)
{
	u32 check_sum=0;
	u16 byte16=0;
	u32 byte32=0;
	u16 i=0;

	if (dataBytes == 1) {
		for(i=0; i<data_length; i+= dataBytes)
			check_sum += *(data+i);
	} else if (dataBytes == 2) {
		for (i = 0; i < data_length; i += dataBytes) {
			byte16 = 0;
			byte16 |= ( i	 < data_length ? *(data+i+0) : 0x00) << 0;
			byte16 |= ((i+1) < data_length ? *(data+i+1) : 0x00) << 8;
			check_sum += byte16;
		}
	} else if (dataBytes == 4) {
		for(i=0; i<data_length; i += dataBytes) {
			byte32 = 0;
			byte32 |= ( i	 < data_length ? *(data+i+0) : 0x00) << 0;
			byte32 |= ((i+1) < data_length ? *(data+i+1) : 0x00) << 8;
			byte32 |= ((i+2) < data_length ? *(data+i+2) : 0x00) << 16;
			byte32 |= ((i+3) < data_length ? *(data+i+3) : 0x00) << 24;
			check_sum += byte32;
		}
	}

	if (endianType != LITTLE_ENDIAN)
		return SwapUInt32(check_sum);

	return check_sum;
}

#define E2PROM_CTRL_REG			0x1000
#define E2PROM_LOCK_1_REG		0x1003
#define E2PROM_LOCK_2_REG		0x1004
#define E2PROM_ERASE_REG		0x1005
#define E2PROM_DATA_REG			0x8000

#define E2PROM_CTRL_ENABLE		0x01
#define E2PROM_CTRL_TEST		0x02
#define E2PROM_CTRL_READ		0x04
#define E2PROM_CTRL_LOCK_SEL	0x08
#define E2PROM_CTRL_RESET		0x10
#define E2PROM_CTRL_SYS_RESET	0x20
#define E2PROM_CTRL_READY		0x40
#define E2PROM_CTRL_PROGRAM		0x80
#define E2PROM_ERASE_ENABLE		0x04

#define E2PROM_ERASE_TIME		5 /* ms */

static int leadingui_e2prom_write(bool standalone, struct leadingui_ts_data *ts_data)
{
	int ret = 0;
	int retry = 0;
	int remain_size = 0;
	int page_size = 0, write_size;
	u16 write_pos;

	u8 	reg_data[2];

	switch (ts_data->device_id) {
	case DEVICE_LU2010:
	case DEVICE_LU3010:
		page_size = 64;
		break;
	default:
		if (printk_ratelimit()) {
			LUI_ERROR("leadingui_platform_data->device_id is invalid.\n");
			ret = -EIO;
			goto err_fw_writing;
		}
	}

	if (standalone) {
		/* Host to device communication request */
		ret = leadingui_ts_host_request_comm(ts_data->client);

		if (ret)
			return ret;
	}

	/* program mode */
	reg_data[0] = E2PROM_CTRL_PROGRAM;
	ret = touch_i2c_write(ts_data->client, E2PROM_CTRL_REG, 1, reg_data);
	if (ret) {
		LUI_ERROR("command write error\n");
		goto err_fw_writing;
	}

	reg_data[0] = 0x75;
	reg_data[1] = 0x6C;
	ret = touch_i2c_write(ts_data->client, E2PROM_LOCK_1_REG, 2, reg_data);
	if (ret) {
		LUI_ERROR("command write error\n");
		goto err_fw_writing;
	}

	write_pos   = 0;
	remain_size = ts_data->fw_info.fw_length;

	/* select erase page */
	reg_data[0] = E2PROM_CTRL_PROGRAM | E2PROM_CTRL_LOCK_SEL;
	ret = touch_i2c_write(ts_data->client, E2PROM_CTRL_REG, 1, reg_data);

	if (ret) {
		LUI_ERROR("flash command error\n");
		goto err_fw_writing;
	}

	/* set erase */
	reg_data[0] = E2PROM_ERASE_ENABLE;
	ret = touch_i2c_write(ts_data->client, E2PROM_ERASE_REG, 1, reg_data);

	if (ret) {
		LUI_ERROR("flash command error\n");
		goto err_fw_writing;
	}

	/* wait erase */
	msleep(E2PROM_ERASE_TIME);

	/* e2prom ctrl ready */
	retry = 0;

	do{
		ret = touch_i2c_read(ts_data->client, E2PROM_CTRL_REG, 1, reg_data);

		if (ret){
			LUI_ERROR("rom control register read error.\n");
			goto err_fw_writing;
		}

		/* write complete? */
		if ((reg_data[0] & E2PROM_CTRL_READY) == E2PROM_CTRL_READY){
			break;
		}

		msleep(1);
		retry++;
		LUI_ERROR("E2PROM_CTRL_REG read(retry=%d).\n", retry);

	} while (retry < 10);


	/* write firmware */
	while(remain_size > 0) {

		if (remain_size >= page_size)
			write_size = page_size;
		else
			write_size = page_size - remain_size;

		/* set write mode */
		if (ts_data->device_id == DEVICE_LU2010)
			reg_data[0] = E2PROM_CTRL_PROGRAM | E2PROM_CTRL_ENABLE | E2PROM_CTRL_TEST;
		else
			reg_data[0] = E2PROM_CTRL_PROGRAM | E2PROM_CTRL_ENABLE;

		ret = touch_i2c_write(ts_data->client, E2PROM_CTRL_REG, 1, reg_data);

		if (ret) {
			LUI_ERROR("command write error\n");
			goto err_fw_writing;
		}

		/* write firmware */
		retry = 0;
		do {
			ret = touch_i2c_write(ts_data->client,
								  E2PROM_DATA_REG + write_pos,
								  write_size,
								  ts_data->fw_info.fw_data + write_pos);
			if (ret >= 0)
				break;

			retry++;
		} while (retry < 10);

		if (retry > 10) {
			LUI_ERROR("Retry count exceed1ed\n");
			goto err_fw_writing;
		}

		/* e2prom programming */
		if (ts_data->device_id == DEVICE_LU2010)
			reg_data[0] = E2PROM_CTRL_PROGRAM | E2PROM_CTRL_TEST;
		else
			reg_data[0] = E2PROM_CTRL_PROGRAM;

		ret = touch_i2c_write(ts_data->client, E2PROM_CTRL_REG, 1, reg_data);

		if (ret) {
			LUI_ERROR("flash command error\n");
			goto err_fw_writing;
		}

		/* wait programming */
		msleep(8);

		/* check ready */
		retry = 0;

		do {
			ret = touch_i2c_read(ts_data->client, E2PROM_CTRL_REG, 1, reg_data);

			if (ret) {
				LUI_ERROR("rom control register read error.\n");
				goto err_fw_writing;
			}

			/* write complete? */
			if ((reg_data[0] & E2PROM_CTRL_READY) == E2PROM_CTRL_READY)
				break;

			msleep(1);
			retry++;
			LUI_ERROR("E2PROM_CTRL_REG read(retry=%d).\n", retry);

		} while (retry < 10);


		if (retry > 10) {
			LUI_ERROR("E2PROM_CTRL_REG Retry count exceeded.\n");
			ret = -EPERM;
			goto err_fw_writing;
		}

		write_pos   += write_size;
		remain_size -= write_size;
	}

	if (standalone) {
		/* clear access (program start) */
		ret = touch_i2c_write_byte(ts_data->client, E2PROM_CTRL_REG, 0x00);

		if (ret) {

			if ((ts_data->device_id == DEVICE_LU2010) ||
				(ts_data->device_id == DEVICE_LU3010)) {
   				LUI_TRACE("LU%04X device reset value is always false. \n", ts_data->device_id);
   				ret = 0;
   			}else{
				LUI_ERROR("exit program mode  error\n");
			}
		}
	}

err_fw_writing:

	return ret;
}

#define FLASH_CTRL_REG				0x1000
#define FLASH_OPTION_REG			0x1001
#define FLASH_LOCK_1_REG			0x1003
#define FLASH_LOCK_2_REG			0x1004
#define FLASH_SECTOR_ADDR_REG		0x1005
#define FLASH_PAGE_ADDR_REG			0x1006

#define FLASH_CTRL_PROGRAM_MODE		0x80
#define FLASH_CTRL_READY			0x40
#define FLASH_CTRL_PROGRAM_ENABLE	0x08
#define FLASH_CTRL_I2C_ERASE_ALL	0x04
#define FLASH_CTRL_I2C_ERASE		0x02
#define FLASH_CTRL_I2C_READ			0x01

#define FLASH_OPTI_SEL_AREA_N		0x00
#define FLASH_OPTI_SEL_AREA_I		0x04
#define FLASH_OPTI_ERASE_SEL_PAGE	0x00
#define FLASH_OPTI_ERASE_SEL_SECTOR	0x01
#define FLASH_OPTI_ERASE_SEL_ALL	0x01

/*
	flash Sector erase time 40ms
	flash? byte ??? ????. 35u
*/

static int leadingui_flash_write(bool standalone, struct leadingui_ts_data *ts_data)
{
	int ret = 0;
	int retry = 0;
	u8 	reg_data[2];
	int Length = (31*1024);
	int i;

	if (standalone) {
		/* Host to device communication request */
		ret = leadingui_ts_host_request_comm(ts_data->client);
		if (ret)
			return ret;
	}

	/* program mode */
	reg_data[0] = FLASH_CTRL_PROGRAM_MODE;
	ret = touch_i2c_write(ts_data->client, FLASH_CTRL_REG, 1, reg_data);
	if (ret) {
		LUI_ERROR("command write error\n");
		goto err_leadingui_fw_flash_write;
	}

	reg_data[0] = 0x75;
	reg_data[1] = 0x6C;
	ret = touch_i2c_write(ts_data->client, FLASH_LOCK_1_REG, 2, reg_data);
	if (ret) {
		LUI_ERROR("command write error\n");
		goto err_leadingui_fw_flash_write;
	}
    #if 1
	for(i=0; i<31; i++){
		/*Erase function On	*/
		reg_data[0] = 0x02;
		ret = touch_i2c_write(ts_data->client, FLASH_OPTION_REG, 1, reg_data);

		/*Page Addr	*/
		reg_data[0] = i;
		ret = touch_i2c_write(ts_data->client, FLASH_PAGE_ADDR_REG, 1, reg_data);


		/*Erase*/
		reg_data[0] = 0x82;
		ret = touch_i2c_write(ts_data->client, FLASH_CTRL_REG, 1, reg_data);

		/*Ready Check*/
		retry = 0;
		msleep(5);
		while(retry < 10) {
			ret = touch_i2c_read(ts_data->client, FLASH_CTRL_REG, 1, reg_data);

			if (ret) {
				LUI_ERROR("rom control register read error.\n");
				goto err_leadingui_fw_flash_write;
			}

			/* erase complete? */
			if ((reg_data[0] & FLASH_CTRL_READY) == FLASH_CTRL_READY){
				break;
			}

			msleep(1);
			retry++;

			if (retry > 10) {
				LUI_ERROR("FLASH_CTRL_REG Retry count exceeded.\n");
				ret = -EPERM;
				goto err_leadingui_fw_flash_write;
			}
		}
	}
    #else

	/* select all sector */
	reg_data[0] = FLASH_OPTI_ERASE_SEL_ALL;
	ret = touch_i2c_write(ts_data->client, FLASH_OPTION_REG, 1, reg_data);

	if (ret) {
		LUI_ERROR("command write error\n");
		goto err_leadingui_fw_flash_write;
	}

	/* erase all sector */
	reg_data[0] = FLASH_CTRL_PROGRAM_MODE | FLASH_CTRL_I2C_ERASE_ALL;
	ret = touch_i2c_write(ts_data->client, FLASH_CTRL_REG, 1, reg_data);

	if (ret) {
		LUI_ERROR("command write error\n");
		goto err_leadingui_fw_flash_write;
	}

	msleep(40);

	/* wait erase complete */
	retry = 0;

	while(retry < 10) {
		ret = touch_i2c_read(ts_data->client, FLASH_CTRL_REG, 1, reg_data);

		if (ret) {
			LUI_ERROR("rom control register read error.\n");
			goto err_leadingui_fw_flash_write;
		}

		/* erase complete? */
		if ((reg_data[0] & FLASH_CTRL_READY) == FLASH_CTRL_READY){
			break;
		}

		msleep(1);
		retry++;

		if (retry > 10) {
			LUI_ERROR("FLASH_CTRL_REG Retry count exceeded.\n");
			ret = -EPERM;
			goto err_leadingui_fw_flash_write;
		}
	}
    #endif

	/* write mode */
	reg_data[0] = FLASH_CTRL_PROGRAM_MODE | FLASH_CTRL_PROGRAM_ENABLE;
	ret = touch_i2c_write(ts_data->client, FLASH_CTRL_REG, 1, reg_data);

	if (ret) {
		LUI_ERROR("command write error\n");
		goto err_leadingui_fw_flash_write;
	}

	/* write firmware */
    #if 0	
	ret = touch_i2c_write(ts_data->client,
	                      E2PROM_DATA_REG,
						  ts_data->fw_info.fw_length,
						  ts_data->fw_info.fw_data);
    #endif 
	ret = touch_i2c_write(ts_data->client,
	                      E2PROM_DATA_REG,
						  Length,
						  ts_data->fw_info.fw_data);

	/* check ready */
	retry = 0;

	while(retry < 10) {
		ret = touch_i2c_read(ts_data->client, FLASH_CTRL_REG, 1, reg_data);

		if (ret){
			LUI_ERROR("rom control register read error.\n");
			goto err_leadingui_fw_flash_write;
		}

		/* write complete? */
		if ((reg_data[0] & FLASH_CTRL_READY) == FLASH_CTRL_READY)
			break;

		msleep(1);
		retry++;
		LUI_ERROR("FLASH_CTRL_REG read(retry=%d).\n", retry);

		if (retry > 10){
			LUI_ERROR("FLASH_CTRL_REG Retry count exceeded.\n");
			ret = -EPERM;
			goto err_leadingui_fw_flash_write;
		}
	}

	if (standalone) {
		/* clear access (program start) */
		ret = touch_i2c_write_byte(ts_data->client, FLASH_CTRL_REG, 0x00);

		if (ret) {
			LUI_ERROR("exit program mode  error\n");
			goto err_leadingui_fw_flash_write;
		}
	}

err_leadingui_fw_flash_write:

	return ret;
}

static int leadingui_fw_write(bool standalone, struct leadingui_ts_data *ts_data)
{
	int ret = 0;
	if ((ts_data->device_id == DEVICE_LU2010) || (ts_data->device_id == DEVICE_LU3010))
		ret = leadingui_e2prom_write(standalone, ts_data);
	else if ((ts_data->device_id == DEVICE_LU3100) || (ts_data->device_id == DEVICE_LU310A))
		ret = leadingui_flash_write(standalone, ts_data);
	else
		ret = -EPERM;

	return ret;
}

static int leadingui_fw_read(bool standalone, struct leadingui_ts_data *ts_data, u8* buff, u16 buff_length)
{
	int ret = 0;
	u8 	reg_data[2];

	if (!ts_data->fw_info.fw_data) {
		LUI_ERROR("f/w data buffer is null\n");
		ret = -EINVAL;
		goto err_fw_uploading;
	}

	if (standalone)	{
		/* Host to device communication request */
		ret = leadingui_ts_host_request_comm(ts_data->client);
		if (ret)
			return ret;

		reg_data[0] = E2PROM_CTRL_PROGRAM;
		ret = touch_i2c_write(ts_data->client, E2PROM_CTRL_REG, 1, reg_data);

		if (ret) {
			LUI_ERROR("command write error\n");
			goto err_fw_uploading;
		}

		reg_data[0] = 0x75;
		reg_data[1] = 0x6C;
		touch_i2c_write(ts_data->client, E2PROM_LOCK_1_REG, 2, reg_data);
	}

	/* read mode */
	if ((ts_data->device_id == DEVICE_LU3100) || (ts_data->device_id == DEVICE_LU310A))
		reg_data[0] = FLASH_CTRL_PROGRAM_MODE | FLASH_CTRL_I2C_READ;
	else
		reg_data[0] = E2PROM_CTRL_PROGRAM | E2PROM_CTRL_READ;

	/* set read mode */
	ret = touch_i2c_write(ts_data->client, E2PROM_CTRL_REG, 1, reg_data);

	if (ret) {
		LUI_ERROR("command write error\n");
		goto err_fw_uploading;
	}

	/* read f/w */
	ret = touch_i2c_read(ts_data->client, E2PROM_DATA_REG, buff_length, buff);

	if (ret) {
		LUI_ERROR("rom read error.\n");
		goto err_fw_uploading;
	}

	if (standalone) {
		ret = touch_i2c_write_byte(ts_data->client, E2PROM_CTRL_REG, 0x00);

		if (ret) {

			if ((ts_data->device_id == DEVICE_LU2010) ||
				(ts_data->device_id == DEVICE_LU3010)) {
   				LUI_TRACE("LU%04X device reset value is always false. \n", ts_data->device_id);
   				ret = 0;
			}else{
				LUI_ERROR("exit program mode error\n");
			}
		}
	}

err_fw_uploading:

	return ret;
}

#define LUI_FI_MAX_ROM_SIZE			32768
#define LUI_FI_FW_SIZE				32704
#define LUI_FI_CHECKSUM_SIZE		4
#define LUI_FI_VERSION_POS			8
#define LUI_FI_DEVICE_TYPE_POS		268
#define LUI_FI_FW_CHECKSUM_POS		299
#define LUI_FI_FW_DATA_POS			311
#define LUI_FI_CHECKSUM_POS			33015
#define LUI_FI_V2_FW_SIZE_POS		311
#define LUI_FI_V2_FW_DATA_POS		315

static int leadingui_fw_get_file_name(struct device *dev, const char *buf, size_t count)
{
	struct leadingui_ts_data *ts_data = dev_get_drvdata(dev);

	if (count > (MAX_PATH - 1)){
		LUI_ERROR("file name too long\n");
		return -EINVAL;
	}

	memset(ts_data->fw_info.fw_name, 0, MAX_PATH);
	memcpy(ts_data->fw_info.fw_name, buf, count);

	/* Echo into the sysfs entry may append newline at the end of buf */
	if (buf[count - 1] == '\n')
		(ts_data->fw_info.fw_name)[count - 1] = '\0';
	else
		(ts_data->fw_info.fw_name)[count] = '\0';

	LUI_INFO("f/w file name = %s", ts_data->fw_info.fw_name);

	return 0;
}

static int leadingui_fw_file_read(struct device *dev, const char *buf, size_t count)
{
	struct leadingui_ts_data *ts_data = dev_get_drvdata(dev);
	struct file *filp = NULL;

	mm_segment_t old_fs = {0,};

	int ret = 0;
	u8 *fw_buff;
	u32 checksum = 0;
	long fsize = 0;
	bool fi_ver2 = false;

	mutex_lock(&ts_data->fw_info.mutex);
	ts_data->fw_info.state   = FI_FILE_READ;
	ts_data->fw_info.results = (ts_data->fw_info.results & FI_FILE_READ_MASK) | FI_RESULT_READY;
	mutex_unlock(&ts_data->fw_info.mutex);

	/* get file name */
	ret = leadingui_fw_get_file_name(dev, buf, count);
	if (ret){
		LUI_ERROR("leadingui_fw_get_file_name error\n");
		goto err_fw_fname;
	}

	old_fs = get_fs();
	set_fs(get_ds());

	//LUI_TRACE("f/w file name = %s\n", ts_data->fw_info.fw_name);

	/* open file */
	filp = filp_open(ts_data->fw_info.fw_name, O_RDONLY, 0);
	if (IS_ERR(filp)) {
		LUI_ERROR("open error\n");
		ret = -EINVAL;
		goto err_fw_open;
	}

	/* get file size */
	fsize = filp->f_path.dentry->d_inode->i_size;

	/* allocate buffer */
	fw_buff = kzalloc(fsize, GFP_KERNEL);
	if (!fw_buff) {
		LUI_ERROR("memory allocate error.\n");
		ret = -ENOMEM;
		goto err_fw_alloc;
	}

	/* read file */
	if (fsize != vfs_read(filp, fw_buff, fsize, &filp->f_pos)) {
		LUI_ERROR("sys_read error.\n");
		ret = -EIO;
		goto err_fw_info;
	}

	/* identifier */
	if (0 != memcmp(fw_buff, "LUI-LMF1", 8)) {
		LUI_ERROR("identifier error.\n");
		ret = -EINVAL;
		goto err_fw_info;
	}

	/* version */
	ts_data->fw_info.version = ((fw_buff[9]<<8) | fw_buff[8]);
	ts_data->fw_info.fw_ver  = ((fw_buff[11]<<8) | fw_buff[10]);

	if (ts_data->fw_info.version == 0x0100)
		fi_ver2 = false;
	else if (ts_data->fw_info.version == 0x0200)
		fi_ver2 = true;
	else {
		LUI_ERROR("file version error(0x%08x).\n", ts_data->fw_info.version);
		ret = -EINVAL;
		goto err_fw_info;
	}

	LUI_INFO("file version = %d.%d\n", ts_data->fw_info.version >> 8, ts_data->fw_info.version & 0xff);
	LUI_INFO("f/w  version = %d.%d\n", ts_data->fw_info.fw_ver >> 8, ts_data->fw_info.fw_ver & 0xff);

	/* check sum */
	checksum = make_checksum(fw_buff, fsize - LUI_FI_CHECKSUM_SIZE, LUI_FI_CHECKSUM_SIZE, LITTLE_ENDIAN);
	ts_data->fw_info.checksum = (u32)(fw_buff[fsize - 1] << 24) | (u32)(fw_buff[fsize - 2] << 16) |
			                    (u32)(fw_buff[fsize - 3] << 8)  | (u32)(fw_buff[fsize - 4]);

	if (checksum != ts_data->fw_info.checksum) {
		LUI_ERROR("fw file checksum error(0x%08x = 0x%08x).\n", ts_data->fw_info.checksum, checksum);
		ret = -EINVAL;
		goto err_fw_info;
	}

	/* check device type */
	switch(ts_data->device_id)
	{
	case DEVICE_LU2010:
		if (fw_buff[LUI_FI_DEVICE_TYPE_POS] != DT_LU2010) {
			LUI_ERROR("invalid device id.\n");
			ret = -EINVAL;
			goto err_fw_info;
		}
		break;
	case DEVICE_LU3010:
		if (fw_buff[LUI_FI_DEVICE_TYPE_POS] != DT_LU3010) {
			LUI_ERROR("invalid device id.\n");
			ret = -EINVAL;
			goto err_fw_info;
		}
		break;
	case DEVICE_LU3100:
		if (fw_buff[LUI_FI_DEVICE_TYPE_POS] != DT_LU3100) {
			LUI_ERROR("invalid device id.\n");
			ret = -EINVAL;
			goto err_fw_info;
		}
		break;
	case DEVICE_LU310A:
		if (fw_buff[LUI_FI_DEVICE_TYPE_POS] != DT_LU310A) {
			LUI_ERROR("invalid device id.\n");
			ret = -EINVAL;
			goto err_fw_info;
		}
		break;
	}

	/* rom size check */
	if (ts_data->fw_info.version == 0x0100) {
		/* allocate f/w data */
		ts_data->fw_info.fw_data = kzalloc(LUI_FI_MAX_ROM_SIZE, GFP_KERNEL);
		memset(ts_data->fw_info.fw_data, 0, LUI_FI_MAX_ROM_SIZE);
		ts_data->fw_info.fw_length = LUI_FI_MAX_ROM_SIZE;

		if (!ts_data->fw_info.fw_data) {
			LUI_ERROR("f/w memory allocation error.\n");
			ret = -ENOMEM;
			goto err_fw_info;
		}
		/* copy f/w data */
		memcpy(ts_data->fw_info.fw_data, fw_buff + LUI_FI_FW_DATA_POS, LUI_FI_FW_SIZE);

	} else if (ts_data->fw_info.version == 0x0200) {
		/* allocate f/w data */
		ts_data->fw_info.fw_length = ((fw_buff[LUI_FI_V2_FW_SIZE_POS+1] << 8) | fw_buff[LUI_FI_V2_FW_SIZE_POS]);
		ts_data->fw_info.fw_data   = kzalloc(ts_data->fw_info.fw_length, GFP_KERNEL);
		memset(ts_data->fw_info.fw_data, 0, ts_data->fw_info.fw_length);

		if (!ts_data->fw_info.fw_data) {
			LUI_ERROR("f/w memory allocation error.\n");
			ret = -ENOMEM;
			goto err_fw_info;
		}
		/* check f/w size */
		if ((ts_data->fw_info.fw_length + LUI_FI_V2_FW_DATA_POS) > (fsize - LUI_FI_CHECKSUM_SIZE)) {
			LUI_ERROR("f/w memory allocation error.\n");
			ret = -EINVAL;
			goto err_fw_info;
		}
		/* copy f/w data */
		memcpy(ts_data->fw_info.fw_data, fw_buff + LUI_FI_V2_FW_DATA_POS, ts_data->fw_info.fw_length);
	}

	/* check fw checksum */
	/* check td checksum */
	/* check md checksum */

err_fw_info:
	kfree(fw_buff);

err_fw_alloc:
	filp_close(filp, NULL);

err_fw_open:
	set_fs(old_fs);

err_fw_fname:
	/* update result */
	mutex_lock(&ts_data->fw_info.mutex);

	if (ret)
		ts_data->fw_info.results = (ts_data->fw_info.results & FI_FILE_READ_MASK) | (FI_RESULT_ERROR);
	else
		ts_data->fw_info.results = (ts_data->fw_info.results & FI_FILE_READ_MASK) | (FI_RESULT_OK);

	mutex_unlock(&ts_data->fw_info.mutex);

	return ret;
}

static int leadingui_fw_update(struct leadingui_ts_data *ts_data)
{
	int ret = 0, pos = 0;
	u8  *read_buff;
	int VerifyLength=(31*1024);

	bool standalone = false; /*  means as a pair of write and read operations */

	if (!ts_data->fw_info.fw_data) {
		LUI_ERROR("f/w data buffer is null\n");
		ret = -EINVAL;
		goto err_fw_update_ready;
	}

	/* allocate read buffer */
	read_buff = kzalloc(ts_data->fw_info.fw_length, GFP_KERNEL);
	memset(read_buff, 0, ts_data->fw_info.fw_length);

	if (!read_buff) {
		LUI_ERROR("f/w data buffer is null\n");
		ret= -ENOMEM;
		goto err_read_buff;
	}

	/* set operation ready*/
	if (!standalone) {
		/* Host to device communication request */
		ret = leadingui_ts_host_request_comm(ts_data->client);

		if (ret)
			return ret;
	}

	/* write */
	mutex_lock(&ts_data->fw_info.mutex);
	ts_data->fw_info.state   = FI_FW_WRITE;
	ts_data->fw_info.results = (ts_data->fw_info.results & FI_FW_WRITE_MASK) | (FI_RESULT_READY << 4);
	mutex_unlock(&ts_data->fw_info.mutex);

	ret = leadingui_fw_write(standalone, ts_data);

	LUI_INFO("f/w write %s\n", ret < 0 ? "Fail" : "OK");

	mutex_lock(&ts_data->fw_info.mutex);
	if (ret) {
		ts_data->fw_info.results = (ts_data->fw_info.results & FI_FW_WRITE_MASK) | (FI_RESULT_ERROR << 4);
		goto err_fw_readwrite;
	}
	else
		ts_data->fw_info.results = (ts_data->fw_info.results & FI_FW_WRITE_MASK) | (FI_RESULT_OK << 4);
	mutex_unlock(&ts_data->fw_info.mutex);

	/* Delay */
	msleep(3);

	/* read */
	mutex_lock(&ts_data->fw_info.mutex);
	ts_data->fw_info.state   = FI_FW_READ;
	ts_data->fw_info.results = (ts_data->fw_info.results & FI_FW_READ_MASK) | (FI_RESULT_READY << 8);
	mutex_unlock(&ts_data->fw_info.mutex);

	ret = leadingui_fw_read(standalone, ts_data, read_buff, ts_data->fw_info.fw_length);

	LUI_INFO("f/w read %s\n", ret < 0 ? "Fail" : "OK");

	mutex_lock(&ts_data->fw_info.mutex);

	if (ret) {
		ts_data->fw_info.results = (ts_data->fw_info.results & FI_FW_READ_MASK) | (FI_RESULT_ERROR << 8);
		//goto err_fw_readwrite;
	} else {
		ts_data->fw_info.results = (ts_data->fw_info.results & FI_FW_READ_MASK) | (FI_RESULT_OK << 8);

		if (!standalone){

			ret = touch_i2c_write_byte(ts_data->client, E2PROM_CTRL_REG, 0x00);

			if (ret) {
				if ((ts_data->device_id == DEVICE_LU2010) || (ts_data->device_id == DEVICE_LU3010)) {
	   				LUI_TRACE("LU%04X device reset value is always false. \n", ts_data->device_id);
	   				ret = 0;
				}else{
					ts_data->fw_info.results = (ts_data->fw_info.results & FI_FW_READ_MASK) | (FI_RESULT_ERROR << 8);
					LUI_ERROR("exit program mode error.\n");
				}
			}
		}
	}
	mutex_unlock(&ts_data->fw_info.mutex);

	/* verify */
	if (ret == 0) {
		mutex_lock(&ts_data->fw_info.mutex);
		ts_data->fw_info.state   = FI_FW_VERIFY;
		ts_data->fw_info.results = (ts_data->fw_info.results & FI_FW_VERIFY_MASK);
		mutex_unlock(&ts_data->fw_info.mutex);

		/*ret = memcmp(ts_data->fw_info.fw_data, read_buff, ts_data->fw_info.fw_length);*/
		ret = memcmp(ts_data->fw_info.fw_data, read_buff, VerifyLength);
		
		LUI_INFO("f/w verify %s\n", ret == 0 ? "OK" : "Fail");

		if (ret != 0) {
			LUI_INFO("-------------f/w veryfy error : error data dump-------------\n");
			for (pos = 0; pos < ts_data->fw_info.fw_length; pos++){
				if (ts_data->fw_info.fw_data[pos] != read_buff[pos]){
					LUI_ERROR("%d, 0x%02x, 0x%02x\n", pos, ts_data->fw_info.fw_data[pos], read_buff[pos]);
				}
			}
		}

		mutex_lock(&ts_data->fw_info.mutex);
		if (ret == 0)
			ts_data->fw_info.results = (ts_data->fw_info.results & FI_FW_VERIFY_MASK) | (FI_RESULT_OK << 12);
		else
			ts_data->fw_info.results = (ts_data->fw_info.results & FI_FW_VERIFY_MASK) | (FI_RESULT_ERROR << 12);
		mutex_unlock(&ts_data->fw_info.mutex);
	}

err_fw_readwrite:

	if(ts_data->fw_info.fw_data != NULL) {
		kfree(ts_data->fw_info.fw_data);
		ts_data->fw_info.fw_data = NULL;
	}

	ts_data->fw_info.fw_length = 0;

err_read_buff:
	kfree(read_buff);

err_fw_update_ready:

	return ret;
}

static ssize_t leadingui_fw_version_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct leadingui_ts_data *ts_data = dev_get_drvdata(dev);
	return scnprintf(buf, PAGE_SIZE, "firmware version = %u.%u\n", ts_data->fw_version >> 8, ts_data->fw_version & 0xff);
}

static ssize_t leadingui_fw_upgrade_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct leadingui_ts_data *ts_data = dev_get_drvdata(dev);
	int ret = 0;
	u16 fw_version, device_id;
	//u8 model[16];
	u32 fw_chksum;
	u16 param_chksum;	

	/* check f/w update state */
	if (ts_data->fw_info.is_updating) {
		LUI_ERROR("update running.\n");
		return ret;
	}

	/* check device power state */
	if (ts_data->power_state == POWER_OFF) {
		LUI_ERROR("device power off\n");
		return ret;
	}

	if (ts_data->power_state == POWER_SUSPEND){
		LUI_ERROR("device suspended\n");
		return ret;
	}

	if (atomic_read(&ts_data->device_init) != 1){
		LUI_ERROR("device does not initialize\n");
		return ret;
	}

	mutex_lock(&ts_data->fw_info.mutex);
	ts_data->fw_info.is_updating = true;
	ts_data->fw_info.state       = FI_IDLE;
	ts_data->fw_info.results  	 = 0x0000;
	mutex_unlock(&ts_data->fw_info.mutex);

    #if (LUI_WATCHDOG_TIMER)
	leadingui_wd_timer_stop(ts_data);
    #endif

	mutex_lock(&g_ts_mutex);

	leadingui_ts_irq_enable(ts_data, false);
	release_all_ts_event(ts_data);

	/* read firmware from file*/
	ret = leadingui_fw_file_read(dev, buf, count);
	if (ret)
		goto err_file_read;

	/* write firmware */
	ret = leadingui_fw_update(ts_data);
	if (ret)
		goto err_fw_update;

	/* device reset */
	ret = leadingui_ts_reset(ts_data);
	if (ret)
		goto err_fw_update;

	/* check chip id */
	ret = leadingui_ts_check_chip_id(ts_data);
	if (ret)
		goto err_fw_update;
		
	/* read device information */
	ret = leadingui_ts_simple_info(ts_data, &device_id, &fw_version, &fw_chksum, &param_chksum);
	if (ret)
		goto err_fw_update;

	if (ts_data->pdata->allow_device_id != device_id) {
		LUI_ERROR("invalid device id.\n");
		ret = -EINVAL;
		goto err_fw_update;
	}

	ts_data->device_id = device_id;
	ts_data->fw_version = fw_version;
	ts_data->fw_chksum= fw_chksum;
	ts_data->param_chksum= param_chksum;

err_file_read:
err_fw_update:

	/* set interrupt mode */
	MTK_EINT_PIN_MODE (MODE_EINT, DIR_IN);

	/* interrupt enable */
	leadingui_ts_irq_enable(ts_data, true);

	atomic_set(&ts_data->device_init, 1);

	mutex_lock(&ts_data->fw_info.mutex);
	ts_data->fw_info.is_updating = false;
	mutex_unlock(&ts_data->fw_info.mutex);


	mutex_unlock(&g_ts_mutex);

    #if (LUI_WATCHDOG_TIMER)
	leadingui_wd_timer_start(ts_data);
    #endif

	if (ret)
		return ret;

	/* must return value is count. */
	return count;
}

static ssize_t leadingui_fw_upgrade_state_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct leadingui_ts_data *ts_data = dev_get_drvdata(dev);

	return scnprintf(buf, PAGE_SIZE, "state: %d %d %d\n",ts_data->fw_info.is_updating,
														ts_data->fw_info.state,
														ts_data->fw_info.results );
}

static ssize_t leadingui_power_state_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct leadingui_ts_data *ts_data = dev_get_drvdata(dev);
	int ret = 0;

	switch(ts_data->power_state) {
	case POWER_OFF:		ret = scnprintf(buf, PAGE_SIZE, "POWER OFF\n"); break;
	case POWER_ON:		ret = scnprintf(buf, PAGE_SIZE, "POWER ON\n"); break;
	case POWER_SUSPEND:	ret = scnprintf(buf, PAGE_SIZE, "POWER SUSPEND\n"); break;
	case POWER_WAKE:	ret = scnprintf(buf, PAGE_SIZE, "POWER WAKE\n"); break;
	default:			ret = scnprintf(buf, PAGE_SIZE, "Unknown Power State\n"); break;
	}

	return ret;
}

static ssize_t leadingui_device_info_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct leadingui_ts_data *ts_data = dev_get_drvdata(dev);
	int ret = 0;
	u8 model[16];
	u16 fw_version = 0, device_id = 0;

	/* check device power state */
	if (ts_data->power_state == POWER_OFF) {
		LUI_ERROR("device power off\n");
		return 0;
	}

	if (ts_data->power_state == POWER_SUSPEND){
		LUI_ERROR("device suspended\n");
		return 0;
	}

	if (atomic_read(&ts_data->device_init) != 1) {
		LUI_ERROR("device does not initialized.\n");
		return 0;
	}

    #if (LUI_WATCHDOG_TIMER)
	leadingui_wd_timer_stop(ts_data);
    #endif

	mutex_lock(&g_ts_mutex);

	/* disable irq */
	leadingui_ts_irq_enable(ts_data, false);

	mutex_unlock(&g_ts_mutex);

	/* read ts information */
	ret = leadingui_ts_model_info(ts_data, model, &device_id, &fw_version);

	if (ret) {
		LUI_ERROR("request device information fail.\n");
	}

	/* set interrupt mode */
	MTK_EINT_PIN_MODE (MODE_EINT, DIR_IN);

	/* enable irq */
	leadingui_ts_irq_enable(ts_data, true);

	atomic_set(&ts_data->device_init, 1);

    #if (LUI_WATCHDOG_TIMER)
	leadingui_wd_timer_start(ts_data);
    #endif

	if (ret) {
		return 0;
	}

	return scnprintf(buf,
	                 PAGE_SIZE,
	                 "Phys:%s, Driver:v%s, Device:0x%04X, F/W:v%u.%u, Model:%s, \n", ts_data->phys,
																				     DRIVER_VERSION,
																				     device_id,
																				     fw_version >> 8,
																				     fw_version & 0xff,
																				     model);
}

#define LUI_FW_FILENAME "/mnt/sdcard/MFP_ROM.mfp"

//#define LUI_FW_FILENAME "/mnt/shell/emulated/0/MFPROM.mfp"

static ssize_t leadingui_fw_upgrade_typeB_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct leadingui_ts_data *ts_data = dev_get_drvdata(dev);
	int ret = 0;
	int count;

	u16 fw_version, device_id;
	u8 model[16];
	char fw_path[256];	
	
	/*make file name*/
	snprintf(fw_path, 255, "%s", LUI_FW_FILENAME);
	count = strlen(fw_path);

	LUI_INFO("============>leadingui_fw_upgrade_typeB_show\n");
	
	/* check f/w update state */
	if (ts_data->fw_info.is_updating) {
		LUI_ERROR("update running.\n");
		return scnprintf(buf, PAGE_SIZE, "update running. %d \n", count );
	}

	/* check device power state */
	if (ts_data->power_state == POWER_OFF) {
		LUI_ERROR("device power off\n");
		return scnprintf(buf, PAGE_SIZE, "device power off. %d \n", count );
	}

	if (ts_data->power_state == POWER_SUSPEND){
		LUI_ERROR("device suspended\n");
		return scnprintf(buf, PAGE_SIZE, "device suspended. %d \n", count );
	}

	if (atomic_read(&ts_data->device_init) != 1){
		LUI_ERROR("device does not initialize\n");
		return scnprintf(buf, PAGE_SIZE, "device does not initialize. %d \n", count );
	}

	mutex_lock(&ts_data->fw_info.mutex);
	ts_data->fw_info.is_updating = true;
	ts_data->fw_info.state       = FI_IDLE;
	ts_data->fw_info.results  	 = 0x0000;
	mutex_unlock(&ts_data->fw_info.mutex);

    #if (LUI_WATCHDOG_TIMER)
	leadingui_wd_timer_stop(ts_data);
    #endif
	
	mutex_lock(&ts_data->input_dev->mutex);
	
	leadingui_ts_irq_enable(ts_data, false);
	release_all_ts_event(ts_data);

	//mutex_unlock(&ts_data->input_dev->mutex);

	/* read firmware from file*/
	ret = leadingui_fw_file_read(dev, fw_path, count);
	if (ret){
		count = scnprintf(buf, PAGE_SIZE, "leadingui_fw_file_read error. %d \n", ret );	
		goto err_file_read;
	}
	/* write firmware */
	ret = leadingui_fw_update(ts_data);
	if (ret){
		count = scnprintf(buf, PAGE_SIZE, "leadingui_fw_update error. %d \n", ret );
		goto err_fw_update;	
		}
	
	/* device reset */
	ret = leadingui_ts_reset(ts_data);
	if (ret){
		count = scnprintf(buf, PAGE_SIZE, "leadingui_ts_reset error. %d \n", ret );
		goto err_fw_update;
		}
    #if 0
	/* check chip id */
	ret = leadingui_ts_check_chip_id(ts_data, &device_id);
	if (ret)
		goto err_fw_update;

	ret = leadingui_ts_simple_info(ts_data, &device_id, &fw_version);
    #else
	ret = leadingui_ts_model_info(ts_data, model, &device_id, &fw_version);
    #endif

	if (ret){
		count = scnprintf(buf, PAGE_SIZE, "leadingui_ts_model_info error. %d \n", ret );	
		goto err_fw_update;
		}
	
	if (ts_data->pdata->allow_device_id != device_id) {
		LUI_ERROR("invalid device id.\n");
		ret = -EINVAL;
		count = scnprintf(buf, PAGE_SIZE, "invalid device id. error. %d \n", ret );
		goto err_fw_update;
	}

	ts_data->device_id = device_id;
	ts_data->fw_version = fw_version;

err_file_read:
err_fw_update:

	
	/* set interrupt mode */
	MTK_EINT_PIN_MODE (MODE_EINT, DIR_IN);
	
	/* enable irq */
	leadingui_ts_irq_enable(ts_data, true);
	atomic_set(&ts_data->device_init, 1);

	mutex_lock(&ts_data->fw_info.mutex);
	ts_data->fw_info.is_updating = false;
	mutex_unlock(&ts_data->fw_info.mutex);

	mutex_unlock(&ts_data->input_dev->mutex);

    #if (LUI_WATCHDOG_TIMER)
	leadingui_wd_timer_start(ts_data);
    #endif

	if (ret)
		return count;

	/* must return value is count. */
	
	return scnprintf(buf, PAGE_SIZE, "Upgrade Done! \n");
}
#if LUI_GESTURE_EN
/*
static int _is_open_gesture_mode = 0;
static ssize_t lu31xx_gesture_show(struct device *dev,struct device_attribute *attr,char *buf)
{
    ssize_t num_read_chars = 0;

    num_read_chars = snprintf(buf, PAGE_SIZE, "%d\n", _is_open_gesture_mode);

    return num_read_chars;
}

static ssize_t lu31xx_gesture_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
    if(count == 0)
        return 0;

	disable_irq_nosync(touch_irq);

    if(buf[0] == '1' && _is_open_gesture_mode == 0){
        _is_open_gesture_mode = 1;
    }
    if(buf[0] == '0' && _is_open_gesture_mode == 1){
        _is_open_gesture_mode = 0;
    }

	enable_irq(touch_irq);
    return count;
}

static ssize_t lui_coordinate_x_show(struct device *dev,struct device_attribute *attr,char *buf)
{
    ssize_t num_read_chars = 0, count =0;
    int i;
    for (i=0; i< point_num; i++)
    {
        count = sprintf(buf, "%d ", lu_coordinate_x[i]);
        buf += count;
        num_read_chars += count;
    }

    return num_read_chars;
}

static ssize_t lui_coordinate_y_show(struct device *dev,struct device_attribute *attr,char *buf)
{
    ssize_t num_read_chars = 0, count = 0;
    int i;
    for (i=0; i< point_num; i++)
    {
        count = sprintf(buf, "%d ", lu_coordinate_y[i]);
        buf += count;
        num_read_chars += count;
    }

    return num_read_chars;
}

*/

static ssize_t show_tpgesture_value(struct device* dev, struct device_attribute *attr, char *buf)
{
	printk("show tp gesture value is %s \n",tpgesture_value);
	return scnprintf(buf, PAGE_SIZE, "%s\n", tpgesture_value);
}
static ssize_t show_tpgesture_status_value(struct device* dev, struct device_attribute *attr, char *buf)
{
	printk("show tp gesture status is %s \n",tpgesture_status_value);
	return scnprintf(buf, PAGE_SIZE, "%s\n", tpgesture_status_value);
}
static ssize_t store_tpgesture_status_value(struct device* dev, struct device_attribute *attr, const char *buf, size_t count)
{
	if(!strncmp(buf, "on", 2))
	{
		sprintf(tpgesture_status_value,"on");
		gesture_wakeup_switch = 1;//status --- on
	}
	else
	{
		sprintf(tpgesture_status_value,"off");
		gesture_wakeup_switch = 0;//status --- off
	}
	printk("store_tpgesture_status_value status is %s \n",tpgesture_status_value);
	return count;
}
static DEVICE_ATTR(tpgesture,  0664, show_tpgesture_value, NULL);
static DEVICE_ATTR(tpgesture_status,  0664, show_tpgesture_status_value, store_tpgesture_status_value);

static struct device_attribute *tpd_attr_list[] = {
	&dev_attr_tpgesture,
	&dev_attr_tpgesture_status,
};
#endif


static DEVICE_ATTR(fw_version, 0664, leadingui_fw_version_show, NULL);
static DEVICE_ATTR(fw_upgrade, 0664, NULL, leadingui_fw_upgrade_store);
static DEVICE_ATTR(fw_upgrade_state, 0664, leadingui_fw_upgrade_state_show, NULL);
static DEVICE_ATTR(power_state, 0664, leadingui_power_state_show, NULL);
static DEVICE_ATTR(device_info, 0664, leadingui_device_info_show, NULL);
static DEVICE_ATTR(fw_upgrade_typeB, S_IRUGO, leadingui_fw_upgrade_typeB_show, NULL);
#if LUI_GESTURE_EN
/*
static DEVICE_ATTR(gesture, 0664, lu31xx_gesture_show, lu31xx_gesture_store);
static DEVICE_ATTR(lu_coordinate_x, 0664, lui_coordinate_x_show, NULL);
static DEVICE_ATTR(lu_coordinate_y, 0664, lui_coordinate_y_show, NULL);
*/
#endif

static struct attribute *leadingui_attrs[] = {
	&dev_attr_fw_version.attr,
	&dev_attr_fw_upgrade.attr,
	&dev_attr_fw_upgrade_state.attr,
	&dev_attr_power_state.attr,
	&dev_attr_device_info.attr,
	&dev_attr_fw_upgrade_typeB.attr,
    #if LUI_GESTURE_EN
/*
    &dev_attr_gesture.attr,
    &dev_attr_lu_coordinate_x.attr,
    &dev_attr_lu_coordinate_y.attr,
*/
	
	#endif		
	NULL,
};

static const struct attribute_group leadingui_attr_group = {
	.attrs = leadingui_attrs,
};


/* release_all_ts_event
 *
 * When system enters suspend-state,
 * if user press touch-panel, release them automatically.
 */
static void release_all_ts_event(struct leadingui_ts_data *ts_data)
{
    #ifdef TYPE_B_PROTOCOL
	int id = 0;

	for (id = 0; id < LUI_MAX_FINGER; id++) {
		input_mt_slot(ts_data->input_dev, id);
		input_mt_report_slot_state(ts_data->input_dev, MT_TOOL_FINGER, 0);

		ts_data->cur_tdata[id].state = 0;
	}
    #endif
	input_report_key(ts_data->input_dev, BTN_TOUCH, 0);
	input_report_key(ts_data->input_dev, BTN_TOOL_FINGER, 0);

    #ifndef TYPE_B_PROTOCOL
	input_mt_sync(ts_data->input_dev);
    #endif

	if (!ts_data->cur_tkey.code) {
		if (ts_data->cur_tkey.state == LUI_KEY_PRESS) {
			ts_data->cur_tkey.state = LUI_KEY_RELEASE;

			input_report_key(ts_data->input_dev,
			                 ts_data->pdata->key_map[ts_data->cur_tkey.code-1],
			                 0);

			/* clear key state */
			ts_data->cur_tkey.code  = 0;
			ts_data->cur_tkey.state = -1;
		}
	}

	input_sync(ts_data->input_dev);

    #if (LUI_WATCHDOG_TIMER)
	/* clear report flag */
	ts_data->touch_reported = false;
    #endif
}


static void leadingui_ts_input_report(struct leadingui_ts_data *ts_data, const ktime_t timestamp)
{
	int id;
	int t_count = 0;
	int release_points = 0;
	int total_points = 0;

	for(id = 0; id < LUI_MAX_FINGER; id++) {

		if (!ts_data->cur_tdata[id].state)
			continue;
			
        total_points++;
        
		LUI_TOUCH_DEBUG("touch-(%d) x=%d, y=%d, p=%d, m=%d, s=%d\n",
		                id,
		                ts_data->cur_tdata[id].x,
		                ts_data->cur_tdata[id].y,
		                ts_data->cur_tdata[id].pressure,
		                ts_data->cur_tdata[id].width_major,
		                ts_data->cur_tdata[id].state);
        #ifdef TYPE_B_PROTOCOL
        input_mt_slot(ts_data->input_dev, id);
        input_mt_report_slot_state(ts_data->input_dev,
                                   MT_TOOL_FINGER,
                                   ts_data->cur_tdata[id].state != LUI_TOUCH_RELEASE);
        #endif

		/* LUI_TOUCH_PRESS or LUI_TOUCH_MOVE */
		if (ts_data->cur_tdata[id].state != LUI_TOUCH_RELEASE) {

			input_report_key(ts_data->input_dev, BTN_TOUCH, 1);
			input_report_key(ts_data->input_dev, BTN_TOOL_FINGER, 1);

            #ifndef TYPE_B_PROTOCOL
			input_report_abs(ts_data->input_dev, ABS_MT_TRACKING_ID, id);
            #endif
			input_report_abs(ts_data->input_dev, ABS_MT_POSITION_X,  ts_data->cur_tdata[id].x);
			input_report_abs(ts_data->input_dev, ABS_MT_POSITION_Y,  ts_data->cur_tdata[id].y);
			input_report_abs(ts_data->input_dev, ABS_MT_TOUCH_MAJOR, ts_data->cur_tdata[id].width_major);
			input_report_abs(ts_data->input_dev, ABS_MT_PRESSURE,    ts_data->cur_tdata[id].pressure);

            #ifndef TYPE_B_PROTOCOL
			input_mt_sync(ts_data->input_dev);
            #endif
			t_count++;

            #ifdef TPD_HAVE_BUTTON
			if (NORMAL_BOOT != boot_mode)
			{
				tpd_button( ts_data->cur_tdata[id].x,
							ts_data->cur_tdata[id].y, 1);
			}
            #endif
		} else {

            #ifndef TYPE_B_PROTOCOL    //this code cause the bigest ID flying to (0,0),Phil20161107
			//input_report_abs(ts_data->input_dev, ABS_MT_TRACKING_ID, -1);  
            #endif                               
            release_points++;           
			ts_data->cur_tdata[id].state = 0;

            #ifdef TPD_HAVE_BUTTON
			if (NORMAL_BOOT != boot_mode)
			{
				tpd_button( ts_data->cur_tdata[id].x,
							ts_data->cur_tdata[id].y, 0);
			}
            #endif
		}
	}

	if (ts_data->cur_tkey.code) {
		if (ts_data->cur_tkey.state == LUI_KEY_PRESS) {
			input_report_key(ts_data->input_dev,
			                 ts_data->pdata->key_map[ts_data->cur_tkey.code-1],
			                 1);
			LUI_TOUCH_DEBUG("report key press");
		} else {
			input_report_key(ts_data->input_dev,
			                 ts_data->pdata->key_map[ts_data->cur_tkey.code-1],
			                 0);
			LUI_TOUCH_DEBUG("report key release");
			/* clear key state */
			ts_data->cur_tkey.code  = 0;
			ts_data->cur_tkey.state = -1;
		}
	}
	
    if(total_points != 0)
    {
    	if(release_points == total_points)
    	{
    		input_report_key(ts_data->input_dev, BTN_TOUCH, 0);
    		input_report_key(ts_data->input_dev, BTN_TOOL_FINGER, 0);
		    #ifndef TYPE_B_PROTOCOL
    		input_mt_sync(ts_data->input_dev);
		    #endif
    	}
	}

	if (t_count == 0) {
		input_report_key(ts_data->input_dev, BTN_TOUCH, 0);
		input_report_key(ts_data->input_dev, BTN_TOOL_FINGER, 0);
        #ifndef TYPE_B_PROTOCOL
		input_mt_sync(ts_data->input_dev);
        #endif
	}

    #ifdef TYPE_B_PROTOCOL    	
	input_mt_report_pointer_emulation(ts_data->input_dev, true);
    #endif
	input_sync(ts_data->input_dev);

    #if (LUI_WATCHDOG_TIMER)
	ts_data->touch_reported = true;
    #endif
}

static int leadingui_ts_get_lu2_data(struct leadingui_ts_data *ts_data)
{
	int ret = 0, i = 0;
	int finger_dsize = 0;

	/* device init */
	if (atomic_read(&ts_data->device_init) != 1) {
		LUI_ERROR("device not initialize\n");
		return -EINTR;
	}

	if (ts_data->device_id != DEVICE_LU2010) {
		LUI_ERROR("device not supported\n");
		return -EPERM;
	}

	ret = touch_i2c_read(ts_data->client,
	                     DEVICE_EVENT_REG,
	                     LU2_EVENT_LENGTH,
						 (u8*)(&g_fdata.lu2_tevent));

	if (ret) {
		LUI_ERROR("finger event read error\n");
		return -EIO;
	}

	if (g_fdata.lu2_tevent.status != 0) {
		LUI_ERROR("unknown firmware state- 0x%02x\n", g_fdata.lu2_tevent.status);
		return -EIO;
	}

	/* event fail check */
	if (0==(g_fdata.lu2_tevent.event & DEVICE_EVENT_FAIL_MASK)) {
		LUI_ERROR("event fail mask(0x%02x), event data: 0x%02x\n", DEVICE_EVENT_FAIL_MASK, g_fdata.lu2_tevent.event);
		return -EIO;
	}

	/* check max fingers */
	if (g_fdata.lu2_tevent.tcount > LUI_MAX_FINGER) {
		LUI_ERROR("event max finger error : (0x%02x)\n", g_fdata.lu2_tevent.tcount);
		return -EIO;
	}

	/* read data */
	if ((g_fdata.lu2_tevent.event & LUI_EVENT_ABSOLUTE) == LUI_EVENT_ABSOLUTE) {

		if (g_fdata.lu2_tevent.tcount <= 0) {
			LUI_ERROR("error: touch count is zero\n");
			return -EPERM;
		}

		finger_dsize = 4;
		finger_dsize *= g_fdata.lu2_tevent.tcount;

		ret = touch_i2c_read(ts_data->client,
		                     LU2010_EVENT_DATA_REG,
							 finger_dsize, (u8*)g_fdata.lu2_tdata);

		if (ret) {
			LUI_ERROR("finger data read error\n");
			return -EIO;
		}

		/* clear status */
		for (i=0; i<g_fdata.lu2_tevent.tcount; i++)
			ts_data->cur_tdata[g_fdata.lu2_tdata[i].id].state = 0;

		for (i=0; i<g_fdata.lu2_tevent.tcount; i++) {
			ts_data->cur_tdata[g_fdata.lu2_tdata[i].id].state = g_fdata.lu2_tdata[i].status;
			ts_data->cur_tdata[g_fdata.lu2_tdata[i].id].x     = g_fdata.lu2_tdata[i].x;
			ts_data->cur_tdata[g_fdata.lu2_tdata[i].id].y     = g_fdata.lu2_tdata[i].y;

			/* below variables is not support in lu2010 */
			ts_data->cur_tdata[g_fdata.lu2_tdata[i].id].width_major = 1;
			ts_data->cur_tdata[g_fdata.lu2_tdata[i].id].width_minor = 1;
			ts_data->cur_tdata[g_fdata.lu2_tdata[i].id].pressure    = 1;
		}

        #if LEADINGUI_TRACE_TOUCH
		for (i=0; i<g_fdata.lu2_tevent.tcount; i++) {
			LUI_TOUCH_DEBUG("touch-(%d) x=%d, y=%d status=%d \n",
			          g_fdata.lu2_tdata[i].id,
			          g_fdata.lu2_tdata[i].x,
			          g_fdata.lu2_tdata[i].y,
			          g_fdata.lu2_tdata[i].status);
		}
        #endif
	}

	if ((g_fdata.lu2_tevent.event & LUI_EVENT_KEY) == LUI_EVENT_KEY) {

		if (g_fdata.lu2_tevent.key) {
			if (g_fdata.lu2_tevent.key <= ts_data->pdata->max_keys) {
				ts_data->cur_tkey.code  = g_fdata.lu2_tevent.key;
				ts_data->cur_tkey.state = LUI_KEY_PRESS;

				LUI_TRACE("key-%d is pressed \n", g_fdata.lu2_tevent.key);
			} else {
				LUI_ERROR("invalid key-%d is pressed\n", g_fdata.lu2_tevent.key);
			}
		} else {
			/* last key code check */
			if (ts_data->cur_tkey.code) {
				ts_data->cur_tkey.state = LUI_KEY_RELEASE;
                #if LEADINGUI_TRACE_TOUCH
				LUI_TRACE("key-%d is release \n", ts_data->cur_tkey.code);
                #endif
			} else {
				LUI_ERROR("key-%d is not pressed. release not processing.\n", g_fdata.lu2_tevent.key);
			}
		}
	}

	/* set done */
	ret = touch_i2c_done(ts_data->client);

	return ret;
}


#if LUI_GESTURE_EN

extern struct input_dev *kpd_input_dev;

static void lu2x3x_tpgesture_hander(void){
	input_report_key(kpd_input_dev, KEY_PROG3, 1);	
	input_sync(kpd_input_dev);							
	input_report_key(kpd_input_dev, KEY_PROG3, 0);	
	input_sync(kpd_input_dev);
}

static void hct_check_gesture(int gesture_id)
{	
    printk("hct_check_gesture gesture_id==0x%x\n ",gesture_id);
    
	switch(gesture_id)
	{
		case GESTURE_KNOCK:
    		strcpy(tpgesture_value,"DOUBCLICK");
    		lu2x3x_tpgesture_hander();
			break;
/*
		case GESTURE_UP:
    		strcpy(tpgesture_value,"UP");
    		lu2x3x_tpgesture_hander();
			break;

		case GESTURE_DOWN:
			strcpy(tpgesture_value,"DOWN");
			lu2x3x_tpgesture_hander();
			break;

		case GESTURE_LEFT:
			strcpy(tpgesture_value,"LEFT");
			lu2x3x_tpgesture_hander();
			break;

		case GESTURE_RIGHT:
			strcpy(tpgesture_value,"RIGHT");
			lu2x3x_tpgesture_hander();
			break;
*/
		case GESTURE_C:
			strcpy(tpgesture_value,"c");
			lu2x3x_tpgesture_hander();
			break;
/*
		case GESTURE_O:
			strcpy(tpgesture_value,"o");
			lu2x3x_tpgesture_hander();
			break;
*/
		case GESTURE_W:
			strcpy(tpgesture_value,"w");
			lu2x3x_tpgesture_hander();
			break;
/*
		case GESTURE_E:
			strcpy(tpgesture_value,"e");
			lu2x3x_tpgesture_hander();
			break;
*/
		case GESTURE_V:
			strcpy(tpgesture_value,"v");
			lu2x3x_tpgesture_hander();
			break;

		case GESTURE_M:
			strcpy(tpgesture_value,"m");
			lu2x3x_tpgesture_hander();
			break;

		case GESTURE_Z:
			strcpy(tpgesture_value,"z");
			lu2x3x_tpgesture_hander();
			break;
/*
		case GESTURE_S:
			strcpy(tpgesture_value,"s");
			lu2x3x_tpgesture_hander();
			break;
*/
		default:		
			break;
	}
	printk("hct_check_gesture tpgesture_value==%s\n ",tpgesture_value);
}
/*
static void lui_check_gesture(struct input_dev *input_dev,int gesture_id)
{
	printk("lui_gesture_id==0x%x\n ",gesture_id);

	switch(gesture_id)
    {
        case GESTURE_KNOCK:
			input_report_key(tpd->dev, KEY_POWER, 1);
			input_sync(tpd->dev);
			input_report_key(tpd->dev, KEY_POWER, 0);
			input_sync(tpd->dev);
  	        msleep(100);
            input_report_key(input_dev, KEY_U, 1);
            input_sync(input_dev);
            input_report_key(input_dev, KEY_U, 0);
            input_sync(input_dev);
            break;
        case GESTURE_C:
			input_report_key(tpd->dev, KEY_POWER, 1);
			input_sync(tpd->dev);
			input_report_key(tpd->dev, KEY_POWER, 0);
			input_sync(tpd->dev);
  	        msleep(100);			
            input_report_key(input_dev, KEY_GESTURE_C, 1);
            input_sync(input_dev);
            input_report_key(input_dev, KEY_GESTURE_C, 0);
            input_sync(input_dev);
            break;
        case GESTURE_W:
			input_report_key(tpd->dev, KEY_POWER, 1);
			input_sync(tpd->dev);
			input_report_key(tpd->dev, KEY_POWER, 0);
			input_sync(tpd->dev);
  	        msleep(100);			
            input_report_key(input_dev, KEY_GESTURE_W, 1);
            input_sync(input_dev);
            input_report_key(input_dev, KEY_GESTURE_W, 0);
            input_sync(input_dev);
            break;
        case GESTURE_M:
			input_report_key(tpd->dev, KEY_POWER, 1);
			input_sync(tpd->dev);
			input_report_key(tpd->dev, KEY_POWER, 0);
			input_sync(tpd->dev);
  	        msleep(100);		
            input_report_key(input_dev, KEY_GESTURE_M, 1);
            input_sync(input_dev);
            input_report_key(input_dev, KEY_GESTURE_M, 0);
            input_sync(input_dev);
            break;
        case GESTURE_V:
			input_report_key(tpd->dev, KEY_POWER, 1);
			input_sync(tpd->dev);
			input_report_key(tpd->dev, KEY_POWER, 0);
			input_sync(tpd->dev);
  	        msleep(100);			
            input_report_key(input_dev, KEY_GESTURE_V, 1);
            input_sync(input_dev);
            input_report_key(input_dev, KEY_GESTURE_V, 0);
            input_sync(input_dev);
            break;
        case GESTURE_Z:
			input_report_key(tpd->dev, KEY_POWER, 1);
			input_sync(tpd->dev);
			input_report_key(tpd->dev, KEY_POWER, 0);
			input_sync(tpd->dev);
  	        msleep(100);			
            input_report_key(input_dev, KEY_GESTURE_Z, 1);
            input_sync(input_dev);
            input_report_key(input_dev, KEY_GESTURE_Z, 0);
            input_sync(input_dev);
            break;
        default:
            break;
    }

}
*/
#endif

#if LUI_GESTURE_EN
static int gesture_flag = 0;
#endif

static int leadingui_ts_get_lu3_data(struct leadingui_ts_data *ts_data)
{
	int ret = 0, i = 0;
	int finger_dsize = 0;

    #if LUI_GESTURE_EN
	int gesture_id;
//	unsigned char buf[255 * 3] = { 0 };
    #endif

	//printk("leadingui_ts_get_lu3_data init------\n");
	/* device init */
	if (atomic_read(&ts_data->device_init) != 1) {
		LUI_ERROR("device not initialize\n");
		return -EINTR;
	}

	if (ts_data->device_id == DEVICE_LU2010) {
		LUI_ERROR("device not supported\n");
		return -EPERM;
	}

	ret = touch_i2c_read(ts_data->client,
						 DEVICE_EVENT_REG,
		                 LU3_EVENT_LENGTH,
						 (u8*)(&g_fdata.lu3_tevent));

	printk("leadingui-----state: 0x%02x,event data: 0x%02x,tcount: 0x%02x,key: 0x%02x\n",g_fdata.lu3_tevent.status,g_fdata.lu3_tevent.event,g_fdata.lu3_tevent.tcount,g_fdata.lu3_tevent.key);
	if (ret) {
		LUI_ERROR("finger event read error\n");
		return -EIO;
	}

	/* check event status */
	if (g_fdata.lu3_tevent.status != 0) {
		LUI_ERROR("unknown firmware state- 0x%02x\n", g_fdata.lu3_tevent.status);
		return -EIO;
	}

	/* event */
	if (0==(g_fdata.lu3_tevent.event & DEVICE_EVENT_FAIL_MASK)) {
		LUI_ERROR("event fail mask(0x%02x), event data: 0x%02x\n", DEVICE_EVENT_FAIL_MASK, g_fdata.lu3_tevent.event);
		return -EIO;
	}

	/* check max fingers */
    #if LUI_GESTURE_EN	
	if(0 == gesture_wakeup_mode)
	{
    #endif
    	if (g_fdata.lu3_tevent.tcount > LUI_MAX_FINGER) {
    		LUI_ERROR("event max finger error : (0x%02x)\n", g_fdata.lu3_tevent.tcount);
    		return -EIO;
    	}
    #if LUI_GESTURE_EN	
	}
    #endif

	/* read event data */
	if ((g_fdata.lu3_tevent.event & LUI_EVENT_ABSOLUTE) == LUI_EVENT_ABSOLUTE) {

		if (g_fdata.lu3_tevent.tcount <= 0) {
			//LUI_ERROR("error: touch count is zero\n");
			return -EPERM;
		}

		finger_dsize = 4;

		if ((g_fdata.lu3_tevent.event & LUI_EVENT_PRESSURE) == LUI_EVENT_PRESSURE) {

			if (ts_data->pdata->support_pressure == 0) {
				LUI_ERROR("error: platform is is not support pressure data.\n");
				return -EPERM;
			}
			finger_dsize += 2;
		}

		finger_dsize *= g_fdata.lu3_tevent.tcount;

		/* read finger data */
		ret = touch_i2c_read(ts_data->client,
							 DEVICE_EVENT_DATA_REG,
							 finger_dsize, (u8*)g_fdata.lu3_tdata);
		if (ret) {
			LUI_ERROR("finger data read error\n");
			return -EIO;
		}

		/* clear event status */
		for(i=0; i<g_fdata.lu3_tevent.tcount; i++)
			ts_data->cur_tdata[g_fdata.lu3_tdata[i].id].state = 0;

		for(i=0; i<g_fdata.lu3_tevent.tcount; i++) {
			ts_data->cur_tdata[g_fdata.lu3_tdata[i].id].state = g_fdata.lu3_tdata[i].status;
			ts_data->cur_tdata[g_fdata.lu3_tdata[i].id].x = g_fdata.lu3_tdata[i].x;
			ts_data->cur_tdata[g_fdata.lu3_tdata[i].id].y = g_fdata.lu3_tdata[i].y;
			ts_data->cur_tdata[g_fdata.lu3_tdata[i].id].width_major = g_fdata.lu3_tdata[i].pressure.count;
			//ts_data->cur_tdata[g_fdata.lu3_tdata[i].id].width_minor = g_fdata.lu3_tdata[i].pressure;
			ts_data->cur_tdata[g_fdata.lu3_tdata[i].id].pressure = g_fdata.lu3_tdata[i].pressure.value;    			
		}

        #if (LEADINGUI_TRACE_TOUCH)
		for(i = 0; i < g_fdata.lu3_tevent.tcount; i++) {
			LUI_TOUCH_DEBUG("touch-(%d) x=%d, y=%d status=%d pressure(%d,%d)\n",
				          g_fdata.lu3_tdata[i].id,
				          g_fdata.lu3_tdata[i].x,
				          g_fdata.lu3_tdata[i].y,
				          g_fdata.lu3_tdata[i].status,
				          g_fdata.lu3_tdata[i].pressure.value,
				          g_fdata.lu3_tdata[i].pressure.count);
		}
        #endif
	}

	if ((g_fdata.lu3_tevent.event & LUI_EVENT_KEY) == LUI_EVENT_KEY) {

		if (g_fdata.lu3_tevent.key) {
			if (g_fdata.lu3_tevent.key <= ts_data->pdata->max_keys) {
				ts_data->cur_tkey.code  = g_fdata.lu3_tevent.key;
				ts_data->cur_tkey.state = LUI_KEY_PRESS;
                #if LEADINGUI_TRACE_TOUCH
				LUI_TOUCH_DEBUG("key-%d is pressed \n", g_fdata.lu3_tevent.key);
                #endif
			} else {
				LUI_ERROR("invalid key-%d is pressed\n", g_fdata.lu3_tevent.key);
			}
		} else {
			/* last key code check */
			if (ts_data->cur_tkey.code){
				ts_data->cur_tkey.state = LUI_KEY_RELEASE;
                #if LEADINGUI_TRACE_TOUCH
				LUI_TOUCH_DEBUG("key-%d is release \n", ts_data->cur_tkey.code);
                #endif
			} else {
				LUI_ERROR("key-%d is not pressed. release not processing.\n", g_fdata.lu3_tevent.key);
			}
		}
	}
    #if LUI_GESTURE_EN
	if ((g_fdata.lu3_tevent.event & LUI_EVENT_GESTURE) == LUI_EVENT_GESTURE) {
		printk("leadingui_ts_get_lu3_data  check_gesture------\n");
		gesture_id = g_fdata.lu3_tevent.key;
		hct_check_gesture(g_fdata.lu3_tevent.key);
//		lui_check_gesture(ts_data->input_dev,g_fdata.lu3_tevent.key);
/*
// tp 手势轨迹， 没有用到，注释
		if (g_fdata.lu3_tevent.tcount <= 0) {
			//LUI_ERROR("error: touch count is zero\n");
			return -EPERM;
		}
		point_num = g_fdata.lu3_tevent.tcount;
		ret = touch_i2c_read(ts_data->client,
							 DEVICE_DATA_REG,
							 point_num, buf);
		for(i = 0; i < point_num; i++)
		{
			lu_coordinate_x[i] = (((s16)buf[0+(4*i)])&0x0F)<<8|(((s16)buf[1+(4*i)])&0xFF);
			lu_coordinate_y[i] = (((s16)buf[2+(4*i)])&0x0F)<<8|(((s16)buf[3+(4*i)])&0xFF);
		}
*/
		gesture_flag = 1;
	}
    #endif

	/* set done */
	ret = touch_i2c_done(ts_data->client);

	return ret;
}

#if LUI_GESTURE_EN
/*
int lui_gesture_init(struct input_dev *input_dev)
{
	input_set_capability(input_dev, EV_KEY, KEY_POWER);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_KNOCK); 
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_C);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_M); 
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_W);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_V);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_Z);
		
	__set_bit(KEY_GESTURE_KNOCK, input_dev->keybit);
	__set_bit(KEY_GESTURE_C, input_dev->keybit);
	__set_bit(KEY_GESTURE_M, input_dev->keybit);
	__set_bit(KEY_GESTURE_W, input_dev->keybit);
	__set_bit(KEY_GESTURE_V, input_dev->keybit);
	__set_bit(KEY_GESTURE_Z, input_dev->keybit);

	return 0;
}
*/
#endif

static int leadingui_ts_get_data(struct leadingui_ts_data *ts_data)
{
	if (ts_data->device_id == DEVICE_LU2010)
		return leadingui_ts_get_lu2_data(ts_data);
	else
		return leadingui_ts_get_lu3_data(ts_data);
}

static int leadingui_ts_work_pre_proc(struct leadingui_ts_data *ts_data)
{
	int ret;

    #if 0    //test int status, marked by Phil
	//KGB
    //if (mt_get_gpio_in(GPIO_CTP_EINT_PIN)) {
	if (gpio_get_value(ts_data->pdata->irq_gpio)) {

		LUI_WARNING("irq_gpio is High.\n");
		return -EIO;
	}
    #endif
	/* read data */
	ret = leadingui_ts_get_data(ts_data);

	if (ret) {
		/* set done */
		touch_i2c_done(ts_data->client);
	}

	return ret;
}


/* workq_ts_init
 *
 * In order to reduce the booting-time,
 * we used delayed_work_queue instead of msleep or mdelay.
 */
static void workq_ts_init(struct work_struct *work_init)
{
	int ret = 0;
	struct leadingui_ts_data *ts_data =	container_of(to_delayed_work(work_init),
													struct leadingui_ts_data, work_init);
	mutex_lock(&g_ts_mutex);

    #if (LUI_WATCHDOG_TIMER)
	leadingui_wd_timer_stop(ts_data);
    #endif
	mutex_unlock(&g_ts_mutex);

    /* clear host access */
    touch_i2c_done(ts_data->client);

	/* init */
	ret = leadingui_ts_dev_irq_init(ts_data);

	if (ret){

		/* error: start recovery */
		if (unlikely(ts_data->recovery_err_cnt >= DEVIE_MAX_RECOVERY_COUNT)) {
			ts_data->recovery_err_cnt = 0;
			LUI_ERROR("Device initialize failed: has some unknown problems\n");
		}else {
			ts_data->recovery_err_cnt++;
			LUI_ERROR("recovery: count=%d\n", ts_data->recovery_err_cnt);
			queue_delayed_work(leadingui_wq, &ts_data->work_init, msecs_to_jiffies(10));
		}
	} else {

		if (ts_data->power_state != POWER_WAKE)
			ts_data->power_state = POWER_WAKE;

		ts_data->recovery_err_cnt = 0;
	}

    #if (LUI_WATCHDOG_TIMER)
	if (ret >= 0)
		leadingui_wd_timer_start(ts_data);
    #endif

}

/* workq_ts_recover
 *
 * In order to reduce the booting-time,
 * we used delayed_work_queue instead of msleep or mdelay.
 */
static void workq_ts_recover(struct work_struct *work_recover)
{
	int ret = 0;
	struct leadingui_ts_data *ts_data =	container_of(work_recover,
											struct leadingui_ts_data, work_recover);
	mutex_lock(&g_ts_mutex);

    #if (LUI_WATCHDOG_TIMER)
	leadingui_wd_timer_stop(ts_data);
    #endif

	leadingui_ts_irq_enable(ts_data, false);

	release_all_ts_event(ts_data);

	mutex_unlock(&g_ts_mutex);

	atomic_set(&ts_data->device_init, 0);

	ret = leadingui_ts_dev_irq_init(ts_data);

	if (ret) {

		/* error: start recovery */
		if (unlikely(ts_data->recovery_err_cnt >= DEVIE_MAX_RECOVERY_COUNT)) {
			ts_data->recovery_err_cnt = 0;
			LUI_ERROR("Device initialize failed: has some unknown problems\n");
		}else{
			ts_data->recovery_err_cnt++;
			LUI_ERROR("recovery: count=%d\n", ts_data->recovery_err_cnt);
			queue_delayed_work(leadingui_wq, &ts_data->work_init, msecs_to_jiffies(10));
		}

	} else {
		ts_data->recovery_err_cnt = 0;
	}

    #if (LUI_WATCHDOG_TIMER)
	if (ret >= 0)
		leadingui_wd_timer_start(ts_data);
    #endif

}

#if BUILT_IN_UPGRADE

/*
 * built-in f/w check
 */

static int leadingui_fw_data_check( struct leadingui_ts_data *ts_data,
									const u8 *fw_data,
									u32 fw_data_length )
{
	int ret = 0;
	u32 cs_code = 0, cs_calc = 0;
	u16 fi_ver, fw_ver;
	bool fi_ver2 = false;

	/* identifier */
	if (0 != memcmp(fw_data, "LUI-LMF1", 8)){
		LUI_ERROR("identifier error.\n");
		ret = -EINVAL;
		goto err_fw_code_info;
	}
    #if 0           //Phil marked 20161105
	if(ts_data->urgent_upgrade){
		ret = leadingui_ts_check_chip_id(ts_data);
		if (ret){
			LUI_INFO("Need Urgent Upgrade. But I2C Communication Fail\n , ret");
			ret = -EINVAL;
			goto err_fw_code_info;
		}		
		LUI_INFO("Need Urgent Upgrade.\n , ret");
		
	}else{
		//Old & New checksum verification.
		if(ts_data->fw_chksum == 0x00000000){
			LUI_ERROR("Currently Engineering Mode\n");
			ret = -EPERM;
		}
		
		if(ts_data->fw_chksum == LeadingUI_fw_chksum &&
			ts_data->param_chksum == LeadingUI_param_chksum){
			LUI_ERROR("checksum data is same.(0x%X, 0x%X) (0x%X, 0x%X)\n", 
					ts_data->fw_chksum, LeadingUI_fw_chksum, 
					ts_data->param_chksum, LeadingUI_param_chksum);
			ret = -EPERM;
		}

	}
	#endif
	/* version */
	fi_ver = (u16)((fw_data[9]<<8) | fw_data[8]);
	fw_ver = (u16)((fw_data[11]<<8) | fw_data[10]);

	if (ts_data->fw_version >= fw_ver){
		LUI_INFO("device f/w version is same or high. f/w upgrade is not executed.\n");
		//ret = -EPERM;
	}else{
		LUI_ERROR("device f/w = %d, rom code f/w = %d \n", ts_data->fw_version, fw_ver);
	}

	if (fi_ver == 0x0100)
		fi_ver2 = false;
	else if (fi_ver == 0x0200)
		fi_ver2 = true;
	else{
		LUI_ERROR("file version error(0x%08x).\n", ts_data->fw_info.version);
		ret = -EINVAL;
		goto err_fw_code_info;
	}

	/* check device type */
	switch(ts_data->device_id)
	{
	case DEVICE_LU2010:
		if (fw_data[LUI_FI_DEVICE_TYPE_POS] != DT_LU2010){
			LUI_ERROR("invalid device id.\n");
			ret = -EINVAL;
			goto err_fw_code_info;
		}
		break;
	case DEVICE_LU3010:
		if (fw_data[LUI_FI_DEVICE_TYPE_POS] != DT_LU3010){
			LUI_ERROR("invalid device id.\n");
			ret = -EINVAL;
			goto err_fw_code_info;
		}
		break;
	case DEVICE_LU3100:
		if (fw_data[LUI_FI_DEVICE_TYPE_POS] != DT_LU3100){
			LUI_ERROR("invalid device id.\n");
			ret = -EINVAL;
			goto err_fw_code_info;
		}
		break;
	case DEVICE_LU310A:
		if (fw_data[LUI_FI_DEVICE_TYPE_POS] != DT_LU310A){
			LUI_ERROR("invalid device id.\n");
			ret = -EINVAL;
			goto err_fw_code_info;
		}
		break;
	default:
		LUI_ERROR("unknown device id.\n");
		ret = -EINVAL;
		goto err_fw_code_info;
		break;
	}

	/* check sum */
	cs_calc = make_checksum(fw_data, fw_data_length - LUI_FI_CHECKSUM_SIZE, LUI_FI_CHECKSUM_SIZE, LITTLE_ENDIAN);
	cs_code = (u32)(fw_data[fw_data_length - 1] << 24) | (u32)(fw_data[fw_data_length - 2] << 16) |
              (u32)(fw_data[fw_data_length - 3] << 8)  | (u32)(fw_data[fw_data_length - 4]);

	if (cs_code != cs_calc){
		LUI_ERROR("f/w code checksum error(0x%08x = 0x%08x).\n", cs_calc, cs_code);
		ret = -EINVAL;
		goto err_fw_code_info;
	}

	/* update f/w size */
	if (fi_ver2)
		ts_data->fw_info.fw_length = ((fw_data[LUI_FI_V2_FW_SIZE_POS+1] << 8) | fw_data[LUI_FI_V2_FW_SIZE_POS]);
	else
		ts_data->fw_info.fw_length = LUI_FI_FW_SIZE;

	ts_data->fw_info.version  = fi_ver;
	ts_data->fw_info.checksum = cs_code;
	ts_data->fw_info.fw_ver   = fw_ver;

err_fw_code_info:

	return ret;
}

/*
 * read firmware data from code
*/
static int leadingui_fw_data_read(struct leadingui_ts_data *ts_data )
{
	int ret = 0;

	mutex_lock(&ts_data->fw_info.mutex);
	ts_data->fw_info.state   = FI_CODE_READ;
	ts_data->fw_info.results = (ts_data->fw_info.results & FI_CODE_READ_MASK) | FI_RESULT_READY;
	mutex_unlock(&ts_data->fw_info.mutex);

	switch(ts_data->device_id)
	{
	case DEVICE_LU2010:
	case DEVICE_LU3010:
	case DEVICE_LU3100:
	case DEVICE_LU310A:
		break;
	default:
		LUI_ERROR("unknown device id.\n");
		ret = -EINVAL;
		goto err_fw_device_id;
	}

	LUI_TRACE("f/w file name = %s", ts_data->fw_info.fw_name);

	/* allocate f/w data */
	ts_data->fw_info.fw_data = kzalloc(ts_data->fw_info.fw_length, GFP_KERNEL);

	if (!ts_data->fw_info.fw_data){
		LUI_ERROR("f/w memory allocation error.\n");
		ret = -ENOMEM;
		goto err_fw_data;
	}

	memset(ts_data->fw_info.fw_data, 0, ts_data->fw_info.fw_length);

	if (ts_data->fw_info.version == 0x0100) {
		memcpy(ts_data->fw_info.fw_data, &LeadingUI_fw_data[LUI_FI_FW_DATA_POS], ts_data->fw_info.fw_length);
	}else if (ts_data->fw_info.version == 0x0200) {
		memcpy(ts_data->fw_info.fw_data, &LeadingUI_fw_data[LUI_FI_V2_FW_DATA_POS], ts_data->fw_info.fw_length);
	}

	/* check fw checksum */
	/* check td checksum */
	/* check md checksum */

err_fw_device_id:
err_fw_data:

	/* update result */
	mutex_lock(&ts_data->fw_info.mutex);
	if (ret)
		ts_data->fw_info.results = (ts_data->fw_info.results & FI_FILE_READ_MASK) | (FI_RESULT_ERROR);
	else
		ts_data->fw_info.results = (ts_data->fw_info.results & FI_FILE_READ_MASK) | (FI_RESULT_OK);

	mutex_unlock(&ts_data->fw_info.mutex);

	return ret;
}

static void workq_ts_fw_upgrade(struct work_struct *work_fw_upgrade)
{
	struct leadingui_ts_data *ts_data =
			container_of(work_fw_upgrade, struct leadingui_ts_data, work_fw_upgrade);
	int ret = 0;
	u8 model[16];
	u16 fw_version, device_id;

	/* check f/w update state */
	if (ts_data->fw_info.is_updating) {
		LUI_ERROR("update running.\n");
		goto err_wq_device_state;
	}

	mutex_lock(&ts_data->fw_info.mutex);
	ts_data->fw_info.is_updating = true;
	ts_data->fw_info.state       = FI_IDLE;
	ts_data->fw_info.results  	 = 0x0000;
	mutex_unlock(&ts_data->fw_info.mutex);

	/* check device state */
	if (ts_data->power_state == POWER_OFF) {
		LUI_ERROR("device power off or suspended\n");
		goto err_wq_device_state;
	}

	if (ts_data->power_state == POWER_SUSPEND) {
		LUI_ERROR("device suspended\n");
		goto err_wq_device_state;
	}

	if (atomic_read(&ts_data->device_init) != 1){
		LUI_ERROR("device does not initialize\n");
		goto err_wq_device_state;
	}

    #if (LUI_WATCHDOG_TIMER)
	leadingui_wd_timer_stop(ts_data);
    #endif

	/* disable irq */
	leadingui_ts_irq_enable(ts_data, false);
	release_all_ts_event(ts_data);

	LUI_INFO("Upgrade : FW Data Check\n");
	/* built-in firmware Check */
	if (leadingui_fw_data_check(ts_data,
	                            LeadingUI_fw_data,
	                            sizeof(LeadingUI_fw_data)) < 0)
		goto err_wq_fw_data_check;

	LUI_INFO("Upgrade : FW Data Read\n");
	/* read firmware from header file*/
	if (leadingui_fw_data_read(ts_data) < 0)
		goto err_wq_fw_data_read;

	LUI_INFO("Upgrade : Do Update\n");
	/* write firmware */
	if (leadingui_fw_update(ts_data) < 0)
		goto err_wq_fw_update;

	/* device reset */
	if (leadingui_ts_reset(ts_data) < 0)
		goto err_wq_fw_update;

	LUI_INFO("Upgrade : Get Chip ID\n");
	/* check chip id */
	if (leadingui_ts_check_chip_id(ts_data) < 0)
		goto err_wq_fw_update;

	LUI_INFO("Upgrade : Get Model Info\n");
	/* ts info */
	if (leadingui_ts_model_info(ts_data, model, &device_id, &fw_version) < 0)
		goto err_wq_fw_update;

err_wq_fw_data_read:
err_wq_fw_data_check:

	/* set interrupt mode */
	MTK_EINT_PIN_MODE (MODE_EINT, DIR_IN);

	/* enable irq */
	leadingui_ts_irq_enable(ts_data, true);

	atomic_set(&ts_data->device_init, 1);

	LUI_INFO("Upgrade : Built In Upgrade Done\n");

    #if (LUI_WATCHDOG_TIMER)
	leadingui_wd_timer_start(ts_data);
    #endif

err_wq_fw_update:
err_wq_device_state:

	mutex_lock(&ts_data->fw_info.mutex);

	if (ts_data->fw_info.fw_data != NULL){
		kfree(ts_data->fw_info.fw_data);
		ts_data->fw_info.fw_data = NULL;
	}

	ts_data->fw_info.fw_length = 0;
	ts_data->fw_info.is_updating = false;

	mutex_unlock(&ts_data->fw_info.mutex);

}

#endif

static int leadingui_ts_dev_irq_init (struct leadingui_ts_data *ts_data)
{
	int ret = 0;
	//int retry_cnt = 0;
	u16 fw_version, device_id;
	u32 fw_chksum;
	u16 param_chksum;

	ts_data->urgent_upgrade = 0;

	memset(&ts_data->cur_tdata, 0, (sizeof(struct touch_data) * LUI_MAX_FINGER));
	memset(&ts_data->cur_tkey, 0, sizeof(struct key_data));

	if (unlikely(ts_data->recovery_err_cnt > DEVIE_MAX_RECOVERY_COUNT)) {
		LUI_ERROR("Device initialize failed: has some unknown problems\n");
		goto err_ts_init;
	}

	/* reset & get device id*/
	ret = leadingui_ts_reset(ts_data);

	if (ret)
		goto err_ts_init;

	/* check chip id */
	ret = leadingui_ts_check_chip_id(ts_data);

	if (ret)
		goto err_ts_init;

	/* read device information */
	ret = leadingui_ts_simple_info(ts_data, &device_id, &fw_version, &fw_chksum, &param_chksum);

	if (ret){
		LUI_ERROR("device info check error.\n");
		goto err_ts_init;
	}
	LUI_INFO("ts_data->pdata->allow_device_id=0x%x,device_id=0x%x\n",ts_data->pdata->allow_device_id,device_id);
    //device_id=0x310a;
	if (ts_data->pdata->allow_device_id != device_id){
		LUI_ERROR("Can't supported device.\n");
		ret = -EPERM;
		ts_data->urgent_upgrade=1;
		goto err_ts_init;
	}

	/* store device id and f/w version */
	ts_data->device_id = device_id;
	ts_data->fw_version = fw_version;
	ts_data->fw_chksum= fw_chksum;
	ts_data->param_chksum= param_chksum;

	/* set interrupt mode */
	//MTK_EINT_PIN_MODE (MODE_EINT, DIR_IN);

	/* enable irq */
	//leadingui_ts_irq_enable(ts_data, true);

err_ts_init:

	if (ret)
		atomic_set(&ts_data->device_init, 0);
	else
		atomic_set(&ts_data->device_init, 1);

	return ret;
}

static int leadingui_init_input_device(struct leadingui_ts_data *ts_data)
{
    #if 1
	ts_data->input_dev = tpd->dev;


	i2c_set_clientdata(ts_data->client, g_ts_data);

    #ifdef TYPE_B_PROTOCOL
    #if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 8, 0))
    input_mt_init_slots(input_dev, LUI_MAX_FINGER);    /* initialize MT input slots */
    #else
    input_mt_init_slots(input_dev, LUI_MAX_FINGER, INPUT_MT_DIRECT|INPUT_MT_DROP_UNUSED); /* initialize MT input slots */
    #endif
    #endif

	#if LUI_GESTURE_EN
//	lui_gesture_init(ts_data->input_dev);
	#endif


	
	
	return 0;
    #else
	int ret = 0, i = 0;
	struct input_dev *input_dev;

	/* input device setting */
	input_dev = input_allocate_device();

	if (!input_dev)
	{
		LUI_ERROR("Failed to allocate input device\n");
		return -ENOMEM;
	}

	/* Initialize Platform data */
	input_dev->name		  = LEADINGUI_TS_NAME;
	//input_dev->phys	  = ts_data->phys;
	input_dev->dev.parent = &ts_data->client->dev;
	input_dev->id.bustype = BUS_I2C;

	#if LUI_GESTURE_EN
	lui_gesture_init(input_dev);
	#endif		

	set_bit(EV_ABS, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(EV_SYN, input_dev->evbit);

	set_bit(BTN_TOUCH, input_dev->keybit);
	set_bit(BTN_TOOL_FINGER, input_dev->keybit);

    //#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0))
    #ifdef INPUT_PROP_DIRECT
	set_bit(INPUT_PROP_DIRECT, input_dev->propbit);
    #endif

	/* For multi touch */
	set_bit(MT_TOOL_FINGER, input_dev->keybit);

    #ifdef TYPE_B_PROTOCOL
    
    #if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 8, 0))
	ret = input_mt_init_slots (input_dev, LUI_MAX_FINGER);    /* initialize MT input slots */
    #else
	ret = input_mt_init_slots (input_dev, LUI_MAX_FINGER, 2); /* initialize MT input slots */
    #endif
	if (ret) {
		LUI_ERROR("Error initializing slots\n");
		goto err_init_input_device;
	}
    #endif

	input_set_abs_params(input_dev, ABS_MT_POSITION_X,  0, ts_data->pdata->max_x-1, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y,  0, ts_data->pdata->max_y-1, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_PRESSURE,    0, 255, 0, 0);
	//input_set_abs_params(input_dev, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
    #ifndef TYPE_B_PROTOCOL
	input_set_abs_params(input_dev, ABS_MT_TRACKING_ID, 0, LUI_MAX_FINGER, 0, 0);
    #endif

	/* set key array supported key */
	if (ts_data->pdata->key_map) {
		for (i = 0; i < ts_data->pdata->max_keys; i++) {
			if (ts_data->pdata->key_map[i]) {
				input_set_capability(input_dev, EV_KEY, ts_data->pdata->key_map[i]);
			}
		}
	}

	/* mark device as capable of a certain event */
	input_set_drvdata(input_dev, ts_data);

	i2c_set_clientdata(ts_data->client, g_ts_data);

	/* register input device */
	ret = input_register_device(input_dev);
	if (ret) {
		LUI_ERROR("Unable to register %s input device\n", input_dev->name);
		goto err_init_input_device;
	}

	ts_data->input_dev = input_dev;

	return 0;

err_init_input_device:

	input_free_device(input_dev);
	return ret;
    #endif
}

#if (PLATFORMDATA_DEVICETREE)
/*
 * Parsing device tree
 */
static int leadingui_parse_devicetree(struct device *dev, struct leadingui_platform_data *pdata)
{
	int ret = -1;

	LUI_ERROR("DeviceTree : Not yet supported\n");

	return ret;
}

#endif
/*
static void dump_platform_data(struct leadingui_platform_data *pdata)
{
	int i;

	LUI_INFO("--------------Dump platform data ----------------\n");
	LUI_INFO(".reset_gpio       = %d \n",   pdata->reset_gpio);
	LUI_INFO(".irq_gpio         = %d \n",   pdata->irq_gpio);
	LUI_INFO(".support device   = 0x%04X \n", pdata->allow_device_id);
	LUI_INFO(".max_x            = %d \n", pdata->max_x);
	LUI_INFO(".max_y            = %d \n", pdata->max_y);
	LUI_INFO(".max_keys         = %d \n", pdata->max_keys);
	for(i=0; i<pdata->max_keys; i++)
		LUI_TRACE(".key_map[%d]       = 0x%04X \n", i, pdata->key_map[i]);

	LUI_INFO(".support_pressure = %d \n", pdata->support_pressure);
	LUI_INFO(".max_pressure     = %d \n", pdata->max_pressure);
	LUI_INFO(".max_touch_major  = %d \n", pdata->max_touch_major);
	LUI_INFO(".max_touch_minor  = %d \n", pdata->max_touch_minor);

	LUI_INFO(".use_regulator    = %d \n", pdata->use_regulator);
	LUI_INFO(".use_reset_gpio   = %d \n", pdata->use_reset_gpio);
	LUI_INFO(".support_suspend  = %d \n", pdata->support_suspend);

	LUI_INFO("------------------------------------------------\n");
}
*/
static int leadingui_ts_get_platform_data(struct device_node *np, struct leadingui_ts_data *ts_data)
{
	int ret = 0;

    #if (PLATFORMDATA_DEVICETREE)
	ret = leadingui_parse_devicetree(&ts_data->client->dev, ts_data->pdata);
    #else
	/* Use global definition of platform data*/
	ts_data->pdata = &leadingui_touch_pdata;
    #endif

    #if 0  // rm by dengzy
	ts_data->pdata->reset_gpio = GPIO_TPD_RESET;//of_get_named_gpio(np, "reset-gpio", 0);
	LUI_INFO("ts_data->pdata->reset_gpio = %d\n", ts_data->pdata->reset_gpio);
	if (ts_data->pdata->reset_gpio < 0) {
		LUI_ERROR("%s: of_get_named_gpio failed: tsp_gpio %d\n", __func__, ts_data->pdata->reset_gpio);
		return -EINVAL;
	}

	ts_data->pdata->irq_gpio = GPIO_TPD_EINT;//of_get_named_gpio(np, "eint-gpio", 0);
	LUI_INFO("ts_data->pdata->irq_gpio = %d\n", ts_data->pdata->irq_gpio);
	if (ts_data->pdata->irq_gpio < 0) {
		LUI_ERROR("%s: of_get_named_gpio failed: tsp_gpio %d\n", __func__, ts_data->pdata->irq_gpio);
		return -EINVAL;
	}
	if (!ts_data->pdata || ret) {
		ret = -ENOMEM;
		LUI_ERROR("platform_data is null\n");
	}
	else
		dump_platform_data(ts_data->pdata);
    #endif 
	return ret;
}


static int leadingui_regulator_config(struct leadingui_ts_data *ts_data, bool power_on)
{
	int ret = 0;

	if (power_on) {

		if (ts_data->pdata->use_regulator){
            #ifdef CONFIG_TPD_POWER_SOURCE_VIA_VGP
	        ret = regulator_set_voltage(tpd->reg, 2800000, 2800000);	/* set 2.8v */
	        if (ret)
		        printk("regulator_set_voltage() failed!\n");
	        ret = regulator_enable(tpd->reg);	/* enable regulator */
	        if (ret)
		        printk("regulator_enable() failed!\n");

            /* Vanzo:yuntaohe on: Mon, 11 Jan 2016 14:30:15 +0800
             * TODO: replace this line with your comment
            #else
            	        hwPowerOn(MT65XX_POWER_LDO_VGP2, VOL_2800, "TP");
            */
            // End of Vanzo:yuntaohe
            #endif

            #ifdef VANZO_TPD_POWER_SOURCE_VIA_EXT_LDO
            tpd_ldo_power_enable(1);
            #endif
			//msleep(POWER_ON_DELAY);
		}
	} else {

		if (ts_data->pdata->use_regulator){
            /* Power off TP */
            #ifdef CONFIG_TPD_POWER_SOURCE_VIA_VGP
	        ret = regulator_disable(tpd->reg);
	        if (ret)
		        printk("regulator_disable() failed!\n");
            /* Vanzo:yuntaohe on: Mon, 11 Jan 2016 14:30:31 +0800
            * TODO: replace this line with your comment
            #else
            hwPowerDown(MT65XX_POWER_LDO_VGP2, "TP");
            */
            // End of Vanzo:yuntaohe
            #endif

            #ifdef VANZO_TPD_POWER_SOURCE_VIA_EXT_LDO
            tpd_ldo_power_enable(0);
            #endif
		}
	}

	//LUI_TRACE("Regulator -%s", (power_on) ? "on" : "off");

	return ret;
}


static int leadingui_ts_power_ctrl(struct leadingui_ts_data *ts_data, int power_mode)
{
	int ret = 0;

	if (ts_data == NULL) {
		LUI_ERROR("ts_data is null.\n");
		return -EINVAL;
	}

	if (!ts_data->pdata->support_suspend) {
		LUI_ERROR("Suspend Mode: Not Supported\n");
		return ret;
	}

	switch(power_mode) {
	case POWER_ON:
		{
			if (ts_data->pdata->use_regulator) {
				ret = leadingui_regulator_config(ts_data, POWER_ON);

				if (ret)
					break;
			}

			ts_data->power_state = POWER_ON;
		}
		break;
	case POWER_OFF:
	case POWER_SUSPEND:
		{
			//ret = leadingui_ts_suspend(&ts_data->client->dev);
			ret = leadingui_ts_suspend(NULL);

			if (ret) {
				if (power_mode == POWER_OFF) {
					LUI_ERROR("power off failed\n");
				}else {
					LUI_ERROR("power suspend failed\n");
				}
			}
		}
		break;
	case POWER_WAKE:
		{
			//ret = leadingui_ts_resume(&ts_data->client->dev);
			ret = leadingui_ts_resume(NULL);

			if (ret) {
				LUI_ERROR("power wake failed\n");
			}
			else
				ts_data->power_state = POWER_WAKE;
		}
		break;
	default:
		ret = -EIO;
		break;
	}

    #if (LEADINGUI_TRACE_POWER)
	if (ret >= 0) {
		switch(ts_data->power_state){
		case POWER_OFF:     LUI_TRACE_POWER("[leadingui] POWER OFF\n");     break;
		case POWER_ON:      LUI_TRACE_POWER("[leadingui] POWER ON\n");      break;
		case POWER_WAKE:    LUI_TRACE_POWER("[leadingui] POWER WAKE\n");    break;
		case POWER_SUSPEND: LUI_TRACE_POWER("[leadingui] POWER SUSPEND\n"); break;
		default:            LUI_TRACE_POWER("[leadingui] Unknown Power State\n"); break;
		}
	}
    #endif
	return ret;
}


static void tpd_eint_handler(void)
{
	TPD_DEBUG_PRINT_INT;
	disable_irq_nosync(touch_irq);
	tpd_flag = 1;
	wake_up_interruptible(&waiter);
}

static int touch_event_handler(void *data)
{
	struct leadingui_ts_data *ts_data = data;
	struct sched_param param = { .sched_priority = RTPM_PRIO_TPD };

	int ret = -1;

	ktime_t timestamp = ktime_get();

	sched_setscheduler(current, SCHED_RR, &param);

	printk("touch_event_interrupt happened! \n");
	do {
		set_current_state(TASK_INTERRUPTIBLE);

		while (tpd_halt) {
			tpd_flag = 0;
			msleep(20);
		}

		wait_event_interruptible(waiter, tpd_flag != 0);
		tpd_flag = 0;
		TPD_DEBUG_SET_TIME;
		set_current_state(TASK_RUNNING);

		mutex_lock(&g_ts_mutex);

		if (atomic_read(&ts_data->device_init) == 1) {
			//mutex_lock(&g_ts_mutex);

            #if (LUI_WATCHDOG_TIMER)
	 		leadingui_wd_timer_stop(ts_data);
            #endif
			ret = leadingui_ts_work_pre_proc(ts_data);

			if (ret == 0) {
				leadingui_ts_input_report(ts_data, timestamp);
			}

            #if (LUI_WATCHDOG_TIMER)
			leadingui_wd_timer_start(ts_data);
            #endif
            #if LUI_GESTURE_EN
            if(gesture_flag)
            {
           	    gesture_flag = 0;
             	LUI_INFO("leadingui_ts_gesture_int \n");
             	//continue;
            }	
            #endif
            //mutex_unlock(&g_ts_mutex);

		} else {
			LUI_ERROR("device does not initialize\n");
		}

		enable_irq(touch_irq);

		mutex_unlock(&g_ts_mutex);

	}while(!kthread_should_stop());

	return 0;
}

static int leadingui_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;
	struct device_node *node;
	u32 ints[2] = {0,0};
	LUI_INFO("**************************************************\n");
	LUI_INFO("                  LeadingUI -v%s                  \n", DRIVER_VERSION);
	LUI_INFO("**************************************************\n");

	leadingui_wq = create_singlethread_workqueue("leadingui_wq");
	if (!leadingui_wq) {
		LUI_ERROR("create_singlethread_workqueue error\n");
		return -ENOMEM;
	}

	/* Plain i2c-level commands */
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		LUI_ERROR("i2c functionality check error\n");
		return -EPERM;
	}

	/* create driver data structure */
	g_ts_data = kzalloc(sizeof(struct leadingui_ts_data), GFP_KERNEL);

	if (!g_ts_data) {
		LUI_ERROR("Can not allocate memory\n");
		goto err_kzalloc_failed;
	}

	/* init driver data */
	g_ts_data->client              = client;
	g_ts_data->power_state         = POWER_OFF;
	g_ts_data->recovery_err_cnt    = 0;
	g_ts_data->fw_info.is_updating = false;
	g_ts_data->irq_enabled		   = false;

	snprintf(g_ts_data->phys, sizeof(g_ts_data->phys),
	         "i2c-%u-%04x/input0", client->adapter->nr, client->addr);

	LUI_INFO("g_ts_data->phys = %s", g_ts_data->phys);

	mutex_init(&g_ts_data->fw_info.mutex);

	/* get platform data */
	ret = leadingui_ts_get_platform_data(client->dev.of_node,g_ts_data);

	if (ret)
		goto err_get_platform_data_failed;

	ret = leadingui_init_input_device(g_ts_data);

	if (ret)
		goto err_init_input_device_failed;

	/* init work queue */
	INIT_DELAYED_WORK(&g_ts_data->work_init,       workq_ts_init);
	INIT_WORK(&g_ts_data->work_recover,    workq_ts_recover);
    #if BUILT_IN_UPGRADE
	INIT_WORK(&g_ts_data->work_fw_upgrade, workq_ts_fw_upgrade);
    #endif

	/* configure reset gpio */
  	MTK_RST_PIN_INIT();
  	MTK_RST_PIN_TOGGLE();

	/* configure irq gpio */
	MTK_EINT_PIN_MODE (MODE_EINT, DIR_IN);

	LUI_TRACE("client->name = %s \n",             g_ts_data->client->name);
	LUI_TRACE("client->dev.driver->name = %s \n", g_ts_data->client->dev.driver->name);
	//LUI_TRACE("Requesting IRQ: %d\n",           g_ts_data->client->irq);

	/* Power on */
	if (g_ts_data->pdata->use_regulator) {
    	ret = leadingui_ts_power_ctrl(g_ts_data, POWER_ON);
    	//if (ret)
    		//goto err_power_failed;
    }
	if (g_ts_data->pdata->use_reset_gpio) {
		//tpd_gpio_output(tpd_rst_gpio_number, 0);
		tpd_gpio_output(GTP_RST_PORT, 0);
		msleep(CHIP_RESET_MDELAY_20);
			
	} else {
		LUI_ERROR("FAIL: reset gpio not provided\n");
	}
    //if (tpd_int_gpio_number) {
	//tpd_gpio_as_int(tpd_int_gpio_number);
    tpd_gpio_as_int(GTP_INT_PORT); //by hct
    //} else {
    //LUI_ERROR("FAIL: irq gpio not provided\n");
    //}
    thread = kthread_run(touch_event_handler, g_ts_data, LEADINGUI_TS_NAME);
    if ( IS_ERR(thread) ) {
	    ret = PTR_ERR(thread);
	    pr_err(" %s: failed to create kernel thread: %d\n",__func__, ret);
	    goto err_kthread;
    }
	
    node = of_find_matching_node(NULL, touch_of_match);
    if (node) {
	    of_property_read_u32_array(node , "debounce", ints, ARRAY_SIZE(ints));
	    gpio_set_debounce(ints[0], ints[1]);

	    g_ts_data->pdata->irq_gpio = ints[0];// GPIO_TPD_EINT;//of_get_named_gpio(np, "eint-gpio", 0);
	    LUI_INFO("ts_data->pdata->irq_gpio = %d\n", g_ts_data->pdata->irq_gpio);
	    if (g_ts_data->pdata->irq_gpio < 0) {
		    LUI_ERROR("%s: of_get_named_gpio failed: tsp_gpio %d\n", __func__, g_ts_data->pdata->irq_gpio);
		    ret = -EINVAL;
		    goto err_irq_gpio_request_failed;
	    }
        //ret = gpio_direction_input(g_ts_data->pdata->irq_gpio);
	    /* configure irq gpio */
	    if (gpio_is_valid(g_ts_data->pdata->irq_gpio)) {
		    ret = gpio_request(g_ts_data->pdata->irq_gpio, "leadingui_irq_gpio");
		    if (ret) {
			    LUI_ERROR("FAIL: reset irq_request\n");
			    goto err_irq_gpio_request_failed;
		    }
		    //ts_data->irq = ts_data->client->irq = gpio_to_irq(ts_data->pdata->irq_gpio);

	    } else {
		    LUI_ERROR("FAIL: irq gpio not provided\n");
		    goto err_irq_gpio_request_failed;
	    }

	    touch_irq = irq_of_parse_and_map(node, 0);
	    ret = request_irq(touch_irq,
			              (irq_handler_t)tpd_eint_handler,
			              IRQF_TRIGGER_FALLING,
			              "TOUCH_PANEL-eint", NULL);
	    if (ret > 0) {
		    ret = -1;
		    printk("tpd request_irq IRQ LINE NOT AVAILABLE!.");
	    }
    }
    disable_irq_nosync(touch_irq);
    
    /* configure reset gpio */
  	MTK_RST_PIN_INIT();
  	MTK_RST_PIN_TOGGLE();

	/* configure irq gpio */
	MTK_EINT_PIN_MODE (MODE_EINT, DIR_IN);
	mdelay(10);
	/* initialize leadingui touch */
	ret = leadingui_ts_dev_irq_init(g_ts_data);
    if (ret && !g_ts_data->urgent_upgrade)
	    goto err_ts_init;

	/* disable interrupt */
	//leadingui_ts_irq_enable(g_ts_data, false);

	/* sysfs */
	ret = sysfs_create_group(&client->dev.kobj, &leadingui_attr_group);

	if (ret) {
		LUI_ERROR("sysfs_create_group fail\n");
		goto err_unregister_device;
	}

    #if 0//defined(CONFIG_HAS_EARLYSUSPEND)
	g_ts_data->early_suspend.level   = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + LUI_SUSPEND_LEVEL;
	g_ts_data->early_suspend.suspend = leadingui_ts_early_suspend;
	g_ts_data->early_suspend.resume  = leadingui_ts_late_resume;

	/* register early suspend */
	register_early_suspend(&g_ts_data->early_suspend);
    #endif

    #if (LUI_WATCHDOG_TIMER)
    #if 0
	g_ts_data->touch_reported = false;
	//KGB
	INIT_WORK(&g_ts_data->wd_timer_work, leadingui_wd_timer_work);
	leadingui_wd_timer_init(g_ts_data);
    #endif
	g_ts_data->touch_reported = false;
	INIT_DELAYED_WORK(&g_ts_data->wd_timer_work, leadingui_wd_timer_work);
	leadingui_wdt_check_workqueue = create_workqueue("leadingui_wdt_check");
	//queue_delayed_work(leadingui_wdt_check_workqueue, &leadingui_wdt_check_work, LUI_WATCHDOG_TIMEOUT);
    #endif

    g_ts_data->power_state = POWER_WAKE;

	MTK_EINT_PIN_MODE (MODE_EINT, DIR_IN);
	leadingui_ts_irq_enable(g_ts_data, true);

    #if BUILT_IN_UPGRADE
	/* Firmware Upgrade Check - use thread for booting time reduction */
	queue_work(leadingui_wq, &g_ts_data->work_fw_upgrade);
    #endif

    tpd_load_status = 1;

    #if (LUI_WATCHDOG_TIMER)
	leadingui_wd_timer_start(g_ts_data);
    #endif

	return ret;

err_unregister_device:

err_kthread:

err_ts_init:
//err_power_failed:
    if (gpio_is_valid(g_ts_data->pdata->irq_gpio))
	    gpio_free(g_ts_data->pdata->irq_gpio);
err_irq_gpio_request_failed:
//err_reset_gpio_request_failed:
//	input_unregister_device(g_ts_data->input_dev);
//	input_free_device(g_ts_data->input_dev);

err_init_input_device_failed:
err_get_platform_data_failed:
	mutex_destroy(&g_ts_data->fw_info.mutex);

	kfree(g_ts_data);

err_kzalloc_failed:

	destroy_workqueue(leadingui_wq);
	leadingui_wq = NULL;
	LUI_ERROR("%s is failed\n",__func__);
	return ret;
}

static int leadingui_ts_remove(struct i2c_client *client)
{
	int ret = 0;
	struct leadingui_ts_data *ts_data = i2c_get_clientdata(client);

    #if (LUI_WATCHDOG_TIMER)
	//leadingui_wd_timer_stop(ts_data);
	destroy_workqueue(leadingui_wdt_check_workqueue);	
    #endif

	leadingui_ts_irq_enable(ts_data, false);
    if (gpio_is_valid(g_ts_data->pdata->irq_gpio))
	    gpio_free(g_ts_data->pdata->irq_gpio);

	flush_work(&ts_data->work_fw_upgrade);
	flush_work(&ts_data->work_recover);

	/* Power off */
	if (ts_data->pdata->support_suspend)
		leadingui_ts_power_ctrl(ts_data, POWER_OFF);

	sysfs_remove_group(&client->dev.kobj, &leadingui_attr_group);

    #if 0//defined(CONFIG_HAS_EARLYSUSPEND)
	unregister_early_suspend(&ts_data->early_suspend);
    #endif

	mutex_destroy(&ts_data->fw_info.mutex);

	destroy_workqueue(leadingui_wq);
	leadingui_wq = NULL;

    //input_unregister_device(ts_data->input_dev);
    //input_free_device(ts_data->input_dev);

	kfree(ts_data);

	return ret;
}

//static int leadingui_ts_suspend(struct device *dev)
static int leadingui_ts_suspend(struct device *h)
{
	int ret = 0;
	struct leadingui_ts_data *ts_data = g_ts_data;

	if (ts_data == NULL) {
		LUI_ERROR("ts_data is NULL\n");
		return -EINVAL;
	}
	
    #if LUI_GESTURE_EN
    if(1 == gesture_wakeup_switch)
    {
    #if (LUI_WATCHDOG_TIMER)
	leadingui_wd_timer_stop(ts_data);
    #endif
	mutex_lock(&g_ts_mutex);

	/* disable irq */
	leadingui_ts_irq_enable(ts_data, false);

	release_all_ts_event(ts_data);		

    gesture_mask = ALL_GESTURE_WAKEUP_ENABLE;
	ret = leadingui_ts_command(ts_data->client,
				DEVICE_CMD_OP_MODE, DEVICE_OP_MA_GESTURE_WAKEUP, gesture_mask,
				0, NULL);
	gesture_wakeup_mode = 1;
	LUI_INFO("leadingui_ts_enter_gesture_wakeup \n");
	leadingui_ts_irq_enable(ts_data, true);
	mutex_unlock(&g_ts_mutex);
	return ret;    	
	}
    #endif

	mutex_lock(&g_ts_mutex);

	//UI_ERROR("mutex_lock\n");

	if (ts_data->fw_info.is_updating) {
		LUI_ERROR("Do not allow the device to enter suspend during the update process. \n");
		ret = -EPERM;
		goto leadingui_ts_suspend_exit;
	}

	if (!ts_data->pdata->support_suspend) {
	 	LUI_WARNING("suspend_mode:SUSPEND_NOT_SUPPORT. \n");
	 	ret = 0;
	 	goto leadingui_ts_suspend_exit;
	}

	if ((ts_data->power_state == POWER_SUSPEND) || (ts_data->power_state == POWER_OFF)) {
		if (ts_data->power_state == POWER_SUSPEND) {
			LUI_ERROR("device state is suspended.\n");
		} else {
			LUI_ERROR("device state is power-off.\n");
		}
		ret = 0;
		goto leadingui_ts_suspend_exit;
	}

	if (atomic_read(&ts_data->device_init) != 1) {
		LUI_ERROR("device does not initialize\n");
		ret = -EPERM;
		goto leadingui_ts_suspend_exit;
	}

    #if (LUI_WATCHDOG_TIMER)
	leadingui_wd_timer_stop(ts_data);
    #endif

	/* disable irq */
	leadingui_ts_irq_enable(ts_data, false);

	release_all_ts_event(ts_data);

	/* set device suspend */
	ret = leadingui_ts_command(ts_data->client,DEVICE_CMD_OP_MODE, DEVICE_OP_MA_PWR_DN, 0,0, NULL);

	/* set interrupt mode */
	MTK_EINT_PIN_MODE (MODE_EINT, DIR_IN);

	if (ret)
		goto leadingui_ts_suspend_exit;

	msleep(SUSPEND_CMD_DELAY);

	ts_data->power_state = POWER_SUSPEND;

    #if 0
	/* power off */
	if (ts_data->pdata->use_regulator) {
		LUI_ERROR("leadingui_regulator_config\n");
		ret = leadingui_regulator_config(ts_data, POWER_OFF);

		if (ret)
			goto leadingui_ts_suspend_exit;

		ts_data->power_state = POWER_OFF;
		atomic_set(&ts_data->device_init, 0);
    }
    #endif

leadingui_ts_suspend_exit:

	//mutex_unlock(&ts_data->input_dev->mutex);
	mutex_unlock(&g_ts_mutex);

	tpd_halt = 1;

	return ret;
}

//static int leadingui_ts_resume(struct device *dev)
static int leadingui_ts_resume(struct device *h)
{
	int ret = 0;
	struct leadingui_ts_data *ts_data = g_ts_data;

	if (ts_data == NULL) {
		LUI_ERROR("ts_data == NULL\n");
		return -EINVAL;
	}

	if (ts_data->fw_info.is_updating) {
		LUI_ERROR("Do not allow resume operate during the update process. \n");
		return 0;
	}

	if (!ts_data->pdata->support_suspend) {
		LUI_WARNING("Resume mode:RESUME_NOT_SUPPORT.\n");
		return 0;
	}

    #if LUI_GESTURE_EN
    if(1 == gesture_wakeup_switch)
    {
	if(gesture_wakeup_mode == 0)
    	{
        	if ((ts_data->power_state != POWER_SUSPEND) && (ts_data->power_state != POWER_OFF)) {
        		switch (ts_data->power_state)
        		{
        		case POWER_WAKE: LUI_WARNING("device state is waking.\n"); break;
        		case POWER_ON:   LUI_WARNING("device state is power-on.\n"); break;
        		}
        
        		return 0;
        	}
    	}
    	else
    	{
    	    gesture_wakeup_mode = 0;
    		LUI_INFO("leadingui_ts_gesture_wakeup_exit \n");
    	}
	}
    #endif
    #if LUI_GESTURE_EN
    if(0 == gesture_wakeup_switch)
	{
    #endif	
	if ((ts_data->power_state != POWER_SUSPEND) && (ts_data->power_state != POWER_OFF)) {
		switch (ts_data->power_state)
		{
		case POWER_WAKE: LUI_WARNING("device state is waking.\n"); break;
		case POWER_ON:   LUI_WARNING("device state is power-on.\n"); break;
		}

		return 0;
	}
    #if LUI_GESTURE_EN
	}

    #endif
    
	//mutex_lock(&ts_data->input_dev->mutex);
	mutex_lock(&g_ts_mutex);

    #if 0
	/* power on */
	if (ts_data->pdata->use_regulator) {
		ret = leadingui_regulator_config(ts_data, POWER_ON);

		if (ret)
			goto err_resume_ts_reset;

		ts_data->power_state = POWER_ON;
	}
    #endif

	/* reset device */
	ret = leadingui_ts_reset(ts_data);

	if (ret){
		goto err_resume_ts_reset;
	}

	memset(&ts_data->cur_tdata, 0, (sizeof(struct touch_data) * LUI_MAX_FINGER));
	memset(&ts_data->cur_tkey, 0, sizeof(struct key_data));

	/* set interrupt mode */
	MTK_EINT_PIN_MODE (MODE_EINT, DIR_IN);

	/* enable irq */
	leadingui_ts_irq_enable(ts_data, true);

	atomic_set(&ts_data->device_init, 1);

	ts_data->power_state = POWER_WAKE;
	ts_data->recovery_err_cnt = 0;

err_resume_ts_reset:
	if (ret) {
		/* error: start recovery */
		if (unlikely(ts_data->recovery_err_cnt >= DEVIE_MAX_RECOVERY_COUNT)) {
			ts_data->recovery_err_cnt = 0;
			LUI_ERROR("Device initialize failed: has some unknown problems.\n");
			return -EPERM;
		}

		LUI_ERROR("recovery: count = %d.\n", ts_data->recovery_err_cnt);
		ts_data->recovery_err_cnt++;
		queue_delayed_work(leadingui_wq, &ts_data->work_init, msecs_to_jiffies(10));
		ret = 0;
	} else {
        #if (LUI_WATCHDOG_TIMER)
		leadingui_wd_timer_start(ts_data);
        #endif
	}

	//mutex_unlock(&ts_data->input_dev->mutex);
	mutex_unlock(&g_ts_mutex);

	tpd_halt = 0;

	return ret;
}

static const struct i2c_device_id leadingui_id_table[] = {
	{LEADINGUI_TS_NAME, 0},
	{}
};

unsigned short force[] = {0,TPD_I2C_ADDR,I2C_CLIENT_END,I2C_CLIENT_END};
static const unsigned short * const forces[] = { force, NULL };
// static int tpd_detect (struct i2c_client *client, struct i2c_board_info *info)
// {
// 	strcpy(info->type, TPD_DEVICE);
// 	return 0;
// }

MODULE_DEVICE_TABLE(i2c, leadingui_id_table);
static const struct of_device_id tpd_of_match[] = {
	{.compatible = "mediatek,cap_touch_lu2x3x"},
	{},
};

static struct i2c_driver tpd_i2c_driver = {
	.probe		  = leadingui_ts_probe,
	.remove		  = leadingui_ts_remove,
	.id_table	  = leadingui_id_table,
	//.detect     = tpd_detect,
	.driver.name  = LEADINGUI_TS_NAME,
	.driver.of_match_table = tpd_of_match,
	.address_list = (const unsigned short*) forces,
};

static int tpd_local_init(void)
{
	TPD_DMESG("leadingui I2C Touchscreen Driver \n");
    #ifdef CONFIG_TPD_POWER_SOURCE_VIA_VGP
	LUI_INFO("Device Tree get regulator!");
	tpd->reg = regulator_get(tpd->tpd_dev, "vtouch");
	if (IS_ERR(tpd->reg))
		LUI_ERROR("regulator_get() failed!\n");
    #endif

	if (i2c_add_driver(&tpd_i2c_driver) != 0)
	{
		TPD_DMESG("lu310a unable to add i2c driver.\n");
		return -1;
	}

	if (tpd_load_status == 0)
	{
		TPD_DMESG("lu310a add error touch panel driver.\n");
		i2c_del_driver(&tpd_i2c_driver);
		return -1;
	}

    #ifdef TPD_HAVE_BUTTON
	tpd_button_setting(TPD_KEY_COUNT, tpd_keys_local, tpd_keys_dim_local);// initialize tpd button data
    #endif
	boot_mode = get_boot_mode();
	if (boot_mode == 3) {
		boot_mode = NORMAL_BOOT;
	}
	return 0;
}

static void tpd_suspend(struct device *h)
{
	leadingui_ts_suspend(h);
}

static void tpd_resume(struct device *h)
{
	leadingui_ts_resume(h);
}

static struct tpd_driver_t tpd_device_driver = {
	.tpd_device_name = LEADINGUI_TS_NAME,
	.tpd_local_init  = tpd_local_init,
	.suspend         = tpd_suspend,
	.resume          = tpd_resume,
    #ifdef TPD_HAVE_BUTTON
	.tpd_have_button = 1,
    #else
	.tpd_have_button = 0,
    #endif
#if LUI_GESTURE_EN
	.attrs = {
		.attr = tpd_attr_list,
		.num = (int)(sizeof(tpd_attr_list)/sizeof(tpd_attr_list[0])),
	},
#endif
};

static int __init leadingui_i2c_init(void)
{
    printk("MediaTek leadingui touch panel driver init\n");
	tpd_get_dts_info();
	if (tpd_driver_add(&tpd_device_driver) < 0) {
		pr_err("Fail to add tpd driver\n");
		return -1;
	}

	return 0;
}

static void __exit leadingui_i2c_exit(void)
{
	printk("MediaTek leadingui exit touch panel driver init\n");
	tpd_driver_remove(&tpd_device_driver);
}

module_init(leadingui_i2c_init);
module_exit(leadingui_i2c_exit);

/* Module information */
MODULE_AUTHOR("LeadingUI Co.,Ltd.<jaehan@leadingui.com>");
MODULE_DESCRIPTION("LeadingUI LU201x LU301x LU31xx Touchscreen driver");
MODULE_LICENSE("GPL");
