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
#include <mach/pwm.h>

#include <linux/mutex.h>

#if (0)
#define DBGOUT(msg...)		{ printk(KERN_INFO "pwm: " msg); }
#else
#define DBGOUT(msg...)		do {} while (0)
#endif

/*------------------------------------------------------------------------------
 * 	PWM ops
 */
static int nx_pwm_ops_open(struct inode *inode, struct file *flip)
{
	DBGOUT("%s\n", __func__);
	return 0;
}

static int nx_pwm_ops_release(struct inode *inode, struct file *flip)
{
	DBGOUT("%s\n", __func__);
	return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36) 
static DEFINE_MUTEX(extio_mutex);
static int nx_pwm_ops_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
#else
static int nx_pwm_ops_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
#endif
{
	DBGOUT("%s(cmd:0x%x)\n", __func__, cmd);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36) 
	mutex_lock(&extio_mutex);
#endif
	switch (cmd)	{
	case IOCTL_PWM_SET_STATUS:
		{
			struct pwm_info pwm;
			if (copy_from_user(&pwm, (const void __user *)arg, sizeof(struct pwm_info)))
				return -EFAULT;

			soc_pwm_set_frequency(pwm.ch, pwm.freq, pwm.duty);
		}
		break;

	case IOCTL_PWM_GET_STATUS:
		{
			struct pwm_info pwm;
			if (copy_from_user(&pwm, (const void __user *)arg, sizeof(struct pwm_info)))
				return -EFAULT;

			soc_pwm_get_frequency(pwm.ch, &pwm.freq, &pwm.duty);

			if (copy_to_user((void __user *)arg, (const void *)&pwm, sizeof(struct pwm_info)))
				return -EFAULT;
		}
		break;

	default:
		printk(KERN_ERR "%s: fail, unknown command 0x%x, \n",
			PWM_DEV_NAME, cmd);
		return -EINVAL;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36) 
	mutex_unlock(&extio_mutex);
#endif
	DBGOUT("IoCtl (cmd:0x%x) \n\n", cmd);
	return 0;
}

struct file_operations nx_pwm_ops = {
	.owner 	= THIS_MODULE,
	.open 	= nx_pwm_ops_open,
	.release= nx_pwm_ops_release,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36) 
	.ioctl 	= nx_pwm_ops_ioctl,
#else
	.unlocked_ioctl 	= nx_pwm_ops_ioctl,
#endif
};

/*--------------------------------------------------------------------------------
 * PWM platform_driver functions
 ---------------------------------------------------------------------------------*/
static int __init nx_pwm_drv_probe(struct platform_device *pdev)
{
	int ret = register_chrdev(
					PWM_DEV_MAJOR, "PWM(Pulse Width Modulation)", &nx_pwm_ops
					);

	if (0 > ret) {
		printk(KERN_ERR "fail, register device (%s, major:%d)\n",
			PWM_DEV_NAME, PWM_DEV_MAJOR);
		return ret;
	}

	printk(KERN_INFO "%s: register major:%d\n", pdev->name, PWM_DEV_MAJOR);
	return 0;
}

static int nx_pwm_drv_remove(struct platform_device *pdev)
{
	DBGOUT("%s\n", __func__);
	unregister_chrdev(PWM_DEV_MAJOR, "PWM(Pulse Width Modulation)");
	return 0;
}

static int nx_pwm_drv_suspend(struct platform_device *dev, pm_message_t state)
{
	PM_DBGOUT("+%s\n", __func__);

	soc_pwm_set_suspend();

	PM_DBGOUT("-%s\n", __func__);
	return 0;
}

static int nx_pwm_drv_resume(struct platform_device *dev)
{
	PM_DBGOUT("+%s\n", __func__);

	soc_pwm_set_resume();

	PM_DBGOUT("-%s\n", __func__);
	return 0;
}

static struct platform_driver pwm_plat_driver = {
	.probe		= nx_pwm_drv_probe,
	.remove		= nx_pwm_drv_remove,
	.suspend	= nx_pwm_drv_suspend,
	.resume		= nx_pwm_drv_resume,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= PWM_DEV_NAME,
	},
};

static int __init nx_pwm_drv_init(void)
{
	DBGOUT("%s\n", __func__);
	return platform_driver_register(&pwm_plat_driver);
}

static void __exit nx_pwm_drv_exit(void)
{
	DBGOUT("%s\n", __func__);
	platform_driver_unregister(&pwm_plat_driver);
}

module_init(nx_pwm_drv_init);
module_exit(nx_pwm_drv_exit);

MODULE_AUTHOR("jhkim <jhkin@nexell.co.kr>");
MODULE_DESCRIPTION("PWM (Pulse Width Modulation) driver for the Nexell");
MODULE_LICENSE("GPL");

