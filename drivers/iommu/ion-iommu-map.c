/*
 * Copyright (C) 20013-2013 hisilicon. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#include <linux/debugfs.h>
#include <linux/genalloc.h>
#include <linux/iommu.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/seq_file.h>
#include <linux/sizes.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/hisi/hisi-iommu.h>
#include <linux/hisi/ion-iommu.h>

#define MAX_IOVA_SIZE_2G 0x80000000UL
#define MAX_IOVA_START_ADDR_4G 0x100000000UL

struct map_result {
	unsigned long iova_start;
	unsigned long iova_size;
	unsigned long iommu_ptb_base;
	unsigned long iommu_iova_base;
	unsigned long is_tile;
};

static struct iommu_domain_data *ion_domain_info;
static struct ion_iommu_domain *ion_iommu_domain;
DEFINE_MUTEX(iova_pool_mutex);

static unsigned long hisi_alloc_iova(struct gen_pool *pool,
		unsigned long size, unsigned long align)
{
	unsigned long iova = 0;

	mutex_lock(&iova_pool_mutex);

	iova = gen_pool_alloc(pool, size);
	if (!iova) {
		mutex_unlock(&iova_pool_mutex);
		pr_err("hisi iommu gen_pool_alloc failed! size = %lu\n", size);
		return 0;
	}

	if (align > (1 << pool->min_alloc_order))
		WARN(1, "hisi iommu domain cant align to 0x%lx\n", align);

	mutex_unlock(&iova_pool_mutex);
	return iova;
}

static void hisi_free_iova(struct gen_pool *pool,
		unsigned long iova, size_t size)
{
	mutex_lock(&iova_pool_mutex);
	gen_pool_free(pool, iova, size);

	mutex_unlock(&iova_pool_mutex);
}

unsigned long hisi_iommu_alloc_iova(size_t size, unsigned long align)
{
	struct ion_iommu_domain *ion_domain = ion_iommu_domain;

	return hisi_alloc_iova(ion_domain->iova_pool, size, align);
}
EXPORT_SYMBOL_GPL(hisi_iommu_alloc_iova);

void hisi_iommu_free_iova(unsigned long iova, size_t size)
{
	int ret;
	struct ion_iommu_domain *ion_domain = ion_iommu_domain;

	ret = addr_in_gen_pool(ion_domain->iova_pool, iova, size);
	if(!ret) {
		pr_err("%s:illegal para!!iova = %lx, size = %lx\n",
				__func__, iova, size);
	}
	hisi_free_iova(ion_domain->iova_pool, iova, size);
}
EXPORT_SYMBOL_GPL(hisi_iommu_free_iova);

static struct gen_pool *iova_pool_setup(unsigned long start,
		unsigned long size, unsigned long align)
{
	struct gen_pool *pool = NULL;
	int ret = 0;

	pool = gen_pool_create(order_base_2(align), -1);/*lint !e666 */
	if (!pool) {
		pr_err("Create gen pool failed!\n");
		return NULL;
	}
	/* iova start should not be 0, because return
	   0 when alloc iova is considered as error */
	if (!start)
		WARN(1, "iova start should not be 0!\n");

	ret = gen_pool_add(pool, start, size, -1);
	if (ret) {
		pr_err("Gen pool add failed!\n");
		gen_pool_destroy(pool);
		return NULL;
	}

	return pool;
}


static void iova_pool_destroy(struct gen_pool *pool)
{
	gen_pool_destroy(pool);
}

static int iommu_check_tile_format(struct iommu_map_format *format,
				   unsigned long phys_len)
{
	unsigned long header_size = format->header_size;
	unsigned long phys_page_line = format->phys_page_line;
	unsigned long virt_page_line = format->virt_page_line;

	/* check invalid args */
	if ((phys_len <= header_size) || !phys_page_line || !virt_page_line) {
		pr_err("[%s]wrong arg:phys_len 0x%lx, header_size 0x%lx\n",
			 __func__, phys_len, header_size);
		pr_err("[%s]wrong arg:phys line 0x%lx, virt line 0x%lx\n",
			 __func__, phys_page_line, virt_page_line);
		return -EINVAL;
	}

	/* virt_page_line <= 128 and
	 * virt_page_line / phys_page_line = 1 in normal process */
	if ((virt_page_line * PAGE_SIZE > SZ_512K) ||
	    (virt_page_line / phys_page_line != 1)) {
		pr_err("%s:virt line/phys line out of standard\n", __func__);
		return -EINVAL;
	}

	/* check overflow */
	if ((virt_page_line * PAGE_SIZE <= virt_page_line) ||
	    (phys_page_line * PAGE_SIZE <= phys_page_line)) {
		pr_err("%s:virt_line/phys_line too large!!!\n", __func__);
		return -EINVAL;
	}

	return 0;
}

