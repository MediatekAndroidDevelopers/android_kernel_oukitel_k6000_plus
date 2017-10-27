/*
 * 
 * synchronous driver interface to SPI finger print sensor   
 *
 *	Copyright (c) 2015  ChipSailing Technology.
 *	All rights reserved.
***********************************************************/
  
#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>   
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/compat.h>
#include <linux/spi/spi.h>
#include <asm/uaccess.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h> 
#include <linux/irq.h> 
#include <linux/delay.h>
#include <linux/ktime.h>
#include <linux/jiffies.h>
#include <linux/of_gpio.h>
#include <linux/kthread.h>
#include <mt-plat/mt_gpio.h>
#include <linux/platform_data/spi-mt65xx.h>
#include "../../../spi/mediatek/mt6755/mt_spi.h"
#include "cs15xx-spi.h"
#include<linux/timer.h>
#include <linux/wakelock.h>       
#include<linux/of_irq.h>
#include "../fingerprint.h"

static struct timer_list	csTimer;
static struct timer_list	csFingerDetecTimer;
static struct timer_list	csTimerKick; 
static struct timer_list	csTimerLongTouch;
 
#include <linux/input.h>
#define Driver_Version   "3.0.2"


#define DRIVER_NAME      "cs_spi"                  

#define SPI_CLOCK_SPEED  7000000
#define cs_fp_MAJOR			 155	/* assigned 153*/
#define N_SPI_MINORS	   33//32	/* ... up to 256 */
 
#define  LONGTOUCHTIME   100     //  Seconds  100=1S <----长按时间可以修改这个宏
 
#define IMAGE_SIZE      112*88
#define image_dummy     4
static u8 *image;

//for nav
static int reg_val[10];       //每一位代表0x48寄存器记录下来的8个区域的数值 ，如：值位0xff则表示0~7的位置全有数据1
static int valid_num;
static u16 cs_id;
static int calc_nav_start;
static int csTimerKickCalc = 0;   //csTimerKickCalc统计手指kick数量
//static struct pinctrl *pinctrl1;
//static struct pinctrl_state *eint_as_int, *eint_pulldown, *eint_pulldisable, *rst_output0, *rst_output1;

static unsigned  int finger_status;  //finger_status=1：按下的状态     finger_status=0：未抬起		

static int IRQ_FLAG = 0;
static struct task_struct *cs_irq_thread = NULL;
static u64 kickNum,num1,num2,irqNum,irq_current_num;
static int csTimerOn = 0;   //0:csTimer is off        1:csTimer is on
static int  cs_work_func_finish;
static	u64 tempNum1,tempNum2,temp_currentMs;
static struct fasync_struct *async;
static DECLARE_WAIT_QUEUE_HEAD(waiter);
struct device_node *cs_finger_irq_node;//sunwei
static int cs15xx_create_inputdev(void);
static int  timer_init(void);
static DECLARE_BITMAP(minors, N_SPI_MINORS);
static struct input_dev *cs15xx_inputdev = NULL;
struct cs_fp_data *cs15xx;   
static LIST_HEAD(device_list);
static DEFINE_MUTEX(device_list_lock);
static unsigned bufsiz = 4096;
void forceMode(int mod);
void cs_report_key(int key_type,int key_status);

static int  KEY_MODE_FLAG,NAV_MODE_FLAG;  //0:off 1:on
module_param(bufsiz, uint, S_IRUGO);
MODULE_PARM_DESC(bufsiz, "data bytes in biggest supported SPI message");
 
//for wakelock
//struct wake_lock cs_lock;

extern  int cs_finger_navigation(int pos[],int len);


extern int hct_finger_set_power(int cmd);
extern int hct_finger_get_gpio_info(struct platform_device *pdev);
extern int hct_finger_set_18v_power(int cmd);
extern int hct_finger_set_reset(int cmd);
extern int hct_finger_set_spi_mode(int cmd);
extern int hct_finger_set_eint(int cmd);


struct cs_fp_data {
	              dev_t	               devt;
	              spinlock_t           spi_lock;
	              struct spi_device	   *spi;
                unsigned int         irq;
                unsigned int         gpio_irq;
                unsigned int         gpio_reset;
	              struct list_head	   device_entry;
	              struct  mutex	       buf_lock;
	              unsigned		         users;
	              u8	                *buffer;    /* buffer is NULL unless this device is open (users > 0) */
				  };
 
static struct mt_chip_conf spi_conf = {
	.setuptime = 7,//15,cs
	.holdtime = 7,//15, cs
	.high_time = 16,//6 sck
	.low_time =  17,//6  sck
	.cs_idletime = 3,//20, 
	//.ulthgh_thrsh = 0,
	.cpol = 0,
	.cpha = 0,
	.rx_mlsb = 1,
	.tx_mlsb = 1,
	.tx_endian = 0,
	.rx_endian = 0,
	.com_mod = DMA_TRANSFER,
	//.com_mod = FIFO_TRANSFER,
	.pause = 0,
	.finish_intr = 1,
	.deassert = 0,
	.ulthigh = 0,
	.tckdly = 0,
};

static void cs_fp_complete(void *arg)
{
	complete(arg);
}

static ssize_t cs_fp_sync(struct spi_message *message)
{
	DECLARE_COMPLETION_ONSTACK(done);
	int status;

	message->complete = cs_fp_complete;
	message->context = &done;

	spin_lock_irq(&cs15xx->spi_lock);
	if (cs15xx->spi == NULL)
		status = -ESHUTDOWN;
	else
		status = spi_async(cs15xx->spi, message);
	spin_unlock_irq(&cs15xx->spi_lock);
	if (status == 0) {
		wait_for_completion(&done);
		status = message->status;
		if (status == 0)
			status = message->actual_length;
	}
	return status;
}

static inline ssize_t cs_fp_wrbyte(u8 *buff,int length)      
{
	
	struct spi_message m;
	
	struct spi_transfer t = {
    .cs_change = 0,
		.delay_usecs = 1, 
		.speed_hz = SPI_CLOCK_SPEED,
    .bits_per_word = 8,
		.tx_buf  = buff,
		.len	 = length,
		};
    FUNC_ENTRY();	
	spi_message_init(&m);	
	spi_message_add_tail(&t, &m);
	FUNC_EXIT();
  cs_debug("cs_fp_sync( &m) return = %zx\n",cs_fp_sync( &m));
	return cs_fp_sync( &m);
}


static int cs_read_sfr( u8 addr,u8 *data,u8 rd_count)
{   
 	 int ret ;
	struct spi_message m;
	
	u8 rx[4] = {0}; 
	u8 tx[4] = {0};                        //only rd 2bytes
	struct spi_transfer t ={
    .cs_change = 0,
		.delay_usecs = 1,
		.speed_hz = SPI_CLOCK_SPEED,
    .bits_per_word = 8,
		.tx_buf =tx,
		.rx_buf =rx,
		.len = 2+rd_count,
	};
 //FUNC_ENTRY();
	tx[0] = 0xDD;
	tx[1] = addr;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	
	ret= cs_fp_sync(&m);
	if(ret < 0)
		  return ret;
	for(ret = 0; ret < rd_count; ret++) {  
       data[ret] = rx[2+ret];
     }  
  
	//FUNC_EXIT();
	return data[0];
		
}


static inline void cs_write_sfr( u8 addr, u8 value )
{
  u8 send_buf[3]={0};
	//u8 test[6] = {0}; 
	//cs_fp_wrbyte(send_buf,3);
  struct spi_message m;
	
	struct spi_transfer t = {
    	.cs_change = 0,
			.delay_usecs = 1, 
			.speed_hz = SPI_CLOCK_SPEED,
	    	.bits_per_word = 8,
			.tx_buf  = send_buf,
			.len	 = 3,
		};
  send_buf[0] = 0xCC;
	send_buf[1] = addr;
	send_buf[2] = value;
	spi_message_init(&m);	
	spi_message_add_tail(&t, &m);
	cs_fp_sync(&m);
	//cs_read_sfr(addr,test,1);
	//cs_debug("cs_write_sfr addr=%x value=%x ---- check right?= %x\n",addr,value,test[0]);
//	FUNC_EXIT();
       
}


static void cs_write_sram(u16 addr,u16 value ) 
{
	u8 send_buf[5]={0};
	//cs_fp_wrbyte(send_buf,5);
	struct spi_message m;	
	struct spi_transfer t = {
    	.cs_change = 0,
			.delay_usecs = 1, 
			.speed_hz = SPI_CLOCK_SPEED,
	    .bits_per_word = 8,
			.tx_buf  = send_buf,
			.len	 = 5,
		};
	//FUNC_ENTRY();
	send_buf[0] = 0xAA;
	send_buf[1] = (addr&0xff00)>>8;    //addr_h
	send_buf[2] = addr&0x00ff;         //addr_l
	send_buf[3] = value&0x00ff;  
	send_buf[4] = (value&0xff00)>>8;
	spi_message_init(&m);	
	spi_message_add_tail(&t, &m);
	cs_fp_sync(&m);
  //FUNC_EXIT();	
}

