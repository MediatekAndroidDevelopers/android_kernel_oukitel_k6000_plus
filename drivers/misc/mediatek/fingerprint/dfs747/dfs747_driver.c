#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/poll.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#endif  
#include "dfs747_driver.h"
#include "../fingerprint.h"
#define TRANSFER_MODE_DMA
#define USER_KEY_F11   87

unsigned int fp_detect_irq = 0;
#ifdef CONFIG_OF
static const struct of_device_id fp_of_match[] = {
	{.compatible = "mediatek,hct_finger",},
	{},
};
#endif


////////////////////////////////////////////////////////////////////////////////
//
// Sensor Operations
//

#if 0
void mt_spi_dma_mode_setup(struct spi_device *spi, int enable)
{
    struct mt_chip_conf *chip_config = (struct mt_chip_conf *) spi->controller_data;
    chip_config->com_mod = ((enable != 0) ? DMA_TRANSFER : FIFO_TRANSFER);
    spi_setup(spi);
}
#endif

// dfs747_burst_read_image:
//     *buf     : pointer to store image data
//     read_len : how many bytes of *buf
static int dfs747_burst_read_image(struct dfs747_data *dfs747, u8 *buf, int read_len)
{
    int                status = 0;
    u8 *tx_data = NULL;
    u8 *rx_data = NULL;
    struct spi_device  *spi;
    struct spi_message msg;
    struct spi_transfer transfer;
    int                trans_len;
    int                i;

    // Command: Opcode + Data 0 + Data 1 + ... + Data N
    trans_len = 1 + read_len;
    if (trans_len % 1024) {
        trans_len = ((trans_len / 1024) + 1) * 1024;
    }

    tx_data = (u8 *) kzalloc(trans_len, GFP_KERNEL);
    if (tx_data == NULL) {
        DFS747_ERROR("%s(): alloc memory error.\n", __func__);
        status = -1;
        goto dfs747_burst_read_image_end;
    }

    tx_data[0] = DFS747_CMD_BURST_RD_IMG;
    for (i = 1; i < trans_len; i++) {
        tx_data[i] = DUMMY_DATA;
    }

    rx_data = (u8 *) kzalloc(trans_len, GFP_KERNEL);
    if (rx_data == NULL) {
        DFS747_ERROR("%s(): alloc memory error.\n", __func__);
        status = -1;
        goto dfs747_burst_read_image_end;
    }

    for (i = 0; i < trans_len; i++) {
        rx_data[i] = 0;
    }

    transfer.tx_buf = tx_data;
    transfer.rx_nbits=SPI_NBITS_SINGLE;
    transfer.tx_nbits=SPI_NBITS_SINGLE;
    transfer.rx_buf = rx_data;
    transfer.len    = trans_len;

    transfer.bits_per_word = 8;
    spi = dfs747->spi;
    spi_message_init(&msg);
    spi_message_add_tail(&transfer, &msg);
    status = spi_sync(spi, &msg);

    if (status < 0) {
        DFS747_ERROR("%s(): read data error. status = %0d\n", __func__, status);
        goto dfs747_burst_read_image_end;
    }

    DFS747_DEBUG("%s(): read_len = %d\n", __func__, read_len);
    memcpy(buf, &rx_data[1], read_len);

dfs747_burst_read_image_end :

    if (tx_data) {
        kfree(tx_data);
    }

    if (rx_data) {
        kfree(rx_data);
    }

    return status;
}


// dfs747_multiple_read_register:
//     *addr    : register addresses {addr0, addr1, addr2, ... addrN}
//     *buf     : pointer to store register data {data0, data1, data2, ... dataN}
//     read_len : how many bytes of *buf
static int dfs747_multiple_read_register(struct dfs747_data *dfs747, u8 *addr, u8 *buf, int read_len)
{
    int                status = 0;
    struct spi_device  *spi;
    struct spi_message msg;        
    struct spi_transfer transfer;
    u8 *tx_data = NULL;            
    u8 *rx_data = NULL;            
    int                trans_len;
    int                i;

    // Command: Opcode + Address 0 + Data 0 + Address1 + Data 1 + ... + Address N + Data N
    trans_len = 1 + (read_len * 2);

    tx_data = (u8 *) kzalloc(trans_len, GFP_KERNEL);
    if (tx_data == NULL) {
        DFS747_ERROR("%s(): alloc memory error.\n", __func__);
        status = -1;
        goto dfs747_multiple_read_register_end;
    }

    tx_data[0] = DFS747_CMD_RD_REG;
    for (i = 0; i < read_len; i++) {
        tx_data[1 + (i * 2)] = addr[i];
        tx_data[2 + (i * 2)] = DUMMY_DATA;
    }

    rx_data = (u8 *) kzalloc(trans_len, GFP_KERNEL);
    if (rx_data == NULL) {
        DFS747_ERROR("%s(): alloc memory error.\n", __func__);
        status = -1;
        goto dfs747_multiple_read_register_end;
    }

    for (i = 0; i < trans_len; i++) {
        rx_data[i] = 0;
    }

    transfer.tx_buf = tx_data;
    transfer.rx_buf = rx_data;
    transfer.len    = trans_len;
    
    transfer.bits_per_word = 8;
    transfer.rx_nbits=SPI_NBITS_SINGLE;
    transfer.tx_nbits=SPI_NBITS_SINGLE;

    spi = dfs747->spi;
    spi_message_init(&msg);
    spi_message_add_tail(&transfer, &msg);
    status = spi_sync(spi, &msg);

    if (status < 0) {
        DFS747_ERROR("%s(): read data error. status = %0d\n", __func__, status);
        goto dfs747_multiple_read_register_end;
    }

    for (i = 0; i < read_len; i++) {
        buf[i] = rx_data[2 + (i * 2)];
    }

    DFS747_DEBUG("%s(): read_len = %0d\n", __func__, read_len);
    for (i = 0; i < read_len; i++) {
        DFS747_DEBUG("  -> [%0d] addr = %02X, data = %02X\n", i, addr[i], buf[i]);
    }

dfs747_multiple_read_register_end :

    if (tx_data) {
        kfree(tx_data);
    }

    if (rx_data) {
        kfree(rx_data);
    }

    return status;
}