static int do_iommu_domain_map(struct ion_iommu_domain *ion_domain,
		struct scatterlist *sgl, struct iommu_map_format *format,
		struct map_result *result)
{
	int ret;
	unsigned long phys_len, iova_size;
	unsigned long iova_start;

	struct gen_pool *pool;
	struct iommu_domain *domain;
	struct scatterlist *sg;
	struct tile_format fmt;

	if (format->prot & IOMMU_SEC) {
		pr_err("prot 0x%lx\n", format->prot);
		return -EINVAL;
	}

	/* calculate whole phys mem length */
	for (phys_len = 0, sg = sgl; sg; sg = sg_next(sg))
		phys_len += (unsigned long)ALIGN(sg->length, PAGE_SIZE);

	/* get io virtual address size */
	if (format->is_tile) {
		unsigned long lines;
		unsigned long body_size;

		if (iommu_check_tile_format(format, phys_len)) {
			pr_err("%s:check foramt fail\n", __func__);
			return -EINVAL;
		}
		body_size = phys_len - format->header_size;
		lines = body_size / (format->phys_page_line * PAGE_SIZE);

		/*header need more lines virtual space*/
		if (format->header_size) {
			unsigned long header_size;

			header_size = ALIGN(format->header_size,
					format->virt_page_line * PAGE_SIZE);
			lines +=  header_size
					/ (format->virt_page_line * PAGE_SIZE);
		}

		iova_size = lines * format->virt_page_line * PAGE_SIZE - (format->virt_page_line - format->phys_page_line) * PAGE_SIZE;
	} else {
		iova_size = phys_len;
	}

	/* Limit not exceeding 2G */
	if (MAX_IOVA_SIZE_2G < iova_size) {
		pr_err("[%s]hisi_alloc_iova alloc size more than 2G(0x%lx).\n",__func__, iova_size);
		return -EINVAL;
	}

	/* alloc iova */
	pool = ion_domain->iova_pool;
	domain = ion_domain->domain;
	iova_start = hisi_alloc_iova(pool, iova_size, ion_domain->range.align);
	if (!iova_start) {
		pr_err("[%s]hisi_alloc_iova alloc size 0x%lx failed!"
		        "hisi ion pool avail 0x%lx\n",
			__func__, iova_size, gen_pool_avail(pool));
		return -EINVAL;
	}

	if (MAX_IOVA_START_ADDR_4G < (iova_start + iova_size)) {
		pr_err("hisi iommu can not deal with iova 0x%lx size 0x%lx\n",
			iova_start, iova_size);
	}

	/* do map */
	if (format->is_tile) {
		fmt.is_tile = format->is_tile;
		fmt.phys_page_line = format->phys_page_line;
		fmt.virt_page_line = format->virt_page_line;
		fmt.header_size = format->header_size;
		ret = iommu_map_tile(domain, iova_start,
					sgl, iova_size, 0, &fmt);
	} else {
		ret = iommu_map_sg(domain, iova_start, sgl,
				   sg_nents(sgl), format->prot);
	}

	if (ret != iova_size) {
		pr_err("[%s]map failed!iova_start = %lx, iova_size = %lx\n",
			__func__, iova_start, iova_size);
		hisi_free_iova(pool, iova_start, iova_size);
		return ret;
	}

	/* out put result */
	result->iova_start = iova_start;
	result->iova_size = iova_size;

	return 0;
}

int hisi_iommu_map_domain(struct scatterlist *sgl,
				struct iommu_map_format *format)
{
	int ret = 0;
	struct map_result result;
	struct ion_iommu_domain *ion_domain;

	ion_domain = ion_iommu_domain;

	memset(&result, 0, sizeof(result));

