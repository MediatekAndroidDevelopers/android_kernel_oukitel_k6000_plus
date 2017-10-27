/*
 * Generic GPIO card-detect helper
 *
 * Copyright (C) 2011, Guennadi Liakhovetski <g.liakhovetski@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/mmc/host.h>
#include <linux/mmc/slot-gpio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/hct_include/hct_project_all_config.h>

struct mmc_gpio {
	struct gpio_desc *ro_gpio;
	struct gpio_desc *cd_gpio;
	bool override_ro_active_level;
	bool override_cd_active_level;
	char *ro_label;
	char cd_label[0];
};

#if 1

static struct timer_list * sd_debance_timer;
#if defined(__HCT_SETTING_SD_DEBANCE_TIME__)
	#define SD_DEBANCE_TIME_CHECK __HCT_SETTING_SD_DEBANCE_TIME__
#else
	#define SD_DEBANCE_TIME_CHECK  128 //700  // 512 ms
#endif
static int irq_card_states;
static int previous_time_card_states;
static int sd_card_irq_level=0;
static int irq_same_state_count=0;
void mmc_cd_debounce_process(void *dev_id);
extern unsigned int hct_msdc1_cd_irq;
/*
detect card is exit or not, refect sd_detect gpin pin states
1: card exit
0: card is not exit
*/
int hct_detect_mmc_card_removed(void *dev_id)
{
	struct mmc_host *host = dev_id;
      int card_insert=0;
	if(host->ops->get_cd)
		card_insert=host->ops->get_cd(host);
//	printk("jay: hct_detect_mmc_card =%d\n",card_insert);
      return card_insert;
}

static void sd_eint_timer_handler(void *dev_id)
{
	unsigned int status;
//	unsigned long flags;
	struct mmc_host *host = (struct mmc_host *)dev_id;

	/* disable interrupt for core 0 and it will run on core 0 only */
	
	status = hct_detect_mmc_card_removed(dev_id);
      if(status == previous_time_card_states)
      {
           irq_same_state_count ++;
      }
	else
	{
	     	previous_time_card_states = status;
		irq_same_state_count=0;
	 }
       if(irq_same_state_count>3)
       {
		irq_same_state_count=0;
		
		printk("jay: sd_eint_timer_handler irq_s=%d, previ_s=%d \n",irq_card_states ,previous_time_card_states);
		if(previous_time_card_states==irq_card_states)
		{
		
	   	   	printk("jay: sd_eint_timer_handler: repo card changed for core and enable irq~~~~ \n");
			sd_card_irq_level=~sd_card_irq_level;
			if(sd_card_irq_level)
				irq_set_irq_type(hct_msdc1_cd_irq, IRQF_TRIGGER_HIGH);
			else
				irq_set_irq_type(hct_msdc1_cd_irq, IRQF_TRIGGER_LOW);
                    
			host->trigger_card_event = true;
			mmc_detect_change(host, msecs_to_jiffies(200));        
		}
		
		enable_irq(hct_msdc1_cd_irq);
       }
	else
	{
		mmc_cd_debounce_process(dev_id);
	}
	return;

}


void mmc_cd_debounce_process(void *dev_id)
{
	struct timer_list *eint_timer = sd_debance_timer;
	/* assign this handler to execute on core 0 */
	
	init_timer(eint_timer);

	/* register timer for this sw debounce eint */
	eint_timer->expires = jiffies + msecs_to_jiffies(SD_DEBANCE_TIME_CHECK);

	eint_timer->data = (unsigned long)dev_id;
	eint_timer->function = ( void (*)(unsigned long))sd_eint_timer_handler;
//	 void (*function)(unsigned long)
	if (!timer_pending(eint_timer)) {
		add_timer(eint_timer);
	}

}
extern void msdc_sd_power_off_quick(void); //hct new add,sd power down
static irqreturn_t mmc_gpio_cd_irqt(int irq, void *dev_id)
{
	/* Schedule a card detection after a debounce timeout */
//	struct mmc_host *host = (struct mmc_host *)dev_id;
	hct_msdc1_cd_irq= irq;


	msdc_sd_power_off_quick(); //hct new add.sd power down
	irq_card_states = hct_detect_mmc_card_removed(dev_id);
	previous_time_card_states = irq_card_states;
	printk("jay:irq_prcess mmc_gpio_cd_irqt irq, card_cd = %d,\n",irq_card_states);
	disable_irq_nosync(hct_msdc1_cd_irq);


    mmc_cd_debounce_process(dev_id);
	return IRQ_HANDLED;
}

