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
#define	U_BOOT_NAND		(0)

#if (U_BOOT_NAND)
#include <common.h>
#include <asm/errno.h>
#include <asm/io.h>
#include <malloc.h>
#include <linux/mtd/nand.h>
#include <nand.h>
#include <platform.h>
#else
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/mtd/nand.h>
#include <asm/io.h>
#include <mach/platform.h>
#include <mach/soc.h>
#endif

#if defined (CONFIG_MTD_NAND_NEXELL_HWECC) || defined (CONFIG_SYS_NAND_HW_ECC)

#define	NAND_WRITE_PAGE				(1)
#define	NAND_HW_WAIT				(1)
#define	NAND_WRITE_VERIFY_DATA		(1)
#define	NAND_READ_RETRY_MAX			(3)

#if (U_BOOT_NAND)
#if     NAND_HW_WAIT
#undef  NAND_HW_WAIT
#define	NAND_HW_WAIT		(0)
#endif
#endif /* U_BOOT_NAND */

#include "nand_ecc.h"

/*----------------------------------------------------------------------------*/
#ifdef  DBGOUT
#undef	DBGOUT
#endif
#ifdef  ECCERR
#undef	ECCERR
#endif
#if	(0)
#define DBGOUT(msg...)		{ printk(KERN_INFO "ecc: " msg); }
#else
#define DBGOUT(msg...)		do {} while (0)
#endif
#if	(0)
#define ECCERR(msg...)		{ printk(KERN_INFO "" msg); }
#else
#define ECCERR(msg...)		do {} while (0)
#endif

#define ERROUT(msg...)		{ 					\
		printk(KERN_ERR "ERROR: %s, %s line %d: \n",		\
			__FILE__, __FUNCTION__, __LINE__),	\
		printk(KERN_ERR msg); }


/*
 * 0x05	= Bad marker (256/512 byte pages)
 * 0x00 = Bad marker (2048/4096   byte pages)
 * 0x01 = Reserved   (2048/4096   byte pages)
 */
static struct nand_ecclayout nx_nand_oob = {
	.eccbytes 	=   0,
	.eccpos 	= { 0, },
	.oobfree 	= { {.offset = 0, .length = 0} }
};

/*------------------------------------------------------------------------------
 * u-boot nand hw ecc interface
 */
static int nand_sw_ecc_verify_buf(struct mtd_info *mtd, const uint8_t *buf, int len)
{
	int i;
	struct nand_chip *chip = mtd->priv;

	for (i = 0; len > i; i++)
		if (buf[i] != readb(chip->IO_ADDR_R))
			return -EFAULT;
	return 0;
}

static int nand_hw_ecc_read_page(struct mtd_info *mtd, struct nand_chip *chip,
				uint8_t *buf, int page)
{
	int i, n, k, ret = 0;

	int eccsteps = chip->ecc.steps;
	int eccbytes = chip->ecc.bytes;
	int eccsize  = chip->ecc.size;

	uint32_t  ecc_buff[ECC_HW_MAX_BYTES/4];
	uint8_t  *ecc_code = (uint8_t*)ecc_buff;
	uint32_t *ecc_pos  = chip->ecc.layout->eccpos;
	uint8_t  *p = buf;
	int retry = 0, err  = 0;

	int	o_syn[HW_ECC_MODE], e_pos[HW_ECC_MODE];
	uint32_t *e_dat;

	DBGOUT("%s, page=%d, ecc mode=%d, bytes=%d\n", __func__, page, HW_ECC_MODE, eccbytes);

	do {
		/* reset value */
		eccsteps = chip->ecc.steps;
		p 		 = buf;

		if (512 >= mtd->writesize) {
			/* read oob */
			chip->ecc.read_oob(mtd, chip, page, 1);
			/* read data */
			chip->cmdfunc(mtd, NAND_CMD_READ0, 0x00, page);
		} else {
			#if (0)
			/* read oob */
			chip->cmdfunc(mtd, NAND_CMD_RNDOUT, mtd->writesize, -1);
			chip->read_buf(mtd, chip->oob_poi, mtd->oobsize);
			/* read data */
			chip->cmdfunc(mtd, NAND_CMD_RNDOUT, 0, -1);
			#else
			chip->cmdfunc(mtd, NAND_CMD_READOOB, 0, page);
			chip->read_buf(mtd, chip->oob_poi, mtd->oobsize);
			chip->cmdfunc(mtd, NAND_CMD_READ0, 0, page);
			#endif
		}

		for (n = 0; eccsteps; eccsteps--, p += eccsize) {

			for (i = 0; eccbytes > i; i++, n++)
				ecc_code[i] = chip->oob_poi[ecc_pos[n]];

			/* write ecc data to orignal ecc register */
			NX_NAND_SetResetECC(HW_ECC_MODE);
			NX_NAND_SetOriECC((uint32_t*)ecc_code, HW_ECC_MODE);

			/* read data */
			chip->read_buf(mtd, p, eccsize);

			/* wait decoding  and error correct */
			NX_NAND_WaitForDecoding();

			if (NX_NAND_GetErrorStatus()) {
				/*
			 	 * check nand erase status
			 	 */
				int eccb = eccbytes >> 2;

				for (i = 0; eccb > i; i++)
					if (0xFFFFFFFF != ecc_buff[i])	break;
				for (i <<= 2 ; eccbytes > i; i++)
					if (0xFF != ecc_code[i]) break;
				if (i >= eccbytes)
					continue;

				/*
			  	 * Error correct
			 	 */
				NX_NAND_GetOddSyndrome(&o_syn[0]);
				err = NX_NAND_GetErrorLocation(&o_syn[0], &e_pos[0]);
				if (0 > err) {
					ERROUT("nand detect ecc errs, can't correct step:%2d(%2d), page:%d\n",
						(chip->ecc.steps-eccsteps+1), chip->ecc.steps, page);
					mtd->ecc_stats.failed++;
					ret = -EBADMSG;
					printk("Nand read retry [%d], retry: %d \n", page, retry);
					goto retry_r;	/* EXIT */
				} else {
					if (0 == err)
						continue;
					ECCERR("nand step %2d(%2d), errs %2d, page %d: \n",
						(chip->ecc.steps-eccsteps+1), chip->ecc.steps, err, page);
					for (k=0 ; err > k; k++) {
						ECCERR("ecc pos[%4d]\n", e_pos[k]);
						if (4096 > e_pos[k]) {
							e_dat = (uint32_t*)p;
							e_dat[e_pos[k] / 32] ^= 1U<<(e_pos[k] % 32);
						}
					}
					if (err > CFG_NAND_ECC_LIMIT) {
						mtd->ecc_stats.failed++;
						ret = -EUCLEAN;	// add to replace for yaffs2
					}

					#if !(U_BOOT_NAND)
					mtd->ecc_stats.corrected += err;
					#endif
				//	ECCERR("\n");
				}
			}
		}

		ret = 0;	/* no error */
		DBGOUT("DONE %s, ret=%d\n", __func__, ret);
		return ret;

retry_r:
		retry++;
	} while (NAND_READ_RETRY_MAX > retry);

	DBGOUT("FAIL %s, ret=%d, retry=%d\n", __func__, ret, retry);
	return ret;
}

