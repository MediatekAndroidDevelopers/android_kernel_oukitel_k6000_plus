#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/ioport.h>
#include <linux/errno.h>
#include <linux/spi/spi.h>
#include <linux/interrupt.h>
#include <linux/irqreturn.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/wakelock.h>
#include <linux/kthread.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/spi/spidev.h>
#include <linux/semaphore.h>
#include <linux/poll.h>
#include <linux/fcntl.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/input.h>
#include <linux/signal.h>
#include <linux/spi/spi.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include "mt_spi.h"

extern void hct_waite_for_finger_dts_paser(void);
extern int hct_finger_set_spi_mode(int cmd);
extern int hct_finger_set_eint(int cmd);
extern int hct_finger_set_power(int cmd);
extern int hct_finger_set_reset(int cmd);

static u8 cdfinger_debug = 0x01;

#define CDFINGER_DBG(fmt, args...) \
	do{ \
		if(cdfinger_debug & 0x01) \
			printk( "[DBG][cdfinger]:%5d: <%s>" fmt, __LINE__,__func__,##args ); \
	}while(0)
#define CDFINGER_FUNCTION(fmt, args...) \
	do{ \
		if(cdfinger_debug & 0x02) \
			printk( "[DBG][cdfinger]:%5d: <%s>" fmt, __LINE__,__func__,##args ); \
	}while(0)
#define CDFINGER_REG(fmt, args...) \
	do{ \
		if(cdfinger_debug & 0x04) \
			printk( "[DBG][cdfinger]:%5d: <%s>" fmt, __LINE__,__func__,##args ); \
	}while(0)
#define CDFINGER_ERR(fmt, args...) \
    do{ \
		printk( "[DBG][cdfinger]:%5d: <%s>" fmt, __LINE__,__func__,##args ); \
    }while(0)

#define HAS_RESET_PIN
//#define DTS_PROBE

//#ifdef HAS_RESET_PIN
#define ENABLE_SPI_FREQUENCY_CALIBRTION
//#endif

#define VERSION                         "cdfinger version 2.4"
#define DEVICE_NAME                     "fpsdev0"
#define SPI_DRV_NAME                    "cdfinger"
#define CDFINGER_HEIGHT                  128
#define CDFINGER_WIDTH                   56
#define CDFINGER_IMAGE_SIZE              (CDFINGER_HEIGHT*CDFINGER_WIDTH)
#ifdef ENABLE_SPI_FREQUENCY_CALIBRTION
#define CDFINGER_RD_PIX_SIZE             (14*1024)
#else
#define CDFINGER_RD_PIX_SIZE             (15*1024)
#endif
#define CDFINGER_SPI_CALIBRATION_SIZE    (CDFINGER_RD_PIX_SIZE - 1000)

#define CDFINGER_IOCTL_MAGIC_NO          0xFB
#define CDFINGER_INIT                    _IOW(CDFINGER_IOCTL_MAGIC_NO, 0, uint8_t)
#define CDFINGER_GETIMAGE                _IOW(CDFINGER_IOCTL_MAGIC_NO, 1, uint8_t)
#define CDFINGER_INITERRUPT_MODE	     _IOW(CDFINGER_IOCTL_MAGIC_NO, 2, uint8_t)
#define CDFINGER_INITERRUPT_KEYMODE      _IOW(CDFINGER_IOCTL_MAGIC_NO, 3, uint8_t)
#define CDFINGER_INITERRUPT_FINGERUPMODE _IOW(CDFINGER_IOCTL_MAGIC_NO, 4, uint8_t)
#define CDFINGER_RELEASE_WAKELOCK        _IO(CDFINGER_IOCTL_MAGIC_NO, 5)
#define CDFINGER_CHECK_INTERRUPT         _IO(CDFINGER_IOCTL_MAGIC_NO, 6)
#define CDFINGER_SET_SPI_SPEED           _IOW(CDFINGER_IOCTL_MAGIC_NO, 7, uint8_t)
#define CDFINGER_REPORT_KEY              _IOW(CDFINGER_IOCTL_MAGIC_NO, 10, uint8_t)
#define CDFINGER_POWERDOWN               _IO(CDFINGER_IOCTL_MAGIC_NO, 11)

#define KEY_INTERRUPT                   KEY_F11

enum work_mode {
	CDFINGER_MODE_NONE       = 1<<0,
	CDFINGER_INTERRUPT_MODE  = 1<<1,
	CDFINGER_KEY_MODE        = 1<<2,
	CDFINGER_FINGER_UP_MODE  = 1<<3,
	CDFINGER_READ_IMAGE_MODE = 1<<4,
	CDFINGER_MODE_MAX
};

enum spi_speed {
	CDFINGER_SPI_4M1 = 1,
	CDFINGER_SPI_4M4,
	CDFINGER_SPI_4M7,
	CDFINGER_SPI_5M1,
	CDFINGER_SPI_5M5,
	CDFINGER_SPI_6M1,
	CDFINGER_SPI_6M7,
	CDFINGER_SPI_7M4,
	CDFINGER_SPI_8M
};

static struct cdfinger_data {
	struct spi_device *spi;
	struct mutex buf_lock;
	struct mutex transfer_lock;
	unsigned int irq;
	int irq_enabled;

	u8 *imagetxcmd;
	u8 *imagerxpix;
	u8 *imagebuf;

	u32 vdd_ldo_enable;
	u32 vio_ldo_enable;
	u32 config_spi_pin;

	struct pinctrl *fps_pinctrl;
	struct pinctrl_state *fps_reset_high;
	struct pinctrl_state *fps_reset_low;
	struct pinctrl_state *fps_power_on;
	struct pinctrl_state *fps_power_off;
	struct pinctrl_state *fps_vio_on;
	struct pinctrl_state *fps_vio_off;
	struct pinctrl_state *cdfinger_spi_miso;
	struct pinctrl_state *cdfinger_spi_mosi;
	struct pinctrl_state *cdfinger_spi_sck;
	struct pinctrl_state *cdfinger_spi_cs;
	struct pinctrl_state *cdfinger_irq;

	int thread_wakeup;
	int process_interrupt;
	int key_report;
	enum work_mode device_mode;
	uint8_t int_count;
	struct timer_list int_timer;
	struct input_dev *cdfinger_inputdev;
	struct wake_lock cdfinger_lock;
	struct task_struct *cdfinger_thread;
#ifdef ENABLE_SPI_FREQUENCY_CALIBRTION
	struct task_struct *cdfinger_spi_calibration_thread;
#endif
	struct fasync_struct *async_queue;
	uint8_t cdfinger_interrupt;
	u8 last_transfer;
}*g_cdfinger;

static DECLARE_WAIT_QUEUE_HEAD(waiter);

static struct mt_chip_conf spi_conf = {
	.setuptime = 7,
	.holdtime = 7,
	.high_time = 13,
	.low_time = 13,
	.cs_idletime = 6,
	.cpol = 0,
	.cpha = 0,
	.rx_mlsb = 1,
	.tx_mlsb = 1,
	.tx_endian = 0,
	.rx_endian = 0,
	.com_mod = DMA_TRANSFER,
	.pause = 1,
	.finish_intr = 1,
	.deassert = 0,
	.ulthigh = 0,
	.tckdly = 0,
};

static void cdfinger_disable_irq(struct cdfinger_data *cdfinger)
{
	if(cdfinger->irq_enabled == 1)
	{
		disable_irq_nosync(cdfinger->irq);
		cdfinger->irq_enabled = 0;
		CDFINGER_DBG("irq disable\n");
	}
}

static void cdfinger_enable_irq(struct cdfinger_data *cdfinger)
{
	if(cdfinger->irq_enabled == 0)
	{
		enable_irq(cdfinger->irq);
		cdfinger->irq_enabled =1;
		CDFINGER_DBG("irq enable\n");
	}
}

static int cdfinger_parse_dts(struct cdfinger_data *cdfinger)
{
	int ret = -1;

#ifndef DTS_PROBE
	cdfinger->spi->dev.of_node = of_find_compatible_node(NULL,NULL,"mediatek,hct_finger");
#endif
	if(!(cdfinger->spi->dev.of_node)){
		CDFINGER_ERR("of node not exist!\n");
		goto parse_err;
	}

	cdfinger->irq = irq_of_parse_and_map(cdfinger->spi->dev.of_node, 0);
	if(cdfinger->irq < 0)
	{
		CDFINGER_ERR("parse irq failed! irq[%d]\n",cdfinger->irq);
		goto parse_err;
	}

	return 0;

parse_err:
	CDFINGER_ERR("parse dts failed!\n");

	return ret;
#if 0
	int ret = -1;

	if(cdfinger->spi == NULL)
	{
		CDFINGER_ERR("spi is NULL !\n");
		goto parse_err;
	}
	
#ifndef DTS_PROBE
	cdfinger->spi->dev.of_node = of_find_compatible_node(NULL,NULL,"mediatek,hct_finger");
#endif

	if(!(cdfinger->spi->dev.of_node)){
		CDFINGER_ERR("of node not exist!\n");
		goto parse_err;
	}

	cdfinger->irq = irq_of_parse_and_map(cdfinger->spi->dev.of_node, 0);
	if(cdfinger->irq < 0)
	{
		CDFINGER_ERR("parse irq failed! irq[%d]\n",cdfinger->irq);
		goto parse_err;
	}

	//of_property_read_u32(cdfinger->spi->dev.of_node,"vdd_ldo_enable",&cdfinger->vdd_ldo_enable);
	//of_property_read_u32(cdfinger->spi->dev.of_node,"vio_ldo_enable",&cdfinger->vio_ldo_enable);
	//of_property_read_u32(cdfinger->spi->dev.of_node,"config_spi_pin",&cdfinger->config_spi_pin);
	cdfinger->vdd_ldo_enable = 1;
	cdfinger->vio_ldo_enable = 1;
	cdfinger->config_spi_pin = 1;
	CDFINGER_DBG("irq[%d], vdd_ldo_enable[%d], vio_ldo_enable[%d], config_spi_pin[%d]\n",
		cdfinger->irq, cdfinger->vdd_ldo_enable, cdfinger->vio_ldo_enable, cdfinger->config_spi_pin);

	cdfinger->fps_pinctrl = devm_pinctrl_get(&cdfinger->spi->dev);
	if (IS_ERR(cdfinger->fps_pinctrl)) {
		ret = PTR_ERR(cdfinger->fps_pinctrl);
		CDFINGER_ERR("Cannot find fingerprint cdfinger->fps_pinctrl! ret=%d\n", ret);
		goto parse_err;
	}

	cdfinger->cdfinger_irq = pinctrl_lookup_state(cdfinger->fps_pinctrl,"finger_int_as_int");
	if (IS_ERR(cdfinger->cdfinger_irq))
	{
		ret = PTR_ERR(cdfinger->cdfinger_irq);
		CDFINGER_ERR("cdfinger->cdfinger_irq ret = %d\n",ret);
		goto parse_err;
	}
	cdfinger->fps_reset_low = pinctrl_lookup_state(cdfinger->fps_pinctrl,"finger_reset_en0");
	if (IS_ERR(cdfinger->fps_reset_low))
	{
		ret = PTR_ERR(cdfinger->fps_reset_low);
		CDFINGER_ERR("cdfinger->fps_reset_low ret = %d\n",ret);
		goto parse_err;
	}
	cdfinger->fps_reset_high = pinctrl_lookup_state(cdfinger->fps_pinctrl,"finger_reset_en1");
	if (IS_ERR(cdfinger->fps_reset_high))
	{
		ret = PTR_ERR(cdfinger->fps_reset_high);
		CDFINGER_ERR("cdfinger->fps_reset_high ret = %d\n",ret);
		goto parse_err;
	}

	if(cdfinger->config_spi_pin == 1)
	{
		cdfinger->cdfinger_spi_miso = pinctrl_lookup_state(cdfinger->fps_pinctrl,"finger_spi0_mi_as_spi0_mi");
		if (IS_ERR(cdfinger->cdfinger_spi_miso))
		{
			ret = PTR_ERR(cdfinger->cdfinger_spi_miso);
			CDFINGER_ERR("cdfinger->cdfinger_spi_miso ret = %d\n",ret);
			goto parse_err;
		}
		cdfinger->cdfinger_spi_mosi = pinctrl_lookup_state(cdfinger->fps_pinctrl,"finger_spi0_mo_as_spi0_mo");
		if (IS_ERR(cdfinger->cdfinger_spi_mosi))
		{
			ret = PTR_ERR(cdfinger->cdfinger_spi_mosi);
			CDFINGER_ERR("cdfinger->cdfinger_spi_mosi ret = %d\n",ret);
			goto parse_err;
		}
		cdfinger->cdfinger_spi_sck = pinctrl_lookup_state(cdfinger->fps_pinctrl,"finger_spi0_clk_as_spi0_clk");
		if (IS_ERR(cdfinger->cdfinger_spi_sck))
		{
			ret = PTR_ERR(cdfinger->cdfinger_spi_sck);
			CDFINGER_ERR("cdfinger->cdfinger_spi_sck ret = %d\n",ret);
			goto parse_err;
		}
		cdfinger->cdfinger_spi_cs = pinctrl_lookup_state(cdfinger->fps_pinctrl,"finger_spi0_cs_as_spi0_cs");
		if (IS_ERR(cdfinger->cdfinger_spi_cs))
		{
			ret = PTR_ERR(cdfinger->cdfinger_spi_cs);
			CDFINGER_ERR("cdfinger->cdfinger_spi_cs ret = %d\n",ret);
			goto parse_err;
		}
	}

	if(cdfinger->vdd_ldo_enable == 1)
	{
		cdfinger->fps_power_on = pinctrl_lookup_state(cdfinger->fps_pinctrl,"finger_power_en1");
		if (IS_ERR(cdfinger->fps_power_on))
		{
			ret = PTR_ERR(cdfinger->fps_power_on);
			CDFINGER_ERR("cdfinger->fps_power_on ret = %d\n",ret);
			goto parse_err;
		}

		cdfinger->fps_power_off = pinctrl_lookup_state(cdfinger->fps_pinctrl,"finger_power_en0");
		if (IS_ERR(cdfinger->fps_power_off))
		{
			ret = PTR_ERR(cdfinger->fps_power_off);
			CDFINGER_ERR("cdfinger->fps_power_off ret = %d\n",ret);
			goto parse_err;
		}
	}

	if(cdfinger->vio_ldo_enable == 1)
	{
		cdfinger->fps_vio_on = pinctrl_lookup_state(cdfinger->fps_pinctrl,"finger_power_18v_en1");
		if (IS_ERR(cdfinger->fps_vio_on))
		{
			ret = PTR_ERR(cdfinger->fps_vio_on);
			CDFINGER_ERR("cdfinger->fps_vio_on ret = %d\n",ret);
			goto parse_err;
		}

		cdfinger->fps_vio_off = pinctrl_lookup_state(cdfinger->fps_pinctrl,"finger_power_18v_en0");
		if (IS_ERR(cdfinger->fps_vio_off))
		{
			ret = PTR_ERR(cdfinger->fps_vio_off);
			CDFINGER_ERR("cdfinger->fps_vio_off ret = %d\n",ret);
			goto parse_err;
		}
	}

	return 0;
parse_err:
	CDFINGER_ERR("parse dts failed!\n");

	return ret;
#endif
}

static int spi_send_cmd(struct cdfinger_data *cdfinger,  u8 *tx, u8 *rx, u16 spilen)
{
	struct spi_message m;
	struct mt_chip_conf *spiconf = &spi_conf;
	struct spi_transfer t = {
		.tx_buf = tx,
		.rx_buf = rx,
		.len = spilen,
	};

	CDFINGER_DBG("transfer msg[0x%x]\n", tx[0]);

	if(tx[0] == 0x14 && 0x14 == cdfinger->last_transfer)
	{
		CDFINGER_DBG("Warning: transfer is same as last transfer.now[0x%x], last[0x%x]\n", tx[0], cdfinger->last_transfer);
		return 0;
	}
	cdfinger->last_transfer = tx[0];

	if(spilen > 32)
	{
		if(spiconf->com_mod != DMA_TRANSFER)
		{
			spiconf->com_mod = DMA_TRANSFER;
			spi_setup(cdfinger->spi);
		}
	}
	else
	{
		if(spiconf->com_mod != FIFO_TRANSFER)
		{
			spiconf->com_mod = FIFO_TRANSFER;
			spi_setup(cdfinger->spi);
		}
	}

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);

	return spi_sync(cdfinger->spi, &m);
}

static int spi_send_cmd_fifo(struct cdfinger_data *cdfinger,  u8 *tx, u8 *rx, u16 spilen)
{
	int ret = 0;

	mutex_lock(&cdfinger->transfer_lock);
	ret = spi_send_cmd(cdfinger, tx, rx, spilen);
	mutex_unlock(&cdfinger->transfer_lock);
	udelay(100);

	return ret;
}

static void cdfinger_power_on(struct cdfinger_data *cdfinger)
{
	hct_finger_set_spi_mode(1);
	hct_finger_set_power(1);
#if 0
	if(cdfinger->config_spi_pin == 1)
	{
		pinctrl_select_state(cdfinger->fps_pinctrl, cdfinger->cdfinger_spi_miso);
		pinctrl_select_state(cdfinger->fps_pinctrl, cdfinger->cdfinger_spi_mosi);
		pinctrl_select_state(cdfinger->fps_pinctrl, cdfinger->cdfinger_spi_sck);
		pinctrl_select_state(cdfinger->fps_pinctrl, cdfinger->cdfinger_spi_cs);
	}

	if(cdfinger->vdd_ldo_enable == 1)
	{
		pinctrl_select_state(cdfinger->fps_pinctrl, cdfinger->fps_power_on);
	}

	if(cdfinger->vio_ldo_enable == 1)
	{
		pinctrl_select_state(cdfinger->fps_pinctrl, cdfinger->fps_vio_on);
	}
#endif
}

static void cdfinger_power_off(struct cdfinger_data *cdfinger)
{
	//hct_finger_set_spi_mode(0);
	//hct_finger_set_power(0);
#if 0
	if(cdfinger->vdd_ldo_enable == 1)
	{
		pinctrl_select_state(cdfinger->fps_pinctrl, cdfinger->fps_power_off);
	}

	if(cdfinger->vio_ldo_enable == 1)
	{
		pinctrl_select_state(cdfinger->fps_pinctrl, cdfinger->fps_vio_off);
	}
#endif
}
#ifdef HAS_RESET_PIN
static void cdfinger_reset(int count)
{
	//struct cdfinger_data *cdfinger = g_cdfinger;

	//pinctrl_select_state(cdfinger->fps_pinctrl, cdfinger->fps_reset_low);
	hct_finger_set_reset(0);
	udelay(500*count);
	//pinctrl_select_state(cdfinger->fps_pinctrl, cdfinger->fps_reset_high);
	hct_finger_set_reset(1);
	udelay(500*count);
}
#endif

static void cdfinger_release_wakelock(struct cdfinger_data *cdfinger)
{
	CDFINGER_FUNCTION("enter\n");
	wake_unlock(&cdfinger->cdfinger_lock);
	CDFINGER_FUNCTION("exit\n");
}

static void cdfinger_set_low_power_mode(struct cdfinger_data *cdfinger)
{
	u8 powerdown_cmd = 0x00, read;
	spi_send_cmd_fifo(cdfinger,&powerdown_cmd,&read,1);
}

static int cdfinger_dev_init(struct cdfinger_data *cdfinger, u8 arg)
{
#ifndef HAS_RESET_PIN
	static u8 reset = 0x0c;
	static u8 start = 0x18;
	u8 read;
#endif
	static u8 reg21_tx[7] = {0x21,0x66,0x66,0xa5,0x00,0x00,0x70};
	u8 reg21_rx[7];
	int ret =0, count = 0;

	CDFINGER_FUNCTION("enter\n");

	cdfinger_disable_irq(cdfinger);
	cdfinger->device_mode = CDFINGER_READ_IMAGE_MODE;
	cdfinger->key_report = 0;
	cdfinger->process_interrupt = 0;
	reg21_tx[5] = arg;

#ifdef HAS_RESET_PIN
	cdfinger_reset(1);
#else
	spi_send_cmd_fifo(cdfinger, &reset, &read, 1);
	spi_send_cmd_fifo(cdfinger, &reset, &read, 1);
	spi_send_cmd_fifo(cdfinger, &start, &read, 1);
#endif
	for(count=0; count<5; count++)
	{
		ret = spi_send_cmd_fifo(cdfinger, reg21_tx, reg21_rx, sizeof(reg21_tx)/sizeof(reg21_tx[0]));
		if (ret !=0){
			goto out;
		}

		CDFINGER_DBG("agc = 0x%x\n",arg);
		CDFINGER_REG("reg[3] = 0x%x reg[4] = 0x%x reg[5] = 0x%x reg[6] = 0x%x\n",reg21_rx[3], reg21_rx[4], reg21_rx[5], reg21_rx[6]);

		if (reg21_rx[6] != 0x70)
		{
			CDFINGER_DBG("register read value error!\n");
#ifdef HAS_RESET_PIN
			cdfinger_reset(10);
#else
			spi_send_cmd_fifo(cdfinger, &reset, &read, 1);
			spi_send_cmd_fifo(cdfinger, &reset, &read, 1);
			spi_send_cmd_fifo(cdfinger, &start, &read, 1);
#endif
			continue;
		}

		break;
	}

	if (reg21_rx[6] != 0x70)
		goto out;

	CDFINGER_FUNCTION("exit\n");

	return 0;
out:
	CDFINGER_ERR("spi transfer failed! ret = %d\n", ret);

	return -1;
}

static int cdfinger_config_interrupt(struct cdfinger_data *cdfinger, u8 arg)
{
#ifndef HAS_RESET_PIN
	static u8 reset = 0x0c;
	static u8 start = 0x18;
#endif
	static u8 interrupt = 0x14;
	static u8 reg27_tx[5] = {0x27,0x66,0x66,0x30,0x05};
	static u8 reg22_tx[7] = {0x22,0x66,0x66,0xa8,0x28,0x01,0x30};
	static u8 reg21_tx[7] = {0x21,0x66,0x66,0xa5,0x00,0x00,0x70};
	u8 read, reg27_rx[5], reg21_rx[7], reg22_rx[7];
	int ret =0, count = 0;

	CDFINGER_FUNCTION("enter\n");

	reg21_tx[5] = arg;
#ifdef HAS_RESET_PIN
	cdfinger_reset(1);
#else
	spi_send_cmd_fifo(cdfinger, &reset, &read, 1);
	spi_send_cmd_fifo(cdfinger, &reset, &read, 1);
	spi_send_cmd_fifo(cdfinger, &start, &read, 1);
#endif
	for(count=0; count<5; count++)
	{
		ret = spi_send_cmd_fifo(cdfinger, reg27_tx,reg27_rx, sizeof(reg27_tx)/sizeof(reg27_tx[0]));
		if (ret !=0){
			goto out;
		}

		ret = spi_send_cmd_fifo(cdfinger, reg22_tx,reg22_rx, sizeof(reg22_tx)/sizeof(reg22_tx[0]));
		if (ret !=0){
			goto out;
		}

		ret = spi_send_cmd_fifo(cdfinger, reg21_tx, reg21_rx, sizeof(reg21_tx)/sizeof(reg21_tx[0]));
		if (ret !=0){
			goto out;
		}

		CDFINGER_DBG("interrupt = 0x%x\n",arg);
		CDFINGER_REG("reg[3] = 0x%x reg[4] = 0x%x reg[5] = 0x%x reg[6] = 0x%x\n",reg21_rx[3], reg21_rx[4], reg21_rx[5], reg21_rx[6]);

		if (reg21_rx[6] != 0x70)
		{
			CDFINGER_ERR("register read value error!\n");
#ifdef HAS_RESET_PIN
			cdfinger_reset(10);
#else
			spi_send_cmd_fifo(cdfinger, &reset, &read, 1);
			spi_send_cmd_fifo(cdfinger, &reset, &read, 1);
			spi_send_cmd_fifo(cdfinger, &start, &read, 1);
#endif
			continue;
		}

		break;
	}

	if (reg21_rx[6] != 0x70)
		goto out;

	if(spi_send_cmd_fifo(cdfinger, &interrupt, &read, 1))
	{
		CDFINGER_ERR("set interrupt mode failed!\n");
		goto out;
	}

	CDFINGER_FUNCTION("exit\n");

	return 0;
out:
	CDFINGER_ERR("spi transfer failed! ret = %d\n", ret);

	return -1;
}

static int cdfinger_check_interrupt(struct cdfinger_data *cdfinger)
{
	static u8 regval, interrupt = 0;
	int timeout = 40;

	CDFINGER_FUNCTION("enter\n");
	cdfinger->device_mode = CDFINGER_MODE_NONE;
	cdfinger->key_report = 0;
	cdfinger->process_interrupt = 0;
	cdfinger_enable_irq(cdfinger);

	for(regval=0xf0; regval>=0xa0; regval-=16)
	{
		cdfinger->cdfinger_interrupt = 0;
		if(cdfinger_config_interrupt(cdfinger,regval))
			return -1;
		msleep(50);
		if (cdfinger->cdfinger_interrupt == 0)
		{
			interrupt = regval;
			break;
		}
	}
	if(interrupt == 0)
	{
		CDFINGER_ERR("cannot find interrupt value!\n");
		return 0;
	}

	CDFINGER_DBG("interrupt = 0x%x\n", interrupt);

	for (regval=interrupt+15; regval>interrupt; regval--)
	{
		cdfinger->cdfinger_interrupt = 0;
		if(cdfinger_config_interrupt(cdfinger,regval))
			return -1;
		msleep(50);
		if (cdfinger->cdfinger_interrupt == 0)
		{
			CDFINGER_DBG("interrupt value:0x%x\n",regval);
			break;
		}
	}

	CDFINGER_DBG("regval = 0x%x\n", regval);

	for(; regval>=0xa0; regval--)
	{
		cdfinger->cdfinger_interrupt = 0;
		if(cdfinger_config_interrupt(cdfinger,regval))
				return -1;
		timeout = 40;
		while(cdfinger->cdfinger_interrupt == 0 && timeout > 0)
		{
			msleep(50);
			timeout -- ;
		}
		if (cdfinger->cdfinger_interrupt == 0)
		{
			CDFINGER_DBG("interrupt value:0x%x\n",regval);
			return regval;
		}
	}
	
	CDFINGER_FUNCTION("exit\n");

	return 0;
}

static int cdfinger_mode_init(struct cdfinger_data *cdfinger, uint8_t arg, enum work_mode mode)
{
	CDFINGER_FUNCTION("enter\n");
	CDFINGER_DBG("mode=0x%x\n", mode);

	cdfinger->process_interrupt = 0;
	cdfinger_disable_irq(cdfinger);
	if(cdfinger_config_interrupt(cdfinger,arg))
	{
		CDFINGER_ERR("interrupt config failed! mode=0x%x\n", mode);
		return -1;
	}
	cdfinger->process_interrupt = 1;
	cdfinger->device_mode = mode;
	cdfinger->key_report = 0;
	cdfinger_enable_irq(cdfinger);
	CDFINGER_FUNCTION("exit\n");

	return 0;
}

static int cdfinger_set_spi_speed(struct cdfinger_data *cdfinger, uint8_t arg)
{
	struct mt_chip_conf *spi_par = &spi_conf;
	enum spi_speed speed = arg;

	switch(speed){
		case CDFINGER_SPI_4M1:
			spi_par->high_time = 16;
			spi_par->low_time = 16;
			break;
		case CDFINGER_SPI_4M4:
			spi_par->high_time = 15;
			spi_par->low_time = 15;
			break;
		case CDFINGER_SPI_4M7:
			spi_par->high_time = 14;
			spi_par->low_time = 14;
			break;
		case CDFINGER_SPI_5M1:
			spi_par->high_time = 13;
			spi_par->low_time = 13;
			break;
		case CDFINGER_SPI_5M5:
			spi_par->high_time = 12;
			spi_par->low_time = 12;
			break;
		case CDFINGER_SPI_6M1:
			spi_par->high_time = 11;
			spi_par->low_time = 11;
			break;
		case CDFINGER_SPI_6M7:
			spi_par->high_time = 10;
			spi_par->low_time = 10;
			break;
		case CDFINGER_SPI_7M4:
			spi_par->high_time = 9;
			spi_par->low_time = 9;
			break;
		case CDFINGER_SPI_8M:
			spi_par->high_time = 8;
			spi_par->low_time = 8;
			break;
		default:
			return -ENOTTY;
	}

	CDFINGER_DBG("spi high_time[%d],low_time[%d]\n",spi_par->high_time,spi_par->low_time);

	return spi_setup(cdfinger->spi);
}

static int cdfinger_read_image(struct cdfinger_data *cdfinger, uint32_t timeout)
{
	int pix_count = 0, ret = 0;
	u8  linenum = 0;

	CDFINGER_FUNCTION("enter\n");
	memset(cdfinger->imagerxpix, 0x00, CDFINGER_RD_PIX_SIZE);
	cdfinger->imagetxcmd[0] = 0x90;
	memset(&cdfinger->imagetxcmd[1], 0x66, (CDFINGER_RD_PIX_SIZE - 1));
	memset(cdfinger->imagebuf, 0x00, CDFINGER_IMAGE_SIZE);
	ret = spi_send_cmd_fifo(cdfinger, cdfinger->imagetxcmd, cdfinger->imagerxpix, CDFINGER_RD_PIX_SIZE);
	if (ret != 0){
		CDFINGER_ERR("read image fail, spi transfer fail\n");
		return -1;
	}
	for (pix_count=0;pix_count<(CDFINGER_RD_PIX_SIZE-CDFINGER_WIDTH-2);pix_count++){
		if(cdfinger->imagerxpix[pix_count] == 0xaa){
			++pix_count;
			linenum = cdfinger->imagerxpix[pix_count];
			memcpy((cdfinger->imagebuf+linenum*CDFINGER_WIDTH),(cdfinger->imagerxpix+(++pix_count)),CDFINGER_WIDTH);
			pix_count+=CDFINGER_WIDTH;
			if (linenum == CDFINGER_HEIGHT-1){
				if(pix_count < CDFINGER_IMAGE_SIZE)
					goto out;
				CDFINGER_DBG("line:%d, pix count index:%d\n",linenum, pix_count);

				return 0;
			}
		}
	}
out:
	CDFINGER_ERR("read image falied! line:%d, pix count index:%d\n",linenum, pix_count);
	return -2;
}

#ifdef ENABLE_SPI_FREQUENCY_CALIBRTION
static int spi_calibration_cdfinger_read_image(struct cdfinger_data *cdfinger, uint32_t timeout)
{
	int pix_count = 0, ret = 0;
	u8  linenum = 0;

	CDFINGER_FUNCTION("enter\n");
	memset(cdfinger->imagerxpix, 0x00, CDFINGER_RD_PIX_SIZE);
	cdfinger->imagetxcmd[0] = 0x90;
	memset(&cdfinger->imagetxcmd[1], 0x66, (CDFINGER_RD_PIX_SIZE - 1));
	memset(cdfinger->imagebuf, 0x00, CDFINGER_IMAGE_SIZE);
	ret = spi_send_cmd_fifo(cdfinger, cdfinger->imagetxcmd, cdfinger->imagerxpix, CDFINGER_RD_PIX_SIZE);
	if (ret != 0){
		CDFINGER_ERR("read image fail, spi transfer fail\n");
		return -1;
	}
	for (pix_count=0;pix_count<(CDFINGER_RD_PIX_SIZE-CDFINGER_WIDTH-2);pix_count++){
		if(cdfinger->imagerxpix[pix_count] == 0xaa){
			++pix_count;
			linenum = cdfinger->imagerxpix[pix_count];
			memcpy((cdfinger->imagebuf+linenum*CDFINGER_WIDTH),(cdfinger->imagerxpix+(++pix_count)),CDFINGER_WIDTH);
			pix_count+=CDFINGER_WIDTH;
			if (linenum == CDFINGER_HEIGHT-1){
				if(pix_count < CDFINGER_IMAGE_SIZE)
					goto out;
				CDFINGER_DBG("line:%d, pix count index:%d\n",linenum, pix_count);

				return pix_count;
			}
		}
	}

	memset(cdfinger->imagerxpix, 0x00, CDFINGER_RD_PIX_SIZE);
	ret = spi_send_cmd_fifo(cdfinger, cdfinger->imagetxcmd, cdfinger->imagerxpix, CDFINGER_RD_PIX_SIZE);
	if (ret != 0){
		CDFINGER_ERR("read image fail, spi transfer fail\n");
		return -1;
	}
	for (pix_count=0;pix_count<(CDFINGER_RD_PIX_SIZE-CDFINGER_WIDTH-2);pix_count++){
		if(cdfinger->imagerxpix[pix_count] == 0xaa){
			++pix_count;
			linenum = cdfinger->imagerxpix[pix_count];
			memcpy((cdfinger->imagebuf+linenum*CDFINGER_WIDTH),(cdfinger->imagerxpix+(++pix_count)),CDFINGER_WIDTH);
			pix_count+=CDFINGER_WIDTH;
			if (linenum == CDFINGER_HEIGHT-1){
				CDFINGER_DBG("line:%d, pix count index:%d\n",linenum, pix_count);
				goto out;
			}
		}
	}
out:
	CDFINGER_ERR("read image falied! line:%d, pix count index:%d\n",linenum, pix_count);
	return -2;
}
#endif

int cdfinger_report_key(struct cdfinger_data *cdfinger, uint8_t arg)
{
	CDFINGER_FUNCTION("enter\n");
	input_report_key(cdfinger->cdfinger_inputdev, KEY_INTERRUPT, !!arg);
	input_sync(cdfinger->cdfinger_inputdev);
	CDFINGER_FUNCTION("exit\n");

	return 0;
}

static long cdfinger_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct cdfinger_data *cdfinger = filp->private_data;
	int ret = 0;

	CDFINGER_FUNCTION("enter\n");
	if(cdfinger == NULL)
	{
		CDFINGER_ERR("%s: fingerprint please open device first!\n", __func__);
		return -EIO;
	}

	mutex_lock(&cdfinger->buf_lock);
	switch (cmd) {
		case CDFINGER_INIT:
			ret = cdfinger_dev_init(cdfinger, arg);
			break;
		case CDFINGER_GETIMAGE:
			ret = cdfinger_read_image(cdfinger, arg);
			break;
		case CDFINGER_INITERRUPT_MODE:
			ret = cdfinger_mode_init(cdfinger,arg,CDFINGER_INTERRUPT_MODE);
			break;
		case CDFINGER_INITERRUPT_FINGERUPMODE:
			ret = cdfinger_mode_init(cdfinger,arg,CDFINGER_FINGER_UP_MODE);
			break;
		case CDFINGER_RELEASE_WAKELOCK:
			cdfinger_release_wakelock(cdfinger);
			break;
		case CDFINGER_INITERRUPT_KEYMODE:
			ret = cdfinger_mode_init(cdfinger,arg,CDFINGER_KEY_MODE);
			break;
		case CDFINGER_CHECK_INTERRUPT:
			ret = cdfinger_check_interrupt(cdfinger);
			break;
		case CDFINGER_SET_SPI_SPEED:
			ret = cdfinger_set_spi_speed(cdfinger,arg);
			break;
		case CDFINGER_REPORT_KEY:
			ret = cdfinger_report_key(cdfinger, arg);
			break;
		case CDFINGER_POWERDOWN:
			cdfinger_set_low_power_mode(cdfinger);
			break;
		default:
			ret = -ENOTTY;
			break;
	}
	mutex_unlock(&cdfinger->buf_lock);
	CDFINGER_FUNCTION("exit\n");

	return ret;
}

static int cdfinger_open(struct inode *inode, struct file *file)
{
	CDFINGER_FUNCTION("enter\n");
	file->private_data = g_cdfinger;
	CDFINGER_FUNCTION("exit\n");

	return 0;
}

static ssize_t cdfinger_write(struct file *file, const char *buff, size_t count, loff_t * ppos)
{
	return -ENOMEM;
}

static int cdfinger_async_fasync(int fd, struct file *filp, int mode)
{
	struct cdfinger_data *cdfinger = g_cdfinger;

	CDFINGER_FUNCTION("enter\n");
	return fasync_helper(fd, filp, mode, &cdfinger->async_queue);
}

static ssize_t cdfinger_read(struct file *file, char *buff, size_t count, loff_t * ppos)
{
	int ret = 0;
	struct cdfinger_data *cdfinger = file->private_data;
	ssize_t status = 0;

	CDFINGER_FUNCTION("enter\n");
	if(cdfinger == NULL)
	{
		CDFINGER_ERR("%s: fingerprint please open device first!\n", __func__);
		return -EIO;
	}
	mutex_lock(&cdfinger->buf_lock);
	ret = copy_to_user(buff, cdfinger->imagebuf, count);
	if (ret) {
		status = -EFAULT;
	}
	mutex_unlock(&cdfinger->buf_lock);
	CDFINGER_FUNCTION("exit\n");

	return status;
}

static int cdfinger_release(struct inode *inode, struct file *file)
{
	struct cdfinger_data *cdfinger = file->private_data;

	CDFINGER_FUNCTION("enter\n");
	if(cdfinger == NULL)
	{
		CDFINGER_ERR("%s: fingerprint please open device first!\n", __func__);
		return -EIO;
	}
	file->private_data = NULL;
	CDFINGER_FUNCTION("exit\n");

	return 0;
}

static const struct file_operations cdfinger_fops = {
	.owner = THIS_MODULE,
	.open = cdfinger_open,
	.write = cdfinger_write,
	.read = cdfinger_read,
	.release = cdfinger_release,
	.fasync = cdfinger_async_fasync,
	.unlocked_ioctl = cdfinger_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = cdfinger_ioctl,
#endif
};

static struct miscdevice cdfinger_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEVICE_NAME,
	.fops = &cdfinger_fops,
};

//clear interrupt
static int cdfinger_clear_interrupt(struct cdfinger_data *cdfinger)
{
	u8 start = 0x18,clc_cmd = 0xA3, read;
	int retry = 10;

	CDFINGER_FUNCTION("enter\n");
	for(; retry>0; retry--)
	{
		if(spi_send_cmd_fifo(cdfinger, &start, &read, 1) < 0)
		{
			CDFINGER_ERR("spi transfer failed!\n");
			msleep(10);
			continue;
		}
		if(spi_send_cmd_fifo(cdfinger, &clc_cmd, &read, 1) < 0)
		{
			CDFINGER_ERR("spi transfer failed!\n");
			msleep(10);
			continue;
		}
		break;
	}
	CDFINGER_DBG("retry = %d\n", 10-retry);
	CDFINGER_FUNCTION("exit\n");

	return 0;
}

static void cdfinger_async_Report(void)
{
	struct cdfinger_data *cdfinger = g_cdfinger;

	CDFINGER_FUNCTION("enter\n");
	kill_fasync(&cdfinger->async_queue, SIGIO, POLL_IN);
	CDFINGER_FUNCTION("exit\n");
}

static void int_timer_handle(unsigned long arg)
{
	struct cdfinger_data *cdfinger = g_cdfinger;

	cdfinger->int_count = 0;
	CDFINGER_DBG("enter\n");
	if ((cdfinger->device_mode == CDFINGER_KEY_MODE) && (cdfinger->key_report == 1)) {
		input_report_key(cdfinger->cdfinger_inputdev, KEY_INTERRUPT, 0);
		input_sync(cdfinger->cdfinger_inputdev);
		cdfinger->key_report = 0;
	}

	if (cdfinger->device_mode == CDFINGER_FINGER_UP_MODE){
		cdfinger->process_interrupt = 0;
		cdfinger_async_Report();
	}
	CDFINGER_DBG("exit\n");
}

static int cdfinger_thread_func(void *arg)
{
	struct cdfinger_data *cdfinger = (struct cdfinger_data *)arg;
	static u8 int_cmd = 0x14,read;

	do {
		wait_event_interruptible(waiter, cdfinger->thread_wakeup != 0);
		CDFINGER_DBG("cdfinger:%s,thread wakeup\n",__func__);
		cdfinger->thread_wakeup = 0;
		cdfinger->int_count++;
		wake_lock_timeout(&cdfinger->cdfinger_lock, 3*HZ);
		cdfinger_clear_interrupt(cdfinger);

		if (cdfinger->int_count >= 2) {
			cdfinger->int_count = 0;
			if (cdfinger->device_mode == CDFINGER_INTERRUPT_MODE) {
				cdfinger->process_interrupt = 0;
				cdfinger_async_Report();
				del_timer_sync(&cdfinger->int_timer);
				continue;
			} else if ((cdfinger->device_mode == CDFINGER_KEY_MODE) && (cdfinger->key_report == 0)) {
				input_report_key(cdfinger->cdfinger_inputdev, KEY_INTERRUPT, 1);
				input_sync(cdfinger->cdfinger_inputdev);
				cdfinger->key_report = 1;
			}
		}
		spi_send_cmd_fifo(cdfinger, &int_cmd, &read, 1);
	}while(!kthread_should_stop());

	CDFINGER_ERR("thread exit\n");
	return -1;
}

static irqreturn_t cdfinger_interrupt_handler(unsigned irq, void *arg)
{
	struct cdfinger_data *cdfinger = (struct cdfinger_data *)arg;

	cdfinger->cdfinger_interrupt = 1;
	if (cdfinger->process_interrupt == 1)
	{
		mod_timer(&cdfinger->int_timer, jiffies + HZ / 10);
		cdfinger->thread_wakeup = 1;
		wake_up_interruptible(&waiter);
	}

	return IRQ_HANDLED;
}

static ssize_t cdfinger_debug_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "cdfinger_debug = %x\n", cdfinger_debug);
}