#else
static irqreturn_t mmc_gpio_cd_irqt(int irq, void *dev_id)
{
	/* Schedule a card detection after a debounce timeout */
	struct mmc_host *host = dev_id;
	printk("mmc_gpio_cd_irqt irq\n");

	host->trigger_card_event = true;
	mmc_detect_change(host, msecs_to_jiffies(200));

	return IRQ_HANDLED;
}
#endif

static int mmc_gpio_alloc(struct mmc_host *host)
{
	size_t len = strlen(dev_name(host->parent)) + 4;
	struct mmc_gpio *ctx;

	mutex_lock(&host->slot.lock);

	ctx = host->slot.handler_priv;
	if (!ctx) {
		/*
		 * devm_kzalloc() can be called after device_initialize(), even
		 * before device_add(), i.e., between mmc_alloc_host() and
		 * mmc_add_host()
		 */
		ctx = devm_kzalloc(&host->class_dev, sizeof(*ctx) + 2 * len,
				   GFP_KERNEL);
		if (ctx) {
			ctx->ro_label = ctx->cd_label + len;
			snprintf(ctx->cd_label, len, "%s cd", dev_name(host->parent));
			snprintf(ctx->ro_label, len, "%s ro", dev_name(host->parent));
			host->slot.handler_priv = ctx;
		}
	}

	mutex_unlock(&host->slot.lock);

	return ctx ? 0 : -ENOMEM;
}

int mmc_gpio_get_ro(struct mmc_host *host)
{
	struct mmc_gpio *ctx = host->slot.handler_priv;

	if (!ctx || !ctx->ro_gpio)
		return -ENOSYS;

	if (ctx->override_ro_active_level)
		return !gpiod_get_raw_value_cansleep(ctx->ro_gpio) ^
			!!(host->caps2 & MMC_CAP2_RO_ACTIVE_HIGH);

	return gpiod_get_value_cansleep(ctx->ro_gpio);
}
EXPORT_SYMBOL(mmc_gpio_get_ro);

int mmc_gpio_get_cd(struct mmc_host *host)
{
	struct mmc_gpio *ctx = host->slot.handler_priv;

	if (!ctx || !ctx->cd_gpio)
		return -ENOSYS;

	if (ctx->override_cd_active_level)
		return !gpiod_get_raw_value_cansleep(ctx->cd_gpio) ^
			!!(host->caps2 & MMC_CAP2_CD_ACTIVE_HIGH);

	return gpiod_get_value_cansleep(ctx->cd_gpio);
}
EXPORT_SYMBOL(mmc_gpio_get_cd);

/**
 * mmc_gpio_request_ro - request a gpio for write-protection
 * @host: mmc host
 * @gpio: gpio number requested
 *
 * As devm_* managed functions are used in mmc_gpio_request_ro(), client
 * drivers do not need to explicitly call mmc_gpio_free_ro() for freeing up,
 * if the requesting and freeing are only needed at probing and unbinding time
 * for once.  However, if client drivers do something special like runtime
 * switching for write-protection, they are responsible for calling
 * mmc_gpio_request_ro() and mmc_gpio_free_ro() as a pair on their own.
 *
 * Returns zero on success, else an error.
 */
int mmc_gpio_request_ro(struct mmc_host *host, unsigned int gpio)
{
	struct mmc_gpio *ctx;
	int ret;

	if (!gpio_is_valid(gpio))
		return -EINVAL;

	ret = mmc_gpio_alloc(host);
	if (ret < 0)
		return ret;

	ctx = host->slot.handler_priv;

	ret = devm_gpio_request_one(&host->class_dev, gpio, GPIOF_DIR_IN,
				    ctx->ro_label);
	if (ret < 0)
		return ret;

	ctx->override_ro_active_level = true;
	ctx->ro_gpio = gpio_to_desc(gpio);

	return 0;
}
EXPORT_SYMBOL(mmc_gpio_request_ro);

extern void hct_msdc_sd_set_debounce(void);

