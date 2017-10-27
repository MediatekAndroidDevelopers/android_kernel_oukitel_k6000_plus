/* Copyright (C) MicroArray
 * MicroArray Fprint Driver Code
 * mtk-settings.c
 * Date: 2016-10-12
 * Version: v4.0.01
 * Author: guq
 * Contact: guq@microarray.com.cn
 */

#include "mtk-settings.h"

static int ret;

extern int hct_finger_set_power(int cmd);
extern int hct_finger_set_18v_power(int cmd);
extern int hct_finger_set_reset(int cmd);
extern int hct_finger_set_eint(int cmd);
extern int hct_finger_set_spi_mode(int cmd);

static struct device_node *node;
#ifdef HCT_UN
//pin control sturct data, define using for the dts settings,
struct pinctrl *mas_finger_pinctrl;
struct pinctrl_state 		*mas_finger_power2v8_on, *mas_finger_power2v8_off, 	//power2v8
							*mas_finger_power1v8_on, *mas_finger_power1v8_off,	//power1v8
							*mas_finger_eint_on, *mas_finger_eint_off,			//eint
							*mas_spi_ck_on, *mas_spi_ck_off,					//for ck
							*mas_spi_cs_on, *mas_spi_cs_off,					//for cs
							*mas_spi_mi_on, *mas_spi_mi_off,					//for mi
							*mas_spi_mo_on, *mas_spi_mo_off,					//for mo
							*mas_spi_default;									//same odms only use default to setting the dts



/**
 *    the platform struct start,for getting the platform device to set gpio state
 */
//#ifdef CONFIG_OF
static struct of_device_id sof_match[] = {
        { .compatible = MA_DTS_NAME, },						//this name is used for matching the dts device for settings the gpios
};
MODULE_DEVICE_TABLE(of, sof_match);
//#endif
static struct platform_driver spdrv = {
        .probe    = mas_plat_probe,
        .remove  = mas_plat_remove,
        .driver = {
                .name  = MA_DRV_NAME,
                .owner = THIS_MODULE,                   
//#ifdef CONFIG_OF
                .of_match_table = sof_match,
//#endif
        }
};
#endif
/**
  *  the platform struct start,for getting the platform device to set gpio state end
  */


/**
 *  the spi struct date start,for getting the spi_device to set the spi clock enable start
 */

struct spi_device_id sdev_id = {MA_DRV_NAME, 0};
struct spi_driver sdrv = {
        .driver = {
                .name = MA_DRV_NAME,
                .bus = &spi_bus_type,
                .owner = THIS_MODULE,
        },
        .probe = mas_probe,
        .remove = mas_remove,
        .id_table = &sdev_id,
};
//driver end
static struct mt_chip_conf smt_conf = {
    .setuptime=10,
    .holdtime=10,
//#ifdef CONFIG_FINGERPRINT_MICROARRAY_A80T
#if defined(CONFIG_FINGERPRINT_MICROARRAY_A80T) || defined(CONFIG_FINGERPRINT_MICROARRAY_A80T_TEE)//microarray IC A80T
    .high_time=15, // 10--6m 15--4m 20--3m 30--2m [ 60--1m 120--0.5m  300--0.2m]
    .low_time=15,
#else
    .high_time=10, // 10--6m 15--4m 20--3m 30--2m [ 60--1m 120--0.5m  300--0.2m]80 
    .low_time=10,
#endif
    .cs_idletime=10,
    .ulthgh_thrsh=0,
    .cpol=0,
    .cpha=0,
    .rx_mlsb=SPI_MSB,
    .tx_mlsb=SPI_MSB,
    .tx_endian=0,
    .rx_endian=0,
    .com_mod=FIFO_TRANSFER,
    .pause=0,
    .finish_intr=5,
    .deassert=0,
    .ulthigh=0,
    .tckdly=0,
};

struct spi_board_info smt_info[] __initdata = {
        [0] = {
                .modalias = MA_DRV_NAME,
                .max_speed_hz = (6*1000000),
                .bus_num = 0,
                .chip_select = FINGERPRINT_MICROARRAY_CS,
                .mode = SPI_MODE_0,
                .controller_data = &smt_conf,
        },
};
//device end

/**
 *  the spi struct date start,for getting the spi_device to set the spi clock enable end
 */