// dfs747_multiple_write_register:
//     *buf      : pointer to register context to be wriiten.
//                 Format: {addr0, data0, addr1, data1, data2, ... addrN, dataN}
//     write_len : how many bytes of *buf
static int dfs747_multiple_write_register(struct dfs747_data *dfs747, u8 *buf, int write_len)
{
    int                 status = 0;
    struct spi_device   *spi;
    struct spi_message  msg;
    struct spi_transfer transfer;
    u8                  *tx_data = NULL;
#if 1
    u8                  *rx_data = NULL;
#endif
    int                 trans_len;
    int                 i;

    // Command: Opcode + Address 0 + Data 0 + Address1 + Data 1 + ... + Address N + Data N + Dummy
    trans_len = 1 + write_len + 1;

    tx_data = (u8 *) kzalloc(trans_len, GFP_KERNEL);
    if (tx_data == NULL) {
        DFS747_ERROR("%s(): alloc memory error.\n", __func__);
        status = -1;
        goto dfs747_multiple_write_register_end;
    }

#if 1
    rx_data = (u8 *) kzalloc(trans_len, GFP_KERNEL);
    if (rx_data == NULL) {
        printk("%s(): alloc memory error.\n", __func__);
        status = -2;
        goto dfs747_multiple_write_register_end;
    }
#endif

#if 0
    tx_data[0] = DFS747_CMD_WR_REG;
    for (i = 0; i < write_len; i++) {
        tx_data[1 + i] = buf[i];
    }
    tx_data[trans_len - 1] = DUMMY_DATA;
#else
    tx_data[0] = DFS747_CMD_WR_REG;
	if (copy_from_user(&tx_data[1], (const u8 __user *) (uintptr_t) buf, write_len)) {
		printk("%s(): calling copy_from_user() fail.\n", __func__);
		status = -EFAULT;
        goto dfs747_multiple_write_register_end;
	}
    tx_data[trans_len - 1] = DUMMY_DATA;
#endif

#if 1
    transfer.rx_buf = rx_data;
#endif
    transfer.tx_buf = tx_data;
    transfer.len    = trans_len;

    transfer.bits_per_word = 8;
    transfer.rx_nbits=SPI_NBITS_SINGLE;
    transfer.tx_nbits=SPI_NBITS_SINGLE;
    spi = dfs747->spi;
    spi_message_init(&msg);
    spi_message_add_tail(&transfer, &msg);
    status = spi_sync(spi, &msg);

    if (status < 0) {
        DFS747_ERROR("%s(): write data error. status = %0d\n", __func__, status);
        goto dfs747_multiple_write_register_end;
    }

    DFS747_DEBUG("%s(): write_len = %d\n", __func__, write_len);
    for (i = 0; i < write_len; i += 2) {
        DFS747_DEBUG("  -> [%0d] addr = %02X, data = %02X\n", i, buf[i], buf[i+1]);
    }

dfs747_multiple_write_register_end :

    if (tx_data) {
        kfree(tx_data);
    }

#if 1
    if (rx_data) {
        kfree(rx_data);
    }
#endif

    return status;
}

