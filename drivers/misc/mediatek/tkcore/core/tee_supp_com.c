#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/mutex.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/anon_inodes.h>
#include <linux/semaphore.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/device.h>

#include "tee_shm.h"
#include "tee_core.h"
#include "tee_supp_com.h"

#define TEE_RPC_BUFFER	0x00000001
#define TEE_RPC_VALUE	0x00000002

enum teec_rpc_result tee_supp_cmd(struct tee *tee,
				  uint32_t id, void *data, size_t datalen)
{
	struct tee_rpc *rpc = tee->rpc;
	enum teec_rpc_result res = TEEC_RPC_FAIL;
	size_t size;
	struct task_struct *task = current;

	(void) task;

#ifdef TKCORE_KDBG
	dev_dbg(tee->dev, "> tgid:[%d] id:[0x%08x]\n", task->tgid, id);
#endif

	if (atomic_read(&rpc->used) == 0) {
		dev_err(tee->dev, "%s: ERROR teed application NOT ready id=0x%x\n" , __func__, id);
		goto out;
	}

	switch (id) {
	case TEE_RPC_ICMD_ALLOCATE:
		{
			struct tee_rpc_alloc *alloc;
			struct tee_shm *shmint;

			alloc = (struct tee_rpc_alloc *)data;
			size = alloc->size;
			memset(alloc, 0, sizeof(struct tee_rpc_alloc));
			shmint = tee_shm_alloc_from_rpc(tee, size);
			if (IS_ERR_OR_NULL(shmint))
				break;

			alloc->size = size;
			alloc->data = (void *)shmint->paddr;
			alloc->shm = shmint;
			res = TEEC_RPC_OK;

			break;
		}
	case TEE_RPC_ICMD_FREE:
		{
			struct tee_rpc_free *free;

			free = (struct tee_rpc_free *)data;
			tee_shm_free_from_rpc(free->shm);
			res = TEEC_RPC_OK;
			break;
		}
	case TEE_RPC_ICMD_INVOKE:
		{
			if (sizeof(rpc->commToUser) < datalen)
				break;

			/*
			 * Other threads blocks here until we've copied our
			 * answer from the teed 
			 */
			mutex_lock(&rpc->thrd_mutex);

			mutex_lock(&rpc->outsync);
			memcpy(&rpc->commToUser, data, datalen);
			mutex_unlock(&rpc->outsync);

#ifdef TKCORE_KDBG
			dev_dbg(tee->dev,
				"TEED Cmd: %x. Give hand to teed \n",
				rpc->commToUser.cmd);
#endif

			up(&rpc->datatouser);

			down(&rpc->datafromuser);

#ifdef TKCORE_KDBG
			dev_dbg(tee->dev,
				"TEED Cmd: %x. Give hand to fw\n",
				rpc->commToUser.cmd);
#endif

			mutex_lock(&rpc->insync);
			memcpy(data, &rpc->commFromUser, datalen);
			mutex_unlock(&rpc->insync);

			mutex_unlock(&rpc->thrd_mutex);

			res = TEEC_RPC_OK;

			break;
		}
	default:
		/* not supported */
		break;
	}

out:
#ifdef TKCORE_KDBG
	dev_dbg(tee->dev, "< res: [%d]\n", res);
#endif

	return res;
}
EXPORT_SYMBOL(tee_supp_cmd);

ssize_t tee_supp_read(struct file *filp, char __user *buffer,
		  size_t length, loff_t *offset)
{
	struct tee_context *ctx = (struct tee_context *)(filp->private_data);
	struct tee *tee;
	struct tee_rpc *rpc;
	struct task_struct *task = current;
	int ret;

	(void) task;

	BUG_ON(!ctx);
	tee = ctx->tee;
	BUG_ON(!tee);
	BUG_ON(!tee->dev);
	BUG_ON(!tee->rpc);

#ifdef TKCORE_KDBG
	dev_dbg(tee->dev, "> ctx %p\n", ctx);
#endif

	rpc = tee->rpc;

	if (atomic_read(&rpc->used) == 0) {
#ifdef TKCORE_KDBG
		dev_err(tee->dev, "%s: ERROR teed application NOT ready\n"
				, __func__);
#endif
		ret = -EPERM;
		goto out;
	}

	if (down_interruptible(&rpc->datatouser))
		return -ERESTARTSYS;

#ifdef TKCORE_KDBG
	dev_dbg(tee->dev, "> tgid:[%d]\n", task->tgid);
#endif

	mutex_lock(&rpc->outsync);

	ret =
	    sizeof(rpc->commToUser) - sizeof(rpc->commToUser.cmds) +
	    sizeof(rpc->commToUser.cmds[0]) * rpc->commToUser.nbr_bf;
	if (length < ret) {
		ret = -EINVAL;
	} else {
		if (copy_to_user(buffer, &rpc->commToUser, ret)) {
#ifdef TKCORE_KDBG
			dev_err(tee->dev,
				"[%s] error, copy_to_user failed!\n", __func__);
#endif
			ret = -EINVAL;
		}
	}

	mutex_unlock(&rpc->outsync);

out:
#ifdef TKCORE_KDBG
	dev_dbg(tee->dev, "< [%d]\n", ret);
#endif
	return ret;
}