static void nand_hw_ecc_write_page(struct mtd_info *mtd, struct nand_chip *chip,
				  const uint8_t *buf)
{
	int i, n;
	int eccsteps = chip->ecc.steps;
	int eccbytes = chip->ecc.bytes;
	int eccsize  = chip->ecc.size;

	uint32_t  ecc_buff[ECC_HW_MAX_BYTES/4];
	uint8_t  *ecc_code = (uint8_t*)ecc_buff;
	uint32_t *ecc_pos  = chip->ecc.layout->eccpos;
	uint8_t  *p = (uint8_t *)buf;

	DBGOUT("%s\n", __func__);

	/* write data and get ecc */
	for (n = 0; eccsteps; eccsteps--, p += eccsize) {

		NX_NAND_SetResetECC(HW_ECC_MODE);

		/* write page */
		chip->write_buf(mtd, p, eccsize);

		/* get ecc code from ecc register */
		NX_NAND_WaitForEncoding();
		NX_NAND_GetGenECC((uint32_t *)ecc_code, HW_ECC_MODE);

		/* set oob with ecc */
		for (i = 0; eccbytes > i; i++, n++)
			chip->oob_poi[ecc_pos[n]] = ecc_code[i];
	}

	/* write oob */
	chip->write_buf(mtd, chip->oob_poi, mtd->oobsize);
}

static int nand_hw_write_page(struct mtd_info *mtd, struct nand_chip *chip,
			   const uint8_t *buf, int page, int cached, int raw)
{
#ifdef CONFIG_MTD_NAND_VERIFY_WRITE
	struct mtd_ecc_stats stats;
	int ret = 0;
#endif
	int status;

	DBGOUT("%s page %d\n", __func__, page);
	chip->cmdfunc(mtd, NAND_CMD_SEQIN, 0x00, page);

	if (unlikely(raw))
		chip->ecc.write_page_raw(mtd, chip, buf);
	else
		chip->ecc.write_page(mtd, chip, buf);

	/*
	 * Cached progamming disabled for now, Not sure if its worth the
	 * trouble. The speed gain is not very impressive. (2.3->2.6Mib/s)
	 */
	cached = 0;

	if (!cached || !(chip->options & NAND_CACHEPRG)) {

		chip->cmdfunc(mtd, NAND_CMD_PAGEPROG, -1, -1);
		status = chip->waitfunc(mtd, chip);
		/*
		 * See if operation failed and additional status checks are
		 * available
		 */
		if ((status & NAND_STATUS_FAIL) && (chip->errstat))
			status = chip->errstat(mtd, chip, FL_WRITING, status,
					       page);

		if (status & NAND_STATUS_FAIL)
			return -EIO;
	} else {
		chip->cmdfunc(mtd, NAND_CMD_CACHEDPROG, -1, -1);
		status = chip->waitfunc(mtd, chip);
	}

#ifdef CONFIG_MTD_NAND_VERIFY_WRITE
	stats = mtd->ecc_stats;
	/* Send command to read back the data */
	chip->cmdfunc(mtd, NAND_CMD_READ0, 0, page);
	ret = nand_hw_ecc_read_page(mtd, chip, (uint8_t *)buf, page);
	if (ret)
		return ret;

	if (mtd->ecc_stats.failed - stats.failed)
		return -EBADMSG;	// EBADMSG
#endif
	return 0; // mtd->ecc_stats.corrected - stats.corrected ? -EUCLEAN : 0
}

