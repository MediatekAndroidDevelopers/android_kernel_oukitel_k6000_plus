/* MicroArray Fprint
 * madev.c
 * date: 2016-02-20
 * version: v2.3
 * Author: czl
 */

#include <asm/memory.h>
#include <asm/uaccess.h>
#include <asm/dma.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/compat.h>
#include <linux/spi/spi.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/input.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/kobject.h> 
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/wakelock.h>
#include "print.h"
#include "madev.h"
//#include "plat-general.h"
#include "plat-mtk.h"
#include "../fingerprint.h"


struct fprint_dev {
	dev_t idd;
	int major;
	int minor;
	struct cdev *chd;
	struct class *cls;
	struct device *dev;
};
static struct fprint_dev *sdev = NULL;

struct fprint_spi {
	u8 do_what;				//工作内容
	u8 f_wake;				//唤醒标志	
	int value;				
	volatile u8 f_irq;		//中断标志
	volatile u8 f_repo;		//上报开关	
	spinlock_t spi_lock;
	struct spi_device *spi;
	struct list_head dev_entry;
	struct spi_message msg;
	struct spi_transfer xfer;
	struct input_dev *input;
	struct work_struct work;
	struct workqueue_struct *workq;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend suspend;
#endif
	struct wake_lock wl;
};
static struct fprint_spi *smas = NULL;
static u8 *stxb = NULL;
static u8 *srxb = NULL;

static LIST_HEAD(dev_list);
static DEFINE_MUTEX(dev_lock);
static DEFINE_MUTEX(drv_lock);
static DECLARE_WAIT_QUEUE_HEAD(drv_waitq);

int mas_sync(u8 *txb, u8 *rxb, int len);

static void mas_work(struct work_struct *pws) {
	printd("%s: start.\n", __func__);
		
	wake_up_interruptible(&drv_waitq);
}

static irqreturn_t mas_interrupt(int irq, void *dev_id) {
	printd("%s: start. f_irq=%d f_repo=%d\n", __func__, smas->f_irq, smas->f_repo);

	if(smas->f_irq==FALSE) { //判断中断触发
		smas->f_irq = TRUE; 
		if(smas->f_repo==TRUE) queue_work(smas->workq, &smas->work);
	}

	printd("%s: end. f_irq=%d f_repo=%d\n", __func__, smas->f_irq, smas->f_repo);

	return IRQ_HANDLED;
}

/*---------------------------------- fops ------------------------------------*/
static int mas_open(struct inode *inode, struct file *filp) {
	int ret = -ENXIO;

	printd("%s: start\n", __func__);

	mutex_lock(&dev_lock);
	list_for_each_entry(smas, &dev_list, dev_entry) {
		if (sdev->idd == inode->i_rdev) {
			ret = 0;
			break;
		}
	}
	if (!ret) nonseekable_open(inode, filp);
	else printw("%s: nothing for minor\n", __func__);
	mutex_unlock(&dev_lock);
		
	printd("%s: end ret=%d\n", __func__, ret);

	return ret;
}

/* 读写数据
 * @buf 数据
 * @len 长度
 * @返回值：0成功，否则失败
 */
int mas_sync(u8 *txb, u8 *rxb, int len) {
	int ret;

	//printd("%s: start. cmd=0x%.2x len=%d\n", __func__, buf[0], len);

	mutex_lock(&drv_lock);
	plat_tansfer(smas->spi, len);
	smas->xfer.tx_nbits=SPI_NBITS_SINGLE;
	smas->xfer.tx_buf = txb;
	smas->xfer.rx_nbits=SPI_NBITS_SINGLE;
	smas->xfer.rx_buf = rxb;
	smas->xfer.delay_usecs = 1;
	smas->xfer.len = len;
	smas->xfer.bits_per_word = 8;
	smas->xfer.speed_hz = smas->spi->max_speed_hz;
	spi_message_init(&smas->msg);
	spi_message_add_tail(&smas->xfer, &smas->msg);
	ret = spi_sync(smas->spi, &smas->msg);
	mutex_unlock(&drv_lock);

	printw("%s: end. ret=%d\n", __func__, ret);

	return ret;
}