static ssize_t cdfinger_version_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%s\n", VERSION);
}

static ssize_t cdfinger_debug_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t size)
{
	int data;

	if (buf != NULL)
		sscanf(buf, "%x", &data);

	cdfinger_debug = (u8)data;

	return size;
}

static ssize_t cdfinger_spi_freq_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct mt_chip_conf *spi_par = &spi_conf;
	int freq = 0;

	freq = 134300/(spi_par->high_time + spi_par->low_time);

	return sprintf(buf, "spi frequence[%d], hight_time[%d], low_time[%d]\n", freq, spi_par->high_time, spi_par->low_time);
}

static ssize_t cdfinger_spi_freq_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t size)
{
	struct mt_chip_conf *spi_par;
	struct cdfinger_data *cdfinger = g_cdfinger;
	u32 time = 0;

	if (buf != NULL)
		sscanf(buf, "%d", &time);
	spi_par = &spi_conf;
	if (!spi_par) {
		return -1;
	}
	CDFINGER_DBG("freq time: %d\n",time);

	spi_par->high_time = time;
	spi_par->low_time = time;
	spi_setup(cdfinger->spi);

	return size;
}

static ssize_t cdfinger_config_spi_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t size)
{
	struct cdfinger_data *cdfinger = g_cdfinger;

	CDFINGER_DBG("cdfinger->config_spi_pin = %d\n",cdfinger->config_spi_pin);
	if(cdfinger->config_spi_pin == 1)
	{
		//pinctrl_select_state(cdfinger->fps_pinctrl, cdfinger->cdfinger_spi_miso);
		//pinctrl_select_state(cdfinger->fps_pinctrl, cdfinger->cdfinger_spi_mosi);
		//pinctrl_select_state(cdfinger->fps_pinctrl, cdfinger->cdfinger_spi_sck);
		//pinctrl_select_state(cdfinger->fps_pinctrl, cdfinger->cdfinger_spi_cs);
	}

	return size;
}

