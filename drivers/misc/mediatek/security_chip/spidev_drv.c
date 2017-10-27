/* 
 * spidev_drv.c
 * zongpeng
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spi/spi.h>
#include "mt_spi.h" //modify zy

static struct mt_chip_conf spidev_chip_config = {
    .setuptime = 3,
    .holdtime = 3,
    .high_time = 5,
    .low_time = 5,
    .cs_idletime = 2,
    .ulthgh_thrsh = 0,
    .cpol = 0,
    .cpha = 0,
    .rx_mlsb = 1,
    .tx_mlsb = 1,
    .tx_endian = 0,
    .rx_endian = 0,
    .com_mod = DMA_TRANSFER,
    .pause = 0,
    .finish_intr = 1,
    .deassert = 0,
    .ulthigh = 0,
    .tckdly = 0,
};

static struct spi_board_info spi_board_devs[] __initdata = {
	[0] = {
		.modalias        = "ssx1207",
		.bus_num         = 1,
		.chip_select     = 0,
		.mode            = SPI_MODE_3,
		//.max_speed_hz    = 10000000,
		
		.controller_data = &spidev_chip_config,
	},
};

static int spidev_init(void)
{
	/* create spi_device */
	return spi_register_board_info(spi_board_devs, ARRAY_SIZE(spi_board_devs));
}

static void spidev_exit(void)
{
	return;
}

module_init(spidev_init);
module_exit(spidev_exit);
MODULE_LICENSE("GPL");

