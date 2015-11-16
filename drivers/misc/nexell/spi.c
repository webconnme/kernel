/*
 * (C) Copyright 2010
 * jung hyun kim, Nexell Co, <jhkim@nexell.co.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <asm/uaccess.h>

/* nexell soc headers */
#include <mach/platform.h>
#include <mach/devices.h>
#include <mach/soc.h>
#include <mach/spi.h>

#if (0)
#define DBGOUT(msg...)		{ printk(KERN_INFO "spi: " msg); }
#else
#define DBGOUT(msg...)		do {} while (0)
#endif

#define CFG_SPI_MOD_MASTER		CTRUE
#define CFG_SPI_CLK_SRC			CFG_SYS_CLKSRC_PLL1	/* 0 = PLL0, 1 = PLL1 */
#define CFG_SPI_CLK_FREQ		CFG_SYS_PLL1_FREQ
#define CFG_SPI_CLK_DIV			8					/* 1 ~ 256 */
#define CFG_SPI_CLK_DIV2		24					/* 4 ~ 32 */
#define CFG_SPI_CLK_INV			CTRUE
#define CFG_SPI_BIT_WIDTH		8

/*----------------------------------------------------------------------------*/
struct spi_io_pad {
	unsigned int	io;
	unsigned int	iofunc;
};

struct spi_io_func {
	struct spi_io_pad	frame;
	struct spi_io_pad	clock;
	struct spi_io_pad	rx_d;
	struct spi_io_pad	tx_d;
};

static const struct spi_io_func spi_io[] = {
	{
		{ (PAD_GPIO_A + 28), NX_GPIO_PADFUNC_1 },
		{ (PAD_GPIO_A + 29), NX_GPIO_PADFUNC_1 },
		{ (PAD_GPIO_A + 30), NX_GPIO_PADFUNC_1 },
		{ (PAD_GPIO_A + 31), NX_GPIO_PADFUNC_1 }
	},
/*
	{
	...
	},
*/
};

#define	SPI_CHANNEL_NUM		(sizeof(spi_io)/sizeof(spi_io[0]))
#define	SPI_CLOCK_MAX		(CFG_SPI_CLK_FREQ / CFG_SPI_CLK_DIV /  4)
#define	SPI_CLOCK_MIN		(CFG_SPI_CLK_FREQ / CFG_SPI_CLK_DIV / 32)
#define	SPI_CLOCK_CURR		(CFG_SPI_CLK_FREQ / CFG_SPI_CLK_DIV / CFG_SPI_CLK_DIV2)

static DEFINE_MUTEX(mutex);
static int 			hw_init = 0;
static unsigned int spi_clk = SPI_CLOCK_CURR;	/* 1 MHZ */

/*----------------------------------------------------------------------------*/
static int	set_spi_init(int ch, unsigned int clock)
{
	unsigned int _div = CFG_SPI_CLK_FREQ/CFG_SPI_CLK_DIV/clock;

	if (hw_init)
		return 0;

	if (ch >= SPI_CHANNEL_NUM)
		return -1;

	mutex_lock(&mutex);

	/* set spi pad */
	soc_gpio_set_io_func(spi_io[ch].frame.io, (NX_GPIO_PADFUNC)spi_io[ch].frame.iofunc);
	soc_gpio_set_io_func(spi_io[ch].clock.io, (NX_GPIO_PADFUNC)spi_io[ch].clock.iofunc);
	soc_gpio_set_io_func(spi_io[ch].rx_d.io , (NX_GPIO_PADFUNC)spi_io[ch].rx_d.iofunc );
	soc_gpio_set_io_func(spi_io[ch].tx_d.io , (NX_GPIO_PADFUNC)spi_io[ch].tx_d.iofunc );
	soc_gpio_set_io_pullup(spi_io[ch].tx_d.io, 1);

	/* set spi module */
	NX_SSPSPI_SetClockPClkMode(ch, NX_PCLKMODE_ALWAYS);
	NX_SSPSPI_SetClockSource(ch, 0, CFG_SPI_CLK_SRC);
	NX_SSPSPI_SetClockDivisor(ch, 0, CFG_SPI_CLK_DIV);
	NX_SSPSPI_SetClockDivisorEnable(ch, CTRUE);

	NX_SSPSPI_SetEnable(ch, CFALSE);

	/* master mode */
	NX_SSPSPI_SetExternalClockSource(ch, CFG_SPI_MOD_MASTER ? CFALSE : CTRUE);

 	NX_SSPSPI_SetDividerCount(ch, _div);

	NX_SSPSPI_SetBitWidth(ch, CFG_SPI_BIT_WIDTH);
	NX_SSPSPI_SetDMATransferMode(ch, CFALSE);
	NX_SSPSPI_SetRxBurstEnable(ch, CFALSE);

	NX_SSPSPI_SetProtocol(ch, NX_SSPSPI_PROTOCOL_SPI);
	NX_SSPSPI_SetSPIFormat(ch, NX_SSPSPI_FORMAT_B);
	NX_SSPSPI_SetClockPolarityInvert(ch, CFG_SPI_CLK_INV);
	NX_SSPSPI_SetSlaveMode(ch, CFALSE);

	NX_SSPSPI_SetInterruptEnableAll(ch, CFALSE);
	NX_SSPSPI_ClearInterruptPendingAll(ch);

	hw_init = 1;
	mutex_unlock(&mutex);

	return 0;
}