// dfs747_single_read_register:
//     addr : register address
//     *buf : pointer to register data to store
static int dfs747_single_read_register(struct dfs747_data *dfs747, u8 addr, u8 *buf)
{
    int                status = 0;
    struct spi_device  *spi;
    struct spi_message msg;
    struct spi_transfer transfer;
    u8 *tx_data = NULL;            
    u8 *rx_data = NULL;            

    tx_data = (u8 *) kzalloc(3, GFP_KERNEL);
    if (tx_data == NULL) {
        DFS747_ERROR("%s(): alloc memory error.\n", __func__);
        status = -1;
        goto dfs747_single_read_register_end;
    }

    tx_data[0] = DFS747_CMD_RD_REG;
    tx_data[1] = addr;
    tx_data[2] = DUMMY_DATA;

    rx_data = (u8 *) kzalloc(3, GFP_KERNEL);
    if (rx_data == NULL) {
        DFS747_ERROR("%s(): alloc memory error.\n", __func__);
        status = -1;
        goto dfs747_single_read_register_end;
    }

    rx_data[0] = 0;
    rx_data[1] = 0;
    rx_data[2] = 0;
    transfer.bits_per_word = 8;
    transfer.tx_buf = tx_data;
    transfer.rx_buf = rx_data;
    transfer.len    = 3;

    transfer.rx_nbits=SPI_NBITS_SINGLE;
    transfer.tx_nbits=SPI_NBITS_SINGLE;

    spi = dfs747->spi;
    spi_message_init(&msg);
    spi_message_add_tail(&transfer, &msg);
    status = spi_sync(spi, &msg);

    if (status < 0) {
        DFS747_ERROR("%s(): read data error. status = %d\n", __func__, status);
        goto dfs747_single_read_register_end;
    }

    *buf = rx_data[2];

    DFS747_DEBUG("%s(): addr = %02x, data = %02x\n", __func__, addr, *buf);

dfs747_single_read_register_end :

    if (tx_data) {
        kfree(tx_data);
    }

    if (rx_data) {
        kfree(rx_data);
    }

    return status;
}

// dfs747_single_write_register:
//     addr  : register address
//     value : register value
static int dfs747_single_write_register(struct dfs747_data *dfs747, u8 addr, u8 value)
{
    int                status = 0;
    struct spi_device  *spi;
    struct spi_message msg;
    struct spi_transfer transfer;
    u8 *tx_data = NULL;            

    tx_data = (u8 *) kzalloc(4, GFP_KERNEL);
    if (tx_data == NULL) {
        DFS747_ERROR("%s(): alloc memory error.\n", __func__);
        status = -1;
        goto dfs747_single_write_register_end;
    }

    tx_data[0] = DFS747_CMD_WR_REG;
    tx_data[1] = addr;
    tx_data[2] = value;
    tx_data[3] = DUMMY_DATA;

    transfer.tx_buf = tx_data;
    transfer.rx_buf = tx_data;
    transfer.len    = 4;

    transfer.rx_nbits=SPI_NBITS_SINGLE;
    transfer.tx_nbits=SPI_NBITS_SINGLE;

    transfer.bits_per_word = 8;
    spi = dfs747->spi;
    spi_message_init(&msg);
    spi_message_add_tail(&transfer, &msg);
    status = spi_sync(spi, &msg);

    if (status < 0) {
        DFS747_ERROR("%s() write data error. status = %d\n", __func__, status);
        goto dfs747_single_write_register_end;
    }

    DFS747_DEBUG("%s(): addr = %02x, data = %02x\n", __func__, addr, value);

dfs747_single_write_register_end :

    if (tx_data) {
        kfree(tx_data);
    }

    return status;
}

// dfs747_get_one_image:
//     *buf       : pointer to transfer data paramerer
//     *image_buf : pointer to image data to store
#define MAX_FAIL_COUNT (250)
int dfs747_get_one_image(struct dfs747_data *dfs747, u8 *buf, u8 *image_buf)
{
    int status     = 0;
    u32 read_count = 0;
    u32 fail_count = 0;
    u8  read_val;
    u8  *work_buf  = NULL;

    // Transfer data format
    // -------------------------------------------------------------------------
    // NOTE:
    //     tx_buf[0] : image width
    //     tx_buf[1] : image height
    //     tx_buf[2] : dummy pixels
    //     tx_buf[3] : TBD
    //     tx_buf[4] : TBD
    //     tx_buf[5] : TBD
    //

    u8 *val = (u8 *) kzalloc(6, GFP_KERNEL);
    if (val == NULL) {
        DFS747_ERROR("%s(): alloc memory error.\n", __func__);
        status = -1;
        goto dfs747_get_one_image_end;
    }

    if (copy_from_user(val, (const u8 __user *) (uintptr_t) buf, 6)) {
        DFS747_ERROR("%s copy_from_user() fail", __func__);
        status = -EFAULT;
        goto dfs747_get_one_image_end;
    }

    read_count = val[0] * val[1] + val[2];

    DFS747_DEBUG("%s, read_count = %0d (%0d x %0d) + %0d\n", __func__, read_count, val[0], val[1], val[2]); 

    for (fail_count = 0; fail_count < MAX_FAIL_COUNT; fail_count++) {
        dfs747_single_read_register(dfs747, DFS747_REG_INT_EVENT, &read_val);

        if (read_val & DFS747_FRAME_READY_EVENT) {
            break;
        }
    }

    if (fail_count >= MAX_FAIL_COUNT) {
        DFS747_ERROR("%s(): fail_count = %0d\n", __func__, fail_count);
        status = -1;
        goto dfs747_get_one_image_not_ready;
    }

    work_buf = kzalloc((read_count + 1), GFP_KERNEL);
    if (work_buf == NULL) {
        DFS747_ERROR("%s(): alloc memory error.\n", __func__);
        status = -1;
        goto dfs747_get_one_image_end;
    }

    status = dfs747_burst_read_image(dfs747, work_buf, read_count);

    if (status < 0) {
        DFS747_ERROR("%s(): call dfs747_burst_read_image error. status = %d", __func__, status);
        goto dfs747_get_one_image_end;
    }

dfs747_get_one_image_end :

    if (copy_to_user((u8 __user *) (uintptr_t) image_buf, work_buf, read_count)) {
          DFS747_ERROR("%s(): copy_to_user fail. status = %0d\n",__func__, status);
          status = -EFAULT;
    }

dfs747_get_one_image_not_ready :

    if (work_buf) {
        kfree(work_buf);
    }

    if (val) {
        kfree(val);
    }

    return status;
}