/* 连接设备
 * @return 1成功，0失败
 */
int mas_connect(void) {
	int i, len=4, ret = 0;
	const unsigned char cdat[] = { 0x41, 0x78, 0x4d, 0x41 };

	//printd("%s: start. cmd=0x%.2x par=0x%.2x\n", __func__, cmd, par);

	stxb[0] = 0x8c;
	stxb[1] = 0xff;
	stxb[2] = 0xff;
	stxb[3] = 0xff;
	mas_sync(stxb, srxb, len); //reset
	msleep(5);
	for(i=2; i<4; i++) {	
		stxb[0] = i*4;
		stxb[1] = cdat[i];
		ret = mas_sync(stxb, srxb, len);
		if(ret!=0) return 0;
		if(srxb[2]!=cdat[i]) return 0;
	}

	return 1;
}

/* 读数据
 * @return 成功:count, -1count太大，-2通讯失败, -3拷贝失败
 */
static ssize_t mas_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
	int val, ret = 0;

	printw("%s: start. count=%d\n", __func__, count);
		
	if(count>FBUF) {
		printw("%s: too long. count=%dn", __func__, count);
		return -1;
	}
	ret = mas_sync(stxb, srxb, count);	
	if(ret) {
		printw("%s: mas_sync failed.\n", __func__);
		return -2;
	}
	ret = copy_to_user(buf, srxb, count);
	if(!ret) val = count;
	else {
		val = -3;
		printw("%s: copy_to_user failed.\n", __func__);
	}		

	printw("%s: end. ret=%d\n", __func__, ret);

	return val;
}

int mas_ioctl(int cmd, int arg) {
	int ret = 0;

	printw("%s: start cmd=0x%.3x arg=%d\n", __func__, cmd, arg);

	switch (cmd) {
	case IOCTL_DEBUG:	
		sdeb = (u8) arg;
		break;
	case IOCTL_IRQ_ENABLE:
		//plat_enable_irq(smas->spi, arg);
		break;
	case IOCTL_SPI_SPEED: //设置SPI速度
		smas->spi->max_speed_hz = (u32) arg;
		spi_setup(smas->spi);
		break;	
	case IOCTL_COVER_NUM: 
		ret = COVER_NUM;
		break;
	case IOCTL_GET_VDATE:
		ret = 20160425;
		break;
	case IOCTL_CLR_INTF:
		smas->f_irq = FALSE;
		break;
	case IOCTL_GET_INTF:
		ret = smas->f_irq;
		break;
	case IOCTL_REPORT_FLAG:
		smas->f_repo = arg;
		break;	
	case IOCTL_SET_WORK:		
		smas->do_what = arg;			
		break;	
	case IOCTL_GET_WORK:
		ret = smas->do_what;
		break;
	case IOCTL_SET_VALUE:
		smas->value = arg;
		break;
	case IOCTL_GET_VALUE:
		ret = smas->value;
		break;
	case IOCTL_TRIGGER:
		smas->f_wake = TRUE;
		wake_up_interruptible(&drv_waitq);
		break;
	case IOCTL_WAKE_LOCK:
		if (!wake_lock_active(&smas->wl)) wake_lock(&smas->wl);
		break;
	case IOCTL_WAKE_UNLOCK:
		if (wake_lock_active(&smas->wl)) wake_unlock(&smas->wl);
		break;
	case IOCTL_KEY_DOWN:
		printw("mas_ioctl send KEY_f11 up\n");
		input_report_key(smas->input, KEY_F11, 1);
		input_report_key(smas->input, KEY_F11, 0);
		input_sync(smas->input);
		break;
	case IOCTL_KEY_UP:
		printw("mas_ioctl send KEY_f11 down\n");

		break;	
	}

	printw("%s: end. ret=%d f_irq=%d, f_repo=%d\n", __func__, ret, smas->f_irq, smas->f_repo);

	return ret;
}