static int	set_spi_clock(int ch, unsigned int clock)
{
	unsigned int _div = CFG_SPI_CLK_FREQ/CFG_SPI_CLK_DIV/clock;
	unsigned int _clk = _div ? CFG_SPI_CLK_FREQ/CFG_SPI_CLK_DIV/_div : clock;

	if (_clk > SPI_CLOCK_MAX || SPI_CLOCK_MIN > _clk) {
		printk(KERN_ERR "\nInvalid spi %d, clock:%u hz (%lu ~ %lu) \n",
			ch, _clk, SPI_CLOCK_MIN, SPI_CLOCK_MAX);
		return -1;
	}

	mutex_lock(&mutex);

	NX_SSPSPI_SetEnable(ch, CFALSE);
	NX_SSPSPI_SetClockDivisorEnable(ch, CFALSE);

	NX_SSPSPI_SetClockPClkMode(ch, NX_PCLKMODE_ALWAYS);
	NX_SSPSPI_SetClockSource(ch, 0, CFG_SPI_CLK_SRC);
	NX_SSPSPI_SetClockDivisor(ch, 0, CFG_SPI_CLK_DIV);
	NX_SSPSPI_SetClockDivisorEnable(ch, CTRUE);
 	NX_SSPSPI_SetDividerCount(ch, _div);

	spi_clk = _clk;

	mutex_unlock(&mutex);

	DBGOUT("\nspi clock %uhz, %lu hz ~ %lu hz, pll:%lu\n",
		_clk, SPI_CLOCK_MIN, SPI_CLOCK_MAX, CFG_SPI_CLK_FREQ);
	return 0;
}

int soc_spi_readnwrite(int ch, char * inb, char * outb, int size)
{
	int i;

	if (inb == NULL && outb == NULL)
		return 0;

	if (! hw_init)
		set_spi_init(ch, spi_clk);

	/* lock */
	mutex_lock(&mutex);

	NX_ASSERT(size <= 64);
	NX_ASSERT(CFALSE == NX_SSPSPI_GetEnable(ch));

	NX_SSPSPI_SetDMATransferMode(ch, CFALSE);
	NX_SSPSPI_ResetFIFO(ch);

	for (i = 0; size > i; i++)	{
		if (inb)
			NX_SSPSPI_PutByte(ch, inb[i]);
		else
			NX_SSPSPI_PutByte(ch, 0xFF);
	}

	NX_SSPSPI_SetEnable(ch, CTRUE);

	/* wait */
	while (CFALSE == NX_SSPSPI_IsTxFIFOEmpty(ch)) { ; }

	for (i = 0; size > i; i++) {

		/* wait */
		while (CTRUE == NX_SSPSPI_IsRxFIFOEmpty(ch)) { ; }

		if (outb)
			outb[i]= NX_SSPSPI_GetByte(ch);
		else
			NX_SSPSPI_GetByte(ch);
	}

	NX_SSPSPI_SetEnable(ch, CFALSE);

	/* unlock */
	mutex_unlock(&mutex);

	return 1;
}
EXPORT_SYMBOL_GPL(soc_spi_readnwrite);

/*------------------------------------------------------------------------------
 * 	SPI ops
 */
static int nx_spi_ops_open(struct inode *inode, struct file *flip)
{
	DBGOUT("%s\n", __func__);
	return 0;
}

static int nx_spi_ops_release(struct inode *inode, struct file *flip)
{
	DBGOUT("%s\n", __func__);
	return 0;
}