void mas_select_transfer(struct spi_device *spi, int len) {
    static int mode = -1;
    int tmp = len>32? DMA_TRANSFER: FIFO_TRANSFER;
    struct mt_chip_conf *conf = NULL;
    if(tmp!=mode) {
        conf = (struct mt_chip_conf *) spi->controller_data;
        conf->com_mod = tmp;
        spi_setup(spi);
        mode = tmp;
    }
}


void ma_spi_change(struct spi_device *spi, unsigned int  speed, int flag)
{
    struct mt_chip_conf *mcc = (struct mt_chip_conf *)spi->controller_data;
    if(flag == 0) { 
        mcc->com_mod = FIFO_TRANSFER;
    } else {
        mcc->com_mod = DMA_TRANSFER;
    }   
    mcc->high_time = speed;
    mcc->low_time = speed;
    if(spi_setup(spi) < 0){
        MALOGE("change the spi error!\n");
    }    
}

int mas_get_platform(void) {
#ifdef HCT_UN
	ret = platform_driver_register(&spdrv);
	if(ret){
		MALOGE("platform_driver_register");
	}
#endif
	ret = spi_register_board_info(smt_info, ARRAY_SIZE(smt_info));
	if(ret){
		MALOGE("spi_register_board_info");
	}
	ret = spi_register_driver(&sdrv);
	if(ret) {
		MALOGE("spi_register_driver");
	}
	return ret;
}

int mas_remove_platform(void){
	spi_unregister_driver(&sdrv);
	return 0;
}

#ifdef HCT_UN
int mas_finger_get_gpio_info(struct platform_device *pdev){
   // struct device_node *node;
    //MALOGD("start!");
	node = of_find_compatible_node(NULL, NULL, MA_DTS_NAME);
	mas_finger_pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(mas_finger_pinctrl)) {
		ret = PTR_ERR(mas_finger_pinctrl);
		dev_err(&pdev->dev, "mas_finger_pinctrl cannot find pinctrl\n");
		return ret;
	}