/* 写数据
 * @return 成功:count, -1count太大，-2拷贝失败
 */
static ssize_t mas_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {
	int val, ret;

	printd("%s: start. count=%d\n", __func__, count);

	if(count==6) { //命令
		int cmd, arg;
		u8 tmp[6];
		ret = copy_from_user(tmp, buf, count);
		cmd = tmp[0];
		cmd <<= 8;
		cmd += tmp[1];
		arg = tmp[2];
		arg <<= 8;
		arg += tmp[3];
		arg <<= 8;
		arg += tmp[4];
		arg <<= 8;
		arg += tmp[5];
		printd("%s: cmd=0x%.3x arg=%d\n", __func__, cmd, arg);
		val = mas_ioctl(cmd, arg);
	} else {		
		if(count>FBUF) {
			printw("%s: write data too long.\n", __func__);
			return -1;
		}	
		memset(stxb, 0, FBUF);
		ret = copy_from_user(stxb, buf, count);		
		if(ret) {
			printw("%s: copy_from_user failed.\n", __func__);
			val = -2;
		} else {
			val = count;
		//	printn(__func__, stxb, count);
		}		
	}

	printd("%s: end. ret =%d\n", __func__, ret);

	return val;
}

static int mas_release(struct inode *inode, struct file *filp) {
	int ret = 0;

	printd("%s: start\n", __func__);

	printd("%s: end\n", __func__);

	return ret;
}

static unsigned int mas_poll(struct file *filp, struct poll_table_struct *wait) {
	unsigned int mask = 0;
	
	printd("%s: start. f_irq=%d f_repo=%d f_wake=%d\n",
		__func__, smas->f_irq, smas->f_repo, smas->f_wake);
		
	poll_wait(filp, &drv_waitq, wait);
	if(smas->f_irq && smas->f_repo) {
		smas->f_repo = FALSE;
		mask |= POLLIN | POLLRDNORM;
	} else if( smas->f_wake ) {	
		smas->f_wake = FALSE;		
		mask |= POLLPRI;
	}
		
	printd("%s: end. mask=%d\n", __func__, mask);
	
	return mask; 
}

static const struct file_operations sfops = {
	.owner = THIS_MODULE,
	.write = mas_write,
	.read = mas_read,
	.open = mas_open,
	.release = mas_release,
	.poll = mas_poll,
};

static void mas_set_input(void) {
	struct input_dev *input = NULL;
	int ret = 0;

	printd("%s: start.\n", __func__);

	input = input_allocate_device();
	if (!input) {
		printw("%s: input_allocate_device failed.\n", __func__);
		return ;
	}
	set_bit(EV_KEY, input->evbit);
	set_bit(EV_ABS, input->evbit);
	set_bit(EV_SYN, input->evbit);
	set_bit(KEY_F10, input->keybit); 
	set_bit(KEY_F11, input->keybit); 
	set_bit(KEY_F12, input->keybit); 
	set_bit(KEY_UP, input->keybit); 
	set_bit(KEY_DOWN, input->keybit); 
	set_bit(KEY_LEFT, input->keybit); 
	set_bit(KEY_RIGHT, input->keybit); 
	set_bit(KEY_POWER, input->keybit);  

	input->name = "madev";
    input->id.bustype = BUS_SPI;
	ret = input_register_device(input);
    if (ret) {
        input_free_device(input);
        printw("%s: failed to register input device.\n",__func__);
        return;
    }
	smas->input  = input;

	printd("%s: end.\n", __func__);
}