static ssize_t cdfinger_reset_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
#ifdef HAS_RESET_PIN
	cdfinger_reset(1);
	return sprintf(buf, "reset success!\n");
#else
    return sprintf(buf, "no reset pin!\n");
#endif
}


static DEVICE_ATTR(debug, S_IRUGO|S_IWUSR, cdfinger_debug_show, cdfinger_debug_store);
static DEVICE_ATTR(version, S_IRUGO, cdfinger_version_show, NULL);
static DEVICE_ATTR(spi_freq, S_IRUGO|S_IWUSR,cdfinger_spi_freq_show, cdfinger_spi_freq_store);
static DEVICE_ATTR(config_spi, S_IWUSR,NULL, cdfinger_config_spi_store);
static DEVICE_ATTR(reset, S_IRUGO, cdfinger_reset_show, NULL);

static struct attribute *cdfinger_attrs [] =
{
	&dev_attr_debug.attr,
	&dev_attr_version.attr,
	&dev_attr_spi_freq.attr,
	&dev_attr_config_spi.attr,
	&dev_attr_reset.attr,
	NULL
};

static struct attribute_group cdfinger_attribute_group = {
	.name = "cdfinger",
	.attrs = cdfinger_attrs,
};

static int cdfinger_create_inputdev(struct cdfinger_data *cdfinger)
{
	cdfinger->cdfinger_inputdev = input_allocate_device();
	if (!cdfinger->cdfinger_inputdev) {
		CDFINGER_ERR("cdfinger->cdfinger_inputdev create faile!\n");
		return -ENOMEM;
	}
	__set_bit(EV_KEY, cdfinger->cdfinger_inputdev->evbit);
	__set_bit(KEY_INTERRUPT, cdfinger->cdfinger_inputdev->keybit);

	cdfinger->cdfinger_inputdev->id.bustype = BUS_HOST;
	cdfinger->cdfinger_inputdev->name = "cdfinger_inputdev";
	if (input_register_device(cdfinger->cdfinger_inputdev)) {
		CDFINGER_ERR("register inputdev failed\n");
		input_free_device(cdfinger->cdfinger_inputdev);
		return -1;
	}

	if(sysfs_create_group(&cdfinger->cdfinger_inputdev->dev.kobj, &cdfinger_attribute_group) < 0)
	{
		CDFINGER_ERR("sysfs create group failed\n");
		input_unregister_device(cdfinger->cdfinger_inputdev);
		cdfinger->cdfinger_inputdev = NULL;
		input_free_device(cdfinger->cdfinger_inputdev);
		return -2;
	}
	
	return 0;
}

