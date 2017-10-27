#include <linux/slab.h>
#include <linux/types.h>
#include <linux/dma-buf.h>
#include <linux/hugetlb.h>
#include <linux/version.h>

#include <linux/sched.h>
#include <linux/mm.h>

#include "tee_core_priv.h"
#include "tee_shm.h"

#ifdef TKCORE_KDBG
#define INMSG() dev_dbg(_DEV(tee), "%s: >\n", __func__)
#define OUTMSG(val) dev_dbg(_DEV(tee), "%s: < %ld\n", __func__, (long)val)
#define OUTMSGX(val) dev_dbg(_DEV(tee), "%s: < %08x\n",\
		__func__, (unsigned int)(long)val)
#else

#define INMSG() do {} while (0)
#define OUTMSG(val) do {} while(0)
#define OUTMSGX(val) do {} while(0)

#endif

/* TODO
#if (sizeof(TEEC_SharedMemory) != sizeof(tee_shm))
#error "sizeof(TEEC_SharedMemory) != sizeof(tee_shm))"
#endif
*/
struct tee_shm_attach {
	struct sg_table sgt;
	enum dma_data_direction dir;
	bool is_mapped;
};

struct tee_shm *tee_shm_alloc_from_rpc(struct tee *tee, size_t size)
{
	struct tee_shm *shm;

	INMSG();

	mutex_lock(&tee->lock);
	shm = tee_shm_alloc(tee, size, TEE_SHM_TEMP | TEE_SHM_FROM_RPC);
	if (IS_ERR_OR_NULL(shm)) {
#ifdef TKCORE_KDBG
		dev_err(_DEV(tee), "%s: buffer allocation failed (%ld)\n",
			__func__, PTR_ERR(shm));
#endif
		goto out;
	}

	tee_inc_stats(&tee->stats[TEE_STATS_SHM_IDX]);
	list_add_tail(&shm->entry, &tee->list_rpc_shm);

	shm->ctx = NULL;

out:
	mutex_unlock(&tee->lock);
	OUTMSGX(shm);
	return shm;
}

void tee_shm_free_from_rpc(struct tee_shm *shm)
{
	struct tee *tee;
	if (shm == NULL)
		return;

	tee = shm->tee;
	mutex_lock(&tee->lock);
	if (shm->ctx == NULL) {
		tee_dec_stats(&shm->tee->stats[TEE_STATS_SHM_IDX]);
		list_del(&shm->entry);
	}

	tee_shm_free(shm);
	mutex_unlock(&tee->lock);
}

/* for compatibility with linux-3.0.86 (tiny4412) */

int __weak sg_alloc_table_from_pages(struct sg_table *sgt,
		struct page **pages, unsigned int n_pages,
		unsigned long offset, unsigned long size,
		gfp_t gfp_mask)
{
	unsigned int chunks;
	unsigned int i;
	unsigned int cur_page;
	int ret;
	struct scatterlist *s;

	/* compute number of contiguous chunks */
	chunks = 1;
	for (i = 1; i < n_pages; ++i)
		if (page_to_pfn(pages[i]) != page_to_pfn(pages[i - 1]) + 1)
			++chunks;

	ret = sg_alloc_table(sgt, chunks, gfp_mask);
	if (unlikely(ret))
		return ret;

	/* merging chunks and putting them into the scatterlist */
	cur_page = 0;
	for_each_sg(sgt->sgl, s, sgt->orig_nents, i) {
		unsigned long chunk_size;
		unsigned int j;

		/* look for the end of the current chunk */
		for (j = cur_page + 1; j < n_pages; ++j)
			if (page_to_pfn(pages[j]) !=
					page_to_pfn(pages[j - 1]) + 1)
				break;

		chunk_size = ((j - cur_page) << PAGE_SHIFT) - offset;
		sg_set_page(s, pages[cur_page], min(size, chunk_size), offset);
		size -= chunk_size;
		offset = 0;
		cur_page = j;
	}

	return 0;
}

