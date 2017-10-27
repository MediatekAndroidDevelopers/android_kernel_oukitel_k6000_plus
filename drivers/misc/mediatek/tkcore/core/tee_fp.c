#include <linux/kernel.h>

#ifdef CONFIG_MEDIATEK_SOLUTION

#if !defined(CONFIG_MTK_CLKMGR)
#include <linux/clk.h>
#else
#include <linux/slab.h>
#include <linux/string.h>

/* need not include <MTK platform headers> */
extern int enable_clock(int id, char *name);
extern int disable_clock(int id, char *name);

#endif

#endif

#include "tee_fp.h"

/* initialized to zero by default */
#ifdef CONFIG_MEDIATEK_SOLUTION
#if !defined(CONFIG_MTK_CLKMGR)

#define SPI_MAX_BUS_NUM 16
#define TEE_FP_SPI_BUS_NUM	0

struct clk *spi_clks[SPI_MAX_BUS_NUM];

#else

struct tee_mt_clkmgr_clk {
	int id;
	char *name;

	int configured;
} tee_spi_clk;

#endif
#endif

void tee_fp_enable_spi_clk(void)
{
#ifdef CONFIG_MEDIATEK_SOLUTION
	int r;
#if defined(CONFIG_MTK_CLKMGR)

	if (!tee_spi_clk.configured) {
		pr_err("%s() tee_spi_clk not configured.\n", __func__);
		return;
	}

	if ((r = enable_clock(tee_spi_clk.id, tee_spi_clk.name))) {
		pr_err("%s() clk_enable(%d, %s) returns %d\n", __func__,
			tee_spi_clk.id, tee_spi_clk.name, r);
	}
#else
	if (spi_clks[TEE_FP_SPI_BUS_NUM] == NULL) {
		pr_err("%s() spi_clks[%d] is NULL\n", __func__, TEE_FP_SPI_BUS_NUM);
		return;
	}

	if ((r = clk_enable(spi_clks[TEE_FP_SPI_BUS_NUM]))) {
		pr_err("%s() clk_enable() returns %d\n", __func__, r);
	}
#endif
#else
	pr_err("%s() not implemented\n", __func__);
#endif
}
EXPORT_SYMBOL(tee_fp_enable_spi_clk);

void tee_fp_disable_spi_clk(void)
{
#ifdef CONFIG_MEDIATEK_SOLUTION
#if defined(CONFIG_MTK_CLKMGR)
	int r;

	if (!tee_spi_clk.configured) {
		pr_err("%s() tee_spi_clk not configured.\n", __func__);
		return;
	}

	if ((r = disable_clock(tee_spi_clk.id, tee_spi_clk.name))) {
		pr_err("%s() disable_clock(%d, %s) returns %d\n",
			__func__, tee_spi_clk.id, tee_spi_clk.name, r);
	}
#else
	if (spi_clks[TEE_FP_SPI_BUS_NUM] == NULL) {
		pr_err("%s() spi_clks[%d] is NULL\n", __func__, TEE_FP_SPI_BUS_NUM);
	}
	clk_disable(spi_clks[TEE_FP_SPI_BUS_NUM]);
#endif
#else
	pr_err("%s() not implemented\n", __func__);
#endif
}
EXPORT_SYMBOL(tee_fp_disable_spi_clk);

#ifdef CONFIG_MEDIATEK_SOLUTION

#if defined(CONFIG_MTK_CLKMGR)

int tee_register_spi_clk(int id, char *name)
{
	if (id < 0 || name == NULL) {
		pr_err("%s() bad param id %d name %s\n", __func__, id, name);
		return -EINVAL;
	}

	/* do not need to hold lock since the spi device
	   is initialized one by one */
	if (tee_spi_clk.configured) {
		/* always take the latest result */
		pr_warn("%s() tee_spi_clk re-registered. old id: %d name: %s\n",
			__func__, tee_spi_clk.id, tee_spi_clk.name);

		kfree(tee_spi_clk.name);
		tee_spi_clk.name = NULL;
	}

	if ((tee_spi_clk.name = kzalloc(strlen(name) + 1, GFP_KERNEL)) == NULL) {
		pr_err("%s() bad alloc memory for clkname: %s\n", __func__, name);
		return -ENOMEM;
	}

	strcpy(tee_spi_clk.name, name);
	tee_spi_clk.id = id;

	tee_spi_clk.configured = 1;

	return 0;
}
#else
int tee_register_spi_clk(struct clk *clk, s16 bus_num)
{
	printk("SPI bus_num %d clk %p\n", bus_num, clk); 

	if (bus_num >= SPI_MAX_BUS_NUM) {
		pr_err("%s bus_num %d exceeding the largest %d\n",
			__func__, bus_num, SPI_MAX_BUS_NUM);
		return -EINVAL;
	}

	if (clk == NULL) {
		pr_warn("%s NULL clk\n", __func__);
		return 0;
	}
	
	if (spi_clks[bus_num] && spi_clks[bus_num] != clk) {
		pr_warn("%s bus_num %d already set to %p\n",
			__func__, bus_num, spi_clks[bus_num]);
	}

	spi_clks[bus_num] = clk;

	return 0;
}
#endif