void mmc_gpiod_request_cd_irq(struct mmc_host *host)
{
	struct mmc_gpio *ctx = host->slot.handler_priv;
	int ret, irq;

	if (host->slot.cd_irq >= 0 || !ctx || !ctx->cd_gpio)
		return;

	irq = gpiod_to_irq(ctx->cd_gpio);

	/*
	 * Even if gpiod_to_irq() returns a valid IRQ number, the platform might
	 * still prefer to poll, e.g., because that IRQ number is already used
	 * by another unit and cannot be shared.
	 */
	if (irq >= 0 && host->caps & MMC_CAP_NEEDS_POLL)
		irq = -EINVAL;

	if (irq >= 0) {
#if 1
		if(sd_card_irq_level==0)
			ret = devm_request_threaded_irq(&host->class_dev, irq,
				NULL, mmc_gpio_cd_irqt,
				IRQF_TRIGGER_LOW | IRQF_ONESHOT,
				ctx->cd_label, host);
		else
			ret = devm_request_threaded_irq(&host->class_dev, irq,
				NULL, mmc_gpio_cd_irqt,
				IRQF_TRIGGER_HIGH | IRQF_ONESHOT,
				ctx->cd_label, host);               

#else
		ret = devm_request_threaded_irq(&host->class_dev, irq,
			NULL, mmc_gpio_cd_irqt,
			IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
			ctx->cd_label, host);
#endif
		if (ret < 0)
			irq = ret;
		else
			enable_irq_wake(irq);
	}

	host->slot.cd_irq = irq;

	if (irq < 0)
		host->caps |= MMC_CAP_NEEDS_POLL;
}
EXPORT_SYMBOL(mmc_gpiod_request_cd_irq);

/**
 * mmc_gpio_request_cd - request a gpio for card-detection
 * @host: mmc host
 * @gpio: gpio number requested
 * @debounce: debounce time in microseconds
 *
 * As devm_* managed functions are used in mmc_gpio_request_cd(), client
 * drivers do not need to explicitly call mmc_gpio_free_cd() for freeing up,
 * if the requesting and freeing are only needed at probing and unbinding time
 * for once.  However, if client drivers do something special like runtime
 * switching for card-detection, they are responsible for calling
 * mmc_gpio_request_cd() and mmc_gpio_free_cd() as a pair on their own.
 *
 * If GPIO debouncing is desired, set the debounce parameter to a non-zero
 * value. The caller is responsible for ensuring that the GPIO driver associated
 * with the GPIO supports debouncing, otherwise an error will be returned.
 *
 * Returns zero on success, else an error.
 */
int mmc_gpio_request_cd(struct mmc_host *host, unsigned int gpio,
			unsigned int debounce)
{
	struct mmc_gpio *ctx;
	int ret;

	ret = mmc_gpio_alloc(host);
	if (ret < 0)
		return ret;

	ctx = host->slot.handler_priv;

	ret = devm_gpio_request_one(&host->class_dev, gpio, GPIOF_DIR_IN,
				    ctx->cd_label);
	if (ret < 0)
		/*
		 * don't bother freeing memory. It might still get used by other
		 * slot functions, in any case it will be freed, when the device
		 * is destroyed.
		 */
		return ret;

	if (debounce) {
		ret = gpio_set_debounce(gpio, debounce);
		if (ret < 0)
			return ret;
	}

	ctx->override_cd_active_level = true;
	ctx->cd_gpio = gpio_to_desc(gpio);

	return 0;
}
EXPORT_SYMBOL(mmc_gpio_request_cd);

/**
 * mmc_gpio_free_ro - free the write-protection gpio
 * @host: mmc host
 *
 * It's provided only for cases that client drivers need to manually free
 * up the write-protection gpio requested by mmc_gpio_request_ro().
 */
void mmc_gpio_free_ro(struct mmc_host *host)
{
	struct mmc_gpio *ctx = host->slot.handler_priv;
	int gpio;

	if (!ctx || !ctx->ro_gpio)
		return;

	gpio = desc_to_gpio(ctx->ro_gpio);
	ctx->ro_gpio = NULL;

	devm_gpio_free(&host->class_dev, gpio);
}
EXPORT_SYMBOL(mmc_gpio_free_ro);

/**
 * mmc_gpio_free_cd - free the card-detection gpio
 * @host: mmc host
 *
 * It's provided only for cases that client drivers need to manually free
 * up the card-detection gpio requested by mmc_gpio_request_cd().
 */
void mmc_gpio_free_cd(struct mmc_host *host)
{
	struct mmc_gpio *ctx = host->slot.handler_priv;
	int gpio;

	if (!ctx || !ctx->cd_gpio)
		return;

	if (host->slot.cd_irq >= 0) {
		devm_free_irq(&host->class_dev, host->slot.cd_irq, host);
		host->slot.cd_irq = -EINVAL;
	}

	gpio = desc_to_gpio(ctx->cd_gpio);
	ctx->cd_gpio = NULL;

	devm_gpio_free(&host->class_dev, gpio);
}
EXPORT_SYMBOL(mmc_gpio_free_cd);