struct tee_shm *tee_shm_alloc(struct tee *tee, size_t size, uint32_t flags)
{
	struct tee_shm *shm;
	unsigned long pfn;
	unsigned int nr_pages;
	struct page *page;
	int ret;

	INMSG();

	shm = tee->ops->alloc(tee, size, flags);
	if (IS_ERR_OR_NULL(shm)) {
#ifdef TKCORE_KDBG
		dev_err(_DEV(tee),
			"%s: allocation failed (s=%d,flags=0x%08x) err=%ld\n",
			__func__, (int)size, flags, PTR_ERR(shm));
#endif
		goto exit;
	}

	shm->tee = tee;

#ifdef TKCORE_KDBG
	dev_dbg(_DEV(tee), "%s: shm=%p, paddr=%p,s=%d/%d app=\"%s\" pid=%d\n",
		 __func__, shm, (void *)shm->paddr, (int)shm->size_req,
		 (int)shm->size_alloc, current->comm, current->pid);
#endif

	pfn = shm->paddr >> PAGE_SHIFT;
	page = pfn_to_page(pfn);
	if (IS_ERR_OR_NULL(page)) {
#ifdef TKCORE_KDBG
		dev_err(_DEV(tee), "%s: pfn_to_page(%lx) failed\n",
				__func__, pfn);
#endif
		tee->ops->free(shm);
		return (struct tee_shm *)page;
	}

	/* Only one page of contiguous physical memory */
	nr_pages = 1;

	ret = sg_alloc_table_from_pages(&shm->sgt, &page,
			nr_pages, 0, nr_pages * PAGE_SIZE, GFP_KERNEL);
	if (IS_ERR_VALUE(ret)) {
#ifdef TKCORE_KDBG
		dev_err(_DEV(tee), "%s: sg_alloc_table_from_pages() failed\n",
				__func__);
#endif
		tee->ops->free(shm);
		shm = ERR_PTR(ret);
	}
exit:
	OUTMSGX(shm);
	return shm;
}

void tee_shm_free(struct tee_shm *shm)
{
	struct tee *tee;

	if (IS_ERR_OR_NULL(shm))
		return;
	tee = shm->tee;
	if (tee == NULL) {
#ifdef TKCORE_KDBG
		pr_warn("invalid call to tee_shm_free(%p): NULL tee\n", shm);
#endif
	}
	else if (shm->tee == NULL) {
#ifdef TKCORE_KDBG
		dev_warn(_DEV(tee), "tee_shm_free(%p): NULL tee\n", shm);
#endif
	}
	else {
		sg_free_table(&shm->sgt);
		shm->tee->ops->free(shm);
	}
}

static int _tee_shm_attach_dma_buf(struct dma_buf *dmabuf,
					struct device *dev,
					struct dma_buf_attachment *attach)
{
	struct tee_shm_attach *tee_shm_attach;
	struct tee_shm *shm;
	struct tee *tee;

	shm = dmabuf->priv;
	tee = shm->tee;

	INMSG();

	tee_shm_attach = devm_kzalloc(_DEV(tee),
			sizeof(*tee_shm_attach), GFP_KERNEL);
	if (!tee_shm_attach) {
		OUTMSG(-ENOMEM);
		return -ENOMEM;
	}

	tee_shm_attach->dir = DMA_NONE;
	attach->priv = tee_shm_attach;

	OUTMSG(0);
	return 0;
}

static void _tee_shm_detach_dma_buf(struct dma_buf *dmabuf,
					struct dma_buf_attachment *attach)
{
	struct tee_shm_attach *tee_shm_attach = attach->priv;
	struct sg_table *sgt;
	struct tee_shm *shm;
	struct tee *tee;

	shm = dmabuf->priv;
	tee = shm->tee;

	INMSG();

	if (!tee_shm_attach) {
		OUTMSG(0);
		return;
	}

	sgt = &tee_shm_attach->sgt;

	if (tee_shm_attach->dir != DMA_NONE)
		dma_unmap_sg(attach->dev, sgt->sgl, sgt->nents,
			tee_shm_attach->dir);