static int mas_probe(struct spi_device *spi) {
	int ret=0;
    	u8 r[4]={0};
    	u8 t[4]={0};
	int mas_id=0;
	printk("%s: start\n", __func__);

	sdev = kmalloc(sizeof(struct fprint_dev), GFP_KERNEL);
	smas = kmalloc(sizeof(struct fprint_spi), GFP_KERNEL);
	srxb = kmalloc(FBUF*sizeof(u8), GFP_DMA);
	stxb = kmalloc(FBUF*sizeof(u8), GFP_DMA);
	if (sdev==NULL || smas==NULL || srxb==NULL || stxb==NULL) {
		printw("%s: smas kmalloc failed.\n", __func__);
		ret = -ENOMEM;
		goto fail_kmalloc;
	}
	plat_set_gpio();
//	plat_power(1);
	smas->spi = spi;
	smas->spi->max_speed_hz = SPI_SPEED;
	spi_setup(spi);
	
   	t[0]=0x8c;
    	t[1]=0xff;
    	t[2]=0xff;
    	t[3]=0xff;
    	mas_sync(t, r, 4);
     	msleep(10);
    	t[0]=0x00;
    	t[1]=0xff;
    	t[2]=0xff;
    	t[3]=0xff;
    	mas_sync(t, r, 4);
	mas_id = r[3];
	printw("mas_id r[3]:%#x\n",mas_id);
    	if(r[3] != 0x41){
		ret = -ENOMEM;
		goto fail_kmalloc;	
	} 

	smas->value = 0;
	smas->f_irq = smas->f_repo = FALSE;
	smas->f_wake = FALSE;
	smas->do_what = 0;
	spin_lock_init(&smas->spi_lock);
	wake_lock_init(&smas->wl,WAKE_LOCK_SUSPEND,"ma_wakelock");
	//初始化cdev
	sdev->chd = cdev_alloc();
	cdev_init(sdev->chd, &sfops);
	sdev->chd->owner = THIS_MODULE;

	//动态获取主设备号(dev_t idd中包含"主设备号"和"次设备号"信息)
	ret = alloc_chrdev_region(&sdev->idd, 0, 1, "madev");
	if (ret) {
		printk("mas alloc_chrdev_regio err!");
		ret = -1;
		goto fail_chardev;
	}
	sdev->major = MAJOR(sdev->idd);
	sdev->minor = MINOR(sdev->idd);
	printd("%s: major=%d minor=%d\n", __func__, sdev->major, sdev->minor);

	//注册字符设备 (1)
	ret = cdev_add(sdev->chd, sdev->idd, 1);
	if (ret) {
		printw("%s: cdev_add failed. ret=%d\n", ret);
		ret = -1;
		goto fail_cdev;
	}
	sdev->cls = class_create(THIS_MODULE, "madev");
	if (IS_ERR(sdev->cls)) {
		printw("%s: class_create failed.\n", __func__);
		ret = -1;
		goto fail_cdev;
	}
	smas->workq = create_singlethread_workqueue("mas_workqueue");
	if (!smas->workq) {
		printw("%s: create_single_workqueue failed\n", __func__);
		ret = -ENOMEM;
	        goto fail_workqueue;
	}
//	ret = plat_register_driver();


	INIT_LIST_HEAD(&smas->dev_entry);
	mutex_lock(&dev_lock);

/*	ret = mas_connect();
	printd("%s: mas_connect ret=%d\n", __func__, ret);
	if( !ret ) {
		printw("%s: conncet failed.\n", __func__);
		goto fail;
	}  */
	sdev->dev = device_create(sdev->cls, NULL, sdev->idd, NULL, "madev0");
	ret = IS_ERR(sdev->dev) ? PTR_ERR(sdev->dev) : 0;
	if (ret == 0) {
		list_add(&smas->dev_entry, &dev_list);
	} else {
		printw("%s: device_create failed. ret=%d\n", __func__, ret);
		goto fail_device;
	}
/*
	//device_init_wakeup(&spi->dev, 1);

#ifdef CONFIG_HAS_EARLYSUSPEND
	smas->suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	smas->suspend.suspend = mas_suspend;
	smas->suspend.resume = mas_resume;
	register_early_suspend(&smas->suspend);
#endif 

*/

	spi_set_drvdata(spi, smas);
	mas_set_input();

	INIT_WORK(&smas->work, mas_work);
	ret = plat_request_irq(spi->irq, mas_interrupt, IRQF_TRIGGER_RISING,
		dev_name(&spi->dev), &spi->dev);
	printd("%s: request_irq ret=%d\n", __func__, ret);
	if (ret) {
		printw("%s: request_irq failed.\n", __func__);
		goto fail_irq;
	}
	plat_enable_irq(spi, 1);	
	mutex_unlock(&dev_lock);

 
	printw("%s: insmod successfully. version:2.0 time:0919_am11\n", __func__);
	return ret;
/*
fail:
	spi_set_drvdata(spi, NULL);
	free_irq(spi->irq, &spi->dev);
*/
fail_irq:
	if(smas->input != NULL) input_unregister_device(smas->input);
	device_destroy(sdev->cls, sdev->idd);
fail_device:
	if(smas->workq) destroy_workqueue(smas->workq); 
	mutex_unlock(&dev_lock);	
fail_workqueue:
	class_destroy(sdev->cls);
fail_cdev:
	unregister_chrdev_region(sdev->idd, 1);
fail_chardev:
	cdev_del(sdev->chd);
fail_kmalloc:
	if(sdev!=NULL) kfree(sdev);
	if(smas!=NULL) kfree(smas);
	if(stxb!=NULL) kfree(stxb);
	if(srxb!=NULL) kfree(srxb);
	printw("%s: insmod failed.\n", __func__);
	return ret;
}