static u16 cs_read_sram(u16 addr,u8 *data,u8 rd_count)
{   

  u16 ret ;
	struct spi_message m;  
	u8 rx[6] = {0}; 
	u8 tx[6] = {0};                         //only rd 2bytes
	
	struct spi_transfer t ={
		.cs_change = 0,
		.delay_usecs = 1,
		.speed_hz = SPI_CLOCK_SPEED,
		.tx_buf =tx,
		.rx_buf =rx,
		.len = 3+rd_count+1,                 //first nop
	};

	tx[0] = 0xBB;
	tx[1] = addr>>8;
	tx[2] = addr;
	
	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	
	ret= cs_fp_sync(&m);
	if(ret < 0)
		return ret;
	for (ret = 0; ret < rd_count; ret++) {  
       data[ret] = rx[4+ret];                 //first nop
      }  
    
	return  (data[1]&0x00ff)<<8|data[0];  
		
}
/*
static void cs_write_sfr_bit(u8 addr,u8 bit,bool vu)
{
	
        u8 buff[2]={0};
	u8 temp=0x00;
	
	
	temp = cs_read_sfr(addr,buff,1);
	
	if(vu)
	{
		temp = temp | (1<<bit);
	}
	else
	{
		temp = temp & (~(1<<bit));	
	}
	cs_write_sfr( addr, temp );
}

*/
static void cs_write_sram_bit(u16 addr,u16 bit,bool vu)
{
  u8 buff[2] ={0};
	u16   temp = 0;

	
	temp = cs_read_sram(addr,buff,2);

	if(vu)
	{
		temp = temp | (1<<bit);
	}
	else
	{
		temp = temp & (~(1<<bit));
	}

	cs_write_sram(addr ,temp);
}


static inline ssize_t cs_fp_rd_id(u8 *buffer) 
{
	int ret;
	ret = cs_read_sfr(0x3E,buffer,2);
	return ret;
}

 
static void cs_fp_reset(unsigned int delay_ms)
{ 
	FUNC_ENTRY();
	/*
	gpio_set_value(GPIO_FINGER_RST, 0);
	mdelay(delay_ms);
	gpio_set_value(GPIO_FINGER_RST, 1);
	*/
//	pinctrl_select_state(pinctrl1, rst_output0);
	hct_finger_set_reset(0);
  msleep(5);
//  pinctrl_select_state(pinctrl1, rst_output1);
  	hct_finger_set_reset(1);
	mdelay(5);
	FUNC_EXIT();  
	 
}

int cs_fp_config(void){	
	//reset IC
	FUNC_ENTRY();
  cs_fp_reset(5);

	cs_write_sfr(0x0F, 0x01);
	cs_write_sfr(0x1C, 0x1D);	
	cs_write_sfr(0x1F, 0x0A);
	cs_write_sfr(0x22, 0x47);
	cs_write_sfr(0x42, 0xAA);		
	cs_write_sfr(0x60, 0x08);     
	cs_write_sfr(0x63, 0x60); 
	//cs_write_sfr(0x47, 0x60);//add 20160712
	//cs_write_sfr(0x13, 0x31);//add 20160712
	//cs_write_sram(0xFC1E, 0x0);//add 20160712
	
  /*****for 3.3V********/
  //cs_write_sfr(0x22, 0x07);
  cs_write_sram(0xFC8C, 0x0001);
  cs_write_sram(0xFC90, 0x0001);
  /*********************/
  
	cs_write_sram(0xFC02, 0x0420);	
	cs_write_sram(0xFC1A, 0x0C30);
	
	cs_write_sram(0xFC22, 0x085C);//cs_write_sram(0xFC22, 0x0848);打开Normal下8个敏感区域	 
	cs_write_sram(0xFC2E, 0x00F9);//cs_write_sram(0xFC2E, 0x008F);	//cs_write_sram(0xFC2E, 0x00F6); 20160624
	cs_write_sram(0xFC06, 0x0039);		
  	cs_write_sram(0xFC08, 0x0008);//add  times    20160624
	cs_write_sram(0xFC0A, 0x0016);
	cs_write_sram(0xFC0C, 0x0022);
	cs_write_sram(0xFC12, 0x002A);
	cs_write_sram(0xFC14, 0x0035);
	cs_write_sram(0xFC16, 0x002B);
	cs_write_sram(0xFC18, 0x0039);
	cs_write_sram(0xFC28, 0x002E);	
	cs_write_sram(0xFC2A, 0x0018);	
	cs_write_sram(0xFC26, 0x282D); 
	cs_write_sram(0xFC30, 0x0270);//cs_write_sram(0xFC30, 0x0260);  //cs_write_sram(0xFC30, 0x0300);	// 20160624
	cs_write_sram(0xFC82, 0x01FF);	
	cs_write_sram(0xFC84, 0x0007);	
	cs_write_sram(0xFC86, 0x0001);	
	
	cs_write_sram_bit(0xFC80, 12,0);
	cs_write_sram_bit(0xFC88, 9, 0);	
	cs_write_sram_bit(0xFC8A, 2, 0);
	cs_write_sram(0xFC8E, 0x3354);
	FUNC_EXIT();
	cs_debug("ChipSailing IC data load succeed !\n");	
	return 0;
}
static u16 cs_read_info(void){	
	u16 hwid,data;
	u8 rx[6] = {0}; 
	 
	printk("[ChipSailing] Fingerprint driver version :%s \n",Driver_Version);	
	//read IC ID	
	cs_read_sfr(0x3F,rx,1);
	hwid = rx[0];
	hwid = hwid<<8;
	cs_read_sfr(0x3E,rx,1);
	hwid = hwid | rx[0];		
	printk("[ChipSailing] read ID : 0x%x \n",hwid);
	
	//read ic mode	
	cs_read_sfr(0x46,rx,1);	
	cs_debug("[ChipSailing] read mode : %x \n",rx[0]);

	//read gain
	data=cs_read_sram(0xFC2E,rx,2);
	printk("[ChipSailing] read gain : %x \n",data);			
	return hwid;
}


//CS chip init
static int cs_init(void)
{	
	int status;
	status = cs_fp_config();
	if(status < 0){
		cs_debug("[ChipSailing] %s : config cs chip data error !!\n ",__func__);
		return -1;
		}	
		cs_debug("[ChipSailing] %s : config cs chip data success !! \n",__func__);
	if(cs_read_info() !=  0xa062){
		cs_error("ChipSailing %s : read cs chip info error !! ",__func__);
		return -1;
		}
	cs_debug("[ChipSailing] %s : read cs chip info  success !!\n ",__func__);
	return 0;
}

/*static int cs_fp_spi_transfer(u8 *tx_cmd1,u8 *tx_cmd2,u8 *rx_data,int length)
{    
     int ret ;
     struct spi_message m;

     struct spi_transfer t1 =
    {
		.cs_change = 1,
		.delay_usecs = 0,
		.speed_hz = SPI_CLOCK_SPEED,
		.tx_buf = tx_cmd1,
		.rx_buf = NULL,
		.len = 5,
	   };
   struct spi_transfer t2 =
    {
    	.cs_change = 0,
		.delay_usecs = 0,
		.speed_hz = SPI_CLOCK_SPEED,
		.tx_buf =tx_cmd2,
		.rx_buf =rx_data,
		.len = length,                          
	   };
     spi_message_init(&m);
     
     spi_message_add_tail(&t1, &m);
     spi_message_add_tail(&t2, &m);
     
     ret= cs_fp_sync( &m);
   
     return ret; 

}*/

static int cs_fp_spi_transfer(u8 *tx_cmd,u8 *rx_data,int length)
{    
     int ret ;
     struct spi_message m;
     struct spi_transfer t ={
                .cs_change = 0,
		.delay_usecs = 1,
		.speed_hz = SPI_CLOCK_SPEED,
		.tx_buf =tx_cmd,
		.rx_buf =rx_data,
		.len = length,                          
	    };
     spi_message_init(&m);
     spi_message_add_tail(&t, &m);

     ret= cs_fp_sync( &m);
   
    return ret; 

}

static int cs_read_image(void)
{   

    	int ret = 0;
//    int totalSize= IMAGE_SIZE+image_dummy;
    	//u8 *image;
    	u8 tx[1024];
    	//u16 num_count;
    	//FUNC_ENTRY();
			//tx = kmalloc(totalSize, GFP_KERNEL);
			//image = kmalloc(10240, GFP_KERNEL);	
					    
			tx[0] = 0xBB;
			tx[1] = 0xff;
			tx[2] = 0xff;
	
      //num_count=0;

     	disable_irq(cs15xx->irq);	

     cs_write_sfr(0x60,0x08);       
     cs_write_sram(0xfc00,0x0003);     

     cs_fp_spi_transfer(tx,image,10240);
     			
	enable_irq(cs15xx->irq);
			//wake_lock_timeout(&cs_lock, 2*HZ); 		
			//kfree(image);  
			//kfree(tx);
			//FUNC_EXIT();
			return ret; 
 }