	sg_free_table(sgt);
	devm_kfree(_DEV(tee), tee_shm_attach);
	attach->priv = NULL;
	OUTMSG(0);
}

static struct sg_table *_tee_shm_dma_buf_map_dma_buf(
		struct dma_buf_attachment *attach, enum dma_data_direction dir)
{
	struct tee_shm_attach *tee_shm_attach = attach->priv;
	struct tee_shm *tee_shm = attach->dmabuf->priv;
	struct sg_table *sgt = NULL;
	struct scatterlist *rd, *wr;
	unsigned int i;
	int nents, ret;
	struct tee *tee;

	tee = tee_shm->tee;

	INMSG();

	/* just return current sgt if already requested. */
	if (tee_shm_attach->dir == dir && tee_shm_attach->is_mapped) {
		OUTMSGX(&tee_shm_attach->sgt);
		return &tee_shm_attach->sgt;
	}

	sgt = &tee_shm_attach->sgt;

	ret = sg_alloc_table(sgt, tee_shm->sgt.orig_nents, GFP_KERNEL);
	if (ret) {
#ifdef TKCORE_KDBG
		dev_err(_DEV(tee), "failed to alloc sgt.\n");
#endif
		return ERR_PTR(-ENOMEM);
	}

	rd = tee_shm->sgt.sgl;
	wr = sgt->sgl;
	for (i = 0; i < sgt->orig_nents; ++i) {
		sg_set_page(wr, sg_page(rd), rd->length, rd->offset);
		rd = sg_next(rd);
		wr = sg_next(wr);
	}

	if (dir != DMA_NONE) {
		nents = dma_map_sg(attach->dev, sgt->sgl, sgt->orig_nents, dir);
		if (!nents) {
#ifdef TKCORE_KDBG
			dev_err(_DEV(tee), "failed to map sgl with iommu.\n");
#endif
			sg_free_table(sgt);
			sgt = ERR_PTR(-EIO);
			goto err_unlock;
		}
	}

	tee_shm_attach->is_mapped = true;
	tee_shm_attach->dir = dir;
	attach->priv = tee_shm_attach;

err_unlock:
	OUTMSGX(sgt);
	return sgt;
}

static void _tee_shm_dma_buf_unmap_dma_buf(struct dma_buf_attachment *attach,
					  struct sg_table *table,
					  enum dma_data_direction dir)
{
	return;
}

static void _tee_shm_dma_buf_release(struct dma_buf *dmabuf)
{
	struct tee_shm *shm = dmabuf->priv;
	struct tee_context *ctx;
	struct tee *tee;

	tee = shm->ctx->tee;

	INMSG();

	ctx = shm->ctx;
#ifdef TKCORE_KDBG
	dev_dbg(_DEV(ctx->tee), "%s: shm=%p, paddr=%p,s=%d/%d app=\"%s\" pid=%d\n",
		 __func__, shm, (void *)shm->paddr, (int)shm->size_req,
		 (int)shm->size_alloc, current->comm, current->pid);
#endif

	tee_shm_free_io(shm);

	OUTMSG(0);
}

static int _tee_shm_dma_buf_mmap(struct dma_buf *dmabuf,
				struct vm_area_struct *vma)
{
	struct tee_shm *shm = dmabuf->priv;
	size_t size = vma->vm_end - vma->vm_start;
	struct tee *tee;
	int ret;
	pgprot_t prot;
	unsigned long pfn;

	tee = shm->ctx->tee;

	pfn = shm->paddr >> PAGE_SHIFT;

	INMSG();

	//shm->flags |= TEE_SHM_CACHED;

	if (shm->flags & TEE_SHM_CACHED)
		prot = vma->vm_page_prot;
	else
		prot = pgprot_noncached(vma->vm_page_prot);

	ret =
	    remap_pfn_range(vma, vma->vm_start, pfn, size, prot);
	if (!ret)
		vma->vm_private_data = (void *)shm;

#ifdef TKCORE_KDBG
	dev_dbg(_DEV(shm->ctx->tee), "%s: map the shm (p@=%p,s=%dKiB) => %x\n",
		__func__, (void *)shm->paddr, (int)size / 1024,
		(unsigned int)vma->vm_start);
#endif