#if (NAND_HW_WAIT)
static int nand_hw_wait(struct mtd_info *mtd, struct nand_chip *chip)
{
	unsigned long timeo = jiffies;
	int status, state = chip->state;

	if (state == FL_ERASING)
		timeo += (HZ * 400) / 1000;
	else
		timeo += (HZ * 40) / 1000;	/* 20 -> 40 */

	/* Apply this short delay always to ensure that we do wait tWB in
	 * any case on any machine. */
	ndelay(100);

	if ((state == FL_ERASING) && (chip->options & NAND_IS_AND))
		chip->cmdfunc(mtd, NAND_CMD_STATUS_MULTI, -1, -1);
	else
		chip->cmdfunc(mtd, NAND_CMD_STATUS, -1, -1);

	if (in_interrupt() || oops_in_progress) {
		int i;
		for (i = 0; i < timeo; i++) {
			if (chip->dev_ready) {
				if (chip->dev_ready(mtd))
					break;
			} else {
				if (chip->read_byte(mtd) & NAND_STATUS_READY)
					break;
			}
			mdelay(1);
		}
	} else {
		while (time_before(jiffies, timeo)) {
			if (chip->dev_ready) {
				if (chip->dev_ready(mtd))
					break;
			} else {
				if (chip->read_byte(mtd) & NAND_STATUS_READY)
					break;
			}
			cond_resched();
		}
	}
	status = (int)chip->read_byte(mtd);
	return status;
}
#endif

int nand_hw_ecc_check(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	struct nand_ecclayout *layout = chip->ecc.layout;
	struct nand_oobfree *oobfree  = chip->ecc.layout->oobfree;
	uint32_t *ecc_pos = chip->ecc.layout->eccpos;
	int ecctotal = chip->ecc.total;
	int oobsize	 = mtd->oobsize;
	int i = 0, n = 0;
	int ret = 0;

	if (512 > mtd->writesize) {
		printk(KERN_INFO "NAND ecc: page size %d not support hw ecc\n",
			mtd->writesize);
		chip->ecc.mode 			= NAND_ECC_SOFT;
		chip->ecc.read_page 	= NULL;
		chip->ecc.read_subpage 	= NULL;
		chip->ecc.write_page 	= NULL;
		chip->ecc.layout		= NULL;
		chip->verify_buf		= nand_sw_ecc_verify_buf;

		if ( chip->buffers &&
			!(chip->options & NAND_OWN_BUFFERS)) {
			kfree(chip->buffers);
			chip->buffers = NULL;
		}
		ret = nand_scan_tail(mtd);
		printk(KERN_INFO "NAND ecc: Software \n");
		return ret;
	}

	if (ecctotal > oobsize)  {
		printk(KERN_INFO "\n");
		printk(KERN_INFO "==================================================\n");
		printk(KERN_INFO "error: %d bit hw ecc mode requires ecc %d byte	\n", HW_ECC_MODE, ecctotal);
		printk(KERN_INFO "       it's over the oob %d byte for page %d byte	\n", oobsize, mtd->writesize);
		printk(KERN_INFO "==================================================\n");
		printk(KERN_INFO "\n");
		return -EINVAL;
	}

	/*
	 * set ecc layout
	 */
	if (16 >= oobsize) {
		for (i = 0, n = 0; ecctotal>i; i++, n++) {
			if (5 == n) n += 1;	// Bad marker
			ecc_pos[i] = n;
		}
		oobfree->offset  = n;
		oobfree->length  = oobsize - ecctotal - 1;
		layout->oobavail = oobfree->length;
		printk("hw ecc %d bit, bad '5', ecc 0~4,6~%d(%d), free %d~%d(%d), oob %d ",
			HW_ECC_MODE, ecctotal+1-1, ecctotal, oobfree->offset,
			oobfree->offset+oobfree->length-1, oobfree->length, oobsize);
	} else {

		oobfree->offset  = 2;
		oobfree->length  = oobsize - ecctotal - 2;
		layout->oobavail = oobfree->length;

		n = oobfree->offset + oobfree->length;
		for (i = 0; ecctotal>i; i++, n++)
			ecc_pos[i] = n;

		printk("hw ecc %d bit, bad '0,1', ecc %d~%d(%d), free 2~%d(%d), oob %d ",
			HW_ECC_MODE, oobfree->offset+oobfree->length, n-1,
			ecctotal, oobfree->length+2-1, oobfree->length, oobsize);
	}

	/* must reset mtd : set when u-boot version is up u-boot-2012*/
	mtd->ecclayout = chip->ecc.layout;
	mtd->oobavail  = chip->ecc.layout->oobavail;
	return ret;
}

int nand_hw_ecc_init(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	int eccbytes = 0;
	DBGOUT("%s\n", __func__);

	/*
	 * ecc.bytes:
	 *  4 bit ecc need  52 bit ( 6.5B) ecc code per 512 Byte	(13 *  4 =  52)
	 *  8 bit ecc need 104 bit (13.0B) ecc code per 512 Byte	(13 *  8 = 104)
     * 12 bit ecc need 156 bit (19.5B) ecc code per 512 Byte	(13 * 12 = 156)
	 * 16 bit ecc need 208 bit (26.0B) ecc code per 512 Byte	(13 * 16 = 208)
	 * 24 bit ecc need 336 bit (42.0B) ecc code per 1024 Byte	(14 * 24 = 336)
	 * 40 bit ecc need 560 bit (70.0B) ecc code per 1024 Byte	(14 * 40 = 560)
	 *
	 *  Page  512 Byte +  16 Byte
	 *  Page 2048 Byte +  64 Byte
	 *  Page 4096 Byte + 128 Byte
     *
     *  Page 8192 Byte + 436 Byte (MLC)
	 */
	switch (HW_ECC_MODE) {
	case  4: 	eccbytes =  7; 	break;
	case  8: 	eccbytes = 13; 	break;
	case 16: 	eccbytes = 26; 	break;
	case 24: 	eccbytes = 42; 	break;
	default:
		ERROUT("not supoort ecc %d mode !!!\n", HW_ECC_MODE);
		return -1;
	}

	chip->ecc.mode 			= NAND_ECC_HW;
	chip->ecc.read_page 	= nand_hw_ecc_read_page;
	chip->ecc.write_page 	= nand_hw_ecc_write_page;
	chip->ecc.size 			= HW_ECC_SIZE;		/* per 512 byte */
	chip->ecc.bytes 		= eccbytes;
	chip->ecc.layout		= &nx_nand_oob;

	#if (! NAND_WRITE_PAGE)
	chip->verify_buf		= nand_hw_verify_buf;
	#else
	chip->write_page		= nand_hw_write_page;
	#endif
	#if (NAND_HW_WAIT)
	chip->waitfunc 			= nand_hw_wait;		/* increase wait time */
	#endif

	NX_MCUS_SetECCMode(HW_ECC_MODE);
	NX_NAND_CreateLookupTable();
	return 0;
}