#else

int tee_register_spi_clk(void)
{
	return -1;
}

#endif

EXPORT_SYMBOL(tee_register_spi_clk);

#include <tee_kernel_api.h>

static TEEC_UUID SENSOR_DETECTOR_TA_UUID = { 0x966d3f7c, 0x04ef, 0x1beb, \
    { 0x08, 0xb7, 0x57, 0xf3, 0x7a, 0x6d, 0x87, 0xf9 } };

#define CMD_READ_CHIPID     0x0

int tee_spi_transfer(void *conf, uint32_t conf_size, void *inbuf, void *outbuf, uint32_t size)
{
	TEEC_Context context;
	TEEC_Session session;
	TEEC_Operation op;

	TEEC_Result r;

	char *buf;


	uint32_t returnOrigin;

	pr_alert("%s conf=%p conf_size=%u inbuf=%p outbuf=%p size=%u\n",
		__func__, conf, conf_size, inbuf, outbuf, size);

	if (!conf || !inbuf || !outbuf) {
		pr_err("Bad parameters NULL buf\n");
		return -EINVAL;
	}

	if (size == 0) {
		pr_err("zero buf size\n");
		return -EINVAL;
	}

	memset(&context, 0, sizeof(context));
	memset(&session, 0, sizeof(session));
	memset(&op, 0, sizeof(op));

	memcpy(outbuf, inbuf, size);

	if ((r = TEEC_InitializeContext(NULL, &context)) != TEEC_SUCCESS) {
		pr_err("TEEC_InitializeContext() failed with 0x%08x\n", r);
		return r;
	}
	if ((r = TEEC_OpenSession(
		&context, &session, &SENSOR_DETECTOR_TA_UUID,
		TEEC_LOGIN_PUBLIC,
		NULL, NULL, &returnOrigin)) != TEEC_SUCCESS) {
		pr_err("%s TEEC_OpenSession failed with 0x%x returnOrigun: %u\n",
			__func__, r, returnOrigin); 
		TEEC_FinalizeContext(&context);
		return r;
	}

	op.paramTypes = TEEC_PARAM_TYPES(
		TEEC_MEMREF_TEMP_INPUT,
		TEEC_MEMREF_TEMP_INOUT,
		TEEC_NONE,
		TEEC_NONE);

	op.params[0].tmpref.buffer = conf;
	op.params[0].tmpref.size = conf_size;

	op.params[1].tmpref.buffer = outbuf;
	op.params[1].tmpref.size = size;

	buf = outbuf;

	if ((r = TEEC_InvokeCommand(&session, CMD_READ_CHIPID, &op, &returnOrigin)) != TEEC_SUCCESS) {
		pr_err("%s TEEC_InvokeCommand() failed with 0x%08x returnOrigin: %u\n",
			__func__, r, returnOrigin);
	}

	TEEC_CloseSession(&session);
	TEEC_FinalizeContext(&context);

	pr_alert("[0x%02x 0x%02x 0x%02x 0x%02x]\n",
		buf[0], buf[1], buf[2], buf[3]);

	return r;
}

EXPORT_SYMBOL(tee_spi_transfer);

int tee_fp_init(void)
{
	return 0;
}

void tee_fp_exit(void)
{
#ifdef CONFIG_MEDIATEK_SOLUTION
#if defined(CONFIG_MTK_CLKMGR)
	if (tee_spi_clk.configured) {
		kfree(tee_spi_clk.name);
		tee_spi_clk.configured = 0;
	}
#endif
#endif
}
