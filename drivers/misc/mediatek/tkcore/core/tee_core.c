#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/idr.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/uaccess.h>
#include <asm-generic/ioctl.h>
#include <linux/sched.h>

#include "linux/tee_core.h"
#include "linux/tee_ioc.h"

#include "tee_core_priv.h"
#include "tee_sysfs.h"
#include "tee_shm.h"
#include "tee_supp_com.h"
#include "tee_tui.h"

#ifdef TKCORE_FP_SUPPORT
#include "tee_fp.h"
#endif

#define _TEE_CORE_FW_VER "1:0.1"

static char *_tee_supp_app_name = "teed";

/* Store the class misc reference */
static struct class *misc_class;

static int device_match(struct device *device, const void *devname)
{
	struct tee *tee = dev_get_drvdata(device);
	int ret = strncmp(devname, tee->name, sizeof(tee->name));

	BUG_ON(!tee);
	if (ret == 0)
		return 1;
	else
		return 0;
}

/*
 * For the kernel api.
 * Get a reference on a device tee from the device needed
 */
struct tee *tee_get_tee(const char *devname)
{
	struct device *device;

	if (!devname)
		return NULL;
	device = class_find_device(misc_class, NULL, (void *) devname, device_match);
	if (!device) {
		pr_err("%s:%d - can't find device [%s]\n", __func__, __LINE__,
		       devname);
		return NULL;
	}

	return dev_get_drvdata(device);
}

void tee_inc_stats(struct tee_stats_entry *entry)
{
	entry->count++;
	if (entry->count > entry->max)
		entry->max = entry->count;
}

void tee_dec_stats(struct tee_stats_entry *entry)
{
	entry->count--;
}

int __tee_get(struct tee *tee)
{
	int ret = 0;
	int v;

	BUG_ON(!tee);

	if ((v = atomic_inc_return(&tee->refcount)) == 1) {
		BUG_ON(!try_module_get(tee->ops->owner));
		get_device(tee->dev);

		if (tee->ops->start)
			ret = tee->ops->start(tee);

		if (ret) {
			dev_err(_DEV(tee), "%s: %s::start() failed, err=0x%x\n",
				__func__, tee->name, ret);
			put_device(tee->dev);
			module_put(tee->ops->owner);
			atomic_dec(&tee->refcount);
		}
	} else {
		dev_warn(_DEV(tee), "Unexpected tee->refcount: 0x%x\n", v);
		return -1;
	}

	return ret;
}

EXPORT_SYMBOL(__tee_get);


/**
 * tee_get - increases refcount of the tee
 * @tee:	[in]	tee to increase refcount of
 *
 * @note: If tee.ops.start() callback function is available,
 * it is called when refcount is equal at 1.
 */
int tee_get(struct tee *tee)
{
	int ret = 0;

	BUG_ON(!tee);

	if (atomic_inc_return(&tee->refcount) == 1) {
		dev_warn(_DEV(tee), "%s: unexpected refcount 1\n", __func__);
	} else {
		int count = (int)atomic_read(&tee->refcount);

#ifdef TKCORE_KDBG
		dev_info(_DEV(tee), "%s: refcount=%d\n", __func__, count);
#endif
		if (count > tee->max_refcount)
			tee->max_refcount = count;
	}
	return ret;
}

/**
 * tee_put - decreases refcount of the tee
 * @tee:	[in]	tee to reduce refcount of
 *
 * @note: If tee.ops.stop() callback function is available,
 * it is called when refcount is equal at 0.
 */
int tee_put(struct tee *tee)
{
	int ret = 0;
	int count;

	BUG_ON(!tee);

	if (atomic_dec_and_test(&tee->refcount)) {
		dev_warn(_DEV(tee), "%s: unexpected refcount: 0\n", __func__);

/*
 * tee should never be stopped
 */

/*		
		if (tee->ops->stop)
			ret = tee->ops->stop(tee);
		module_put(tee->ops->owner);
		put_device(tee->dev);
*/
	}
/*
	if (ret) {
#ifdef TKCORE_KDBG
		dev_err(_DEV(tee), "%s: %s::stop() has failed, ret=%d\n",
			__func__, tee->name, ret);
#endif
	}
*/

	count = (int)atomic_read(&tee->refcount);
#ifdef TKCORE_KDBG
	dev_dbg(_DEV(tee), "%s: refcount=%d\n", __func__, count);
#endif
	return ret;
}