static inline ssize_t cs_fp_sync_write( size_t len)
{
	struct spi_transfer	t = {
		.cs_change   = 0,
		.delay_usecs = 0,
		.speed_hz    = SPI_CLOCK_SPEED,
		.tx_buf		   = cs15xx->buffer,
		.len	     	 = len,
		};
	struct spi_message	m;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);                 
	return cs_fp_sync( &m);              
}

static inline ssize_t cs_fp_sync_read(size_t len)
{
	struct spi_transfer	t = {
		.cs_change   = 0,
		.delay_usecs = 0,
		.speed_hz    = SPI_CLOCK_SPEED,
		.rx_buf		   = cs15xx->buffer,
		.len		     = len,
		};
	struct spi_message	m;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	return cs_fp_sync(&m);
}

/*-------------------------------------------------------------------------*/

/* Read-only message with current device setup */
static ssize_t
cs_fp_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	
	ssize_t			status = 0;

	/* chipselect only toggles at start or end of operation */
	if (count > bufsiz)
		return -EMSGSIZE;

	cs15xx = filp->private_data;

	mutex_lock(&cs15xx->buf_lock);
	status = cs_fp_sync_read(count);
	if (status > 0) {
		unsigned long	missing;

		missing = copy_to_user(buf, cs15xx->buffer, status);
		if (missing == status)
			status = -EFAULT;
		else
			status = status - missing;
	}
	mutex_unlock(&cs15xx->buf_lock);

	return status;
}

/* Write-only message with current device setup */
static ssize_t cs_fp_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	
	ssize_t			status = 0;
	unsigned long		missing;
	unsigned char buffer[128] = {0};
	
	/* chipselect only toggles at start or end of operation */
	if (count > bufsiz)
		return -EMSGSIZE;

	cs15xx = filp->private_data;

 //mutex_lock(&cs15xx->buf_lock);
	missing = copy_from_user(buffer, buf, count-1);
	if (missing == 0) {
		cs_debug( "[ChipSailing]%s, copy data from user space, failed\n", __func__);
		status = -EFAULT;
	} 		
	
	if(strcmp(buffer,"info") == 0){
		cs_read_info();	
		cs_debug( "[ChipSailing]NAV_MODE_FLAG=%d\n", NAV_MODE_FLAG);
		}
	if(strcmp(buffer,"reset") == 0){		
		cs_fp_config();
			
		}
	if(strcmp(buffer,"poweron") == 0){		
		  cs_fp_reset(5);	
		  cs_debug("cs_id= %x \n",cs_id);
		}
  //mutex_unlock(&cs15xx->buf_lock);
  

	return -1;
}

static int cs_fp_message(struct spi_ioc_transfer *u_xfers, unsigned n_xfers)
{
	struct spi_message	msg;
	struct spi_transfer	*k_xfers;
	struct spi_transfer	*k_tmp;
	struct spi_ioc_transfer *u_tmp;
	unsigned		n, total;
	u8			*buf;
	int			status = -EFAULT;

	spi_message_init(&msg);
	k_xfers = kcalloc(n_xfers, sizeof(*k_tmp), GFP_KERNEL);
	if (k_xfers == NULL)
		return -ENOMEM;

	/* Construct spi_message, copying any tx data to bounce buffer.
	 * We walk the array of user-provided transfers, using each one
	 * to initialize a kernel version of the same transfer.
	 */
	buf = cs15xx->buffer;
	total = 0;
	for (n = n_xfers, k_tmp = k_xfers, u_tmp = u_xfers;
			n;
			n--, k_tmp++, u_tmp++) {
		k_tmp->len = u_tmp->len;

		total += k_tmp->len;
		if (total > bufsiz) {
			status = -EMSGSIZE;
			goto done;
		}

		if (u_tmp->rx_buf) {
			k_tmp->rx_buf = buf;
			if (!access_ok(VERIFY_WRITE, (u8 __user *)
						(uintptr_t) u_tmp->rx_buf,
						u_tmp->len))
				goto done;
		}
		if (u_tmp->tx_buf) {
			k_tmp->tx_buf = buf;
			if (copy_from_user(buf, (const u8 __user *)
						(uintptr_t) u_tmp->tx_buf,
					u_tmp->len))
				goto done;
		}
		buf += k_tmp->len;

		k_tmp->cs_change = !!u_tmp->cs_change;
		k_tmp->bits_per_word = u_tmp->bits_per_word;
		k_tmp->delay_usecs = u_tmp->delay_usecs;
		k_tmp->speed_hz = u_tmp->speed_hz;
#ifdef VERBOSE
		dev_dbg(&cs15xx->spi->dev,
			"  xfer len %zd %s%s%s%dbits %u usec %uHz\n",
			u_tmp->len,
			u_tmp->rx_buf ? "rx " : "",
			u_tmp->tx_buf ? "tx " : "",
			u_tmp->cs_change ? "cs " : "",
			u_tmp->bits_per_word ? : cs15xx->spi->bits_per_word,
			u_tmp->delay_usecs,
			u_tmp->speed_hz ? : cs15xx->spi->max_speed_hz);
#endif
		spi_message_add_tail(k_tmp, &msg);
	}

	status = cs_fp_sync(&msg);
	if (status < 0)
		goto done;

	/* copy any rx data out of bounce buffer */
	buf = cs15xx->buffer;
	for (n = n_xfers, u_tmp = u_xfers; n; n--, u_tmp++) {
		if (u_tmp->rx_buf) {
			if (__copy_to_user((u8 __user *)
					(uintptr_t) u_tmp->rx_buf, buf,
					u_tmp->len)) {
				status = -EFAULT;
				goto done;  
			}
		}
		buf += u_tmp->len;
	}
	status = total;

done:
	kfree(k_xfers);
	return status;
}

static long cs_fp_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int			err = 0;
	int			retval = 0;
	int			ret = 0;
	struct spi_device	*spi;
	u32			tmp;
	unsigned		n_ioc;
	struct spi_ioc_transfer	*ioc;
	/* Check type and command number */
	
	if (_IOC_TYPE(cmd) != SPI_IOC_MAGIC)
		return -ENOTTY;

	/* Check access direction once here; don't repeat below.
	 * IOC_DIR is from the user perspective, while access_ok is
	 * from the kernel perspective; so they look reversed.
	 */
	if (_IOC_DIR(cmd) & _IOC_READ)	
		err = !access_ok(VERIFY_WRITE,(void __user *)arg, _IOC_SIZE(cmd));
	
	if (err == 0 && _IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ,(void __user *)arg, _IOC_SIZE(cmd));
	
	if (err)
		return -EFAULT;

	/* guard against device removal before, or while,
	 * we issue this ioctl.
	 */
	cs15xx = filp->private_data;
	spin_lock_irq(&cs15xx->spi_lock);
	spi = spi_dev_get(cs15xx->spi);
	spin_unlock_irq(&cs15xx->spi_lock);

	if (spi == NULL)
		return -ESHUTDOWN;

	/* use the buffer lock here for triple duty:
	 *  - prevent I/O (from us) so calling spi_setup() is safe;
	 *  - prevent concurrent SPI_IOC_WR_* from morphing
	 *    data fields while SPI_IOC_RD_* reads them;
	 *  - SPI_IOC_MESSAGE needs the buffer locked "normally".
	 */