#if (0)
static int nand_hw_ecc_read_page_test(struct mtd_info *mtd, struct nand_chip *chip,
				uint8_t *buf, int page, int readoob, int raw)
{
	int i, n, k, ret = 0;

	int eccsteps = chip->ecc.steps;
	int eccbytes = chip->ecc.bytes;
	int eccsize  = chip->ecc.size;

	uint32_t  ecc_buff[ECC_HW_MAX_BYTES/4];
	uint8_t  *ecc_code = (uint8_t*)ecc_buff;
	uint32_t *ecc_pos  = chip->ecc.layout->eccpos;
	uint8_t  *p = buf;

	int err  = 0;
	int	o_syn[HW_ECC_MODE], e_pos[HW_ECC_MODE];
	uint32_t *e_dat;

	DBGOUT("%s, page=%d, ecc mode=%d, bytes=%d\n", __func__, page, HW_ECC_MODE, eccbytes);

	chip->select_chip(mtd, 0);

	if (512 >= mtd->writesize) {
		/* read oob */
		chip->ecc.read_oob(mtd, chip, page, 1);
		/* read data */
		chip->cmdfunc(mtd, NAND_CMD_READ0, 0x00, page);
	} else {
		#if (0)
		/* read oob */
		chip->cmdfunc(mtd, NAND_CMD_RNDOUT, mtd->writesize, -1);
		chip->read_buf(mtd, chip->oob_poi, mtd->oobsize);
		/* read data */
		chip->cmdfunc(mtd, NAND_CMD_RNDOUT, 0, -1);
		#else
		chip->cmdfunc(mtd, NAND_CMD_READOOB, 0, page);
		chip->read_buf(mtd, chip->oob_poi, mtd->oobsize);
		chip->cmdfunc(mtd, NAND_CMD_READ0, 0, page);
		#endif
	}

	for (n = 0; eccsteps; eccsteps--, p += eccsize) {

		for (i = 0; eccbytes > i; i++, n++)
			ecc_code[i] = chip->oob_poi[ecc_pos[n]];

		/* write ecc data to orignal ecc register */
		NX_NAND_SetResetECC(HW_ECC_MODE);
		NX_NAND_SetOriECC((uint32_t*)ecc_code, HW_ECC_MODE);

		/* read data */
		chip->read_buf(mtd, p, eccsize);

		/* wait decoding  and error correct */
		NX_NAND_WaitForDecoding();

		if (NX_NAND_GetErrorStatus() && readoob && !raw) {
			/*
			 * check nand erase status
			 */
			int eccb = eccbytes >> 2;

			for (i = 0; eccb > i; i++)
				if (0xFFFFFFFF != ecc_buff[i])	break;
			for (i <<= 2 ; eccbytes > i; i++)
				if (0xFF != ecc_code[i]) break;
			if (i >= eccbytes)
				continue;

			/*
			 * Error correct
			 */
			NX_NAND_GetOddSyndrome(&o_syn[0]);
			err = NX_NAND_GetErrorLocation(&o_syn[0], &e_pos[0]);
			if (0 > err) {
				ERROUT("nand detect ecc errs, can't correct step:%2d(%2d), page:%d\n",
					(chip->ecc.steps-eccsteps+1), chip->ecc.steps, page);
				mtd->ecc_stats.failed++;
				ret = -EIO;
				break;	/* EXIT */
			} else {
				if (0 == err)
					continue;
				ECCERR("nand step %2d(%2d), errs %2d, page %d: \n",
					(chip->ecc.steps-eccsteps+1), chip->ecc.steps, err, page);
				for (k=0 ; err > k; k++) {
					ECCERR("ecc pos[%4d]\n", e_pos[k]);
					if (4096 > e_pos[k]) {
						e_dat = (uint32_t*)p;
						e_dat[e_pos[k] / 32] ^= 1U<<(e_pos[k] % 32);
					}
				}
				if (err > CFG_NAND_ECC_LIMIT)
					mtd->ecc_stats.failed++;
				#if !(U_BOOT_NAND)
				mtd->ecc_stats.corrected += err;
				#endif
			//	ECCERR("\n");
			}
		}
	}

	if (raw)
		memcpy(p, chip->oob_poi, mtd->oobsize);

	DBGOUT("DONE %s, ret=%d\n", __func__, ret);
	return ret;
}