static int tee_supp_open(struct tee *tee)
{
	int ret = 0;
#ifdef TKCORE_KDBG
	dev_dbg(_DEV(tee), "%s: appclient=\"%s\" pid=%d\n", __func__,
		current->comm, current->pid);
#endif

	BUG_ON(!tee->rpc);

	if (strncmp(_tee_supp_app_name, current->comm,
			strlen(_tee_supp_app_name)) == 0) {
		if (atomic_add_return(1, &tee->rpc->used) > 1) {
			ret = -EBUSY;
#ifdef TKCORE_KDBG
			dev_err(tee->dev, "%s: ERROR Only one teed is allowed\n",
					__func__);
#endif
			atomic_sub(1, &tee->rpc->used);
		}
	}

	return ret;
}

static void tee_supp_release(struct tee *tee)
{
#ifdef TKCORE_KDBG
	dev_dbg(_DEV(tee), "%s: appclient=\"%s\" pid=%d\n", __func__,
		current->comm, current->pid);
#endif

	BUG_ON(!tee->rpc);

	if ((atomic_read(&tee->rpc->used) == 1) &&
			(strncmp(_tee_supp_app_name, current->comm,
					strlen(_tee_supp_app_name)) == 0))
		atomic_sub(1, &tee->rpc->used);
}

static int tee_ctx_open(struct inode *inode, struct file *filp)
{
	struct tee_context *ctx;
	struct tee *tee;
	int ret;

	tee = container_of(filp->private_data, struct tee, miscdev);

	BUG_ON(!tee);
	BUG_ON(tee->miscdev.minor != iminor(inode));
#ifdef TKCORE_KDBG
	dev_dbg(_DEV(tee), "%s: > name=\"%s\"\n", __func__, tee->name);
#endif

	ret = tee_supp_open(tee);
	if (ret)
		return ret;

	ctx = tee_context_create(tee);
	if (IS_ERR_OR_NULL(ctx))
		return PTR_ERR(ctx);

	ctx->usr_client = 1;
	filp->private_data = ctx;
#ifdef TKCORE_KDBG
	dev_dbg(_DEV(tee), "%s: < ctx=%p is created\n", __func__, (void *)ctx);
#endif

	return 0;
}

static int tee_ctx_release(struct inode *inode, struct file *filp)
{
	struct tee_context *ctx = filp->private_data;
	struct tee *tee;

	if (!ctx)
		return -EINVAL;

	BUG_ON(!ctx->tee);
	tee = ctx->tee;
	BUG_ON(tee->miscdev.minor != iminor(inode));

#ifdef TKCORE_KDBG
	dev_dbg(_DEV(tee), "%s: > ctx=%p\n", __func__, ctx);
#endif

	tee_context_destroy(ctx);
	tee_supp_release(tee);

#ifdef TKCORE_KDBG
	dev_dbg(_DEV(tee), "%s: < ctx=%p is destroyed\n", __func__, ctx);
#endif
	return 0;
}

static int tee_do_create_session(struct tee_context *ctx,
				 struct tee_cmd_io __user *u_cmd)
{
	int ret = -EINVAL;
	struct tee_cmd_io k_cmd;
	struct tee *tee;

	tee = ctx->tee;
	BUG_ON(!ctx->usr_client);
#ifdef TKCORE_KDBG
	dev_dbg(_DEV(tee), "%s: >\n", __func__);
#endif

	if (copy_from_user(&k_cmd, (void *)u_cmd, sizeof(struct tee_cmd_io))) {
#ifdef TKCORE_KDBG
		dev_err(_DEV(tee), "%s: copy_from_user failed\n", __func__);
#endif
		goto exit;
	}

	if (k_cmd.fd_sess > 0) {
#ifdef TKCORE_KDBG
		dev_err(_DEV(tee), "%s: invalid fd_sess %d\n", __func__,
			k_cmd.fd_sess);
#endif
		goto exit;
	}

	if ((k_cmd.op == NULL) || (k_cmd.uuid == NULL) ||
	    ((k_cmd.data != NULL) && (k_cmd.data_size == 0)) ||
	    ((k_cmd.data == NULL) && (k_cmd.data_size != 0))) {
#ifdef TKCORE_KDBG
		dev_err(_DEV(tee),
			"%s: op or/and data parameters are not valid\n",
			__func__);
#endif
		goto exit;
	}

	ret = tee_session_create_fd(ctx, &k_cmd);
	put_user(k_cmd.err, &u_cmd->err);
	put_user(k_cmd.origin, &u_cmd->origin);
	if (ret)
		goto exit;

