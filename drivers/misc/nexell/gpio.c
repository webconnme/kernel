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
#include <mach/gpio.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36) 
#include <linux/mutex.h>
#endif

#if (0)
#define DBGOUT(msg...)		{ printk(KERN_INFO "gpio: " msg); }
#else
#define DBGOUT(msg...)		do {} while (0)
#endif

//--------------------------------------------------------------------------
static int nx_gpio_ops_open(struct inode *inode, struct file *flip)
{
	DBGOUT("%s\n", __func__);
	return 0;
}

static int nx_gpio_ops_release(struct inode *inode, struct file *flip)
{
	DBGOUT("%s\n", __func__);
	return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36) 
static DEFINE_MUTEX(extio_mutex);
static long nx_gpio_ops_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
#else
static int nx_gpio_ops_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
#endif
{
	struct gpio_info gpio;

	DBGOUT("%s(cmd:0x%x)\n", __func__, cmd);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36) 
	mutex_lock(&extio_mutex);
#endif
	if (copy_from_user(&gpio, (const void __user *)arg, sizeof(struct gpio_info)))
		return -EFAULT;

	switch (cmd)	{
	case IOCTL_GPIO_SET_OUTPUT:
		soc_gpio_set_out_value(gpio.io, gpio.value);
		break;

	case IOCTL_GPIO_SET_OUTPUT_EN:
		soc_gpio_set_io_dir(gpio.io, gpio.value);
		break;

	case IOCTL_GPIO_SET_INT_EN:
		soc_gpio_set_int_enable(gpio.io, gpio.value);
		break;

	case IOCTL_GPIO_SET_PULLUP_EN:
		soc_gpio_set_io_pullup(gpio.io, gpio.value);
		break;

	case IOCTL_GPIO_SET_DET_MODE:
		soc_gpio_set_int_mode(gpio.io, gpio.mode);
		break;

	case IOCTL_GPIO_SET_ALV_DET_MODE:
		soc_alv_set_det_mode(gpio.io, gpio.mode, gpio.value);
		break;

	case IOCTL_GPIO_SET_ALV_DET_EN:
		soc_alv_set_det_enable(gpio.io, gpio.value);
		break;

	case IOCTL_GPIO_SET_ALT_MODE:
		soc_gpio_set_io_func(gpio.io, gpio.mode);
		break;

	case IOCTL_GPIO_CLEAR_INTPENDING:
		soc_gpio_clr_int_pend(gpio.io);
		break;

	case IOCTL_GPIO_GET_OUTPUT:
		gpio.value = soc_gpio_get_out_value(gpio.io);
		break;

	case IOCTL_GPIO_GET_OUTPUT_EN:
		gpio.value = soc_gpio_get_io_dir(gpio.io);
		break;

	case IOCTL_GPIO_GET_INT_EN:
		gpio.value = soc_gpio_get_int_enable(gpio.io);
		break;

	case IOCTL_GPIO_GET_PULLUP_EN:
		gpio.value = soc_gpio_get_io_pullup(gpio.io);
		break;

	case IOCTL_GPIO_GET_DET_MODE:
		gpio.mode = (u_int)soc_gpio_get_int_mode(gpio.io);
		break;

	case IOCTL_GPIO_GET_ALT_MODE:
		gpio.mode = (u_int)soc_gpio_get_io_func(gpio.io);
		break;

	case IOCTL_GPIO_GET_INPUT_LV:
		gpio.value = soc_gpio_get_in_value(gpio.io);
		break;

	case IOCTL_GPIO_GET_INTPENDING:
		gpio.value = soc_gpio_get_int_pend(gpio.io);
		break;

	default:
		printk(KERN_ERR "%s: fail, unknown command 0x%x, \n",
			GPIO_DEV_NAME, cmd);
		return -EFAULT;
	}

	/* copy to user */
	switch (cmd)	{
	case IOCTL_GPIO_GET_OUTPUT:
	case IOCTL_GPIO_GET_OUTPUT_EN:
	case IOCTL_GPIO_GET_INT_EN:
	case IOCTL_GPIO_GET_PULLUP_EN:
	case IOCTL_GPIO_GET_DET_MODE:
	case IOCTL_GPIO_GET_ALT_MODE:
	case IOCTL_GPIO_GET_INPUT_LV:
	case IOCTL_GPIO_GET_INTPENDING:
		if (copy_to_user((void __user *)arg, (const void *)&gpio, sizeof(struct gpio_info)))
			return -EFAULT;
		break;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36) 
	mutex_unlock(&extio_mutex);
#endif
	DBGOUT("IoCtl (cmd:0x%x) \n\n", cmd);
	return 0;
}

struct file_operations nx_gpio_ops = {
	.owner			= THIS_MODULE,
	.open			= nx_gpio_ops_open,
	.release		= nx_gpio_ops_release,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36) 
	.unlocked_ioctl = nx_gpio_ops_ioctl,
#else
	.ioctl			= nx_gpio_ops_ioctl,
#endif
};

/*--------------------------------------------------------------------------------
 * GPIO platform_driver functions
 ---------------------------------------------------------------------------------*/
static int __init nx_gpio_drv_probe(struct platform_device *pdev)
{
	int ret = register_chrdev(
					GPIO_DEV_MAJOR, "GPIO(General Purpose IO)", &nx_gpio_ops
					);

	if (0 > ret) {
		printk(KERN_ERR "fail, register device (%s, major:%d)\n",
			GPIO_DEV_NAME, GPIO_DEV_MAJOR);
		return ret;
	}

	printk(KERN_INFO "%s: register major:%d\n", pdev->name, GPIO_DEV_MAJOR);
	return ret;
}

static int nx_gpio_drv_remove(struct platform_device *pdev)
{
	DBGOUT("%s\n", __func__);
	return 0;
}

static int nx_gpio_drv_suspend(struct platform_device *dev, pm_message_t state)
{
	PM_DBGOUT("%s\n", __func__);
	return 0;
}


static int nx_gpio_drv_resume(struct platform_device *dev)
{
	PM_DBGOUT("%s\n", __func__);
	return 0;
}

static struct platform_driver gpio_plat_driver = {
	.probe		= nx_gpio_drv_probe,
	.remove		= nx_gpio_drv_remove,
	.suspend	= nx_gpio_drv_suspend,
	.resume		= nx_gpio_drv_resume,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= GPIO_DEV_NAME,
	},
};

static int __init gpio_plat_driver_init(void)
{
	DBGOUT("%s\n", __func__);
	return platform_driver_register(&gpio_plat_driver);
}

static void __exit gpio_plat_driver_exit(void)
{
	DBGOUT("%s\n", __func__);
	platform_driver_unregister(&gpio_plat_driver);
}

module_init(gpio_plat_driver_init);
module_exit(gpio_plat_driver_exit);

MODULE_AUTHOR("Hans <nexell.co.kr>");
MODULE_DESCRIPTION("GPIO(General Purpose IO) driver for the Nexell");
MODULE_LICENSE("GPL");