//	mutex_lock(&cs15xx->buf_lock);

	switch (cmd & 0xff00ffff) {

	 
	   case (SPI_IOC_SCAN_IMAGE & 0xff00ffff):       //(%c)'k'=(%x)4B=(%d)75
				 printk("[chipsailing] cmd====== scan image \n");
				 retval = cs_read_image();
				 ret = __copy_to_user((u8 *)arg, image+4, IMAGE_SIZE);                   //copy to user
	  		 if (ret != 0)
				{
					cs_debug(KERN_WARNING "SPI SENSOR: %s: error copying to user\n", __func__);
					ret = -1;
				}
				input_report_key(cs15xx_inputdev,KEY_F3,1); 
				input_sync(cs15xx_inputdev);
				input_report_key(cs15xx_inputdev,KEY_F3,0);
				input_sync(cs15xx_inputdev);
		 break;
		 
  	 case (SPI_IOC_SENSOR_RESET & 0xff00ffff):
			    cs_debug("[chipsailing] cmd======reset \n");
     	    cs_fp_config();
          forceMode(2);  //set to sleep
     	    cs_debug("[ChipSailing]IOCTL reset sensor ok\n");   
     	    break;
     	    
     case (SPI_IOC_KEYMODE_ON & 0xff00ffff):
					cs_debug("[chipsailing] cmd======SPI_IOC_KEYMODE_ON \n");
					KEY_MODE_FLAG=1;
 	        break;
	   case (SPI_IOC_KEYMODE_OFF & 0xff00ffff):
					cs_debug("[chipsailing] cmd======SPI_IOC_KEYMODE_OFF \n");  
					KEY_MODE_FLAG=0;
	        break;
	   case (SPI_IOC_NAV_ON & 0xff00ffff):
					cs_debug("[chipsailing] cmd======SPI_IOC_NAV_ON \n");
					NAV_MODE_FLAG=1;
 	        break;
	   case (SPI_IOC_NAV_OFF & 0xff00ffff):
					cs_debug("[chipsailing] cmd======SPI_IOC_NAV_OFF \n");  
					NAV_MODE_FLAG=0;
	        break;	 
	   case (SPI_IOC_FINGER_STATUS & 0xff00ffff):
				  __put_user(finger_status,
					(__u32 __user *)arg);
 	   			cs_debug("[chipsailing] cmd======SPI_IOC_FINGER_STATUS finger_status=%d  arg=%ld \n",finger_status,arg); 	
	        break;
	        
	           
	  
	  default:
		      cs_debug("[chipsailing] cmd====== unknow \n");
					/* segmented and/or full-duplex I/O request */
					if (_IOC_NR(cmd) != _IOC_NR(SPI_IOC_MESSAGE(0))|| _IOC_DIR(cmd) != _IOC_WRITE) {
						retval = -ENOTTY;
						cs_debug("[ChipSailing]IOCTL retval = -ENOTTY;\n");  
						break;
		}

		tmp = _IOC_SIZE(cmd);
		if ((tmp % sizeof(struct spi_ioc_transfer)) != 0) {
			retval = -EINVAL;
			cs_debug("[ChipSailing]IOCTL retval = -EINVAL;\n");  
			break;
		}
		
		n_ioc = tmp / sizeof(struct spi_ioc_transfer);
		if (n_ioc == 0){
		    cs_debug("[ChipSailing]IOCTL n_ioc =0;\n");  
			break;
         }
		/* copy into scratch area */
		ioc = kmalloc(tmp, GFP_KERNEL);
		if (!ioc) {
			retval = -ENOMEM;
			cs_debug("[ChipSailing] retval = -ENOMEM;\n");  
			break;
		}
		if (__copy_from_user(ioc, (void __user *)arg, tmp)) {
			kfree(ioc);
			retval = -EFAULT;
			cs_debug("[ChipSailing] retval = -EFAULT;\n");  
			break;
		}

		/* translate to spi_message, execute */
		retval = cs_fp_message(ioc, n_ioc);
		kfree(ioc);
		break;
	}

//	mutex_unlock(&cs15xx->buf_lock);
	spi_dev_put(spi);
 
	return retval;
}

static long cs_fp_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	return cs_fp_ioctl(filp, cmd, (unsigned long)compat_ptr(arg));
}
 

static irqreturn_t cs_irq_handler(int irq, void *dev_id)                                                     
{
	
	wake_up_interruptible(&waiter);
	IRQ_FLAG = 1;	
	return 0;											
	
}


u64  getCurrenMs(void)
{
	if((get_jiffies_64()*10)<0)
		return 0;
	else
	  return (get_jiffies_64()*10);
}

              

void  cs_timer_func(unsigned long arg)
{
	u8 ret=0,i=0;   //记录导航方向
	if(NAV_MODE_FLAG==0){
        csTimerOn=0;     //定时器超时函数执行后设置定时器可以被重新打开
		     return;}
	
	   
     cs_debug(DEFAULT_DEBUG,"csTimer is ok\n");
    // cs_debug("receive data from timer: %ld\n",arg);
     num2=kickNum;
     cs_debug(DEFAULT_DEBUG,"Num1=%lld Num2=%lld,finger_status=%d KEY_MODE_FLAG=%d,NAV_MODE_FLAG=%d\n",num1,num2,finger_status,KEY_MODE_FLAG,NAV_MODE_FLAG);
  
     tempNum2=irqNum;
     //if((KEY_MODE_FLAG == 1))&&(NAV_MODE_FLAG==1))//if( (KEY_MODE_FLAG == 1)&&(finger_status==-1))   //必须在keymode打开而且手指离开后才能上报键值
     {
     //超时后开始计算导航的方向
     cs_debug(DEFAULT_DEBUG,"[ChipSailing]ChipSailing valid_num=%d ..... \n",valid_num);
     ret=cs_finger_navigation(reg_val,valid_num);    //将数组传给导航算法并将方向返回  2-letf  1-right 3-down  4-up  -1:unknow
     for(i=0;i<9;i++) ////计算完成后valid_num和数组值清空 
	 {	  
	 	 reg_val[i]=0x0;
	 } 
     valid_num=0;
	if(ret==2){
   	        	   					input_report_key(cs15xx_inputdev,NAV_LEFT,1);
 
								 input_sync(cs15xx_inputdev);
								 input_report_key(cs15xx_inputdev,NAV_LEFT,0);
								 input_sync(cs15xx_inputdev);
     	           cs_debug(DEFAULT_DEBUG,"[ChipSailing] Direction:--------------------left----------------------   \n");
     	          }  
     	else if(ret==1)   
     		        {
     		         input_report_key(cs15xx_inputdev,NAV_RIGHT,1);
								 input_sync(cs15xx_inputdev);
								 input_report_key(cs15xx_inputdev,NAV_RIGHT,0);
								 input_sync(cs15xx_inputdev);
     		   				cs_debug(DEFAULT_DEBUG,"[ChipSailing] Direction:--------------------right--------------------  \n");
     		   			}
     		else if(ret==3)
     			      {
     			       input_report_key(cs15xx_inputdev,NAV_DOWN,1);
								 input_sync(cs15xx_inputdev);
								 input_report_key(cs15xx_inputdev,NAV_DOWN,0);
								 input_sync(cs15xx_inputdev);
     			        cs_debug(DEFAULT_DEBUG,"[ChipSailing] Direction:--------------------------down--------------------\n");
     			       }
     			else if(ret==4)
     				     {
     				     input_report_key(cs15xx_inputdev,NAV_UP,1);
								 input_sync(cs15xx_inputdev);
								 input_report_key(cs15xx_inputdev,NAV_UP,0);
								 input_sync(cs15xx_inputdev);
     				     cs_debug(DEFAULT_DEBUG,"[ChipSailing] Direction:-------------------------------up--------------------\n");
     				     }
     				else  
     					  {
     						 cs_debug(DEFAULT_DEBUG,"[ChipSailing] Direction:-----UNKNOW-----   \n");
     						}
     if(ret>0){
								 if( (tempNum2-tempNum1)>40 )    //如果在定时器设定时间内超过40次irq中断，说明有手指长按
									  {
										 
										   		 //input_report_key(cs15xx_inputdev,KEY_KICK1S,1);  
													 //input_sync(cs15xx_inputdev);
													 //input_report_key(cs15xx_inputdev,KEY_KICK1S,0);  
													 //input_sync(cs15xx_inputdev);
													 cs_debug(DEFAULT_DEBUG,"[ChipSailing] input_report_key  KEY_LONGTOUCH !\n");
						      	}
	    }
	 }
 	     	num1=kickNum;
		    csTimerOn=0;     //定时器超时函数执行后设置标志表示定时器可以被重新打开
		    
     	
}

/*
static int 	_HOME_KEY= 0;
static int 	_MENU_KEY= 1;
static int 	_BACK_KEY= 2;
static int 	_PWR_KEY= 3; 
static int 	_F11_KEY= 4;
static int 	_F5_KEY = 5;
static int 	_F6_KEY = 6;

*/
void cs_report_key(int key_type,int key_status)
{

	switch(key_type){
								  case 0: 
											input_report_key(cs15xx_inputdev,KEY_HOME,key_status);
						 					input_sync(cs15xx_inputdev);
						 					cs_debug(DEFAULT_DEBUG,"[ChipSailing]  cs input_report_key KEY_HOME \n"); 
								      break;
												    
									case 1: 
											input_report_key(cs15xx_inputdev,KEY_MENU,key_status);
						 					input_sync(cs15xx_inputdev);
						 					cs_debug(DEFAULT_DEBUG,"[ChipSailing] cs input_report_key KEY_MENU \n"); 
								      break;
								   case 2: 
											input_report_key(cs15xx_inputdev,KEY_BACK,key_status);
						 					input_sync(cs15xx_inputdev);
						 					cs_debug(DEFAULT_DEBUG,"[ChipSailing] cs input_report_key KEY_BACK \n"); 
								      break;
								   case 3: 
											input_report_key(cs15xx_inputdev,KEY_POWER,key_status);
						 					input_sync(cs15xx_inputdev);
						 					cs_debug(DEFAULT_DEBUG,"[ChipSailing] cs input_report_key KEY_POWER \n"); 
								      break;
								   case 4: 
											input_report_key(cs15xx_inputdev,KEY_KICK1S,key_status);
						 					input_sync(cs15xx_inputdev);
						 					cs_debug(DEFAULT_DEBUG,"[ChipSailing]cs input_report_key KEY_KICK1S \n"); 
								      break;   
								   case 5: 
											input_report_key(cs15xx_inputdev,KEY_LONGTOUCH,key_status);
						 					input_sync(cs15xx_inputdev);
						 					cs_debug(DEFAULT_DEBUG,"[ChipSailing]cs input_report_key  KEY_LONGTOUCH\n"); 
								      break;
								   case 6: 
											input_report_key(cs15xx_inputdev,KEY_KICK2S,key_status);
						 					input_sync(cs15xx_inputdev);
						 					cs_debug(DEFAULT_DEBUG,"[ChipSailing] cs input_report_key  KEY_KICK2S\n"); 
								      break;  
								      
								    default: break;
								   
								  } 
} 
int BitCount(unsigned int n)
{
	unsigned int cnt = 0;

	while(n>0){
		if((n&1)==1)
			cnt++;
		n>>=1;
	}

	return cnt>7?1:0;       //7为可改变的数据敏感点数量
}

