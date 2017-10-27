#include "finger.h"
#include "config.h"
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/of_irq.h>
#include <linux/completion.h>
#include <linux/delay.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#endif



#ifdef CONFIG_OF
static const struct of_device_id fp_of_match[] = {
	{.compatible = "mediatek,hct_finger",},
	{},
};
#endif
//------------------------------------------------------------------------------


/**
 * this function olny use thread ex.open
 *
 */

typedef enum DTS_STATE {
    STATE_CS_CLR,
    STATE_CS_SET,
    STATE_MI_CLR,
    STATE_MI_SET,
    STATE_MO_CLR,
    STATE_MO_SET,
    STATE_CLK_CLR,
    STATE_CLK_SET,
    STATE_RST_CLR,
    STATE_RST_SET,
    STATE_EINT,
    STATE_EINT_CLR,
    STATE_EINT_SET,
    STATE_MAX,  /* array size */
} dts_status_t;
static struct pinctrl* this_pctrl; /* static pinctrl instance */
/* DTS state mapping name */
static const char* dts_state_name[STATE_MAX] = {
    "spi_cs_clr",
    "spi_cs_set",
    "spi_mi_clr",
    "spi_mi_set",
    "spi_mo_clr",
    "spi_mo_set",
    "spi_mclk_clr",
    "spi_mclk_set",
    "finger_rst_clr",
    "finger_rst_set",
    "eint",
    "eint_clr",
    "eint_set",
};

/* pinctrl implementation */
inline static long dts_set_state(const char* name)
{
    long ret = 0;
    struct pinctrl_state* pin_state = 0;
    BUG_ON(!this_pctrl);
    pin_state = pinctrl_lookup_state(this_pctrl, name);

    if (IS_ERR(pin_state)) {
        pr_err("sunwave ----finger set state '%s' failed\n", name);
        ret = PTR_ERR(pin_state);
        return ret;
    }

    //printk("sunwave ---- dts_set_state = %s \n", name);
    /* select state! */
    ret = pinctrl_select_state(this_pctrl, pin_state);

    if ( ret < 0) {
        printk("sunwave --------pinctrl_select_state %s failed\n", name);
    }

    return ret;
}


inline static long dts_select_state(dts_status_t s)
{
    BUG_ON(!((unsigned int)(s) < (unsigned int)(STATE_MAX)));
    return dts_set_state(dts_state_name[s]);
}
int mt6797_dts_spi_mod(struct spi_device**  spi)
{
    dts_select_state(STATE_CS_SET);
    dts_select_state(STATE_MI_SET);
    dts_select_state(STATE_MO_SET);
    dts_select_state(STATE_CLK_SET);
    return 0;
}

int mt6797_dts_reset(struct spi_device**  spi)
{
#if 0
    dts_select_state(STATE_RST_SET);
    //dts_select_state(STATE_EINT_SET);
    msleep(10);
    dts_select_state(STATE_RST_CLR);
    //dts_select_state(STATE_EINT_CLR);
    msleep(20);
    dts_select_state(STATE_RST_SET);
    //dts_select_state(STATE_EINT_SET);
#else	
    hct_finger_set_reset(1);
    msleep(10);
    hct_finger_set_reset(0);
    msleep(20);
    hct_finger_set_reset(1);
#endif	
    return 0;
}

static int  mt6797_dts_gpio_init(struct spi_device**   spidev)
{
    sunwave_sensor_t*  sunwave = spi_to_sunwave(spidev);
    struct device_node *node = NULL;
    int sw_detect_irq= -1;
    /* set power pin to high */
    hct_waite_for_finger_dts_paser();

    hct_finger_set_power(1);
    msleep(1);

    /*set reset pin to high*/
    hct_finger_set_reset(1);
    //SW_DBG("sw_driver_mtk probe\n");


    //irq
    //mt_set_gpio_mode(GPIO_SW_INT_PIN,GPIO_SUNWAVE_IRQ_M_EINT);
    //mt_set_gpio_dir(GPIO_SW_INT_PIN,GPIO_DIR_IN);
    // mt_set_gpio_pull_enable(GPIO_SW_INT_PIN, GPIO_PULL_DISABLE);
    //mt_set_gpio_pull_enable(GPIO_SW_INT_PIN, GPIO_PULL_ENABLE);
    //mt_set_gpio_pull_select(GPIO_SW_INT_PIN, GPIO_PULL_UP);

    hct_finger_set_spi_mode(1); // set to spi mode
    hct_finger_set_eint(1);
    //sunwave->finger->gpio_rst = GPIO_SW_RST_PIN;


 
    node = of_find_matching_node(node, fp_of_match);
 
    if (node){
           sw_detect_irq = irq_of_parse_and_map(node, 0);

    }else{
           printk("fingerprint request_irq can not find fp eint device node!.");
           return -1;
     }

    sunwave->standby_irq = sw_detect_irq;
    //sunwave->standby_irq = gpio_to_irq(sw_detect_irq);

    return 0;
}


static int  mt6797_dts_irq_hander(struct spi_device** spi)
{
    spi_to_sunwave(spi);
    return 0;
}


static int mt6797_dts_speed(struct spi_device**    spi, unsigned int speed)
{
#define SPI_MODULE_CLOCK   (100*1000*1000)
    struct mt_chip_conf *config;
    unsigned int    time = SPI_MODULE_CLOCK / speed;
    config = (struct mt_chip_conf *)(*spi)->controller_data;
    config->low_time = time / 2;
    config->high_time = time / 2;

    if (time % 2) {
        config->high_time = config->high_time + 1;
    }

    //(chip_config->low_time + chip_config->high_time);
    return 0;
}

static finger_t  finger_sensor = {
    .ver                    = 0,
#if __SUNWAVE_SPI_DMA_MODE_EN
    .attribute              = DEVICE_ATTRIBUTE_NONE,
#else // SPI FIFO MODE
    .attribute              = DEVICE_ATTRIBUTE_SPI_FIFO,
#endif

    .write_then_read        = 0,
    .irq_hander             = mt6797_dts_irq_hander,
    .irq_request            = 0,
    .irq_free               = 0,
    .init                   = mt6797_dts_gpio_init,
    .reset                  = mt6797_dts_reset,
    .speed          = mt6797_dts_speed,
};


void mt6797_dts_register_finger(sunwave_sensor_t*    sunwave)
{
    sunwave->finger = &finger_sensor;
}
EXPORT_SYMBOL_GPL(mt6797_dts_register_finger);