static int nx_spi_ops_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	DBGOUT("%s(cmd:0x%x)\n", __func__, cmd);

	switch (cmd)	{
	case IOCTL_SPI_WRITE:
	case IOCTL_SPI_READ:
		{
			struct spi_info spi;
			unsigned char inb[16], outb[16];
			unsigned char *pin = NULL, *pout = NULL;
			int size = 0;

			if (copy_from_user(&spi, (const void __user *)arg, sizeof(struct spi_info)))
				return -EFAULT;

			if (spi.inbuff) {
				if (copy_from_user(inb, (const void __user *)spi.inbuff, spi.size))
					return -EFAULT;
				pin = inb;
			}

			if (spi.outbuff)
				pout = outb;

			size = spi.size;

			if (! soc_spi_readnwrite(spi.channel, pin, pout, size))
				return -ENXIO;

			if (spi.outbuff) {
				if (copy_to_user((void __user *)spi.outbuff, (const void *)outb, spi.size))
					return -EFAULT;
			}
		}
		break;

	case IOCTL_SPI_GET_CLK:
		{
			struct spi_clock clk;

			clk.min   = SPI_CLOCK_MIN;
			clk.max   = SPI_CLOCK_MAX;
			clk.clock = spi_clk;

			if (copy_to_user((void __user *)arg, (const void *)&clk, sizeof(struct spi_clock)))
				return -EFAULT;
		}
		break;

	case IOCTL_SPI_SET_CLK:
		{
			struct spi_clock clk;

			if (copy_from_user(&clk, (const void __user *)arg, sizeof(struct spi_clock)))
				return -EFAULT;

			if (0 > set_spi_clock(clk.channel, clk.clock))
				return -EFAULT;

			clk.clock = spi_clk;

			if (copy_to_user((void __user *)arg, (const void *)&clk, sizeof(struct spi_clock)))
				return -EFAULT;
		}
		break;

	default:
		printk(KERN_ERR "%s: fail, unknown command 0x%x, \n",
			SPI_MISC_DEV_NAME, cmd);
		return -EINVAL;
	}

	DBGOUT("IoCtl (cmd:0x%x) \n\n", cmd);
	return 0;
}

struct file_operations nx_spi_ops = {
	.owner 	= THIS_MODULE,
	.open 	= nx_spi_ops_open,
	.release= nx_spi_ops_release,
	.ioctl 	= nx_spi_ops_ioctl,
};

/*--------------------------------------------------------------------------------
 * SPI platform_driver functions
 ---------------------------------------------------------------------------------*/
static int __init nx_spi_drv_probe(struct platform_device *pdev)
{
	int ch  = 0;
	int ret = register_chrdev(
					SPI_MISC_DEV_MAJOR, "SPI(Serial Peripheral Interface)", &nx_spi_ops
					);

	if (0 > ret) {
		printk(KERN_ERR "fail, register device (%s, major:%d)\n",
			SPI_MISC_DEV_NAME, SPI_MISC_DEV_MAJOR);
		return ret;
	}

	NX_SSPSPI_Initialize();
	NX_SSPSPI_SetBaseAddress(ch, (U32)IO_ADDRESS(NX_SSPSPI_GetPhysicalAddress(ch)));
 	NX_SSPSPI_OpenModule(ch);

	printk(KERN_INFO "%s: register major:%d\n", pdev->name, SPI_MISC_DEV_MAJOR);
	return ret;
}

static int nx_spi_drv_remove(struct platform_device *pdev)
{
	DBGOUT("%s\n", __func__);
	unregister_chrdev(SPI_MISC_DEV_MAJOR, "SPI(Serial Peripheral Interface)");
	return 0;
}

static int nx_spi_drv_suspend(struct platform_device *dev, pm_message_t state)
{
	PM_DBGOUT("%s\n", __func__);
	hw_init = 0;
	return 0;
}

static int nx_spi_drv_resume(struct platform_device *dev)
{
	PM_DBGOUT("%s\n", __func__);
	return 0;
}

static struct platform_driver spi_plat_misc_drv = {
	.probe		= nx_spi_drv_probe,
	.remove		= nx_spi_drv_remove,
	.suspend	= nx_spi_drv_suspend,
	.resume		= nx_spi_drv_resume,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= SPI_MISC_DEV_NAME,
	},
};

static int __init spi_plat_drv_init(void)
{
	DBGOUT("%s\n", __func__);
	return platform_driver_register(&spi_plat_misc_drv);
}

static void __exit spi_plat_drv_exit(void)
{
	DBGOUT("%s\n", __func__);
	platform_driver_unregister(&spi_plat_misc_drv);
}

module_init(spi_plat_drv_init);
module_exit(spi_plat_drv_exit);

MODULE_AUTHOR("Hans <nexell.co.kr>");
MODULE_DESCRIPTION("SPI(Serial Peripheral Interface) driver for the Nexell");
MODULE_LICENSE("GPL");