static int force_to_idle(void) {
	 
	int ret = 0;
	int count = 0;
	u8 status = 0x00;
	u8 mode = 0x00;
	switch ( cs_read_sfr(0x46, &mode, 1) ){
	case 0x70:
		break;
	case 0x71:
		do {
			cs_write_sfr(0x46, 0x70);
			if (0x70 == cs_read_sfr(0x46, &mode, 1)) {
				break;
			}
		} while (++count < 3);
		if (count == 3)
			ret = -1;

		count = 0;
		do {
			if (cs_read_sfr(0x50, &status, 1) & 0x01) {
				cs_write_sfr(0x50, 0x01);
				break;
			}
		} while (++count < 1);

		break;
	default:
		do {
			//wakeup();
			cs_write_sfr(0x46, 0x70);
			if (0x70 == cs_read_sfr(0x46, &mode, 1)) {
				break;
			}
		} while (++count < 3);
		if (count == 3)
			ret = -2;

		count = 0;
		do {
			if (cs_read_sfr(0x50, &status, 1) & 0x01) {
				cs_write_sfr(0x50, 0x01);
				break;
			}
		} while (++count < 1);
		break;
	}

	return 0;
}
 
void forceMode(int mod)
 {
		force_to_idle();
    //Setmode to normal
    cs_debug(DEFAULT_DEBUG,"[ChipSailing] forceMode to %d \n",mod);  
    switch(mod)
    {
    	cs_debug("[ChipSailing] forceMode in!\n");
    	case 4:  //deep sleep
      cs_write_sfr(0x46, 0x76);break;
    	case 3:  //normal
      cs_write_sfr(0x46, 0x71);break;
      case 2:  //sleep
      cs_write_sfr(0x46, 0x72);break;
      case 1:  //idle
      cs_write_sfr(0x46, 0x70);break;	
      default: 
      break;
    }
 
    cs_debug(DEFAULT_DEBUG,"[ChipSailing] forceMode success!\n");
} 

 void  csTimerLongTouch_func(unsigned long arg)
 {
	   //手指已经长按下1S后且未离开则执行长按事件上报
	    if(finger_status != 2){
  
     	  	   input_report_key(cs15xx_inputdev,KEY_LONGTOUCH,1);  //KEY_LONGTOUCH
						 input_sync(cs15xx_inputdev);
						 input_report_key(cs15xx_inputdev,KEY_LONGTOUCH,0); //KEY_LONGTOUCH
						 input_sync(cs15xx_inputdev);
						}
		  cs_debug(DEFAULT_DEBUG,"[ChipSailing] cs input_report_key KEY_KICK KEY_LONGTOUCH !\n");
	   
 }
  

 //超时后检测手指是那种kick方式 
 void csTimerKick_func(unsigned long arg)
 {
 	 //static int csTimerKickCalc = 0;   //csTimerKickCalc统计kick总数
 	 printk("csTimerKickCalc=%d \n",csTimerKickCalc);
 	 
 	 switch(csTimerKickCalc)
 	  {
 	  	case 1:	
 	  		     //如果已经判断为单击则删除长按定时器
 	  		     del_timer(&csTimerLongTouch);
 	  		     input_report_key(cs15xx_inputdev,KEY_KICK1S,1);
						 input_sync(cs15xx_inputdev);
						 input_report_key(cs15xx_inputdev,KEY_KICK1S,0);
						 input_sync(cs15xx_inputdev);
						 cs_debug(DEFAULT_DEBUG,"[ChipSailing] cs input_report_key KEY_KICK once  !\n");   
						 break;
		  case 2:
		  	     //如果已经判断为双击也删除长按定时器
  					 del_timer(&csTimerLongTouch);  
			     	 input_report_key(cs15xx_inputdev,KEY_KICK2S,1);
						 input_sync(cs15xx_inputdev);
						 input_report_key(cs15xx_inputdev,KEY_KICK2S,0);
						 input_sync(cs15xx_inputdev); 
						 cs_debug(DEFAULT_DEBUG,"[ChipSailing] cs input_report_key KEY_KICK twice  !\n");	 
						 break;
			case 3:
			       //<---
			       //do something
			       
			       //--->
						 cs_debug(DEFAULT_DEBUG,"[ChipSailing] cs input_report_key KEY_KICK 3 times  !\n");	 
						 break;			 
						
			default:
				      break;			 
 
		}
 
   csTimerKickCalc=0;
 	 
 	}
  

 //超时后检测手指是否离开
void csFingerDetecTimer_func(unsigned long arg)
{ 
	
	//等待cs_work_func_finish走完才执行超时函数
 
		if(cs_work_func_finish==0){
				 	//cs_debug("[ChipSailing] getCurrenMs()-temp_currentMs = %lld !\n",(getCurrenMs()-temp_currentMs));
				 	//if( (getCurrenMs()-temp_currentMs)>13)  //两个手指离开最少间隔>13ms
				 	{
					 	 cs_debug("[ChipSailing] finger_status = %d !\n",finger_status);
					 	 if( finger_status!=2 ) 
						 //kill_fasync(&async, SIGIO, POLL_IN);//发送手指离开signle
						 cs_report_key(_F5_KEY,0);   
						 cs_debug(DEFAULT_DEBUG," ChipSailing finger status -    手指已离开 \n");
						 temp_currentMs=getCurrenMs(); 
						 finger_status = 2;
						 
						 csTimerKickCalc++;
						 del_timer(&csTimerKick);
						 cs_debug(DEFAULT_DEBUG,"ChipSailing del csTimerKick \n");
					   add_timer(&csTimerKick);//添加定时器，定时器开始生效
			       mod_timer(&csTimerKick, jiffies + 50);  //重新设定超时时间100~1S  //66 
					}}	
 
}
  