////////////////////////////////////////////////////////////////////////////////
//
// Reset Handling
//

static inline void FPS_reset(void)
{
	pr_info("%s\n", __func__);
	hct_finger_set_reset(0);
	msleep(30);
	hct_finger_set_reset(1);
	msleep(20);
}


////////////////////////////////////////////////////////////////////////////////
//
// Interrupt Handling
//

static volatile char interrupt_values[] = {'0', '0', '0', '0', '0', '0', '0', '0'};
static DECLARE_WAIT_QUEUE_HEAD(interrupt_waitq);
static volatile int ev_press = 0;
struct wakeup_source ws;

static void fingerprint_delay_work_func(struct  work_struct *work)
{

    struct delayed_work *dwork = to_delayed_work(work);
    struct dfs747_data *dfs747 = container_of(dwork, struct dfs747_data, fp_delay_work);
    dfs747->irq_enable_flag = true;
	enable_irq(fp_detect_irq);
}

static irqreturn_t fingerprint_interrupt(int irq, void *dev_id)
{
    struct dfs747_data *dfs747 = (struct dfs747_data *)dev_id;
    DFS747_DEBUG("%s(): Interrupt Triggered!\n", __func__);

    dfs747->irq_enable_flag = false;
	disable_irq_nosync(fp_detect_irq);
    wake_up_interruptible(&interrupt_waitq); 
    ev_press = 1;
    schedule_delayed_work(&dfs747->fp_delay_work, msecs_to_jiffies(10));
    __pm_wakeup_event(&ws,1000);//1s
	return IRQ_HANDLED;
}



int Interrupt_Init(struct dfs747_data *dfs747)
{
        //int i, irq;
        int ret = 0;
        struct device_node *node = NULL;
 
//     fingerprint_irq_registration();
        if(dfs747->irq_enable_flag == true) {
            printk("already enable irq.");
            return 0;
        }

        INIT_DELAYED_WORK(&dfs747->fp_delay_work, fingerprint_delay_work_func); 
        node = of_find_matching_node(node, fp_of_match);
 
        if (node){
                fp_detect_irq = irq_of_parse_and_map(node, 0);
                //printk("fp_irq number %d\n", fp_detect_irq);
                ret = request_irq(fp_detect_irq,
                        fingerprint_interrupt, IRQF_TRIGGER_RISING,
                        "fp_detect-eint", dfs747);
                if (ret > 0){
                        printk("fingerprint request_irq IRQ LINE NOT AVAILABLE!.");
                }
        }else{
                printk("fingerprint request_irq can not find fp eint device node!.");
        }
 
        dfs747->irq_enable_flag = true;
 
        return 0;
}

static int fps_interrupt_read(struct file *filp, char __user *buff, size_t count, loff_t *offp)
{
    unsigned long missing = 0;
   
    if (filp->f_flags & O_NONBLOCK) {
      return -EAGAIN;
    } else {
      wait_event_interruptible(interrupt_waitq, ev_press);
    }
   
    DFS747_DEBUG("%s(): Interrupt read condition = %0d\n", __func__, ev_press); 
    DFS747_DEBUG("%s(): Interrupt value = %0d\n", __func__, interrupt_values[0]); 
   
    missing = copy_to_user((void *)buff, (const void *)(&interrupt_values), min(sizeof(interrupt_values), count));
    return missing ? -EFAULT : min(sizeof(interrupt_values), count);
}

static unsigned int fps_interrupt_poll(struct file *file, struct poll_table_struct *wait)
{
    unsigned int mask = 0;

    poll_wait(file, &interrupt_waitq, wait);
    if (ev_press) {
        mask |= POLLIN | POLLRDNORM;
        ev_press = 0;
    }

    return mask;
}


////////////////////////////////////////////////////////////////////////////////
//
// Device File Operations
//

static LIST_HEAD(device_list);
static DEFINE_MUTEX(device_list_lock);
static unsigned msg_size = 4096;

static struct mt_chip_conf spi_conf =
{
    .setuptime    = 1,
    .holdtime     = 1,
    .high_time    = 6,
    .low_time     = 6,
    .cs_idletime  = 2,
    .ulthgh_thrsh = 0,
    .cpol         = 1,
    .cpha         = 1,
    .rx_mlsb      = SPI_MSB, 
    .tx_mlsb      = SPI_MSB,
    .tx_endian    = 0,
    .rx_endian    = 0,
#ifdef TRANSFER_MODE_DMA
    .com_mod      = DMA_TRANSFER,
#else
    .com_mod      = FIFO_TRANSFER,
#endif
    .pause        = 0,
    .finish_intr  = 1,
    .deassert     = 0,
    .ulthigh      = 0,
    .tckdly       = 0,
};