	put_user(k_cmd.fd_sess, &u_cmd->fd_sess);

exit:
#ifdef TKCORE_KDBG
	dev_dbg(_DEV(tee), "%s: < ret=%d, sessfd=%d\n", __func__, ret,
		k_cmd.fd_sess);
#endif
	return ret;
}

static int tee_do_shm_alloc_perm(struct tee_context *ctx,
			    struct tee_shm_io __user *u_shm)
{
	int ret = -EINVAL;
	struct tee_shm_io k_shm;
	struct tee *tee = ctx->tee;

	(void) tee;

	if (copy_from_user(&k_shm, (void *)u_shm, sizeof(struct tee_shm_io))) {
#ifdef TKCORE_KDBG
		dev_err(_DEV(tee), "%s: copy_from_user failed\n", __func__);
#endif
		goto exit;
	}

	ret = tee_shm_alloc_io_perm(ctx, &k_shm);
	if (ret)
		goto exit;
	
	put_user(k_shm.paddr, &u_shm->paddr);
	put_user(k_shm.fd_shm, &u_shm->fd_shm);
	put_user(k_shm.flags, &u_shm->flags);

exit:
#ifdef TKCORE_KDBG
	dev_dbg(_DEV(tee), "%s: < ret=%d, shmfd=%d\n", __func__, ret,
		k_shm.fd_shm);
#endif
	return ret;
}


static int tee_do_shm_alloc(struct tee_context *ctx,
			    struct tee_shm_io __user *u_shm)
{
	int ret = -EINVAL;
	struct tee_shm_io k_shm;
	struct tee *tee = ctx->tee;

	BUG_ON(!ctx->usr_client);

#ifdef TKCORE_KDBG
	dev_dbg(_DEV(tee), "%s: >\n", __func__);
#endif

	if (copy_from_user(&k_shm, (void *)u_shm, sizeof(struct tee_shm_io))) {
#ifdef TKCORE_KDBG
		dev_err(_DEV(tee), "%s: copy_from_user failed\n", __func__);
#endif
		goto exit;
	}

	if ((k_shm.buffer != NULL) || (k_shm.fd_shm != 0) ||
	    /*(k_shm.flags & ~(tee->shm_flags)) ||*/
	    ((k_shm.flags & tee->shm_flags) == 0) || (k_shm.registered != 0)) {
#ifdef TKCORE_KDBG
		dev_err(_DEV(tee),
			"%s: shm parameters are not valid %p %d %08x %08x %d\n",
			__func__, (void *)k_shm.buffer, k_shm.fd_shm,
			(unsigned int)k_shm.flags, (unsigned int)tee->shm_flags,
			k_shm.registered);
#endif
		goto exit;
	}

	ret = tee_shm_alloc_io(ctx, &k_shm);
	if (ret)
		goto exit;

	put_user(k_shm.fd_shm, &u_shm->fd_shm);
	put_user(k_shm.flags, &u_shm->flags);

exit:
#ifdef TKCORE_KDBG
	dev_dbg(_DEV(tee), "%s: < ret=%d, shmfd=%d\n", __func__, ret,
		k_shm.fd_shm);
#endif
	return ret;
}

static int tee_do_get_fd_for_rpc_shm(struct tee_context *ctx,
				     struct tee_shm_io __user *u_shm)
{
	int ret = -EINVAL;
	struct tee_shm_io k_shm;
	struct tee *tee = ctx->tee;

#ifdef TKCORE_KDBG
	dev_dbg(_DEV(tee), "%s: >\n", __func__);
#endif
	BUG_ON(!ctx->usr_client);

	if (copy_from_user(&k_shm, (void *)u_shm, sizeof(struct tee_shm_io))) {
#ifdef TKCORE_KDBG
		dev_err(_DEV(tee), "%s: copy_from_user failed\n", __func__);
#endif
		goto exit;
	}

	if ((k_shm.buffer == NULL) || (k_shm.size == 0) || (k_shm.fd_shm != 0)
	    || (k_shm.flags & ~(tee->shm_flags))
	    || ((k_shm.flags & tee->shm_flags) == 0)
	    || (k_shm.registered != 0)) {
#ifdef TKCORE_KDBG
		dev_err(_DEV(tee), "%s: shm parameters are not valid\n",
			__func__);
#endif
		goto exit;
	}

	ret = tee_shm_fd_for_rpc(ctx, &k_shm);
	if (ret)
		goto exit;

	put_user(k_shm.fd_shm, &u_shm->fd_shm);

exit:
#ifdef TKCORE_KDBG
	dev_dbg(_DEV(tee), "%s: < ret=%d, shmfd=%d\n", __func__, ret,
		k_shm.fd_shm);
#endif
	return ret;
}