static int cdfinger_check_id(struct cdfinger_data *cdfinger)
{
	u8 id_rx[7]={0};
	u8 id_cmd[7] = {0x21,0x66,0x66,0xb5,0x00,0x43,0x44};
#ifndef HAS_RESET_PIN
	static u8 reset = 0x0c;
	static u8 start = 0x18;
	u8 read;

	spi_send_cmd_fifo(cdfinger, &reset, &read, 1);
	spi_send_cmd_fifo(cdfinger, &reset, &read, 1);
	spi_send_cmd_fifo(cdfinger, &start, &read, 1);
#endif
	spi_send_cmd_fifo(cdfinger,id_cmd,id_rx,7);
	CDFINGER_DBG("reg[5] = 0x%x reg[6] = 0x%x\n",id_rx[5], id_rx[6]);
	if((id_rx[5]==0x70)&&(id_rx[6]==0x70))
		return 0;

	return -1;
}

#ifdef ENABLE_SPI_FREQUENCY_CALIBRTION
static int cdfinger_calibration_spi(void *arg)
{
	struct cdfinger_data *cdfinger = (struct cdfinger_data *)arg;
	enum spi_speed speed=CDFINGER_SPI_8M;
	int ret=-1, i=0;

	mutex_lock(&cdfinger->buf_lock);
	for(; speed>=CDFINGER_SPI_4M1; speed--)
	{
		CDFINGER_DBG("ioctl enum %d\n", speed);
		cdfinger_set_spi_speed(cdfinger, speed);
		
		ret = cdfinger_dev_init(cdfinger, 0xe0);
		if (ret < 0)
		{
			CDFINGER_ERR("cdfinger_dev_init failed, ret = %d\n", ret);
			continue;
		}
		ret = spi_calibration_cdfinger_read_image(cdfinger,1);
		if (ret < 0)
		{
			CDFINGER_ERR("spi_calibration_cdfinger_read_image failed, ret = %d\n", ret);
			continue;
		}
		CDFINGER_DBG("read image buffer size: %d\n", ret);
		
		if(ret<CDFINGER_SPI_CALIBRATION_SIZE && ret>CDFINGER_IMAGE_SIZE)
		{
			for(i=0; i<5; i++)
			{
				ret = cdfinger_dev_init(cdfinger, 0xe0);
				if (ret < 0)
				{
					CDFINGER_ERR("cdfinger_dev_init failed, ret = %d\n", ret);
					break;
				}
				ret = spi_calibration_cdfinger_read_image(cdfinger,1);
				if (ret == -2)
				{
					CDFINGER_ERR("spi_calibration_cdfinger_read_image failed, ret = %d\n", ret);
					break;
				}
				if(ret > CDFINGER_SPI_CALIBRATION_SIZE)
					break;
			}
			if(ret < 0 || ret > CDFINGER_SPI_CALIBRATION_SIZE)
				continue;
			CDFINGER_DBG("find spi speed[%d] buffer[%d]\n",speed, ret);
			break;
		}
	}

	cdfinger_set_low_power_mode(cdfinger);
	mutex_unlock(&cdfinger->buf_lock);

	return 0;
}
#endif