static int cs_work_func(void *data)
{  
 
	u8 mode = 0x00;
	u8 status = 0x00;
	u8 mode2 = 0x00;
	u8 mode3 = 0x00;
	u8 status3 = 0x00;
	u8 status2 = 0x00;
 
	//testing fngr_present_posl 0x48记录了8（0~7区域）个敏感区域是否有手指的信息0-无   1-有
	u8 reg48_val[2] = {0};     
  u8 i=0;      //给导航功能使用的临时变量
	u64 currentMs1,currentMs2;
	irqNum=0;     //irqNum初始化为0
	csTimerOn=0;  //csTimer默认为关闭状态
 

	do{
		//cs_debug("[ChipSailing]get_jiffies_64=%lld \n",getCurrenMs());
		//cs_debug("[ChipSailing]Waiting for event interuptibel....\n");	
		 
		currentMs1=getCurrenMs();    	
		 
		//cs_debug("[ChipSailing]get_jiffies_before=%lld   \n",currentMs1);
		wait_event_interruptible(waiter, IRQ_FLAG != 0);
		cs_debug(DEFAULT_DEBUG,"[ChipSailing]NAV_MODE_FLAG  =%d  \n",NAV_MODE_FLAG);
		cs_work_func_finish=1;  //cs_work_func开始
		currentMs2=getCurrenMs();	
		//cs_debug("[ChipSailing]get_jiffies_after=%lld \n\n\n",currentMs2);
		printk("[ChipSailing]M time of (currentMs2 - currentMs1) =%lld \n\n\n",(currentMs2 - currentMs1));
 
		disable_irq(cs15xx->irq); 
		//cs_debug("[ChipSailing]Disable_irq now!\n");
		
		status = cs_read_sfr(0x50, &status, 1);	
		mode   = cs_read_sfr(0x46, &mode, 1);
		printk("[ChipSailing]---cs_work_func in--- mode is 0x%x , status is 0x%x  \n",mode,status);	
		cs_read_sfr(0x48,reg48_val,1);
	  cs_debug(DEFAULT_DEBUG,"fngr_present_posl 0x48 value=%x  \n",reg48_val[0]);
    if(status != 0x01)       //忽略强制切换模式（72->70） （71->70)而触发的中断事件被误识别为单击事件
		{
			  if( (currentMs2-currentMs1)>60 )   //200ms   100~1（s）  //20
				{
			   
				 ++kickNum;
				 printk("[ChipSailing] kickNum =%lld  !\n",kickNum);
 		 
				}
				printk("[ChipSailing]Got an irq !\n");
				++irqNum;  //每次收到中断就使产生的中断总数加一	
		}
 
	  //nav
    { 
 	 if( (NAV_MODE_FLAG==1) && (status!=0x01) ){  
		   //forceMode(3);//强制切换到normal下
		   mode3   = cs_read_sfr(0x46, &mode3, 1);
		   status3 = cs_read_sfr(0x50, &status3, 1);
		   cs_debug(DEFAULT_DEBUG,"[ChipSailing]cs_work_func  NAV  mode3 is 0x%x , status3 is 0x%x  \n\n\n",mode3,status3);	
		   if(  (status3 != 0x01)&&(mode3==0x71)){
		   cs_read_sfr(0x48,reg48_val,1);
		   cs_debug(DEFAULT_DEBUG,"NAV_MODE_FLAG 0x48 value=%x  \n",reg48_val[0]);
		  					 }
		   }
  
		
		if(csTimerOn==0)     //如果csTimer没在工作就启动定时器
			{
					add_timer(&csTimer);//添加定时器，定时器开始生效
				  mod_timer(&csTimer, jiffies + 50);  //重新设定超时时间100~1S  //66
				  csTimerOn=1;   //启动定时器后改变标志确保定时器超时前不会被修改jiffies
				  
				  tempNum1=irqNum;
				  //导航模式只工作在normal模式下
				  if( (NAV_MODE_FLAG==1) && (status!=0x01) )
				  	{
				   for(i=0;i<9;i++) //刚启动定时器时给记录导航数据的数组清零
					 {	
					 
					 	  reg_val[i]=0x0;
					 }
					  }
					 i=0;  //清零后i回到0为了后面给导航数组赋值
			}
			
			
			if( (csTimerOn==1) && (i<=9) && (mode==0x71)&&(status != 0x01))     //在定时器开启后一直到超时前采集的数据保存在reg_val数组中(数据为空也保存)
				{
					
					
					
						 //如果相邻两个数据一样则只保留一个
				 		if(valid_num!=0){
				     if( (reg48_val[0] != reg_val[i-1] ) ){
						     	 
									reg_val[i++]=reg48_val[0];
									//cs_debug(DEFAULT_DEBUG,"[ChipSailing]ChipSailing reg48_val[%d]=%d ..... \n",i,reg48_val[0]);
									valid_num=i;    // valid_num表示从定时器开始后到超时前记录的有效导航数据num
						 
						   }
						}
						else{
							reg_val[0]=reg48_val[0];
							valid_num=1;
							i++;
							//cs_debug(DEFAULT_DEBUG,"[ChipSailing]i=0 reg_val[0]=%d \n",reg_val[0]);
						}	
				/* if(reg48_val[0] != reg_val[i-1]){
					reg_val[i++]=reg48_val[0];
					cs_debug(DEFAULT_DEBUG,"[ChipSailing]ChipSailing reg48_val[%d]=%d ..... \n",i,reg48_val[0]);
					valid_num=i;    // valid_num表示从定时器开始后到超时前记录的有效导航数据num
					}*/
				}
	  }
		
		
		//只有在sleep模式下才有按键功能（屏蔽了因切换模式而产生的中断上报键值的情况，有手指触摸产生的中断数量肯定是大于1的）
		//判断手指是初次按下的还是未离开
		 // 在定时器超时前使用del_timer删除定时器(del_timer无需考虑定时器状态都可以使用)
			{	
				del_timer(&csFingerDetecTimer);
				cs_debug(DEFAULT_DEBUG,"ChipSailing del csFingerDetecTimer \n"); 
				//启动20ms定时器来判断手指是否离开
			  add_timer(&csFingerDetecTimer);//添加定时器，定时器开始生效
			  mod_timer(&csFingerDetecTimer, jiffies + 10);  //重新设定超时时间100~1S   1~10ms  
				if( (status != 0x01) && (reg48_val[0]!=0)  )//if( (mode == 0x72) && (status != 0x01) && (reg48_val[0]!=0)  )
					{
						 irq_current_num=irqNum;  //记录当前中断的个数 
							if( ((currentMs2 - currentMs1) >13) && (finger_status==2)  )
							{
								 cs_debug(DEFAULT_DEBUG,"ChipSailing finger status - 手指按下\n");
							     calc_nav_start=1; //可以开始计算导航方向
								 cs_report_key(_F5_KEY,1); 
								 finger_status=1;  //finger_status=1：初次按下状态  finger_status=0：未抬起	 finger_status=2:已离开   
								 
								  
								 //长按判断定时器启动
								 del_timer(&csTimerLongTouch);//添加定时器，定时器开始生效
								 add_timer(&csTimerLongTouch);//添加定时器，定时器开始生效
				  			 mod_timer(&csTimerLongTouch, jiffies + 100);  //重新设定超时时间100~1S  //66
								 
								 
						  }
							else
							{ 
							  cs_debug(DEFAULT_DEBUG," ChipSailing finger status 未抬起 \n");
								finger_status=0;
							} 
					}
     }
      
		if(mode == 0x71 && status == 0x04)
		{ 	
			cs_write_sfr(0x46, 0x70);		
			cs_write_sfr(0x50, 0x01);		
			//BitCount如果>=7个区域有数据则返回1，否则返回0
			//cs_debug(" normal 0x48 value=%x   BitCount=%d  \n",reg48_val[0],BitCount(reg48_val[0]));
			if(NAV_MODE_FLAG==1){  //如果在导航模式下产生的中断则直接上报信号
				 kill_fasync(&async, SIGIO, POLL_IN);	
		 		 cs_debug(DEFAULT_DEBUG,"[ChipSailing]ChipSailing normal NAV_MODE_FLAG kill_fasync ..... \n");
		 		}
		 	else{	
		 		
		 		     cs_debug(DEFAULT_DEBUG,"[ChipSailing]BitCount =%d \n",BitCount(reg48_val[0]));
						if( BitCount(reg48_val[0])==1 )
							{									 
						   	//cs_read_image();
						   	//printk("ChipSailing : get an image ! \n");
					 		 kill_fasync(&async, SIGIO, POLL_IN);	
					 		 printk("[ChipSailing]ChipSailing normal kill_fasync ..... \n");	
					 		 cs_debug(DEFAULT_DEBUG,"[ChipSailing]录入面积合格 ..... \n");
							}	
						 	
						else
							{
							 	//force to normal
							 	cs_debug(DEFAULT_DEBUG,"[ChipSailing]录入面积不够 ..... \n");
						    forceMode(3);		    
							}	
		    	}
		}
		else if(mode == 0x72 && status == 0x08)
		{ 			
			
			cs_write_sfr(0x46, 0x70);		
			cs_write_sfr(0x50, 0x01);	
			//cs_read_image();
			//printk("ChipSailing : get an image ! \n");					
			kill_fasync(&async, SIGIO, POLL_IN);			
			printk("[ChipSailing]ChipSailing sleep kill_fasync ..... \n");		
			
						
		}	
		else
		{
			 
			 cs_write_sfr(0x50, 0x01);	
			cs_debug(DEFAULT_DEBUG,"[ChipSailing]ChipSailing idle kill_fasync ..... \n");			
		}
	  mode2   = cs_read_sfr(0x46, &mode2, 1);
		status2 = cs_read_sfr(0x50, &status2, 1);	
		
		
		cs_debug(DEFAULT_DEBUG,"[ChipSailing]---cs_work_func end ---  mode is 0x%x , status is 0x%x  \n\n\n",mode2,status2);	
		
		
		//cs15xx_irq_active = 0;
		IRQ_FLAG = 0;		
		cs_work_func_finish=0;
		/*if( (NAV_MODE_FLAG==0)&&(csTimerOn==0) )  //如果导航结束而且定时器已经关闭
			{
				//导航方向计算完成后切回sleep状态
		    forceMode(2);
		     
		    cs_debug("[ChipSailing]ChipSailing NAV calc done ..... \n");	
		  }*/
		  
   	enable_irq(cs15xx->irq);	
		cs_debug(DEFAULT_DEBUG,"[ChipSailing]enable_irq now!\n");
		
		}while(!kthread_should_stop());
    return 0;
}

