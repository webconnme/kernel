/*
 * (C) Copyright 2010
 * jung hyun kim, Nexell Co, <jhkim@nexell.co.kr>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>

/* Machine Headers */
#include <mach/platform.h>
#include <mach/devices.h>
#include <mach/soc.h>

#if	(0)
#define DBGOUT(msg...)		{ printk(KERN_INFO "nand_dma: " msg); }
#else
#define DBGOUT(msg...)		do {} while (0)
#endif

struct nand_dma {
	unsigned int	  mem_phy;
	unsigned int	  mem_vir;
	unsigned int	  mem_len;

	struct dma_trans *dma_tr;

	unsigned int	  irqno;
	wait_queue_head_t wait_q;
	unsigned int	  irqcond;
	struct mutex	  lock;

	int				  direction; /* 0: read, 1: write */
	uint8_t 		 *data;
	unsigned int	  translen;
	int				  transfer;
	int				  ecc_count;
	uint32_t  		  ecc_buff[ECC_HW_MAX_BYTES/4];
};

#define	NAND_DMA_TIMEOUT_R		100 	// (msec)
#define	NAND_DMA_TIMEOUT_W		100 	// (msec)

static irqreturn_t nand_hw_ecc_irq_dma(int irq, void *desc)
{
	struct mtd_info  *mtd  = desc;
	struct nand_chip *chip = mtd->priv;
	struct nand_dma  *dma  = chip->priv;
	struct dma_trans *dtr  = dma->dma_tr;
	unsigned int direction = dma->direction;
	unsigned int offset    = dma->transfer;

	uint8_t  *ecc_code = (uint8_t*)dma->ecc_buff;
	uint32_t *ecc_pos  = chip->ecc.layout->eccpos;
	int eccbytes = chip->ecc.bytes;
	int dmalen   = chip->ecc.size;
	int i = 0;

	if (dma->transfer == dma->translen) {
		dma->irqcond = 1;
		wake_up(&dma->wait_q);
		DBGOUT("done , dma write %6d\n", offset);
		return IRQ_HANDLED;
	}

	/* write */
	if (direction) {
		NX_NAND_WaitForEncoding();
		NX_NAND_GetGenECC((uint32_t *)ecc_code, HW_ECC_MODE);

		for (i = 0; eccbytes > i; i++, dma->ecc_count++)
			chip->oob_poi[ecc_pos[dma->ecc_count]] = ecc_code[i];

		dtr->srcbase   = dma->mem_phy + offset;
		dma->transfer += dmalen;

		/* next data transfer */
		NX_NAND_SetResetECC(HW_ECC_MODE);
		soc_dma_transfer(dtr);
	}
	return IRQ_HANDLED;
}

static void 	nand_hw_ecc_write_page_dma(struct mtd_info *mtd, struct nand_chip *chip,
				  const uint8_t *buf)
{
	struct nand_dma  *dma = chip->priv;
	struct dma_trans *dtr = dma->dma_tr;
	int dmalen  = chip->ecc.size;
	int pagelen = mtd->writesize;
	int ret = 0;
	long timeout = msecs_to_jiffies(NAND_DMA_TIMEOUT_W);

	DBGOUT("+++%s\n", __func__);
	dma->translen  = pagelen;
	dma->data      = (uint8_t*)buf;
	dma->transfer  = dmalen;
	dma->direction = 1;		/* write */
	dma->irqcond   = 0;
	dma->ecc_count = 0;

	dtr->srcbase   = dma->mem_phy;
	dtr->length    = dmalen;
	dtr->dstbase   = PHY_BASEADDR_NAND_STATIC;
	dtr->dst_id    = DMAINDEX_OF_MCUS_MODULE; 	/* when static to mem, this is ignored */
	dtr->dst_bit   = 32;
	dtr->tr_type   = DMA_TRANS_MEM2SIO;

	NX_NAND_SetResetECC(HW_ECC_MODE);
	memcpy((void*)dma->mem_vir, buf, pagelen);

	soc_dma_transfer(dtr);

	/* complete transfer */
	ret = wait_event_timeout(dma->wait_q, dma->irqcond, timeout);
	dma->irqcond ? (dma->irqcond = 0, ret = 0) : (ret = -1);

	/* write oob */
	if (0 == ret)
		chip->write_buf(mtd, chip->oob_poi, mtd->oobsize);
	else
		printk(KERN_ERR "fail, nand hw ecc dma wrote %d, wait %dms ...\n",
			dma->transfer, jiffies_to_msecs(timeout));

	DBGOUT("---%s (trans %d)\n", __func__, dma->translen);
}

int nand_hw_ecc_init_dma(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	struct device    *dev = &mtd->dev;
	struct nand_dma  *dma = NULL;
	int ret = 0;

	/* not support */
	if (512 > mtd->writesize)
		return ret;

	/* set nand_dma */
	dma = kzalloc(sizeof(struct nand_dma), GFP_KERNEL);
    if (NULL == dma) {
        printk(KERN_ERR "fail, request for nand dma info...\n");
		return -ENOMEM;
    }
//	chip->ecc.read_page = nand_hw_ecc_read_page_dma;
	chip->ecc.write_page = nand_hw_ecc_write_page_dma;
	chip->priv			 = dma;

	/* allocate hw dma */
	#if (CFG_DMA_NAND_RW)
    dma->dma_tr = soc_dma_request(CFG_DMA_NAND_RW, false);
	#else
    dma->dma_tr = soc_dma_request(0, true);
    #endif
    if (NULL == dma->dma_tr) {
        printk(KERN_ERR "fail, request for nand dma channel ...\n");
       	ret = -EINVAL;
       	goto _fail;
    }

	/* allocate dma buffer */
	dev->coherent_dma_mask = 0xffffffff;
    dma->irqno = PB_DMA_IRQ(dma->dma_tr->channel);
	dma->mem_len   = PAGE_ALIGN(chip->ecc.size);
	dma->mem_vir   = (u_int)dma_alloc_writecombine(
								dev, dma->mem_len,
								(dma_addr_t*)&dma->mem_phy,	GFP_KERNEL);
	if (!dma->mem_vir) {
		printk(KERN_ERR "fail, allocate for nand dma buffer...\n");
       	ret = -ENOMEM;
       	goto _fail;
	}

	/* request dma irq */
    ret = request_irq(dma->irqno, nand_hw_ecc_irq_dma,
    					IRQF_DISABLED, mtd->name, mtd);
    if (ret) {
        printk(KERN_ERR "fail, request for nand dma irq %d...\n", dma->irqno);
       	ret = -EINVAL;
       	goto _fail;
    }
	init_waitqueue_head(&dma->wait_q);
	printk("hw nand dma ch %d, irq %d", dma->dma_tr->channel, dma->irqno);
	return ret;

_fail:
	if (dma->mem_vir)
		dma_free_coherent(dev, dma->mem_len, (void *)dma->mem_vir, dma->mem_phy);

	if (dma->dma_tr)
		soc_dma_release(dma->dma_tr);

	if (dma)
		kfree(dma);

	chip->priv = NULL;
	return ret;
}