static int nand_hw_ecc_write_page_test(struct mtd_info *mtd, struct nand_chip *chip,
				  const uint8_t *buf, int page, int writeoob, int raw)
{
	int i, n;
	int eccsteps = chip->ecc.steps;
	int eccbytes = chip->ecc.bytes;
	int eccsize  = chip->ecc.size;

	uint32_t  ecc_buff[ECC_HW_MAX_BYTES/4];
	uint8_t  *ecc_code = (uint8_t*)ecc_buff;
	uint32_t *ecc_pos  = chip->ecc.layout->eccpos;
	uint8_t  *p = (uint8_t *)buf;

	struct mtd_ecc_stats stats;
	int status, ret = 0;

	DBGOUT("%s, 0x%x -> %d, %d, oob(%s)\n", __func__, (uint)buf, page, mtd->writesize, writeoob?"O":"X");
	chip->select_chip(mtd, 0);

	/* write page command */
	chip->cmdfunc(mtd, NAND_CMD_SEQIN, 0x00, page);

	/* write data and get ecc */
	for (n = 0; eccsteps; eccsteps--, p += eccsize) {

		NX_NAND_SetResetECC(HW_ECC_MODE);

		/* write page */
		chip->write_buf(mtd, p, eccsize);

		/* get ecc code from ecc register */
		NX_NAND_WaitForEncoding();
		NX_NAND_GetGenECC((uint32_t *)ecc_code, HW_ECC_MODE);

		/* set oob with ecc */
		for (i = 0; eccbytes > i; i++, n++)
			chip->oob_poi[ecc_pos[n]] = ecc_code[i];
	}

	/* write oob */
	if (writeoob && !raw)
		chip->write_buf(mtd, chip->oob_poi, mtd->oobsize);

	if (raw)
		chip->write_buf(mtd, p, mtd->oobsize);

	if (1) {
		chip->cmdfunc(mtd, NAND_CMD_PAGEPROG, -1, -1);
		status = chip->waitfunc(mtd, chip);

		/*
		 * See if operation failed and additional status checks are
		 * available
		 */
		if ((status & NAND_STATUS_FAIL) && (chip->errstat))
			status = chip->errstat(mtd, chip, FL_WRITING, status,
					       page);

		if (status & NAND_STATUS_FAIL)
			return -EIO;
	}

	if (!writeoob)
		return 0;

	if (raw)
		return 0;

	/*
	 *	write verify
	 */
	stats = mtd->ecc_stats;

	/* Send command to read back the data */
	chip->cmdfunc(mtd, NAND_CMD_READ0, 0, page);
	ret = nand_hw_ecc_read_page(mtd, chip, (uint8_t *)buf, page);
	if (ret)
		return ret;
	if (mtd->ecc_stats.failed - stats.failed)
		return -EIO;	// EBADMSG

	return 0;
}

int do_nand_page(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
	int ret = 0;
	ulong  addr, offs;
	char  *cmd;
	struct mtd_info  *mtd;
	struct nand_chip *chip;
	int read, oob = 1, raw = 0, page, i = 0;

	cmd = argv[1];

	if (nand_curr_device < 0 || nand_curr_device >= CONFIG_SYS_MAX_NAND_DEVICE ||
	    !nand_info[nand_curr_device].name) {
		ERROUT("\nno devices available\n");
		return 1;
	}

	mtd = &nand_info[nand_curr_device];
	if (! mtd) {
		ERROUT("no nand device: %d\n", nand_curr_device);
		return 1;
	}
	chip = mtd->priv;

	/* read and write */
	if (strncmp(cmd, "read", 4) == 0 || strncmp(cmd, "write", 5) == 0) {
		if (4 > argc)
			goto usage;

		addr = simple_strtoul(argv[2], NULL, 16);
		read = strncmp(cmd, "read", 4) == 0; /* 1 = read, 0 = write */
		offs = simple_strtoul(argv[3], NULL, 16);

		if (offs % mtd->writesize) {
			printf("Error: 0x%x is not page aligned 0x%x\n", (uint)offs, (uint)mtd->writesize);
			goto usage;
		}

		if (read) {
			oob = strncmp(cmd, "read.dat", 8)  != 0;
			raw = strncmp(cmd, "read.raw", 8)  == 0;
		} else {
			oob = strncmp(cmd, "write.dat", 9) != 0;
			raw = strncmp(cmd, "write.raw", 9) == 0;
		}

		printf("\n NAND_PAGE: %s %s %s Ecc data offs 0x%x %s 0x%x\n",
				read?"Read":"Write", raw?"raw data":"data", oob?"with":"skip", (uint)offs, read?"to":"from", (uint)addr);

		if (!read)
			printf("\n Note> Please check erase nand block .....\n\n");

		for (i = 0; i < CONFIG_SYS_MAX_NAND_DEVICE; i++) {
			if (nand_info[i].name) {
	    		nand_info_t *nand = &nand_info[i];
    			struct nand_chip *chip = nand->priv;
    			printf(" Device  %2d: ", i);
	    		if (chip->numchips > 1)
    	    		printf("%dx ", chip->numchips);
    			printf("%s\n", nand->name);
    			printf(" oob    size: %4u Byte\n", nand->oobsize);
    			if (nand->writesize >= (1<<10))
    				printf(" page   size: %4u KiB \n", nand->writesize >> 10);
    			else
    				printf(" page   size: %4u Byte\n", nand->writesize);
    			printf(" sector size: %4u KiB \n", nand->erasesize >> 10);
    		}
		}

		page = offs/mtd->writesize;

		if (read)
			ret = nand_hw_ecc_read_page_test (mtd, chip, (u_char*)addr, page, oob, raw);
		else
			ret = nand_hw_ecc_write_page_test(mtd, chip, (u_char*)addr, page, oob, raw);


		printf(" %u bytes %s %s data: %s\n", (uint)mtd->writesize,
	    	   read ? "Read" : "Written", oob ? "ecc" : "no ecc", ret ? "ERROR" : "OK");
	}

	return ret == 0 ? 0 : 1;
usage:
	cmd_usage(cmdtp);
	return 1;
}

