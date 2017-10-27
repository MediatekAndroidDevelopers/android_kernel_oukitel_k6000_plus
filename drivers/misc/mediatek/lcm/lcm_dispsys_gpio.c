#include <linux/string.h>
#include <linux/wait.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of_gpio.h>
#include <asm-generic/gpio.h>

#include "lcm_drv.h"
#include "ddp_irq.h"

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/gpio.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#endif


extern int mt_dsi_get_gpio_info(struct platform_device *pdev);

static int lcm_gpio_probe(struct platform_device *dev)
{
	mt_dsi_get_gpio_info(dev);
	return 0;
}
static const struct of_device_id lcm_gpio_of_ids[] = {
	{.compatible = "mediatek,mt6753-dispsys",},
	{}
};

static struct platform_driver lcm_gpio_driver = {
	.driver = {
		   .name = "hct_lcm",
	#ifdef CONFIG_OF
		   .of_match_table = lcm_gpio_of_ids,
	#endif
		   },
	.probe = lcm_gpio_probe,
};


static int __init lcm_gpio_init(void)
{
	pr_debug("HCT LCM GPIO driver init\n");
	if (platform_driver_register(&lcm_gpio_driver) != 0) {
		pr_err("unable to register LCM GPIO driver.\n");
		return -1;
	}
	return 0;
}

static void __exit lcm_gpio_exit(void)
{
	pr_debug("HCT LCM GPIO driver exit\n");
	platform_driver_unregister(&lcm_gpio_driver);
}
module_init(lcm_gpio_init);
module_exit(lcm_gpio_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Hct LCM GPIO driver");
MODULE_AUTHOR("Lan Shi Ming<lanshiming@hct.sh.cn>");