	OUTMSG(ret);
	return ret;
}

static void *_tee_shm_dma_buf_kmap_atomic(struct dma_buf *dmabuf,
					 unsigned long pgnum)
{
	return NULL;
}

static void *_tee_shm_dma_buf_kmap(struct dma_buf *db, unsigned long pgnum)
{
	struct tee_shm *shm = db->priv;

#ifdef TKCORE_KDBG
	dev_dbg(_DEV(shm->ctx->tee), "%s: kmap the shm (p@=%p, v@=%p, s=%zdKiB)\n",
		__func__, (void *)shm->paddr, (void *)shm->kaddr,
		shm->size_alloc / 1024);
#endif
	/*
	 * A this stage, a shm allocated by the tee
	 * must be have a kernel address
	 */
	return shm->kaddr;
}

static void _tee_shm_dma_buf_kunmap(
		struct dma_buf *db, unsigned long pfn, void *kaddr)
{
	/* unmap is done at the de init of the shm pool */
}

struct dma_buf_ops _tee_shm_dma_buf_ops = {
	.attach = _tee_shm_attach_dma_buf,
	.detach = _tee_shm_detach_dma_buf,
	.map_dma_buf = _tee_shm_dma_buf_map_dma_buf,
	.unmap_dma_buf = _tee_shm_dma_buf_unmap_dma_buf,
	.release = _tee_shm_dma_buf_release,
	.kmap_atomic = _tee_shm_dma_buf_kmap_atomic,
	.kmap = _tee_shm_dma_buf_kmap,
	.kunmap = _tee_shm_dma_buf_kunmap,
	.mmap = _tee_shm_dma_buf_mmap,
};

/******************************************************************************/

static int export_buf(struct tee *tee, struct tee_shm *shm, int *export)
{
	struct dma_buf *dmabuf;
	int ret = 0;
	/* Temporary fix to support both older and newer kernel versions. */
#if defined(DEFINE_DMA_BUF_EXPORT_INFO)
	DEFINE_DMA_BUF_EXPORT_INFO(exp_info);

	exp_info.priv = shm;
	exp_info.ops = &_tee_shm_dma_buf_ops;
	exp_info.size = shm->size_alloc;
	exp_info.flags = O_RDWR;

	dmabuf = dma_buf_export(&exp_info);
#else

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,17,0)
	dmabuf = dma_buf_export(shm, &_tee_shm_dma_buf_ops, shm->size_alloc,
				O_RDWR);
#else
	dmabuf = dma_buf_export(shm, &_tee_shm_dma_buf_ops, shm->size_alloc,
				O_RDWR, NULL);
#endif

#endif
	if (IS_ERR_OR_NULL(dmabuf)) {
#ifdef TKCORE_KDBG
		dev_err(_DEV(tee), "%s: dmabuf: couldn't export buffer (%ld)\n",
			__func__, PTR_ERR(dmabuf));
#endif
		ret = -EINVAL;
		goto out;
	}

	*export = dma_buf_fd(dmabuf, O_CLOEXEC);
out:
	OUTMSG(ret);
	return ret;
}

int tee_shm_alloc_io_perm(struct tee_context *ctx, struct tee_shm_io *shm_io)
{
	struct tee_shm *shm;
	struct tee *tee = ctx->tee;
	int ret;

	INMSG();

	if (ctx->usr_client)
		shm_io->fd_shm = 0;

	mutex_lock(&tee->lock);
	shm = tee_shm_alloc(tee, shm_io->size, shm_io->flags);
	if (IS_ERR_OR_NULL(shm)) {
#ifdef TKCORE_KDBG
		dev_err(_DEV(tee), "%s: buffer allocation failed (%ld)\n",
			__func__, PTR_ERR(shm));
#endif
		ret = PTR_ERR(shm);
		goto out;
	}

	if (ctx->usr_client) {
		ret = export_buf(tee, shm, &shm_io->fd_shm);
		if (ret) {
			tee_shm_free(shm);
			ret = -ENOMEM;
			goto out;
		}

		shm->flags |= TEEC_MEM_DMABUF;
	}
	
	shm_io->paddr = (void *) shm->paddr;

	shm->ctx = ctx;
	shm->dev = get_device(_DEV(tee));
	ret = tee_get(tee);
	BUG_ON(ret);		/* tee_core_get must not issue */
	tee_context_get(ctx);

	tee_inc_stats(&tee->stats[TEE_STATS_SHM_IDX]);
	list_add_tail(&shm->entry, &ctx->list_shm);
out:
	mutex_unlock(&tee->lock);
	OUTMSG(ret);
	return ret;
}