/**		this is the demo, setup follow the requirement
 *		mas_finger_eint_on = pinctrl_lookup_state(mas_finger_pinctrl, "finger_int_as_int");
 *		if (IS_ERR(mas_finger_eint_on)) {
 *			ret = PTR_ERR(mas_finger_eint_on);
 *			dev_err(&pdev->dev, " Cannot find mas_finger pinctrl mas_finger_eint_on!\n");
 *			return ret;
 *		}
 *      if needed, change the dts label and the pinctrl for the other gpio
 */
 	

 	mas_finger_power2v8_on = pinctrl_lookup_state(mas_finger_pinctrl, "finger_power_en1");
	if (IS_ERR(mas_finger_power2v8_on)) {
		ret = PTR_ERR(mas_finger_power2v8_on);
		dev_err(&pdev->dev, " Cannot find mas_finger_power2v8_on pinctrl!\n");
		return ret;
	}
 	mas_finger_power2v8_off = pinctrl_lookup_state(mas_finger_pinctrl, "finger_power_en0");
	if (IS_ERR(mas_finger_power2v8_off)) {
		ret = PTR_ERR(mas_finger_power2v8_off);
		dev_err(&pdev->dev, " Cannot find mas_finger_power2v8_off pinctrl!\n");
		return ret;
	}

	mas_finger_power1v8_on = pinctrl_lookup_state(mas_finger_pinctrl, "finger_power_18v_en1");
	if (IS_ERR(mas_finger_power1v8_on)) {
		ret = PTR_ERR(mas_finger_power1v8_on);
		dev_err(&pdev->dev, " Cannot find mas_finger_power1v8_on pinctrl!\n");
		return ret;
	}

	mas_finger_power1v8_off = pinctrl_lookup_state(mas_finger_pinctrl, "finger_power_18v_en0");
	if (IS_ERR(mas_finger_power1v8_off)) {
		ret = PTR_ERR(mas_finger_power1v8_off);
		dev_err(&pdev->dev, " Cannot find mas_finger_power1v8_off pinctrl!\n");
		return ret;
	}

 	mas_spi_mi_on = pinctrl_lookup_state(mas_finger_pinctrl, "finger_spi0_mi_as_spi0_mi");
	if (IS_ERR(mas_spi_mi_on)) {
		ret = PTR_ERR(mas_spi_mi_on);
		dev_err(&pdev->dev, " Cannot find mas_spi_mi_on pinctrl!\n");
		return ret;
	}
 	mas_spi_mi_off = pinctrl_lookup_state(mas_finger_pinctrl, "finger_spi0_mi_as_gpio");
	if (IS_ERR(mas_spi_mi_off)) {
		ret = PTR_ERR(mas_spi_mi_off);
		dev_err(&pdev->dev, " Cannot find mas_spi_mi_off pinctrl!\n");
		return ret;
	}
 	mas_spi_mo_on = pinctrl_lookup_state(mas_finger_pinctrl, "finger_spi0_mo_as_spi0_mo");
	if (IS_ERR(mas_spi_mo_on)) {
		ret = PTR_ERR(mas_spi_mo_on);
		dev_err(&pdev->dev, " Cannot find mas_spi_mo_on pinctrl!\n");
		return ret;
	}

	mas_spi_mo_off = pinctrl_lookup_state(mas_finger_pinctrl, "finger_spi0_mo_as_gpio");
	if (IS_ERR(mas_spi_mo_off)) {
		ret = PTR_ERR(mas_spi_mo_off);
		dev_err(&pdev->dev, " Cannot find mas_spi_mo_off!\n");
		return ret;
	}

	mas_spi_ck_on = pinctrl_lookup_state(mas_finger_pinctrl, "finger_spi0_clk_as_spi0_clk");
	if (IS_ERR(mas_spi_ck_on)) {
		ret = PTR_ERR(mas_spi_ck_on);
		dev_err(&pdev->dev, " Cannot find mas_spi_ck_on pinctrl!\n");
		return ret;
	}

	mas_spi_ck_off = pinctrl_lookup_state(mas_finger_pinctrl, "finger_spi0_clk_as_gpio");
	if (IS_ERR(mas_spi_ck_off)) {
		ret = PTR_ERR(mas_spi_ck_off);
		dev_err(&pdev->dev, " Cannot find mas_spi_ck_off pinctrl !\n");
		return ret;
	}

	mas_spi_cs_on = pinctrl_lookup_state(mas_finger_pinctrl, "finger_spi0_cs_as_spi0_cs");
	if (IS_ERR(mas_spi_cs_on)) {
		ret = PTR_ERR(mas_spi_cs_on);
		dev_err(&pdev->dev, " Cannot find mas_spi_cs_on pinctrl!\n");
		return ret;
	}

	mas_spi_cs_off = pinctrl_lookup_state(mas_finger_pinctrl, "finger_spi0_cs_as_gpio");
	if (IS_ERR(mas_spi_cs_off)) {
		ret = PTR_ERR(mas_spi_cs_off);
		dev_err(&pdev->dev, " Cannot find mas_spi_cs_off pinctrl!\n");
		return ret;
	}

	mas_finger_eint_on = pinctrl_lookup_state(mas_finger_pinctrl, "finger_int_as_int");
	if (IS_ERR(mas_finger_eint_on)) {
		ret = PTR_ERR(mas_finger_eint_on);
		dev_err(&pdev->dev, " Cannot find mas_finger_eint_on pinctrl!\n");
		return ret;
	}

        return 0;
}



int mas_finger_set_spi(int cmd){
//#if 0
	switch(cmd)
	{
		case 0:
			if( (!IS_ERR(mas_spi_cs_off)) & (!IS_ERR(mas_spi_ck_off)) & (!IS_ERR(mas_spi_mi_off)) & (!IS_ERR(mas_spi_mo_off)) ){
				pinctrl_select_state(mas_finger_pinctrl, mas_spi_cs_off);
				pinctrl_select_state(mas_finger_pinctrl, mas_spi_ck_off);
				pinctrl_select_state(mas_finger_pinctrl, mas_spi_mi_off);
				pinctrl_select_state(mas_finger_pinctrl, mas_spi_mo_off);
			}else{
				MALOGE("mas_spi_gpio_slect_pinctrl cmd=0 err!");
				return -1;
			}
			break;
		case 1:
			if( (!IS_ERR(mas_spi_cs_on)) & (!IS_ERR(mas_spi_ck_on)) & (!IS_ERR(mas_spi_mi_on)) & (!IS_ERR(mas_spi_mo_on)) ){
				pinctrl_select_state(mas_finger_pinctrl, mas_spi_cs_on);
				pinctrl_select_state(mas_finger_pinctrl, mas_spi_ck_on);
	        	pinctrl_select_state(mas_finger_pinctrl, mas_spi_mi_on);
				pinctrl_select_state(mas_finger_pinctrl, mas_spi_mo_on);
			}else{
				MALOGE("mas_spi_gpio_slect_pinctrl cmd=1 err!");
				return -1;
			}
			break;
	}
//#endif
	return 0;
}