static int cdfinger_probe(struct spi_device *spi)
{
	struct cdfinger_data *cdfinger = NULL;
	int status = -ENODEV;

	CDFINGER_DBG("enter\n");
	hct_waite_for_finger_dts_paser();
	cdfinger = kzalloc(sizeof(struct cdfinger_data), GFP_KERNEL);
	if (!cdfinger) {
		CDFINGER_ERR("alloc cdfinger failed!\n");
		return -ENOMEM;;
	}

	mutex_init(&cdfinger->transfer_lock);
	g_cdfinger = cdfinger;
	cdfinger->spi = spi;
	if(cdfinger_parse_dts(cdfinger))
	{
		CDFINGER_ERR("%s: parse dts failed!\n", __func__);
		goto free_cdfinger;
	}

	spi->bits_per_word = 8;
	spi->mode = SPI_MODE_0;
	spi->controller_data = (void *)&spi_conf;
	if(spi_setup(spi) != 0)
	{
		CDFINGER_ERR("%s: spi setup failed!\n", __func__);
		goto free_cdfinger;
	}

	cdfinger_power_on(cdfinger);
#ifdef HAS_RESET_PIN
	cdfinger_reset(1);
#endif
	status = cdfinger_check_id(cdfinger);
	if (status != 0)
	{
		CDFINGER_ERR("cdfinger: check id failed! status=%d\n",status);
		goto power_off;
	}

	cdfinger->imagebuf = (char*)kzalloc(CDFINGER_IMAGE_SIZE*sizeof(char),GFP_KERNEL);
	if (!cdfinger->imagebuf)
	{
		CDFINGER_ERR("%s: imagebuf malloc fail!\n", __func__);
		goto power_off;
	}
	cdfinger->imagetxcmd = (char*)kzalloc(CDFINGER_RD_PIX_SIZE*sizeof(char),GFP_KERNEL);
	if (!cdfinger->imagetxcmd)
	{
		CDFINGER_ERR("%s: imagetxcmd malloc fail!\n", __func__);
		goto free_imagebuf;
	}
	cdfinger->imagerxpix= (char*)kzalloc(CDFINGER_RD_PIX_SIZE*sizeof(char),GFP_KERNEL);
	if (!cdfinger->imagerxpix)
	{
		CDFINGER_ERR("%s: imagerxpix malloc fail!\n", __func__);
		goto free_imagetxcmd;
	}

	mutex_init(&cdfinger->buf_lock);
	wake_lock_init(&cdfinger->cdfinger_lock, WAKE_LOCK_SUSPEND, "cdfinger wakelock");

	status = misc_register(&cdfinger_dev);
	if (status < 0) {
		CDFINGER_ERR("%s: cdev register failed!\n", __func__);
		goto free_imagerxpix;
	}

	if(cdfinger_create_inputdev(cdfinger) < 0)
	{
		CDFINGER_ERR("%s: inputdev register failed!\n", __func__);
		goto free_device;
	}

	init_timer(&cdfinger->int_timer);
	cdfinger->int_timer.function = int_timer_handle;
	add_timer(&cdfinger->int_timer);
	spi_set_drvdata(spi, cdfinger);
	//pinctrl_select_state(cdfinger->fps_pinctrl, cdfinger->cdfinger_irq);
	hct_finger_set_eint(1);

	status = request_threaded_irq(cdfinger->irq, (irq_handler_t)cdfinger_interrupt_handler, NULL,
					IRQF_TRIGGER_RISING | IRQF_ONESHOT, "cdfinger-irq", cdfinger);
	if(status){
		CDFINGER_ERR("request_irq error\n");
		goto free_work;
	}

	enable_irq_wake(cdfinger->irq);
	cdfinger->irq_enabled = 1;
	
	cdfinger->cdfinger_thread = kthread_run(cdfinger_thread_func, cdfinger, "cdfinger_thread");
	if (IS_ERR(cdfinger->cdfinger_thread)) {
		CDFINGER_ERR("kthread_run is failed\n");
		goto free_irq;
	}
#ifdef ENABLE_SPI_FREQUENCY_CALIBRTION
	cdfinger->cdfinger_spi_calibration_thread = kthread_run(cdfinger_calibration_spi, cdfinger, "cdfinger_spi_calibration_thread");
#else
	cdfinger_set_low_power_mode(cdfinger);
#endif

	CDFINGER_DBG("exit\n");

	return 0;

free_irq:
	free_irq(cdfinger->irq, cdfinger);
free_work:
	del_timer(&cdfinger->int_timer);
//free_inputdev:
	sysfs_remove_group(&cdfinger->cdfinger_inputdev->dev.kobj, &cdfinger_attribute_group);
	input_unregister_device(cdfinger->cdfinger_inputdev);
	cdfinger->cdfinger_inputdev = NULL;
	input_free_device(cdfinger->cdfinger_inputdev);
free_device:
	misc_deregister(&cdfinger_dev);
free_imagerxpix:
	wake_lock_destroy(&cdfinger->cdfinger_lock);
	mutex_destroy(&cdfinger->buf_lock);
	kfree(cdfinger->imagerxpix);
	cdfinger->imagerxpix = NULL;
free_imagetxcmd:
	kfree(cdfinger->imagetxcmd);
	cdfinger->imagetxcmd = NULL;
free_imagebuf:
	kfree(cdfinger->imagebuf);
	cdfinger->imagebuf = NULL;
power_off:
	cdfinger_power_off(cdfinger);
free_cdfinger:
	mutex_destroy(&cdfinger->transfer_lock);
	kfree(cdfinger);
	cdfinger = NULL;

	return -1;
}