int tee_shm_alloc_io(struct tee_context *ctx, struct tee_shm_io *shm_io)
{
	struct tee_shm *shm;
	struct tee *tee = ctx->tee;
	int ret;

	INMSG();

	if (ctx->usr_client)
		shm_io->fd_shm = 0;

	mutex_lock(&tee->lock);
	shm = tee_shm_alloc(tee, shm_io->size, shm_io->flags);
	if (IS_ERR_OR_NULL(shm)) {
#ifdef TKCORE_KDBG
		dev_err(_DEV(tee), "%s: buffer allocation failed (%ld)\n",
			__func__, PTR_ERR(shm));
#endif
		ret = PTR_ERR(shm);
		goto out;
	}

	if (ctx->usr_client) {
		ret = export_buf(tee, shm, &shm_io->fd_shm);
		if (ret) {
			tee_shm_free(shm);
			ret = -ENOMEM;
			goto out;
		}

		shm->flags |= TEEC_MEM_DMABUF;
	}

	shm->ctx = ctx;
	shm->dev = get_device(_DEV(tee));
	ret = tee_get(tee);
	BUG_ON(ret);		/* tee_core_get must not issue */
	tee_context_get(ctx);

	tee_inc_stats(&tee->stats[TEE_STATS_SHM_IDX]);
	list_add_tail(&shm->entry, &ctx->list_shm);
out:
	mutex_unlock(&tee->lock);
	OUTMSG(ret);
	return ret;
}

void tee_shm_free_io(struct tee_shm *shm)
{
	struct tee_context *ctx = shm->ctx;
	struct tee *tee = ctx->tee;
	struct device *dev = shm->dev;

#ifdef TKCORE_KDBG
	dev_dbg(_DEV(tee), "%s shm %p\n", __func__, shm);
#endif

	mutex_lock(&ctx->tee->lock);
	tee_dec_stats(&tee->stats[TEE_STATS_SHM_IDX]);
	list_del(&shm->entry);

	tee_shm_free(shm);
	tee_put(ctx->tee);
	tee_context_put(ctx);
	if (dev)
		put_device(dev);
	mutex_unlock(&tee->lock);
}

/* Buffer allocated by rpc from fw and to be accessed by the user
 * Not need to be registered as it is not allocated by the user */
int tee_shm_fd_for_rpc(struct tee_context *ctx, struct tee_shm_io *shm_io)
{
	struct tee_shm *shm = NULL;
	struct tee *tee = ctx->tee;
	int ret;
	struct list_head *pshm;

	INMSG();

	shm_io->fd_shm = 0;

	mutex_lock(&tee->lock);
	if (!list_empty(&tee->list_rpc_shm)) {
		list_for_each(pshm, &tee->list_rpc_shm) {
			shm = list_entry(pshm, struct tee_shm, entry);
			if ((void *)shm->paddr == shm_io->buffer)
				goto found;
		}
	}

#ifdef TKCORE_KDBG
	dev_err(_DEV(tee), "Can't find shm for %p\n", (void *)shm_io->buffer);
#endif
	ret = -ENOMEM;
	goto out;

found:
	ret = export_buf(tee, shm, &shm_io->fd_shm);
	if (ret) {
		ret = -ENOMEM;
		goto out;
	}

	shm->ctx = ctx;
	list_move(&shm->entry, &ctx->list_shm);

	shm->dev = get_device(_DEV(tee));
	ret = tee_get(tee);
	BUG_ON(ret);
	tee_context_get(ctx);

	BUG_ON(!tee->ops->shm_inc_ref(shm));
out:
	mutex_unlock(&tee->lock);
	OUTMSG(ret);
	return ret;
}