// This implementation is for test only.
static ssize_t dfs747_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    struct dfs747_data *dfs747   = NULL;
    u8                 result[2] = {0xFF, 0xFF};
    size_t             size      = 0;
    unsigned long      missing   = 0;
    int                status    = 0;

    DFS747_DEBUG("%s() is called!\n", __func__);

    // Chip-select only toggles at start or end of operation
    if (count > msg_size) {
        return -EMSGSIZE;
    }

    dfs747 = filp->private_data;
    mutex_lock(&dfs747->buf_lock);

    status = dfs747_single_read_register(dfs747, 0x29, result);

    if (status == 0) {
        size = 1;

        missing = copy_to_user(buf, result + 1, count);
        if (missing != 0) {
            size = -EFAULT;
        }
    } else {
        size = -EFAULT;
    }

    mutex_unlock(&dfs747->buf_lock);
    return size;
}

// This implementation is for test only.
static ssize_t dfs747_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    struct dfs747_data *dfs747 = NULL;
    ssize_t            status  = 0;
    unsigned long      missing = 0;

    DFS747_DEBUG("%s() is called!\n", __func__);

    // Chip-select only toggles at start or end of operation
    if (count > msg_size) {
        return -EMSGSIZE;
    }

    dfs747 = filp->private_data;
    mutex_lock(&dfs747->buf_lock);

    missing = copy_from_user(dfs747->buffer, buf, count);

    if (missing == 0) {
        status = dfs747_single_write_register(dfs747, dfs747->buffer[0], dfs747->buffer[1]);
    } else {
        status = -EFAULT;
    }

    mutex_unlock(&dfs747->buf_lock);
    return status;
}