U_BOOT_CMD(nandpage, CONFIG_SYS_MAXARGS, 1, do_nand_page,
	"NAND r/w test",
	"read      - 0xaddr 0xoffs\n"
	"    read error corrected data form offs num to addr\n"
	"nandpage read.dat  - 0xaddr 0xoffs\n"
	"    read not error corrected data form offs to addr\n"
	"nandpage read.raw  - 0xaddr 0xoffs\n"
	"    read raw data page (data + oob)\n"
	"nandpage write     - 0xaddr 0xoffs\n"
	"    write data form addr to offs\n"
	"nandpage write.dat - 0xaddr 0xoffs\n"
	"    write data form addr to offs "
	"nandpage write.raw - 0xaddr 0xoffs\n"
	"    write raw data to page (data + oob)"
);
#endif

//------------------------------------------------------------------------------
// NAND CTRL REGISGER:
//------------------------------------------------------------------------------
#define BASEADDR_NFREG			IO_ADDRESS(PHY_BASEADDR_NAND_MODULE)
#define BASEADDR_NFCTRL			(BASEADDR_NFREG)			// 0xC001587C
#define BASEADDR_NFECC			(BASEADDR_NFREG + 0x04)		// 0xC0015880
#define BASEADDR_NFORGECC		(BASEADDR_NFREG + 0x20)		// 0xC001589C
#define BASEADDR_NFECCSTATUS	(BASEADDR_NFREG + 0x40)		// 0xC00158BC
#define BASEADDR_NFSYNDROME		(BASEADDR_NFREG + 0x44)		// 0xC00158C0

#define REG_NFCTRL				(*(volatile unsigned int *)(BASEADDR_NFREG))
#define REG_NFECCSTATUS			(*(volatile unsigned int *)(BASEADDR_NFECCSTATUS))

#define NX_NFECCSTATUS_ERROR	(1U<< 2)
#define NX_NFECCSTATUS_DECDONE	(1U<< 1)
#define NX_NFECCSTATUS_ENCDONE	(1U<< 0)
#define NX_NFCTRL_ECCRST		(1U<<11)

//------------------------------------------------------------------------------
// BCH variables:
//------------------------------------------------------------------------------
//	k : number of information
//	m : dimension of Galois field.
//	t : number of error that can be corrected.
//	n : length of codeword = 2^m - 1
//	r : number of parity bit = m * t
//------------------------------------------------------------------------------
#define NX_BCH_VAR_K		(512 * 8)
#define NX_BCH_VAR_M		(13)
#define NX_BCH_VAR_T		(HW_ECC_MODE)		// 4 or 8 or 16

#define NX_BCH_VAR_N		(((1<<NX_BCH_VAR_M)-1))
#define NX_BCH_VAR_R		(NX_BCH_VAR_M * NX_BCH_VAR_T)

#define NX_BCH_VAR_TMAX		(16)
#define NX_BCH_VAR_RMAX		(NX_BCH_VAR_M * NX_BCH_VAR_TMAX)

#define NX_BCH_VAR_R32		((NX_BCH_VAR_R   +31)/32)
#define NX_BCH_VAR_RMAX32	((NX_BCH_VAR_RMAX+31)/32)


#define	iNX_BCH_VAR_N	 	NX_BCH_VAR_N
#define	iNX_BCH_VAR_T	 	NX_BCH_VAR_T
#define	iNX_BCH_VAR_R		(NX_BCH_VAR_M * iNX_BCH_VAR_T)

#define nn_max			32768	/* Length of codeword, n = 2**m - 1 */

static int alpha_to[nn_max], index_of[nn_max] ;	// Galois field
static int *BCH_AlphaToTable, *BCH_IndexOfTable;

//------------------------------------------------------------------------------
// Generate GF(2**NX_BCH_VAR_M) from the primitive polynomial p(X) in p[0]..p[NX_BCH_VAR_M]
// The lookup table looks like:
// index -> polynomial form:   pAlphaTo[ ] contains j = alpha**i;
// polynomial form -> index form:  pIndexOf[j = alpha**i] = i
// pAlphaTo[1] = 2 is the primitive element of GF(2**NX_BCH_VAR_M)
//------------------------------------------------------------------------------
void NX_NAND_CreateLookupTable(void)
{
	unsigned int i, mask, p;
	// Primitive polynomials
	// mm = 2^?
	p = 0x25AF;

	BCH_AlphaToTable = alpha_to;
	BCH_IndexOfTable = index_of;
	// Galois field implementation with shift registers
	// Ref: L&C, Chapter 6.7, pp. 217
	mask = 1 ;
	alpha_to[NX_BCH_VAR_M] = 0 ;

	for (i = 0; i < NX_BCH_VAR_M; i++)
	{
		alpha_to[i] = mask ;
		index_of[alpha_to[i]] = i ;

		if (((p>>i)&0x1) != 0)	alpha_to[NX_BCH_VAR_M] ^= mask ;

		mask <<= 1 ;
	}

	index_of[alpha_to[NX_BCH_VAR_M]] = NX_BCH_VAR_M ;
	mask >>= 1 ;

	for (i = NX_BCH_VAR_M + 1; i < (NX_BCH_VAR_N); i++)
	{
		if (alpha_to[i-1] >= mask)
			alpha_to[i] = alpha_to[NX_BCH_VAR_M] ^ ((alpha_to[i-1] ^ mask) << 1) ;
		else alpha_to[i] = alpha_to[i-1] << 1 ;

		index_of[alpha_to[i]] = i ;
	}
	index_of[0] = -1 ;
}
//------------------------------------------------------------------------------
static inline int	NX_BCH_AlphaTo( int index )
{
	return BCH_AlphaToTable[ index ];
}