static int cdfinger_suspend (struct device *dev)
{
	return 0;
}

static int cdfinger_resume (struct device *dev)
{
	return 0;
}

static int cdfinger_remove(struct spi_device *spi)
{
	struct cdfinger_data *cdfinger = spi_get_drvdata(spi);

	kthread_stop(cdfinger->cdfinger_thread);
	free_irq(cdfinger->irq, cdfinger);
	del_timer(&cdfinger->int_timer);
	sysfs_remove_group(&cdfinger->cdfinger_inputdev->dev.kobj, &cdfinger_attribute_group);
	input_unregister_device(cdfinger->cdfinger_inputdev);
	cdfinger->cdfinger_inputdev = NULL;
	input_free_device(cdfinger->cdfinger_inputdev);
	misc_deregister(&cdfinger_dev);
	wake_lock_destroy(&cdfinger->cdfinger_lock);
	mutex_destroy(&cdfinger->buf_lock);
	kfree(cdfinger->imagerxpix);
	cdfinger->imagerxpix = NULL;
	kfree(cdfinger->imagetxcmd);
	cdfinger->imagetxcmd = NULL;
	kfree(cdfinger->imagebuf);
	cdfinger->imagebuf = NULL;
	mutex_destroy(&cdfinger->transfer_lock);
	kfree(cdfinger);
	cdfinger = NULL;
	g_cdfinger = NULL;
	cdfinger_power_off(cdfinger);

	return 0;
}