ssize_t tee_supp_write(struct file *filp, const char __user *buffer,
		   size_t length, loff_t *offset)
{
	struct tee_context *ctx = (struct tee_context *)(filp->private_data);
	struct tee *tee;
	struct tee_rpc *rpc;
	struct task_struct *task = current;
	int ret = 0;

	(void) task;

	BUG_ON(!ctx);
	BUG_ON(!ctx->tee);
	BUG_ON(!ctx->tee->rpc);
	tee = ctx->tee;
	rpc = tee->rpc;
#ifdef TKCORE_KDBG
	dev_dbg(tee->dev, "> tgid:[%d]\n", task->tgid);
#endif

	if (atomic_read(&rpc->used) == 0) {
#ifdef TKCORE_KDBG
		dev_err(tee->dev, "%s: ERROR teed application NOT ready\n"
				, __func__);
#endif
		goto out;
	}

	if (length > 0 && length < sizeof(rpc->commFromUser)) {
		uint32_t i;

		mutex_lock(&rpc->insync);

		if (copy_from_user(&rpc->commFromUser, buffer, length)) {
#ifdef TKCORE_KDBG
			dev_err(tee->dev,
				"%s: ERROR, tee_session copy_from_user failed\n",
				__func__);
#endif
			mutex_unlock(&rpc->insync);
			ret = -EINVAL;
			goto out;
		}

		/* Translate virtual address of caller into physical address */
		for (i = 0; i < rpc->commFromUser.nbr_bf; i++) {
			if (rpc->commFromUser.cmds[i].type == TEE_RPC_BUFFER
			    && rpc->commFromUser.cmds[i].buffer) {
				struct vm_area_struct *vma =
				    find_vma(current->mm,
					     (unsigned long)rpc->
					     commFromUser.cmds[i].buffer);
				if (vma != NULL) {
					struct tee_shm *shm =
					    vma->vm_private_data;
					BUG_ON(!shm);
#ifdef TKCORE_KDBG
					dev_dbg(tee->dev,
						"%d gid2pa(0x%p => %x)\n", i,
						rpc->commFromUser.cmds[i].
						buffer,
						(unsigned int)shm->paddr);
#endif
					rpc->commFromUser.cmds[i].buffer =
					    (void *)shm->paddr;
				} else {
#ifdef TKCORE_KDBG
					dev_dbg(tee->dev,
						" gid2pa(0x%p => NULL\n)",
						rpc->commFromUser.cmds[i].
						buffer);
#endif
				}
			}
		}

		mutex_unlock(&rpc->insync);
		up(&rpc->datafromuser);
		ret = length;
	}

out:
#ifdef TKCORE_KDBG
	dev_dbg(tee->dev, "< [%d]\n", ret);
#endif
	return ret;
}

int tee_supp_init(struct tee *tee)
{
	struct tee_rpc *rpc =
	    devm_kzalloc(tee->dev, sizeof(struct tee_rpc), GFP_KERNEL);
	if (!rpc) {
#ifdef TKCORE_KDBG
		dev_err(tee->dev, "%s: can't allocate tee_rpc structure\n",
				__func__);
#endif
		return -ENOMEM;
	}

	rpc->datafromuser = (struct semaphore)
	    __SEMAPHORE_INITIALIZER(rpc->datafromuser, 0);
	rpc->datatouser = (struct semaphore)
	    __SEMAPHORE_INITIALIZER(rpc->datatouser, 0);
	mutex_init(&rpc->thrd_mutex);
	mutex_init(&rpc->outsync);
	mutex_init(&rpc->insync);
	atomic_set(&rpc->used, 0);
	tee->rpc = rpc;
	return 0;
}

void tee_supp_deinit(struct tee *tee)
{
	devm_kfree(tee->dev, tee->rpc);
	tee->rpc = NULL;
}