static long dfs747_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct dfs747_data         *dfs747 = NULL;
    struct spi_device          *spi    = NULL;
    struct dfs747_ioc_transfer *ioc    = NULL;
    u32                        save    = 1000000;
    u32                        tmp     = 0;
    unsigned                   n_ioc   = 0;
    int                        status  = 0;

    DFS747_DEBUG("%s() is called!\n", __func__);

    // Check type and command number
    if (_IOC_TYPE(cmd) != DFS747_IOC_MAGIC) {
        DFS747_ERROR("%s(): Check _IOC_TYPE(cmd) failed!\n", __func__);
        return -ENOTTY;
    }

    // Check access direction once here; don't repeat below. IOC_DIR is from the
    // user perspective, while access_ok is from the kernel perspective; so they
    // look reversed.
    if (_IOC_DIR(cmd) & _IOC_READ) {
        status = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
    }

    if ((status == 0) && (_IOC_DIR(cmd) & _IOC_WRITE)) {
        status = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
    }

    if (status) {
        DFS747_ERROR("%s(): Check _IOC_DIR(cmd) failed!\n", __func__);
        return -EFAULT;
    }

    // Guard against device removal before, or while, we issue this ioctl.
    dfs747 = filp->private_data;
    spin_lock_irq(&dfs747->spi_lock);

    spi = spi_dev_get(dfs747->spi);
    spin_unlock_irq(&dfs747->spi_lock);

    if (spi == NULL) {
        DFS747_ERROR("%s(): spi == NULL!\n", __func__);
        return -ESHUTDOWN;
    }
    mutex_lock(&dfs747->buf_lock);

    // Segmented and/or full-duplex I/O request
    if ((_IOC_NR(cmd) != _IOC_NR(DFS747_IOC_MESSAGE(0))) || (_IOC_DIR(cmd) != _IOC_WRITE)) {
        status = -ENOTTY;
        goto dfs747_ioctl_error;
    }

    tmp = _IOC_SIZE(cmd);
    if ((tmp % sizeof(struct dfs747_ioc_transfer)) != 0) {
        status = -EINVAL;
        goto dfs747_ioctl_error;
    }

    n_ioc = tmp / sizeof(struct dfs747_ioc_transfer);
    if (n_ioc == 0) {
        goto dfs747_ioctl_error;
    }

    // Copy into scratch area
    ioc = kmalloc(tmp, GFP_KERNEL);
    if (!ioc) {
        status = -ENOMEM;
        goto dfs747_ioctl_error;
    }

    if (__copy_from_user(ioc, (void __user *)arg, tmp)) {
        kfree(ioc);
        status = -EFAULT;
        goto dfs747_ioctl_error;
    }
    switch(ioc->opcode)
    {
        case DFS747_IOC_RESET_SENSOR: {
            // Reset sensor
            DFS747_DEBUG("%s(): DFS747_IOC_RESET_SENSOR\n", __func__);

            if (ioc->len == 0) {
                hct_finger_set_reset(0);
                msleep(30);
            } else {
                hct_finger_set_reset(1);
                msleep(20);
            }
        }
        break;

        case DFS747_IOC_SET_CLKRATE:
            // Set clock rate of SPI controller. Ues dfs747_ioc_transfer->len as speed
            DFS747_INFO("%s(): Modify speed = %0d\n", __func__, ioc->len);
            save              = spi->max_speed_hz;
            tmp               = ioc->len;
            spi->max_speed_hz = tmp;

            status = spi_setup(spi);
            if (status < 0) {
                spi->max_speed_hz = save;
                DFS747_ERROR("%s(): Set SPI speed failed!\n", __func__);
                goto dfs747_ioctl_error;
            } else {
                DFS747_DEBUG("%s(): %0d Hz (original)\n", __func__, save);
                DFS747_DEBUG("%s(): %0d Hz (max)     \n", __func__, tmp);
            }
        break;

        case DFS747_IOC_REGISTER_MASS_READ: {
            // Read registers
            u8 *address = (u8 *)ioc->tx_buf;
            u8 *result  = (u8 *)ioc->rx_buf;

            DFS747_DEBUG("%s(): DFS747_IOC_REGISTER_MASS_READ\n", __func__);
            status = dfs747_multiple_read_register(dfs747, address, result, ioc->len);
            if (status < 0) {
                DFS747_ERROR("%s(): Calling dfs747_multiple_read_register error! status = %0d\n", __func__, status);
                goto dfs747_ioctl_error;
            }
        }
        break;

        case DFS747_IOC_REGISTER_MASS_WRITE: {
        // Write registers

            u8 *buf = (u8 *)ioc->tx_buf;
            DFS747_DEBUG("%s(): DFS747_IOC_REGISTER_MASS_WRITE\n", __func__);

            status = dfs747_multiple_write_register(dfs747, buf, ioc->len);
            if (status < 0) {
                DFS747_ERROR("%s(): Calling dfs747_multiple_write_register error! status = %0d\n", __func__, status);
                goto dfs747_ioctl_error;
            }
        }
        break;

        case DFS747_IOC_GET_ONE_IMG: {
        // Get one image

            u8 *buf       = (u8 *)ioc->tx_buf;
            u8 *image_buf = (u8 *)ioc->rx_buf;

            DFS747_DEBUG("%s(): DFS747_IOC_GET_ONE_IMG\n", __func__);
            status = dfs747_get_one_image(dfs747, buf, image_buf);
            if (status < 0) {
                DFS747_ERROR("%s(): Calling dfs747_get_one_image error! status = %0d\n", __func__, status);
                goto dfs747_ioctl_error;
            }
        }
        break;

        case DFS747_IOC_SENDKEY: {
            // Report keyevent 

            u32 state = ioc->len;
            DFS747_DEBUG("%s(): DFS747_IOC_SENDKEY\n", __func__);

            if (state) {
                input_report_key(dfs747->idev,USER_KEY_F11,1);
            } else {
                input_report_key(dfs747->idev,USER_KEY_F11,0);
            }
            input_sync(dfs747->idev);
        }
        break;

        case DFS747_IOC_WAKELOCK: {
            // controll wakelock 

            u32 state = ioc->len;
            DFS747_DEBUG("%s(): DFS747_IOC_WAKELOCK %s \n", __func__,state?"lock":"unlock");

            if (state) {
                __pm_stay_awake(&ws);
            } else {
                __pm_relax(&ws);
            }
        }
        break;

        case DFS747_IOC_INTR_INIT: {
        // Initialize interrupt

            u8 *trigger_buf = (u8 *)ioc->rx_buf;

            DFS747_DEBUG("%s(): DFS747_IOC_INTR_INIT\n", __func__);
	        status = Interrupt_Init(dfs747);
            status = fps_interrupt_read(filp,trigger_buf, 1, NULL);

            DFS747_DEBUG("%s(): DFS747_IOC_INTR_INIT status = %0d\n", __func__, status);
        }
        break;

        case DFS747_IOC_INTR_CLOSE: {
        // Close interrupt
            DFS747_DEBUG("%s(): DFS747_IOC_INTR_CLOSE\n", __func__);

            if(dfs747->irq_enable_flag == true) {
                dfs747->irq_enable_flag = false;
		        disable_irq(fp_detect_irq);
            }
            DFS747_DEBUG("%s(): DFS747_IOC_INTR_CLOSE status = %0d\n", __func__, status);
        }
        break;

        case DFS747_IOC_INTR_READ: {
        // Read interrupt status

            u8 *trigger_buf = (u8 *)ioc->rx_buf;
            DFS747_DEBUG("%s(): DFS747_IOC_INTR_READ\n", __func__);
            status = fps_interrupt_read(filp, trigger_buf, 1, NULL);
        }
        break;

        default:
            DFS747_DEBUG("%s(): error undefine opcode %0d \n", __func__, ioc->opcode);
            break;
    }

    kfree(ioc);

dfs747_ioctl_error:
    mutex_unlock(&dfs747->buf_lock);
    spi_dev_put(spi);

    if (status < 0) {
        DFS747_ERROR("%s(): status = %0d\n", __func__, status);
    }

    return status;
}

#ifdef CONFIG_COMPAT
static long dfs747_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    DFS747_DEBUG("%s() is called!\n", __func__);
    return dfs747_ioctl(filp, cmd, (unsigned long)compat_ptr(arg));
}
#else
#define dfs747_compat_ioctl NULL
#endif /* CONFIG_COMPAT */