static int mas_remove(struct spi_device *spi) {
	printd("%s: start.\n", __func__);
	wake_lock_destroy(&smas->wl);
	// make sure ops on existing fds can abort cleanly
	spin_lock_irq(&smas->spi_lock);
	plat_enable_irq(spi, 0);
	free_irq(spi->irq, &spi->dev);
	smas->spi = NULL;
	spi_set_drvdata(spi, NULL);
	spin_unlock_irq(&smas->spi_lock);

	// prevent new opens
	mutex_lock(&dev_lock);
	input_unregister_device(smas->input);
	//device_init_wakeup(&spi->dev, 0);
	mutex_unlock(&dev_lock);

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&smas->suspend);
#endif
	plat_power(0);

	device_destroy(sdev->cls, sdev->idd);
	class_destroy(sdev->cls);
	unregister_chrdev_region(sdev->idd, 1);
	cdev_del(sdev->chd);
	if(smas->workq) destroy_workqueue(smas->workq);

	if(sdev!=NULL) kfree(sdev);
	if(smas!=NULL) kfree(smas);
	if(stxb!=NULL) kfree(stxb);
	if(srxb!=NULL) kfree(srxb);

	printk("%s: end\n", __func__);

	return 0;
}



static struct spi_device_id sdev_id = {"madev", 0};
static struct spi_driver sdrv = {
	.driver = {
		.name =	"madev",
		.owner = THIS_MODULE,
	},
	.probe = mas_probe,
	.remove = mas_remove,
	.id_table = &sdev_id,
};

struct spi_board_info smt_info[] __initdata = {
	[0] = {
		.modalias = "madev",
		.max_speed_hz = SPI_SPEED,
		.bus_num = 0,
		.chip_select = FINGERPRINT_MICROARRAY_CS,
		.mode = SPI_MODE_0,
		.controller_data = &smt_conf
	}
};
/*
#ifdef CONFIG_FINGERPRINT_SUNWAVE_9551
extern int  sunwave_devices_status(void);
#endif
*/
/*---------------------------------- module ------------------------------------*/
static int __init mas_init(void) {
	int  ret=0;
//printk("FINGERPRINT_AFS120N_CS:%d\n",FINGERPRINT_MICROARRAY_CS);
	spi_register_board_info(smt_info, ARRAY_SIZE(smt_info));
	ret = spi_register_driver(&sdrv);
	return ret;
}

static void __exit mas_exit(void) {
	printd("%s: start\n", __func__);
	spi_unregister_driver(&sdrv);
}

module_init(mas_init);
module_exit(mas_exit);

MODULE_AUTHOR("czl");
MODULE_DESCRIPTION("for microarray fprint driver");
MODULE_LICENSE("GPL");