	ret = do_iommu_domain_map(ion_domain, sgl, format, &result);
	if (ret) {
		pr_err("alloc iova fail\n");
		return ret;
	}
	format->iova_start = result.iova_start;
	format->iova_size = result.iova_size;

	/* get value which write into iommu register */
	return ret;
}
EXPORT_SYMBOL_GPL(hisi_iommu_map_domain);

static int do_iommu_domain_unmap(struct map_result *result)
{
	int ret;
	unsigned long unmaped_size;
	struct ion_iommu_domain *ion_domain = ion_iommu_domain;
	struct gen_pool *pool = ion_domain->iova_pool;

	/* never unmap a zero length address space */
	if (!result->iova_size) {
		pr_err("[%s]unmap failed! iova_start=%lx, iova_size=%lu\n",
			__func__, result->iova_start, result->iova_size);
		return -EINVAL;
	}

	/* unmap tile equals to unmpa range */
	if (result->is_tile) {
		unmaped_size = iommu_unmap_tile(ion_domain->domain,
		result->iova_start, result->iova_size);
	} else {
		unmaped_size = iommu_unmap(ion_domain->domain,
				result->iova_start, result->iova_size);
	}

	if (unmaped_size != result->iova_size) {
		pr_err("[%s]unmap failed!\n", __func__);
		return -EINVAL;
	}
	/* free iova */
	if (pool) {
		ret = addr_in_gen_pool(pool, result->iova_start,
			result->iova_size);
		if(!ret) {
			pr_err("[%s]illegal para!!iova = %lx, size = %lx\n",
					__func__, result->iova_start, result->iova_size);
		}
		hisi_free_iova(pool, result->iova_start, result->iova_size);
	}
	return 0;
}

#ifdef CONFIG_ARM64_64K_PAGES
#error hisi iommu can not deal with 64k pages!
#endif

/**
 * Called by ION
 */
int hisi_iommu_unmap_domain(struct iommu_map_format *format)
{
	struct map_result result;

	result.iova_start = format->iova_start;
	result.iova_size = format->iova_size;
	result.is_tile = format->is_tile;

	return do_iommu_domain_unmap(&result);
}
EXPORT_SYMBOL_GPL(hisi_iommu_unmap_domain);

/*only used to test*/
phys_addr_t ion_iommu_domain_iova_to_phys(unsigned long iova)
{
	struct iommu_domain *domain;
	domain = ion_iommu_domain->domain;
	return iommu_iova_to_phys(domain, iova);
}
EXPORT_SYMBOL_GPL(ion_iommu_domain_iova_to_phys);

struct iommu_domain *hisi_ion_enable_iommu(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ion_iommu_domain *ion_domain;

	pr_info("in %s start\n", __func__);
	if (ion_iommu_domain) {
		pr_err("ion domain already init return domain\n");
		return ion_iommu_domain->domain;
	}

	ion_domain = kzalloc(sizeof(*ion_domain), GFP_KERNEL);
	if (!ion_domain) {
		pr_err("alloc ion_domain object fail\n");
		return NULL;
	}

	if (!iommu_present(dev->bus)) {
		pr_err("iommu not found\n");
		kfree(ion_domain);
		return NULL;
	}

	/* create iommu domain */
	ion_domain->domain = iommu_domain_alloc(dev->bus);
	if (!ion_domain->domain)
		goto error;

	iommu_attach_device(ion_domain->domain, dev);
	ion_domain_info = (struct iommu_domain_data *)ion_domain->domain->priv;

	/**
	 * Current align is 256K
	 */
	ion_domain->iova_pool = iova_pool_setup(ion_domain_info->iova_start,
			ion_domain_info->iova_size, ion_domain_info->iova_align);
	if (!ion_domain->iova_pool)
		goto error;

	/* this is a global pointer */
	ion_iommu_domain = ion_domain;

	pr_info("in %s end\n", __func__);
	return ion_iommu_domain->domain;

error:
	WARN(1, "ion_iommu_domain_init failed!\n");
	if (ion_domain->iova_pool)
		iova_pool_destroy(ion_domain->iova_pool);

	if (ion_domain->domain)
		iommu_domain_free(ion_domain->domain);

	kfree(ion_domain);

	ion_iommu_domain = NULL;
	return NULL;
}
EXPORT_SYMBOL(hisi_ion_enable_iommu);