static int dfs747_open(struct inode *inode, struct file *filp)
{
    struct dfs747_data *dfs747 = NULL;
    int                status  = -ENXIO;

    DFS747_DEBUG("%s() is called!\n", __func__);


    mutex_lock(&device_list_lock);

    list_for_each_entry(dfs747, &device_list, device_entry) {
        if (dfs747->devt == inode->i_rdev) {
            status = 0;
            break;
        }
    }

    if (status == 0) {
        if (!dfs747->buffer) {
            dfs747->buffer = kmalloc(msg_size, GFP_KERNEL);
            if (!dfs747->buffer) {
                dev_dbg(&dfs747->spi->dev, "open/ENOMEM\n");
                status = -ENOMEM;
            }
        }

        if (status == 0) {
            dfs747->users++;
            filp->private_data = dfs747;
            nonseekable_open(inode, filp);
        }
    } 
    else {
        pr_debug("dfs747: nothing for minor %d\n", iminor(inode));
    }

    mutex_unlock(&device_list_lock);
    return status;
}

static int dfs747_release(struct inode *inode, struct file *filp)
{
    struct dfs747_data *dfs747 = NULL;
    int                status  = 0;

    DFS747_DEBUG("%s() is called!\n", __func__);

    mutex_lock(&device_list_lock);
    dfs747 = filp->private_data;
    filp->private_data = NULL;

    // last close?
    dfs747->users--;
    if (!dfs747->users) {
        int dofree;

        kfree(dfs747->buffer);
        dfs747->buffer = NULL;

        // ... after we unbound from the underlying device?
        spin_lock_irq(&dfs747->spi_lock);
        dofree = (dfs747->spi == NULL);
        spin_unlock_irq(&dfs747->spi_lock);

        if (dofree) {
            kfree(dfs747);
        }
    }

    mutex_unlock(&device_list_lock);
    return status;
}

static const struct file_operations dfs747_fops = 
{
    .owner          = THIS_MODULE,
    .write          = dfs747_write,
    .read           = dfs747_read,
    .unlocked_ioctl = dfs747_ioctl,
    .compat_ioctl   = dfs747_compat_ioctl,
    .open           = dfs747_open,
    .release        = dfs747_release,
    .llseek         = no_llseek,
    .poll           = fps_interrupt_poll,
};


////////////////////////////////////////////////////////////////////////////////
//
// Driver Functions
//

static DECLARE_BITMAP(minors, DFS747_NUM_OF_MINORS);
static struct class *dfs747_class;
extern int hct_finger_probe_isok;//add for hct finger jianrong
static int dfs747_probe(struct spi_device *spi)
{
    struct dfs747_data *dfs747 = NULL;
    int                status  = 0;
    unsigned long      minor   = 0;
    struct device      *dev    = NULL;
    u8                 read_val = 0;

	if(hct_finger_probe_isok)
	{
		printk("dfs747_probe fail, other finger already probe ok!!!!\n");
		return -ENOMEM;
	}
    DFS747_DEBUG("%s() is called!\n", __func__);
	hct_waite_for_finger_dts_paser();

    dfs747 = kzalloc(sizeof(*dfs747), GFP_KERNEL);
    if (!dfs747) {
        return -ENOMEM;
    }

    // Initialize the driver data
    dfs747->spi = spi;
    spi->controller_data = (void *) &spi_conf;
    spi->max_speed_hz = 12 * 1000 * 1000;
    spi_setup(spi);
    spin_lock_init(&dfs747->spi_lock);
    mutex_init(&dfs747->buf_lock);

    INIT_LIST_HEAD(&dfs747->device_entry);

    // If we can allocate a minor number, hook up this device. Reusing minors is
    // fine so long as udev or mdev is working.
    mutex_lock(&device_list_lock);
    minor = find_first_zero_bit(minors, DFS747_NUM_OF_MINORS);
    if (minor < DFS747_NUM_OF_MINORS) {
        dfs747->devt = MKDEV(DFS747_MAJOR, minor);
        dev = device_create(dfs747_class, NULL, dfs747->devt, dfs747, "dfs0");
        status = IS_ERR(dev) ? PTR_ERR(dev) : 0;
        if (status) {
            goto dfs747_probe_error;
        }
    } else {
        dev_dbg(&spi->dev, "dfs747 no minor number available!\n");
        status = -ENODEV;
        goto dfs747_probe_error;
    }

    if (status == 0) {
        set_bit(minor, minors);
        list_add(&dfs747->device_entry, &device_list);
    }
    mutex_unlock(&device_list_lock);
    dfs747->idev = devm_input_allocate_device(&spi->dev);
    if(dfs747->idev == NULL) {
        dev_err(&spi->dev, "fail to allocate input device\n");
        status = -ENOMEM;
        goto dfs747_alloc_input_error;
    }
    dfs747->idev->name = "dfs747";
    dfs747->idev->id.bustype = BUS_SPI;
    dfs747->idev->id.vendor = 0x0001;
    dfs747->idev->id.product= 0x0001;
    dfs747->idev->id.version = 0x0100;
    __set_bit(USER_KEY_F11,dfs747->idev->keybit);
    __set_bit(EV_KEY, dfs747->idev->evbit);
    status = input_register_device(dfs747->idev);
    if(status) {
        dev_err(&spi->dev, "fail to register input device\n");
        goto dfs747_register_input_error;
    }
    spi_set_drvdata(spi, dfs747);

	hct_finger_set_reset(1);
	msleep(20);
	hct_finger_set_power(1);
	hct_finger_set_18v_power(1);

    wakeup_source_init(&ws,dev_name(&spi->dev));
	Interrupt_Init(dfs747);
	dfs747_single_read_register(dfs747, DFS747_REG_IMG_COL_END, &read_val);
	printk("dfs747 read_val %d\n",read_val);
	if(read_val != 0x8f){
		dev_err(&spi->dev,"can not find dfs747\n");
    		dfs747->spi = NULL;
    		spi_set_drvdata(spi, NULL);
		list_del(&dfs747->device_entry);
    		device_destroy(dfs747_class, dfs747->devt);
		if (dfs747->users == 0) {
     	        kfree(dfs747);
    		}
		status = -ENODEV;
		return status;
	}
    hct_finger_probe_isok = 1;
    return status;

dfs747_register_input_error:
	input_free_device(dfs747->idev);
dfs747_alloc_input_error:

    device_destroy(dfs747_class, dfs747->devt);
dfs747_probe_error :

    kfree(dfs747);
    return status;
}