int mas_finger_set_power(int cmd)
{
//#if 0
	switch (cmd)
		{
		case 0 : 		
			if( (!IS_ERR(mas_finger_power2v8_off)) & (!IS_ERR(mas_finger_power1v8_off)) ){
				pinctrl_select_state(mas_finger_pinctrl, mas_finger_power2v8_off);
				pinctrl_select_state(mas_finger_pinctrl, mas_finger_power1v8_off);
			}else{
				MALOGE("mas_power_gpio_slect_pinctrl cmd=0 err!");
				return -1;
			}
		break;
		case 1 : 		
			if( (!IS_ERR(mas_finger_power2v8_on)) & (!IS_ERR(mas_finger_power1v8_on)) ){
				pinctrl_select_state(mas_finger_pinctrl, mas_finger_power2v8_on);
				pinctrl_select_state(mas_finger_pinctrl, mas_finger_power1v8_on);
			}else{
				MALOGE("mas_power_gpio_slect_pinctrl cmd=1 err!");
				return -1;
			}
		break;
		}
//#endif
	return 0;
}


int mas_finger_set_eint(int cmd)
{
	switch (cmd)
		{
		case 0 : 		
			if(!IS_ERR(mas_finger_eint_off)){
				pinctrl_select_state(mas_finger_pinctrl, mas_finger_eint_off);
			}else{
				MALOGE("mas_eint_gpio_slect_pinctrl cmd=0 err!");
				return -1;
			}		
			break;
		case 1 : 		
			if(!IS_ERR(mas_finger_eint_on)){
				pinctrl_select_state(mas_finger_pinctrl, mas_finger_eint_on);
			}else{
				MALOGE("mas_eint_gpio_slect_pinctrl cmd=1 err!");
				return -1;
			}
			break;
		}
	return 0;
}
#endif

/*
 * this is a demo function,if the power on-off switch by other way
 * modify it as the right way
 * on_off 1 on   0 off
 */
int mas_switch_power(unsigned int on_off){
	ret = 0;
	hct_finger_set_power(on_off);	//use this fuction directly if the dts way
//ret = mtk_regulator_enable(&fp_rt5081_ldo, on_off); //ldo power2v8 
	if (ret < 0) {
		printk("%s: mtk_regulator_enable enable rt5081_ldo fail!!\n", __func__);
		return -1;
	}
	return 0;
}




int mas_finger_set_gpio_info(int cmd){
	ret = 0;
#ifdef HCT_UN
	ret |= mas_finger_set_spi(cmd);
	ret |= mas_finger_set_power(cmd);
	ret |= mas_finger_set_eint(cmd);
#else
	hct_finger_set_power(1);
//	hct_finger_set_18v_power(1);
	hct_finger_set_reset(1);
	hct_finger_set_spi_mode(1);
	hct_finger_set_eint(1);
#endif
	return ret;
}

void mas_enable_spi_clock(struct spi_device *spi){
	mt_spi_enable_master_clk(spi); 
}

void mas_disable_spi_clock(struct spi_device *spi){
	mt_spi_disable_master_clk(spi); 
}

unsigned int mas_get_irq(void){
    struct device_node *node = NULL;
    //MALOGF("start");
    node = of_find_compatible_node(NULL, NULL, MA_EINT_DTS_NAME);
    return irq_of_parse_and_map(node, 0);
}
/*
 * this function used for check the interrupt gpio state
 * @index 0 gpio level 1 gpio mode, often use 0
 * @return 0 gpio low 1 gpio high if index = 1,the return is the gpio mode
 *  		under 0  the of_property_read_u32_index return errno,check the dts as below:
 * last but not least use this function must checkt the label on dts file, after is an example:
 * ma_finger: ma_finger{
 *		compatible = "mediatek,afs120x";
 *		finger_int_pin = <100 0>;
 * }
 */
int mas_get_interrupt_gpio(unsigned int index){
	static unsigned int finger_int_pin;
	int val;
	val = of_property_read_u32_index(node, MA_INT_PIN_LABEL, index, &finger_int_pin);
	//the MA_INT_PIN_LABEL as same as the dts description, such as "finger_int_pin",modify in the mtk-settings.h file
	if(val < 0){
		return val;
	}
	finger_int_pin |=  0x80000000;
	val = mt_get_gpio_in(finger_int_pin);
	return val;
}