/* for compatibility with linux-3.0.86 (tiny4412) */
int __weak sg_nents(struct scatterlist *sg)
{
	int nents;
	for (nents = 0; sg; sg = sg_next(sg))
		nents++;
	return nents;
}

/******************************************************************************/

static int tee_shm_db_get(struct tee *tee, struct tee_shm *shm, int fd,
		unsigned int flags, size_t size, int offset)
{
	struct tee_shm_dma_buf *sdb;
	struct dma_buf *dma_buf;
	int ret = 0;

#ifdef TKCORE_KDBG
	dev_dbg(_DEV(tee), "%s: > fd=%d flags=%08x\n", __func__, fd, flags);
#endif

	dma_buf = dma_buf_get(fd);
	if (IS_ERR(dma_buf)) {
		ret = PTR_ERR(dma_buf);
		goto exit;
	}

	sdb = kzalloc(sizeof(*sdb), GFP_KERNEL);
	if (IS_ERR_OR_NULL(sdb)) {
#ifdef TKCORE_KDBG
		dev_err(_DEV(tee), "can't alloc tee_shm_dma_buf\n");
#endif
		ret = PTR_ERR(sdb);
		goto buf_put;
	}
	shm->sdb = sdb;

	if (dma_buf->size < size + offset) {
#ifdef TKCORE_KDBG
		dev_err(_DEV(tee), "dma_buf too small %zd < %zd + %d\n",
			dma_buf->size, size, offset);
#endif
		ret = -EINVAL;
		goto free_sdb;
	}

	sdb->attach = dma_buf_attach(dma_buf, _DEV(tee));
	if (IS_ERR_OR_NULL(sdb->attach)) {
		ret = PTR_ERR(sdb->attach);
		goto free_sdb;
	}

	sdb->sgt = dma_buf_map_attachment(sdb->attach, DMA_NONE);
	if (IS_ERR_OR_NULL(sdb->sgt)) {
		ret = PTR_ERR(sdb->sgt);
		goto buf_detach;
	}

	if (sg_nents(sdb->sgt->sgl) != 1) {
		ret = -EINVAL;
		goto buf_unmap;
	}

	shm->paddr = sg_phys(sdb->sgt->sgl) + offset;
	if (dma_buf->ops->attach == _tee_shm_attach_dma_buf)
		sdb->tee_allocated = true;
	else
		sdb->tee_allocated = false;

	shm->flags |= TEEC_MEM_DMABUF;

#ifdef TKCORE_KDBG
	dev_dbg(_DEV(tee), "fd=%d @p=%p is_tee=%d db=%p\n", fd,
			(void *)shm->paddr, sdb->tee_allocated, dma_buf);
#endif
	goto exit;

buf_unmap:
	dma_buf_unmap_attachment(sdb->attach, sdb->sgt, DMA_NONE);
buf_detach:
	dma_buf_detach(dma_buf, sdb->attach);
free_sdb:
	kfree(sdb);
buf_put:
	dma_buf_put(dma_buf);
exit:
	OUTMSG(ret);
	return ret;
}

#ifdef VA_GET_ENABLED
static unsigned int tee_shm_get_phy_from_kla(
		struct mm_struct *mm, unsigned int kla)
{
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *ptep, pte;
	unsigned int pa = 0;

	/* stolen from kernel3.10:mm/memory.c:__follow_pte */

	pgd = pgd_offset(mm, kla);
	if (pgd_none(*pgd) || pgd_bad(*pgd))
		return 0;

	pud = pud_offset(pgd, kla);
	if (pud_none(*pud) || pud_bad(*pud))
		return 0;

	pmd = pmd_offset(pud, kla);
	VM_BUG_ON(pmd_trans_huge(*pmd));
	if (pmd_none(*pmd) || pmd_bad(*pmd))
		return 0;