static inline int	NX_BCH_IndexOf( int index )
{
	return BCH_IndexOfTable[ index ];
}

//------------------------------------------------------------------------------
void NX_NAND_SetResetECC(int EccMode)
{
	const U32 BIT_SIZE	= 2;
	const U32 BIT_POS	= 28;
	const U32 BIT_MASK	= ((1<<BIT_SIZE)-1) << BIT_POS;

	register U32 regval;

	EccMode /= 8;	// NFECCMODE[1:0] = 0(4), 1(8), 2(16)

	regval  = REG_NFCTRL;
	regval &= ~(BIT_MASK);	// Unmask bits.
	regval |= (EccMode << BIT_POS);

	// Reset H/W BCH decoder.
	REG_NFCTRL = regval | NX_NFCTRL_ECCRST;
}

//------------------------------------------------------------------------------
void NX_NAND_WaitForDecoding(void)
{
	while (0==(REG_NFECCSTATUS & NX_NFECCSTATUS_DECDONE)) { ; }
}

//------------------------------------------------------------------------------
void NX_NAND_WaitForEncoding(void)
{
	while (0==(REG_NFECCSTATUS & NX_NFECCSTATUS_ENCDONE)) { ; }
}

//------------------------------------------------------------------------------
int	NX_NAND_GetErrorStatus(void)
{
	if (REG_NFECCSTATUS & NX_NFECCSTATUS_ERROR)
		return 1;
	return 0;
}

//------------------------------------------------------------------------------
void NX_NAND_GetGenECC(unsigned int *pECC, int EccMode)
{
	int i, num;
	volatile U32 *pRegECC = (volatile U32 *)BASEADDR_NFECC;

	switch (EccMode) {
	case  4: num = 2;	break;
	case  8: num = 4;	break;
	case 16: num = 7;	break;
	case 24:
	default:
		ERROUT("not support ECC %d bit\n", EccMode);
		return;
	}

	for (i=0 ; i<num ; i++)
		*pECC++ = *pRegECC++;
}

void NX_NAND_SetOriECC(unsigned int *pECC, int EccMode)
{
	int i, num;
	volatile U32 *pRegOrgECC = (volatile U32 *)BASEADDR_NFORGECC;

	switch (EccMode) {
	case  4: num = 2;	break;
	case  8: num = 4;	break;
	case 16: num = 7;	break;
	case 24:
	default:
		ERROUT("not support ECC %d bit\n", EccMode);
		return;
	}

	for (i=0 ; num > i; i++)
		*pRegOrgECC++ = *pECC++;
}


//------------------------------------------------------------------------------
void NX_NAND_GetOddSyndrome(int *pSyndrome)
{
	const U32 BIT_SIZE	= 13;
	const U32 BIT_POS	= 13;
	const U32 BIT_MASK	= ((1UL<<BIT_SIZE)-1);

	register volatile U32 *pReg;
	register U32 regval;
	int i;

	NX_ASSERT( CNULL != pSyndrome );

	pReg = (volatile U32 *)BASEADDR_NFSYNDROME;

	for ( i=0 ; i<(NX_BCH_VAR_T/2) ; i++ ) {
		regval = *pReg++;
		*pSyndrome++ = (int)(regval & BIT_MASK);		// Syndrome <= NFSYNDROME[i][12: 0]
		*pSyndrome++ = (int)(regval >> BIT_POS);		// Syndrome <= NFSYNDROME[i][25:13]
	}
}

