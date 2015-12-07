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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <asm/uaccess.h>

/* nexell soc headers */
#include <mach/platform.h>
#include <mach/devices.h>
#include <mach/soc.h>
#include <mach/adc.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36) 
#include <linux/mutex.h>
#endif

#if (0)
#define DBGOUT(msg...)		{ printk(KERN_INFO "adc: " msg); }
#else
#define DBGOUT(msg...)		do {} while (0)
#endif

/*------------------------------------------------------------------------------
 * 	ADC ops
 */
static int nx_adc_ops_open(struct inode *inode, struct file *flip)
{
	DBGOUT("%s\n", __func__);
	soc_adc_attach();
	return 0;
}

static int nx_adc_ops_release(struct inode *inode, struct file *flip)
{
	DBGOUT("%s\n", __func__);
	soc_adc_detach();
	return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36) 
static DEFINE_MUTEX(extio_mutex);
static long nx_adc_ops_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
#else
static int nx_adc_ops_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
#endif
{
	DBGOUT("%s(cmd:0x%x)\n", __func__, cmd);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36) 
	mutex_lock(&extio_mutex);
#endif
	switch (cmd)	{
	case IOCTL_ADC_GET_LEVEL:
		{
			struct adc_info adc;
			if (copy_from_user(&adc, (const void __user *)arg, sizeof(struct adc_info)))
				return -EFAULT;

			if (adc.ch >= 6)
				return -EIO;

			adc.level = soc_adc_read(adc.ch, 100*1000);

			if (copy_to_user((void __user *)arg, (const void *)&adc, sizeof(struct adc_info)))
				return -EFAULT;
		}
		break;

	default:
		printk(KERN_ERR "%s: fail, unknown command 0x%x, \n",
			ADC_DEV_NAME, cmd);
		return -EINVAL;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36) 
	mutex_unlock(&extio_mutex);
#endif
	DBGOUT("IoCtl (cmd:0x%x) \n\n", cmd);
	return 0;
}

struct file_operations nx_adc_ops = {
	.owner			= THIS_MODULE,
	.open			= nx_adc_ops_open,
	.release		= nx_adc_ops_release,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36) 
	.unlocked_ioctl	= nx_adc_ops_ioctl,
#else
	.ioctl			= nx_adc_ops_ioctl,
#endif
};

/*--------------------------------------------------------------------------------
 * ADC platform_driver functions
 ---------------------------------------------------------------------------------*/
static int __init nx_adc_drv_probe(struct platform_device *pdev)
{
	int ret = register_chrdev(
					ADC_DEV_MAJOR, "ADC(Analog Digital Converter)", &nx_adc_ops
					);

	if (0 > ret) {
		printk(KERN_ERR "fail, register device (%s, major:%d)\n",
			ADC_DEV_NAME, ADC_DEV_MAJOR);
		return ret;
	}

	printk(KERN_INFO "%s: register major:%d\n", pdev->name, ADC_DEV_MAJOR);
	return 0;
}

static int nx_adc_drv_remove(struct platform_device *pdev)
{
	DBGOUT("%s\n", __func__);
	unregister_chrdev(ADC_DEV_MAJOR, "ADC(Analog Digital Converter)");
	return 0;
}

static int nx_adc_drv_suspend(struct platform_device *dev, pm_message_t state)
{
	PM_DBGOUT("%s\n", __func__);
	soc_adc_suspend();
	return 0;
}

static int nx_adc_drv_resume(struct platform_device *dev)
{
	PM_DBGOUT("%s\n", __func__);
	soc_adc_resume();
	return 0;
}

static struct platform_driver adc_plat_driver = {
	.probe		= nx_adc_drv_probe,
	.remove		= nx_adc_drv_remove,
	.suspend	= nx_adc_drv_suspend,
	.resume		= nx_adc_drv_resume,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= ADC_DEV_NAME,
	},
};

static int __init nx_adc_drv_init(void)
{
	DBGOUT("%s\n", __func__);
	return platform_driver_register(&adc_plat_driver);
}

static void __exit nx_adc_drv_exit(void)
{
	DBGOUT("%s\n", __func__);
	platform_driver_unregister(&adc_plat_driver);
}

module_init(nx_adc_drv_init);
module_exit(nx_adc_drv_exit);

MODULE_AUTHOR("jhkim <jhkin@nexell.co.kr>");
MODULE_DESCRIPTION("ADC (Analog Digital Converter) driver for the Nexell");
MODULE_LICENSE("GPL");