	/* We cannot handle huge page PFN maps.
	 * Luckily they don't exist. */
	if (pmd_huge(*pmd))
		return 0;

	ptep = pte_offset_map(pmd, kla);

	if (!ptep)
		return 0;

	pte = *ptep;

	if (pte_present(pte))
		pa = __pa(page_address(pte_page(pte)));

	if (!pa)
		return 0;

	return pa;

}

static int tee_shm_va_get(struct tee_context *ctx, struct tee_shm *shm,
		void *buffer, unsigned int flags, size_t size, int offset)
{
	int ret = 0;
	struct mm_struct *mm = current->mm;
	unsigned long va = (unsigned long)buffer;
	unsigned int virt_base = (va / PAGE_SIZE) * PAGE_SIZE;
	unsigned int offset_in_page = va - virt_base;
	unsigned int offset_total = offset_in_page + offset;
	struct vm_area_struct *vma;
	struct tee *tee = ctx->tee;

#ifdef TKCORE_KDBG
	dev_dbg(_DEV(tee), "%s: > %p\n", __func__, buffer);
#endif
	/* if the caller is the kernel api, active_mm is mm */
	if (!mm)
		mm = current->active_mm;

	BUG_ON(!mm);

	vma = find_vma(mm, virt_base);

	if (vma) {
		unsigned long pfn;
		/* It's a VMA => consider it a a user address */

		if (follow_pfn(vma, virt_base, &pfn)) {
#ifdef TKCORE_KDBG
			dev_err(_DEV(tee), "%s can't get pfn for %p\n",
				__func__, buffer);
#endif
			ret = -EINVAL;
			goto out;
		}

		shm->paddr = PFN_PHYS(pfn) + offset_total;

		if (vma->vm_end - vma->vm_start - offset_total < size) {
#ifdef TKCORE_KDBG
			dev_err(_DEV(tee), "%s %p:%x not big enough: %lx - %d < %x\n",
					__func__, buffer, shm->paddr,
					vma->vm_end - vma->vm_start,
					offset_total, size);
#endif
			shm->paddr = 0;
			ret = -EINVAL;
			goto out;
		}
	} else if (!ctx->usr_client) {
		/* It's not a VMA => consider it as a kernel address
		 * And look if it's an internal known phys addr
		 * Note: virt_to_phys is not usable since it can be a direct
		 * map or a vremap address
		*/
		unsigned int phys_base;
		int nb_page = (PAGE_SIZE - 1 + size + offset_total) / PAGE_SIZE;
		int i;

		spin_lock(&mm->page_table_lock);
		phys_base = tee_shm_get_phy_from_kla(mm, virt_base);

		if (!phys_base) {
			spin_unlock(&mm->page_table_lock);
#ifdef TKCORE_KDBG
			dev_err(_DEV(tee), "%s can't get physical address for %p\n",
					__func__, buffer);
#endif
			goto err;
		}

		/* Check continuity on size */
		for (i = 1; i < nb_page; i++) {
			unsigned int pa = tee_shm_get_phy_from_kla(mm,
					virt_base + i*PAGE_SIZE);
			if (pa != phys_base + i*PAGE_SIZE) {
				spin_unlock(&mm->page_table_lock);
#ifdef TKCORE_KDBG
				dev_err(_DEV(tee), "%s %p:%x not big enough: %lx - %d < %x\n",
						__func__, buffer, phys_base,
						i*PAGE_SIZE,
						offset_total, size);
#endif
				goto err;
			}
		}
		spin_unlock(&mm->page_table_lock);

		shm->paddr = phys_base + offset_total;
		goto out;
err:
		ret = -EINVAL;
	}

out:
#ifdef TKCORE_KDBG
	dev_dbg(_DEV(tee), "%s: < %d shm=%p vaddr=%p paddr=%x\n",
			__func__, ret, (void *)shm, buffer, shm->paddr);
#endif
	return ret;
}
#endif