static int dfs747_remove(struct spi_device *spi)
{
    struct dfs747_data *dfs747 = spi_get_drvdata(spi);

    DFS747_DEBUG("%s() is called!\n", __func__);

    // Make sure ops on existing fds can abort cleanly
    spin_lock_irq(&dfs747->spi_lock);
    dfs747->spi = NULL;
    spi_set_drvdata(spi, NULL);
    spin_unlock_irq(&dfs747->spi_lock);

    // Prevent new opens
    mutex_lock(&device_list_lock);
    list_del(&dfs747->device_entry);
    device_destroy(dfs747_class, dfs747->devt);
    clear_bit(MINOR(dfs747->devt), minors);

    if (dfs747->users == 0) {
        kfree(dfs747);
    }
    mutex_unlock(&device_list_lock);

    return 0;
}

static int dfs747_suspend(struct spi_device *spi, pm_message_t mesg)
{
    DFS747_DEBUG("%s() is called!\n", __func__);
    return 0;
}

static int dfs747_resume(struct spi_device *spi)
{
    DFS747_DEBUG("%s() is called!\n", __func__);
    return 0;
}

struct spi_device_id dfs747_id_talbe = {"dfs747", 0};

static struct spi_driver dfs747_spi_driver =
{
    .driver = {
        .name  = "dfs747",
        .bus   = &spi_bus_type,
        .owner = THIS_MODULE,
    },
    .probe    = dfs747_probe,
    .remove   = dfs747_remove,
    .suspend  = dfs747_suspend,
    .resume   = dfs747_resume,
    .id_table = &dfs747_id_talbe,
};


////////////////////////////////////////////////////////////////////////////////
//
// Module Init/Exit
//

static struct spi_board_info dfs747_spi[] __initdata = {
    [0] = {
        .modalias     = "dfs747",
        .bus_num      = 0,
        .chip_select  = FINGERPRINT_DFS747_CS,
        .max_speed_hz = 15000000,
        .mode         = SPI_MODE_3,
    }
};

static int __init dfs747_init(void)
{
    int status = 0;

    DFS747_DEBUG("%s() is called!\n", __func__);

    status = spi_register_board_info(dfs747_spi, ARRAY_SIZE(dfs747_spi));
    if (status < 0) {
        return status;
    }

    // Claim our 256 reserved device numbers. Then register a class that will
    // key udev/mdev to add/remove /dev nodes. Last, register the driver which
    // manages those device numbers.
    BUILD_BUG_ON(DFS747_NUM_OF_MINORS > 256);
    status = register_chrdev(DFS747_MAJOR, "spi", &dfs747_fops);
    if (status < 0) {
        return status;
    }

    dfs747_class = class_create(THIS_MODULE, "dfs747");
    if (IS_ERR(dfs747_class)) {
        unregister_chrdev(DFS747_MAJOR, dfs747_spi_driver.driver.name);
        return PTR_ERR(dfs747_class);
    }

    status = spi_register_driver(&dfs747_spi_driver);
    if (status < 0) {
        class_destroy(dfs747_class);
        unregister_chrdev(DFS747_MAJOR, dfs747_spi_driver.driver.name);
    }

    return status;
}

static void __exit dfs747_exit(void)
{
    DFS747_DEBUG("%s() is called!\n", __func__);

    spi_unregister_driver(&dfs747_spi_driver);
    class_destroy(dfs747_class);
    unregister_chrdev(DFS747_MAJOR, dfs747_spi_driver.driver.name);
}

module_init(dfs747_init);
module_exit(dfs747_exit);
module_param(msg_size, uint, S_IRUGO);

MODULE_AUTHOR("Corey Liu");
MODULE_DESCRIPTION("DFS747 driver");
MODULE_PARM_DESC(msg_size, "data bytes in biggest supported SPI message");
MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:dfs747");