int elp[(NX_BCH_VAR_TMAX*2)+4][(NX_BCH_VAR_TMAX*2)+4]; // Error locator polynomial (ELP)
//------------------------------------------------------------------------------
int	NX_NAND_GetErrorLocation(int *pOddSyn, int *pLocation )
{
	int i, j, elp_sum ;
	int count;

	int L[(NX_BCH_VAR_TMAX*2)+3];			// Degree of ELP
	int u_L[(NX_BCH_VAR_TMAX*2)+3];		// Difference between step number and the degree of ELP
	U16 reg[NX_BCH_VAR_TMAX+3];			// Register state
	int desc[(NX_BCH_VAR_TMAX*2)+4]; 	// Discrepancy 'mu'th discrepancy
	int s[(NX_BCH_VAR_TMAX*2)+2];// = { 0, };

	int u;				// u = 'mu' + 1 and u ranges from -1 to 2*t (see L&C)
	int q;				//

	for(i=0; i<(NX_BCH_VAR_TMAX*2)+2; i++)
		s[i] = 0;

	for( i=0; i<iNX_BCH_VAR_T; i++ )
		s[i*2+1] = pOddSyn[i];

	// Even syndrome = (Odd syndrome) ** 2
	for( i=2; i<=(iNX_BCH_VAR_T*2); i+=2)
	{
		j=i/2;
		if( s[j] == 0 )		s[i] = 0;
		else				s[i] = NX_BCH_AlphaTo( ( 2 * NX_BCH_IndexOf(s[j]) ) % NX_BCH_VAR_N );
	}

	// initialise table entries
	for (i = 1; i <= (iNX_BCH_VAR_T*2); i++) 	s[i] = NX_BCH_IndexOf(s[i]);

	desc[0] = 0;				/* index form */
	desc[1] = s[1];				/* index form */
	elp[0][0] = 1;				/* polynomial form */
	elp[1][0] = 1;				/* polynomial form */
	for (i = 1; i < (iNX_BCH_VAR_T*2); i++) {
		elp[0][i] = 0;			/* polynomial form */
		elp[1][i] = 0;			/* polynomial form */
	}
	L[0] = 0;
	L[1] = 0;
	u_L[0] = -1;
	u_L[1] = 0;
	u = -1;

	do{
		// even loops always produce no discrepany so they can be skipped
		u = u + 2;
		if (desc[u] == -1) {
			L[u + 2] = L[u];
			for (i = 0; i <= L[u]; i++)
				elp[u + 2][i] = elp[u][i];
		}
		else {
			// search for words with greatest u_L[q] for which desc[q]!=0
			q = u - 2;
			if (q<0) q=0;
			// Look for first non-zero desc[q]
			while ((desc[q] == -1) && (q > 0))
				q=q-2;
			if (q < 0) q = 0;

			// Find q such that desc[u]!=0 and u_L[q] is maximum
			if (q > 0) {
				j = q;
			  	do {
					j=j-2;
					if (j < 0) j = 0;
					if ((desc[j] != -1) && (u_L[q] < u_L[j]))
						q = j;
				} while (j > 0);
			}

			// store degree of new elp polynomial
			if (L[u] > L[q] + u - q)
				L[u + 2] = L[u];
			else
				L[u + 2] = L[q] + u - q;

			// Form new elp(x)
			for (i = 0; i < (iNX_BCH_VAR_T*2); i++)
				elp[u + 2][i] = 0;
			for (i = 0; i <= L[q]; i++)
				if (elp[q][i] != 0)
					elp[u + 2][i + u - q] = NX_BCH_AlphaTo((desc[u] + NX_BCH_VAR_N - desc[q] + NX_BCH_IndexOf(elp[q][i])) % NX_BCH_VAR_N);
			for (i = 0; i <= L[u]; i++)
				elp[u + 2][i] ^= elp[u][i];
		}
		u_L[u + 2] = u+1 - L[u + 2];

		// Form (u+2)th discrepancy.  No discrepancy computed on last iteration
		if (u < (iNX_BCH_VAR_T*2))
		{
			if (s[u + 2] != -1)
				desc[u + 2] = NX_BCH_AlphaTo(s[u + 2]);
			else
				desc[u + 2] = 0;

			for (i = 1; i <= L[u + 2]; i++)
				if ((s[u + 2 - i] != -1) && (elp[u + 2][i] != 0))
					desc[u + 2] ^= NX_BCH_AlphaTo((s[u + 2 - i] + NX_BCH_IndexOf(elp[u + 2][i])) % NX_BCH_VAR_N);
			// put desc[u+2] into index form
			desc[u + 2] = NX_BCH_IndexOf(desc[u + 2]);
		}

	} while ((u < ((iNX_BCH_VAR_T*2)-1)) && (L[u + 2] <= iNX_BCH_VAR_T));

	u=u+2;
	L[(iNX_BCH_VAR_T*2)-1] = L[u];

	for( i=0 ; i<=iNX_BCH_VAR_T ; i++ )
		reg[i] = 0 ;

	{
		// Chien's search to find roots of the error location polynomial
		// Ref: L&C pp.216, Fig.6.1
		for( i=1 ; i<=L[(iNX_BCH_VAR_T*2)-1] ; i++ )
			reg[i] = (U16)elp[u][i];

		count	= 0;

		for( i=1 ; i<=NX_BCH_VAR_N ; i++ )
		{
			elp_sum = 1;
			for( j=1 ; j<=L[(iNX_BCH_VAR_T*2)-1] ; j++ )
			{
				if( reg[j] != 0 )
				{
					reg[j] = NX_BCH_AlphaTo( (NX_BCH_IndexOf(reg[j]) + j) % NX_BCH_VAR_N );
					elp_sum ^= reg[j] ;
				}
			}

			if( !elp_sum )		// store root and error location number indices
			{
//				root[count] = i;

				// Convert error location from systematic form to storage form
				pLocation[count] = NX_BCH_VAR_N - i;

				if (pLocation[count] >= iNX_BCH_VAR_R)
				{
					// Data Bit Error
					pLocation[count] = pLocation[count] - iNX_BCH_VAR_R;
					pLocation[count] = (NX_BCH_VAR_K-1) - pLocation[count];
				}
				else
				{
					// ECC Error
					pLocation[count] = pLocation[count] + NX_BCH_VAR_K;
				}

				//NX_CONSOLE_Printf("pLocation[%d] = %d\n", count, pLocation[count] );

				if( pLocation[count] < 0 )
				{
					DBGOUT("\nL[(NX_BCH_VAR_T*2)-1] = %d\n", L[(iNX_BCH_VAR_T*2)-1] );
					DBGOUT("pLocation[%d] = %d\n", count, pLocation[count] );
					//return -1;
				}
				else
				{
					count++;
				}
			}
		}

		if( count == L[(iNX_BCH_VAR_T*2)-1] )	// Number of roots = degree of elp hence <= NX_BCH_VAR_T errors
		{
			return L[(iNX_BCH_VAR_T*2)-1];
		}
		else	// Number of roots != degree of ELP => >NX_BCH_VAR_T errors and cannot solve
			return -1;

		/*
		{
			if( count != L[(NX_BCH_VAR_T*2)-1] )
			{
				NX_TRACE(("count = %d, L[(NX_BCH_VAR_T*2)-1] = %d\n\n", count, L[(NX_BCH_VAR_T*2)-1] ));
				if( count < 4 )			return -1;
				//if( L[(NX_BCH_VAR_T*2)-1] == 16 )	return -1;
			}
			return count;
			//return -1;
		}
		*/
	}
}
#endif /* CONFIG_MTD_NAND_NEXELL_HWECC */