struct tee_shm *tee_shm_get(struct tee_context *ctx, TEEC_SharedMemory *c_shm,
		size_t size, int offset)
{
	struct tee_shm *shm;
	struct tee *tee = ctx->tee;
	int ret;

#ifdef TKCORE_KDBG
	dev_dbg(_DEV(tee), "%s: > fd=%d flags=%08x\n",
			__func__, c_shm->d.fd, c_shm->flags);
#endif

	mutex_lock(&tee->lock);
	shm = kzalloc(sizeof(*shm), GFP_KERNEL);
	if (IS_ERR_OR_NULL(shm)) {
#ifdef TKCORE_KDBG
		dev_err(_DEV(tee), "can't alloc tee_shm\n");
#endif
		ret = -ENOMEM;
		goto err;
	}

	shm->ctx = ctx;
	shm->tee = tee;
	shm->dev = _DEV(tee);
	shm->flags = c_shm->flags | TEE_SHM_MEMREF;
	shm->size_req = size;
	shm->size_alloc = 0;

	if (c_shm->flags & TEEC_MEM_KAPI) {
		struct tee_shm *kc_shm = (struct tee_shm *)c_shm->d.ptr;

		if (!kc_shm) {
#ifdef TKCORE_KDBG
			dev_err(_DEV(tee), "kapi fd null\n");
#endif
			ret = -EINVAL;
			goto err;
		}
		shm->paddr = kc_shm->paddr;

		if (kc_shm->size_alloc < size + offset) {
#ifdef TKCORE_KDBG
			dev_err(_DEV(tee), "kapi buff too small %zd < %zd + %d\n",
				kc_shm->size_alloc, size, offset);
#endif
			ret = -EINVAL;
			goto err;
		}

#ifdef TKCORE_KDBG
		dev_dbg(_DEV(tee), "fd=%d @p=%p\n",
				c_shm->d.fd, (void *)shm->paddr);
#endif
	} else if (c_shm->d.fd) {
		ret = tee_shm_db_get(tee, shm,
				c_shm->d.fd, c_shm->flags, size, offset);
		if (ret)
			goto err;
	} else if (!c_shm->buffer) {
#ifdef TKCORE_KDBG
		dev_dbg(_DEV(tee), "null buffer, pass 'as is'\n");
#endif
	} else {
#ifdef VA_GET_ENABLED
		ret = tee_shm_va_get(ctx, shm,
				c_shm->buffer, c_shm->flags, size, offset);
		if (ret)
			goto err;
#else
		ret = -EINVAL;
		goto err;
#endif
	}

	mutex_unlock(&tee->lock);
	OUTMSGX(shm);
	return shm;

err:
	kfree(shm);
	mutex_unlock(&tee->lock);
	OUTMSGX(ERR_PTR(ret));
	return ERR_PTR(ret);
}

void tee_shm_put(struct tee_context *ctx, struct tee_shm *shm)
{
	struct tee *tee = ctx->tee;

#ifdef TKCORE_KDBG
	dev_dbg(_DEV(tee), "%s: > shm=%p flags=%08x paddr=%p\n",
			__func__, (void *)shm, shm->flags, (void *)shm->paddr);
#endif

	BUG_ON(!shm);
	BUG_ON(!(shm->flags & TEE_SHM_MEMREF));

	mutex_lock(&tee->lock);
	if (shm->flags & TEEC_MEM_DMABUF) {
		struct tee_shm_dma_buf *sdb;
		struct dma_buf *dma_buf;

		sdb = shm->sdb;
		dma_buf = sdb->attach->dmabuf;

#ifdef TKCORE_KDBG
		dev_dbg(_DEV(tee), "%s: db=%p\n", __func__, (void *)dma_buf);
#endif

		dma_buf_unmap_attachment(sdb->attach, sdb->sgt, DMA_NONE);
		dma_buf_detach(dma_buf, sdb->attach);
		dma_buf_put(dma_buf);

		kfree(sdb);
		sdb = 0;
	}

	kfree(shm);
	mutex_unlock(&tee->lock);
	OUTMSG(0);
}