static long tee_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = -EINVAL;
	struct tee_context *ctx = filp->private_data;
	void __user *u_arg;

	BUG_ON(!ctx);
	BUG_ON(!ctx->tee);

#ifdef TKCORE_KDBG
	dev_dbg(_DEV(ctx->tee), "%s: > cmd nr=%d\n", __func__, _IOC_NR(cmd));
#endif

#ifdef CONFIG_COMPAT
	if (is_compat_task())
		u_arg = compat_ptr(arg);
	else
		u_arg = (void __user *)arg;
#else
	u_arg = (void __user *)arg;
#endif

	switch (cmd) {
	case TEE_OPEN_SESSION_IOC:
		ret = tee_do_create_session(ctx,
				(struct tee_cmd_io __user *)u_arg);
		break;
	case TEE_ALLOC_SHM_PERM_IOC:
		ret = tee_do_shm_alloc_perm(ctx, (struct tee_shm_io __user *)u_arg);
		break;
	case TEE_ALLOC_SHM_IOC:
		ret = tee_do_shm_alloc(ctx, (struct tee_shm_io __user *)u_arg);
		break;
	case TEE_GET_FD_FOR_RPC_SHM_IOC:
		ret = tee_do_get_fd_for_rpc_shm(ctx,
				 (struct tee_shm_io __user *)u_arg);
		break;
	case TEE_TUI_NOTIFY_IOC:
		printk("TEE_TUI_NOTIFY_IOC called\n");
		if (teec_notify_event((uint32_t)((unsigned long)(u_arg))))
			ret = 0;
		else
			ret = -EINVAL;
#ifdef TKCORE_KDBG
		dev_dbg(_DEV(ctx->tee), "%s: TEE_TUI_NOTIFY_IOC\n", __func__);
#endif
		break;
	case TEE_TUI_WAITCMD_IOC:
		{
			uint32_t cmd_id;
#ifdef TKCORE_KDBG
		dev_dbg(_DEV(ctx->tee), "%s: TEE_TUI_WAITCMD_IOC\n", __func__);
#endif
			ret = teec_wait_cmd(&cmd_id);
			if (ret)
				return ret;

			if (copy_to_user(u_arg, &cmd_id, sizeof(cmd_id)))
				ret = -EFAULT;
			else
				ret = 0;
		}
		break;
#ifdef TKCORE_FP_SUPPORT
	case TEE_FP_ENABLE_SPI_CLK_IOC:
		{
			tee_fp_enable_spi_clk();
			ret = 0;
		}
		break;
	case TEE_FP_DISABLE_SPI_CLK_IOC:
		{
			tee_fp_disable_spi_clk();
			ret = 0;
		}
		break;
#endif
	default:
		ret = -ENOSYS;
		break;
	}

#ifdef TKCORE_KDBG
	dev_dbg(_DEV(ctx->tee), "%s: < ret=%d\n", __func__, ret);
#endif

	return ret;
}

const struct file_operations tee_fops = {
	.owner = THIS_MODULE,
	.read = tee_supp_read,
	.write = tee_supp_write,
	.open = tee_ctx_open,
	.release = tee_ctx_release,
#ifdef CONFIG_COMPAT
	.compat_ioctl = tee_ioctl,
#endif
	.unlocked_ioctl = tee_ioctl
};

static void tee_plt_device_release(struct device *dev)
{
#ifdef TKCORE_KDBG
	pr_debug("%s: (dev=%p)....\n", __func__, dev);
#endif
}

static spinlock_t tee_idr_lock;
static struct idr tee_idr;

/* let caller to guarantee tee
   and id are not NULL. using lock to protect */
int tee_core_alloc_uuid(void *ptr)
{
	int r;

	idr_preload(GFP_KERNEL);

	spin_lock(&tee_idr_lock);
	if ((r = idr_alloc(&tee_idr, ptr, 1, 0, GFP_NOWAIT)) < 0) {
		pr_err("%s: Bad alloc tee_uuid. rv: %d\n", __func__, r);
	}
	spin_unlock(&tee_idr_lock);

	idr_preload_end();

	return r;
}

void *tee_core_uuid2ptr(int id)
{
	return idr_find(&tee_idr, id);
}

/* let caller to guarantee tee
   and id are not NULL */
void tee_core_free_uuid(int id)
{
	idr_remove(&tee_idr, id);
}