static int cs_fp_fasync(int fd, struct file *filp, int mode)
{
	int ret;	
	ret = fasync_helper(fd, filp, mode, &async);	
	cs_debug("[ChipSailing] : fasync_helper .... \n");
	return ret;
}

static int cs_fp_open(struct inode *inode, struct file *filp)
{
	int			status = -ENXIO;
   
//	mutex_lock(&device_list_lock);

	list_for_each_entry(cs15xx, &device_list, device_entry) {
		if (cs15xx->devt == inode->i_rdev) {
			status = 0;
			break;
		}
	}
	if (status == 0) {
		if (!cs15xx->buffer) {
			cs15xx->buffer = kmalloc(bufsiz, GFP_KERNEL);
			if (!cs15xx->buffer) {
				dev_dbg(&cs15xx->spi->dev, "open/ENOMEM\n");
				status = -ENOMEM;
			}
		}
		if (status == 0) {
			cs15xx->users++;
			filp->private_data = cs15xx;
			nonseekable_open(inode, filp);
		}
	} else
		pr_debug("[ChipSailing]: nothing for minor %d\n", iminor(inode));

//	mutex_unlock(&device_list_lock);	

	return status;
}

// Register interrupt device
static int cs15xx_create_inputdev(void)
{
  cs15xx_inputdev = input_allocate_device();
  if (!cs15xx_inputdev)
   {
    cs_debug("[ChipSailing]cs15xx_inputdev create faile!\n");
    return -ENOMEM;
   }
   
    __set_bit(EV_KEY,cs15xx_inputdev->evbit);
//    __set_bit(KEY_KICK1S,cs15xx_inputdev->keybit);
//    __set_bit(KEY_KICK2S,cs15xx_inputdev->keybit);
//    __set_bit(KEY_LONGTOUCH,cs15xx_inputdev->keybit);
//    __set_bit(KEY_POWER,cs15xx_inputdev->keybit);
      
//    __set_bit(KEY_HOME,cs15xx_inputdev->keybit);
//    __set_bit(KEY_BACK,cs15xx_inputdev->keybit);
//    __set_bit(KEY_MENU,cs15xx_inputdev->keybit);
    //__set_bit(KEY_F11,cs15xx_inputdev->keybit);  
    //__set_bit(KEY_F6,cs15xx_inputdev->keybit); 
   //__set_bit(KEY_F5,cs15xx_inputdev->keybit);   
    
//    __set_bit(NAV_UP,cs15xx_inputdev->keybit);
//    __set_bit(NAV_DOWN,cs15xx_inputdev->keybit);
//    __set_bit(NAV_LEFT,cs15xx_inputdev->keybit);
//    __set_bit(NAV_RIGHT,cs15xx_inputdev->keybit);
      __set_bit(KEY_F3,cs15xx_inputdev->keybit);    
 
   cs15xx_inputdev->id.bustype = BUS_HOST;
   cs15xx_inputdev->name = "cs15xx_inputdev";
   cs15xx_inputdev->id.version = 1;
    
    
   if (input_register_device(cs15xx_inputdev))
    {
      cs_debug("[ChipSailing]:register inputdev failed\n");
      input_free_device(cs15xx_inputdev);
      return -ENOMEM;
    }
    else
    {
			cs_debug("[ChipSailing]:register inputdev success!\n");
      return 0;
		 }
}
		
static int cs_fp_release(struct inode *inode, struct file *filp)
{
	
	int			status = 0;

//	mutex_lock(&device_list_lock);
	cs15xx = filp->private_data;
	filp->private_data = NULL;

	/* last close? */
	cs15xx->users--;
	if (!cs15xx->users) {
		int		dofree;

		kfree(cs15xx->buffer);
		cs15xx->buffer = NULL;

		/* ... after we unbound from the underlying device? */
		spin_lock_irq(&cs15xx->spi_lock);
		dofree = (cs15xx->spi == NULL);
		spin_unlock_irq(&cs15xx->spi_lock);

	if (dofree)
			kfree(cs15xx);
	}
//	mutex_unlock(&device_list_lock);

	return status;
}



/* REVISIT switch to aio primitives, so that userspace
 * gets more complete API coverage.  It'll simplify things
 * too, except for the locking.
 */
static const struct file_operations cs_fp_fops = {
	.owner =	THIS_MODULE,
	.write =	cs_fp_write,
	.read =		cs_fp_read,
	.unlocked_ioctl = cs_fp_ioctl,
	.compat_ioctl = cs_fp_compat_ioctl,
	.open =		cs_fp_open,
	.release =	cs_fp_release,
	.llseek =	no_llseek,
	.fasync =	cs_fp_fasync,
};

static struct class *cs_fp_class;

/*-------------------------------------------------------------------------*/
static int cs_gpio_init(void){
 //   int ret;

	cs_debug("ChipSailing : %s \n",__func__);
	if(cs15xx == NULL)
	{
		cs_debug(" pdev = NULL !\n");
		return -1;
	}
	cs15xx->spi->dev.of_node  = of_find_compatible_node(NULL, NULL,"mediatek,hct_finger");
	cs_finger_irq_node				= cs15xx->spi->dev.of_node;
	if(!(cs_finger_irq_node)){
		cs_debug("ChipSailing : of_find_compatible_node error\n");
		return -1;
	}
	//cs_debug("of_find_compatible_node ok\n");
//	pinctrl1 = devm_pinctrl_get(&cs15xx->spi->dev);

	
//	hct_finger_get_gpio_info(cs15xx->spi);
	
	hct_finger_set_18v_power(1);
	hct_finger_set_reset(1);
	hct_finger_set_eint(1);
	hct_finger_set_spi_mode(1);
/*	
	if (IS_ERR(pinctrl1)) {
		ret = PTR_ERR(pinctrl1);
		cs_debug(" .ret = %d\n",ret);
		dev_err(&cs15xx->spi->dev, "fwq Cannot find fingerprint pinctrl1!\n");
		return ret;
	}
	eint_as_int = pinctrl_lookup_state(pinctrl1, "fingerprint_pin_irq");
	if (IS_ERR(eint_as_int)) {
		ret = PTR_ERR(eint_as_int);
		dev_err(&cs15xx->spi->dev, "fwq Cannot find fingerprint pinctrl fingerprint_pin_irq!\n");
		return ret;
	}
	rst_output0 = pinctrl_lookup_state(pinctrl1, "fingerprint_reset_low");
	if (IS_ERR(rst_output0)) {
		ret = PTR_ERR(rst_output0);
		dev_err(&cs15xx->spi->dev, "fwq Cannot find  ngerprint pinctrl fingerprint_reset_low!\n");
		return ret;
	}
	rst_output1 = pinctrl_lookup_state(pinctrl1, "fingerprint_reset_high");
	if (IS_ERR(rst_output1)) {
		ret = PTR_ERR(rst_output1);
		dev_err(&cs15xx->spi->dev, "fwq Cannot find  ngerprint pinctrl fingerprint_reset_high!\n");
		return ret;
	}
	eint_pulldown = pinctrl_lookup_state(pinctrl1, "fingerprint_eint_pull_down");
	if (IS_ERR(eint_pulldown)) {
		ret = PTR_ERR(eint_pulldown);
		dev_err(&cs15xx->spi->dev, "fwq Cannot find  ngerprint pinctrl state_eint_pulldown!\n");
		return ret;
	}
	eint_pulldisable = pinctrl_lookup_state(pinctrl1, "fingerprint_eint_pull_disable");
	if (IS_ERR(eint_pulldisable)) {
		ret = PTR_ERR(eint_pulldisable);
		dev_err(&cs15xx->spi->dev, "fwq Cannot find  ngerprint pinctrl state_eint_pulldisable!\n");
		return ret;
	}
*/
	cs_debug("[Chipsailing] mt_fingerprint_pinctrl ok----------\n");
	return 0;

}