/**
 * mmc_gpiod_request_cd - request a gpio descriptor for card-detection
 * @host: mmc host
 * @con_id: function within the GPIO consumer
 * @idx: index of the GPIO to obtain in the consumer
 * @override_active_level: ignore %GPIO_ACTIVE_LOW flag
 * @debounce: debounce time in microseconds
 * @gpio_invert: will return whether the GPIO line is inverted or not, set
 * to NULL to ignore
 *
 * Use this function in place of mmc_gpio_request_cd() to use the GPIO
 * descriptor API.  Note that it is paired with mmc_gpiod_free_cd() not
 * mmc_gpio_free_cd().  Note also that it must be called prior to mmc_add_host()
 * otherwise the caller must also call mmc_gpiod_request_cd_irq().
 *
 * Returns zero on success, else an error.
 */
int mmc_gpiod_request_cd(struct mmc_host *host, const char *con_id,
			 unsigned int idx, bool override_active_level,
			 unsigned int debounce, bool *gpio_invert)
{
	struct mmc_gpio *ctx;
	struct gpio_desc *desc;
	int ret;

	ret = mmc_gpio_alloc(host);
	if (ret < 0)
		return ret;

	ctx = host->slot.handler_priv;

	if (!con_id)
		con_id = ctx->cd_label;

	desc = devm_gpiod_get_index(host->parent, con_id, idx, GPIOD_IN);
	if (IS_ERR(desc))
		return PTR_ERR(desc);

	if (debounce) {
		ret = gpiod_set_debounce(desc, debounce);
		if (ret < 0)
			return ret;
	}

	if (gpio_invert)
		*gpio_invert = !gpiod_is_active_low(desc);

	ctx->override_cd_active_level = override_active_level;
	ctx->cd_gpio = desc;

	sd_debance_timer =
			kmalloc(sizeof(struct timer_list) , GFP_KERNEL);
	sd_debance_timer->expires = 0;
	sd_debance_timer->data = 0;
	sd_debance_timer->function = NULL;
	
	init_timer(sd_debance_timer);

	return 0;
}
EXPORT_SYMBOL(mmc_gpiod_request_cd);

/**
 * mmc_gpiod_request_ro - request a gpio descriptor for write protection
 * @host: mmc host
 * @con_id: function within the GPIO consumer
 * @idx: index of the GPIO to obtain in the consumer
 * @override_active_level: ignore %GPIO_ACTIVE_LOW flag
 * @debounce: debounce time in microseconds
 * @gpio_invert: will return whether the GPIO line is inverted or not,
 * set to NULL to ignore
 *
 * Use this function in place of mmc_gpio_request_ro() to use the GPIO
 * descriptor API.  Note that it is paired with mmc_gpiod_free_ro() not
 * mmc_gpio_free_ro().
 *
 * Returns zero on success, else an error.
 */
int mmc_gpiod_request_ro(struct mmc_host *host, const char *con_id,
			 unsigned int idx, bool override_active_level,
			 unsigned int debounce, bool *gpio_invert)
{
	struct mmc_gpio *ctx;
	struct gpio_desc *desc;
	int ret;

	ret = mmc_gpio_alloc(host);
	if (ret < 0)
		return ret;

	ctx = host->slot.handler_priv;

	if (!con_id)
		con_id = ctx->ro_label;

	desc = devm_gpiod_get_index(host->parent, con_id, idx, GPIOD_IN);
	if (IS_ERR(desc))
		return PTR_ERR(desc);

	if (debounce) {
		ret = gpiod_set_debounce(desc, debounce);
		if (ret < 0)
			return ret;
	}

	if (gpio_invert)
		*gpio_invert = !gpiod_is_active_low(desc);

	ctx->override_ro_active_level = override_active_level;
	ctx->ro_gpio = desc;

	return 0;
}
EXPORT_SYMBOL(mmc_gpiod_request_ro);

/**
 * mmc_gpiod_free_cd - free the card-detection gpio descriptor
 * @host: mmc host
 *
 * It's provided only for cases that client drivers need to manually free
 * up the card-detection gpio requested by mmc_gpiod_request_cd().
 */
void mmc_gpiod_free_cd(struct mmc_host *host)
{
	struct mmc_gpio *ctx = host->slot.handler_priv;

	if (!ctx || !ctx->cd_gpio)
		return;

	if (host->slot.cd_irq >= 0) {
		devm_free_irq(&host->class_dev, host->slot.cd_irq, host);
		host->slot.cd_irq = -EINVAL;
	}

	devm_gpiod_put(host->parent, ctx->cd_gpio);

	ctx->cd_gpio = NULL;
}
EXPORT_SYMBOL(mmc_gpiod_free_cd);