static const struct dev_pm_ops cdfinger_pm = {
	.suspend = cdfinger_suspend,
	.resume = cdfinger_resume
};

struct of_device_id cdfinger_of_match[] = {
	{ .compatible = "cdfinger,fps1256", },
	{},
};
MODULE_DEVICE_TABLE(of, cdfinger_of_match);

static const struct spi_device_id cdfinger_id[] = {
	{SPI_DRV_NAME, 0},
	{}
};
MODULE_DEVICE_TABLE(spi, cdfinger_id);

static struct spi_driver cdfinger_driver = {
	.driver = {
		.name = SPI_DRV_NAME,
		.bus = &spi_bus_type,
		.owner = THIS_MODULE,
		.pm = &cdfinger_pm,
		.of_match_table = of_match_ptr(cdfinger_of_match),
	},
	.id_table = cdfinger_id,
	.probe = cdfinger_probe,
	.remove = cdfinger_remove,
};

#ifndef DTS_PROBE 
static struct spi_board_info spi_board_cdfinger[] __initdata = {
	[0] = {
		.modalias = "cdfinger",
		.bus_num = 0,
		.chip_select = 0,
		.mode = SPI_MODE_0,
		.max_speed_hz = 6000000,
	},
};
#endif

static int cdfinger_spi_init(void)
{
#ifndef DTS_PROBE 
	spi_register_board_info(spi_board_cdfinger, ARRAY_SIZE(spi_board_cdfinger));
#endif

	return spi_register_driver(&cdfinger_driver);
}

static void cdfinger_spi_exit(void)
{
	spi_unregister_driver(&cdfinger_driver);
}

late_initcall_sync(cdfinger_spi_init);
module_exit(cdfinger_spi_exit);

MODULE_DESCRIPTION("cdfinger spi Driver");
MODULE_AUTHOR("shuaitao@cdfinger.com");
MODULE_LICENSE("GPL");
MODULE_ALIAS("cdfinger");