static int   cs_fp_probe(struct spi_device *spi)
{
	//int 		err;
	int			status;
	unsigned long		minor;

  //u32 ints[2]={0}; 
 	cs_debug("ChipSailing : %s \n",__func__);
 	 	/* Allocate driver data */
	cs15xx = kzalloc(sizeof(*cs15xx), GFP_KERNEL);
	if (!cs15xx)
		return -ENOMEM;


	spin_lock_init(&cs15xx->spi_lock);
	mutex_init(&cs15xx->buf_lock);
	mutex_lock(&device_list_lock);
  INIT_LIST_HEAD(&cs15xx->device_entry);


/*Initialize  SPI parameters.*/	
	cs15xx->spi = spi;
	cs15xx->spi->mode = SPI_MODE_0;
	cs15xx->spi->bits_per_word = 8;
	//cs15xx->spi->chip_select   = 0;
	if(spi_setup(cs15xx->spi) < 0 )
    {
     cs_debug(KERN_INFO "cs15xx setup failed");
     return -ENODEV;
    }
  
	//gpio init
	cs_gpio_init();
	//hw_reset
	cs_fp_reset(5);
	
	//read id
	cs_id = cs_read_info();   
	//if read id right then go on probe
	if(cs_id != 0xa062){
		if(cs15xx != NULL)
			kfree(cs15xx);
		cs_debug("cs_fp_probe read id failed,id:0xa062,read: %x\n",cs_id);	
		return -1;
	}
	image = kmalloc(10240, GFP_KERNEL);
 	cs_debug("[ChipSailing]read chip id is: %x  	\n",cs_id);
 	 /* Initialize the driver data */
	BUILD_BUG_ON(N_SPI_MINORS > 256);
	status = register_chrdev(cs_fp_MAJOR, "cs_spi", &cs_fp_fops);
  cs_debug("[ChipSailing]cs_fp_init register_chrdev state=%d\n ",status);
	if (status < 0)
		return status;

	cs_fp_class = class_create(THIS_MODULE, "cs_spi");
	if (IS_ERR(cs_fp_class)) {	
		unregister_chrdev(cs_fp_MAJOR, DRIVER_NAME );
		return PTR_ERR(cs_fp_class);
	}
  cs_debug("[ChipSailing]create class success!\n");

	minor = find_first_zero_bit(minors, N_SPI_MINORS);
	if (minor < N_SPI_MINORS) {
		struct device *dev;

		cs15xx->devt = MKDEV(cs_fp_MAJOR, minor);
		dev = device_create(cs_fp_class, &spi->dev, cs15xx->devt,cs15xx, "cs_spi");
		status = IS_ERR(dev) ? PTR_ERR(dev) : 0;
	} else {
		dev_dbg(&spi->dev, "no minor number available!\n");
		status = -ENODEV;
	}
	 cs_debug("[ChipSailing] Create cs_spi node success!\n");
	if (status == 0) {
		set_bit(minor, minors);
		list_add(&cs15xx->device_entry, &device_list);
	}

  	cs_init();
  	
	cs_debug("[ChipSailing]init device file !\n");
	if (status == 0)
		spi_set_drvdata(spi, cs15xx);
	else{
		if(cs15xx != NULL)
		kfree(cs15xx);
	}
	cs15xx->irq = irq_of_parse_and_map(cs_finger_irq_node,0);
	cs_debug(DEFAULT_DEBUG,"cs15xx->irq num=%d\n",cs15xx->irq);
	if(request_irq(cs15xx->irq,cs_irq_handler,IRQF_TRIGGER_RISING,"finger_print",NULL)){
		cs_debug(DEFAULT_DEBUG,"cs finger sensor ---IRQ LINE NOT AVAILABLE!!\n");	
	}

	cs_debug("[ChipSailing]init device file success!\n");
	//for IC Initialize 
	cs15xx_create_inputdev();

  cs_debug("[ChipSailing]cs_init success !\n");

	mutex_unlock(&device_list_lock);
	cs_debug("[ChipSailing]Enter creating thread...!\n");
	cs_irq_thread =kthread_run(cs_work_func,NULL,"cs_thread");
  if(IS_ERR(cs_irq_thread)) {
	      cs_debug("[ChipSailing] Failed to create kernel thread: %ld\n", PTR_ERR(cs_irq_thread));
	}
	
  kickNum = 0;
  num1 = 0;
  num2 = 0;
	timer_init();
	KEY_MODE_FLAG=1; //key模式默认打开
 
  FUNC_EXIT();
	return status;
}

static int  cs_fp_remove(struct spi_device *spi)
{
	struct cs_fp_data	*cs15xx = spi_get_drvdata(spi);

	if(image != NULL)
		kfree(image);
	/* make sure ops on existing fds can abort cleanly */
	spin_lock_irq(&cs15xx->spi_lock);
	cs15xx->spi = NULL;
	spi_set_drvdata(spi, NULL);
	spin_unlock_irq(&cs15xx->spi_lock);

	/* prevent new opens */
	mutex_lock(&device_list_lock);
	
	list_del(&cs15xx->device_entry);
	device_destroy(cs_fp_class, cs15xx->devt);
	//wake_lock_destroy(&cs_lock);
	clear_bit(MINOR(cs15xx->devt), minors);
	if (cs15xx->users == 0)
		 kfree(cs15xx);
	mutex_unlock(&device_list_lock);

	return 0;
}
/*-------------------------------------------------------------------------*/

/* The main reason to have this class is to make mdev/udev create the
 * /dev/cs15xxB.C character device nodes exposing our userspace API.
 * It also simplifies memory management.
 */

static struct of_device_id cs_fp_match_table[] = 
{
    {.compatible = "cs,cs_spi",},
    {},
};
static struct spi_driver cs_fp_spi_driver = {
	.driver = {
		  .name   =	DRIVER_NAME,
		  .owner  =	THIS_MODULE,
      .of_match_table = cs_fp_match_table,  
	    },
	.probe =	cs_fp_probe,
	.remove =	cs_fp_remove,
};




   
static int  timer_init(void)
{
    cs_debug("hello,world\n");
    init_timer(&csTimer);     //初始化定时器
    cs_debug("[timer]init timer finish!\n");
    csTimer.expires = jiffies+1;//设定超时时间，100代表1秒
    csTimer.data = 60;    //传递给定时器超时函数的值
    csTimer.function = cs_timer_func;//设置定时器超时函数
    
    init_timer(&csFingerDetecTimer);     //初始化定时器
    cs_debug("[timer]init csFingerDetecTimer finish!\n");
    csFingerDetecTimer.expires = jiffies+1;//设定超时时间，100代表1秒
    csFingerDetecTimer.data = 60;    //传递给定时器超时函数的值
    csFingerDetecTimer.function = csFingerDetecTimer_func;//设置定时器超时函数
    
     //timer for Kick
    init_timer(&csTimerKick);     //初始化定时器
    cs_debug(DEFAULT_DEBUG,"[timer]init csTimerKick finish!\n");
    csTimerKick.expires = jiffies+1;//设定超时时间，100代表1秒
    csTimerKick.data = 60;    //传递给定时器超时函数的值
    csTimerKick.function = csTimerKick_func;//设置定时器超时函数
    
    //timer for  csTimerLongTouch
    init_timer(&csTimerLongTouch);     //初始化定时器
    cs_debug(DEFAULT_DEBUG,"[timer]init csTimerLongTouch finish!\n");
    csTimerLongTouch.expires = jiffies+LONGTOUCHTIME;//设定超时时间，100代表1秒
    csTimerLongTouch.data = 60;    //传递给定时器超时函数的值
    csTimerLongTouch.function = csTimerLongTouch_func;//设置定时器超时函数
    
    
    
    cs_debug("[timer]add timer success!\n");
    return 0;
}

static void  timer_exit (void)
 
{
    del_timer(&csTimer);//卸载模块时，删除定时器
    del_timer(&csFingerDetecTimer);//卸载模块时，删除定时器
    del_timer(&csTimerKick);//卸载模块时，删除定时器
    del_timer(&csTimerLongTouch);//卸载模块时，删除定时器
    cs_debug("Hello module exit\n");
}   
 
static struct spi_board_info spi_board_chipsailing[] __initdata = {
	[0] = {
		.modalias= DRIVER_NAME,
		.bus_num = 0,
		.chip_select=FINGERPRINT_CS15XX_CS,
		.mode = SPI_MODE_0,
		.controller_data = &spi_conf,
	},
};

static int __init cs_fp_init(void)
{
	int status;       
	FUNC_ENTRY();
	spi_register_board_info(spi_board_chipsailing,ARRAY_SIZE(spi_board_chipsailing));
   cs_debug("[ChipSailing]spi_register_board_info finished\n");
	status = spi_register_driver(&cs_fp_spi_driver);
  cs_debug("[ChipSailing]cs_fp_init spi_register_driver state=%d\n ",status);
	if (status < 0) {
			cs_debug("cs register================\n");
		class_destroy(cs_fp_class);
		unregister_chrdev(cs_fp_MAJOR, DRIVER_NAME );
	}
   cs_debug("[ChipSailing]cs_fp_init success\n");
   FUNC_EXIT();
	return status;
}

static void __exit cs_fp_exit(void)
{
  FUNC_ENTRY();
  spi_unregister_driver(&cs_fp_spi_driver);
  class_destroy(cs_fp_class);
  unregister_chrdev(cs_fp_MAJOR, DRIVER_NAME );

	free_irq(cs15xx->irq, cs15xx->spi);
  //gpio_free(cs15xx->gpio_reset);
	//gpio_free(cs15xx->gpio_irq);
	timer_exit();
 
	
  FUNC_EXIT();
}

module_init(cs_fp_init);
module_exit(cs_fp_exit);

MODULE_AUTHOR("ChipSailing Technology");
MODULE_DESCRIPTION("User mode SPI device interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:cs15xx");