struct tee *tee_core_alloc(struct device *dev, char *name, int id,
			   const struct tee_ops *ops, size_t len)
{
	struct tee *tee;

	if (!dev || !name || !ops ||
	    !ops->open || !ops->close || !ops->alloc || !ops->free)
		return NULL;

	tee = devm_kzalloc(dev, sizeof(struct tee) + len, GFP_KERNEL);
	if (!tee) {
#ifdef TKCORE_KDBG
		dev_err(dev, "%s: kzalloc failed\n", __func__);
#endif
		return NULL;
	}

	if (!dev->release)
		dev->release = tee_plt_device_release;

	tee->dev = dev;
	tee->id = id;
	tee->ops = ops;
	tee->priv = &tee[1];

	snprintf(tee->name, sizeof(tee->name), "%s", name);
#ifdef TKCORE_KDBG
	pr_info("TEE core: Alloc the misc device \"%s\" (id=%d)\n", tee->name,
		tee->id);
#endif

	tee->miscdev.parent = dev;
	tee->miscdev.minor = MISC_DYNAMIC_MINOR;
	tee->miscdev.name = tee->name;
	tee->miscdev.fops = &tee_fops;

	mutex_init(&tee->lock);
	atomic_set(&tee->refcount, 0);
	INIT_LIST_HEAD(&tee->list_ctx);
	INIT_LIST_HEAD(&tee->list_rpc_shm);

	tee->state = TEE_OFFLINE;
	tee->shm_flags = TEEC_MEM_INPUT | TEEC_MEM_OUTPUT;
	tee->test = 0;

	tee_supp_init(tee);

	return tee;
}
EXPORT_SYMBOL(tee_core_alloc);

int tee_core_free(struct tee *tee)
{
	if (tee) {
		tee_supp_deinit(tee);
		devm_kfree(tee->dev, tee);
	}
	return 0;
}
EXPORT_SYMBOL(tee_core_free);

int tee_core_add(struct tee *tee)
{
	int rc = 0;

	if (!tee)
		return -EINVAL;

	rc = misc_register(&tee->miscdev);
	if (rc != 0) {
		pr_err("TEE Core: misc_register() failed name=\"%s\" ret=%d\n",
		       tee->name, rc);
		return rc;
	}

	dev_set_drvdata(tee->miscdev.this_device, tee);

	tee_init_sysfs(tee);

	/* Register a static reference on the class misc
	 * to allow finding device by class */
	BUG_ON(!tee->miscdev.this_device->class);
	if (misc_class)
		BUG_ON(misc_class != tee->miscdev.this_device->class);
	else
		misc_class = tee->miscdev.this_device->class;
#ifdef TKCORE_KDBG
	pr_info("TEE Core: Register the misc device \"%s\" (id=%d,minor=%d)\n",
		dev_name(tee->miscdev.this_device), tee->id,
		tee->miscdev.minor);
#endif
	return rc;
}
EXPORT_SYMBOL(tee_core_add);

int tee_core_del(struct tee *tee)
{
	if (tee) {
#ifdef TKCORE_KDBG
		pr_info("TEE Core: Destroy the misc device \"%s\" (id=%d)\n",
			dev_name(tee->miscdev.this_device), tee->id);
#endif

		tee_cleanup_sysfs(tee);

		if (tee->miscdev.minor != MISC_DYNAMIC_MINOR) {
#ifdef TKCORE_KDBG
			pr_info("TEE Core: Deregister the misc device \"%s\" (id=%d)\n",
			     dev_name(tee->miscdev.this_device), tee->id);
#endif
			misc_deregister(&tee->miscdev);
		}
	}

	tee_core_free(tee);

	return 0;
}
EXPORT_SYMBOL(tee_core_del);

static int __init tee_core_init(void)
{
#ifdef TKCORE_KDBG
	pr_info("\nTEE Core Framework initialization (ver %s)\n",
		_TEE_CORE_FW_VER);
#endif

	spin_lock_init(&tee_idr_lock);
	idr_init(&tee_idr);

#ifdef TKCORE_FP_SUPPORT
	tee_fp_init();
#endif
	return 0;
}

static void __exit tee_core_exit(void)
{
#ifdef TKCORE_KDBG
	pr_info("TEE Core Framework unregistered\n");
#endif

#ifdef TKCORE_FP_SUPPORT
	tee_fp_exit();
#endif
}

rootfs_initcall(tee_core_init);
module_exit(tee_core_exit);

MODULE_AUTHOR("TrustKernel");
MODULE_DESCRIPTION("TrustKernel TKCore TEEC v1.0");
MODULE_SUPPORTED_DEVICE("");
MODULE_VERSION(_TEE_CORE_FW_VER);
MODULE_LICENSE("GPL");
