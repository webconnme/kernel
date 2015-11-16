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
#include <linux/delay.h>
#include <asm/uaccess.h>

/* nexell soc headers */
#include <mach/platform.h>
#include <mach/devices.h>
#include <mach/soc.h>
#include <mach/dib_dxb.h>

#if (0)
#define DBGOUT(msg...)		{ printk(KERN_INFO "dib_dxb: " msg); }
#else
#define DBGOUT(msg...)		do {} while (0)
#endif

//--------------------------------------------------------------------------
static int dib_dxb_ops_open(struct inode *inode, struct file *flip)
{
	uint32_t PWRDWN = 32*1+30;		/* GPIO_B30	*/
	uint32_t RESETN = 32*2+17;		/* GPIO_C17	*/
	uint32_t SCL	= 32*1+6;		/* GPIO_B7	*/
	uint32_t SDA	= 32*1+7;		/* GPIO_B6	*/

	DBGOUT("%s\n", __func__);

	/* gpio pad number, 32*n + bit */
	/* (n= GPIO_A:0, GPIO_B:1, GPIO_C:2, GPIO_D:3, GPIO_E:4, bit= 0 ~ 32) */
	soc_gpio_set_io_func(PWRDWN, 0);	/* GPIO mode : Power Down (GPIO_B30) */
	soc_gpio_set_io_func(RESETN, 0);	/* GPIO mode : nRESET (GPIO_C17)     */
	soc_gpio_set_io_func(SDA, 0);		/* GPIO mode : MCU_SDA1 (GPIO_B7)    */
	soc_gpio_set_io_func(SCL, 0);		/* GPIO mode : MCU_SCL1 (GPIO_B6)    */

	soc_gpio_set_io_dir(PWRDWN, CTRUE);	/* Output mode */
	soc_gpio_set_io_dir(RESETN, CTRUE);	/* Output mode */
	soc_gpio_set_io_dir(SDA, CTRUE);	/* Output mode */
	soc_gpio_set_io_dir(SCL, CTRUE);	/* Output mode */

	soc_gpio_set_io_pullup(PWRDWN, CTRUE);	/* Pull-up Enable */
	soc_gpio_set_io_pullup(SDA, CTRUE);	/* Pull-up Enable */
	soc_gpio_set_io_pullup(SCL, CTRUE);	/* Pull-up Enable */

	soc_gpio_set_out_value(PWRDWN, CFALSE);	/* Value = 1 */
	soc_gpio_set_out_value(PWRDWN, CTRUE);	/* Value = 1 */

	return 0;
}

static int dib_dxb_ops_release(struct inode *inode, struct file *flip)
{
	DBGOUT("%s\n", __func__);
	return 0;
}

static int dib_dxb_ops_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct dib_dxb_info dxb_ctrl;
	uint32_t PWRDWN = 32*1+30;		/* GPIO_B30	*/
	uint32_t RESETN = 32*2+17;		/* GPIO_C17	*/
	uint32_t SCL	= 32*1+6;		/* GPIO_B7	*/
	uint32_t SDA	= 32*1+7;		/* GPIO_B6	*/

	DBGOUT("%s(cmd:0x%x)\n", __func__, cmd);

	if (copy_from_user(&dxb_ctrl, (const void __user *)arg, sizeof(struct dib_dxb_info)))
		return -EFAULT;

	switch (cmd)	{
		case IOCTL_DXB_POWER_HIGH:
			soc_gpio_set_out_value(PWRDWN, CTRUE);
			break;
		case IOCTL_DXB_POWER_LOW:
			soc_gpio_set_out_value(PWRDWN, CFALSE);
			break;
		case IOCTL_DXB_RESET_HIGH:
			soc_gpio_set_out_value(RESETN, CTRUE);
			break;
		case IOCTL_DXB_RESET_LOW:
			soc_gpio_set_out_value(RESETN, CFALSE);
			break;
		case IOCTL_DXB_SCL_OUT:
			soc_gpio_set_io_dir(SCL, CTRUE);			/* Output mode */
			soc_gpio_set_out_value(SCL, dxb_ctrl.value);
			break;
		case IOCTL_DXB_SDA_OUT:
			soc_gpio_set_io_dir(SDA, CTRUE);			/* Output mode */
			soc_gpio_set_out_value(SDA, dxb_ctrl.value);
			break;
		case IOCTL_DXB_SDA_IN:
			soc_gpio_set_io_dir(SDA, CFALSE);			/* Input mode */
			dxb_ctrl.value = soc_gpio_get_in_value(SDA);
			break;
		default:
			printk(KERN_ERR "IOCTL_DXB_POWER_HIGH : 0x%x\n", IOCTL_DXB_POWER_HIGH);
			printk(KERN_ERR "IOCTL_DXB_POWER_LOW  : 0x%x\n", IOCTL_DXB_POWER_LOW);
			printk(KERN_ERR "IOCTL_DXB_RESET_HIGH : 0x%x\n", IOCTL_DXB_RESET_HIGH);
			printk(KERN_ERR "IOCTL_DXB_RESET_LOW  : 0x%x\n", IOCTL_DXB_RESET_LOW);
			printk(KERN_ERR "IOCTL_DXB_SCL_OUT    : 0x%x\n", IOCTL_DXB_SCL_OUT);
			printk(KERN_ERR "IOCTL_DXB_SDA_OUT    : 0x%x\n", IOCTL_DXB_SDA_OUT);
			printk(KERN_ERR "IOCTL_DXB_SDA_IN     : 0x%x\n", IOCTL_DXB_SDA_IN);
			printk(KERN_ERR "%s: fail, unknown command 0x%x, \n", DXB_DEV_NAME, cmd);
			break;
	}

	/* copy to user */
	switch (cmd)	{
		case IOCTL_DXB_SDA_IN:
			if (copy_to_user((void __user *)arg, (const void *)&dxb_ctrl, sizeof(struct dib_dxb_info)))
				return -EFAULT;
			break;
	}
	DBGOUT("IoCtl (cmd:0x%x) \n\n", cmd);
	return 0;
}

struct file_operations dib_dxb_ops = {
	.owner 	= THIS_MODULE,
	.open 	= dib_dxb_ops_open,
	.release= dib_dxb_ops_release,
	.ioctl 	= dib_dxb_ops_ioctl,
};

/*--------------------------------------------------------------------------------
 * GPIO platform_driver functions
 ---------------------------------------------------------------------------------*/
static int __init dib_dxb_drv_probe(struct platform_device *pdev)
{
	int ret = register_chrdev(DXB_DEV_MAJOR, "DiBcom DXB driver", &dib_dxb_ops);

	if (0 > ret) {
		printk(KERN_ERR "fail, register device (%s, major:%d)\n", DXB_DEV_NAME, DXB_DEV_MAJOR);
		return ret;
	}

	printk(KERN_INFO "%s: register major:%d\n", pdev->name, DXB_DEV_MAJOR);
	return ret;
}

static int dib_dxb_drv_remove(struct platform_device *pdev)
{
	DBGOUT("%s\n", __func__);
	return 0;
}

static int dib_dxb_drv_suspend(struct platform_device *dev, pm_message_t state)
{
	PM_DBGOUT("%s\n", __func__);
	return 0;
}


static int dib_dxb_drv_resume(struct platform_device *dev)
{
	PM_DBGOUT("%s\n", __func__);
	return 0;
}

static struct platform_driver dib_dxb_plat_driver = {
	.probe		= dib_dxb_drv_probe,
	.remove		= dib_dxb_drv_remove,
	.suspend	= dib_dxb_drv_suspend,
	.resume		= dib_dxb_drv_resume,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= DXB_DEV_NAME,
	},
};

static int __init dib_dxb_plat_driver_init(void)
{
	DBGOUT("%s\n", __func__);
	return platform_driver_register(&dib_dxb_plat_driver);
}

static void __exit dib_dxb_plat_driver_exit(void)
{
	DBGOUT("%s\n", __func__);
	platform_driver_unregister(&dib_dxb_plat_driver);
}

module_init(dib_dxb_plat_driver_init);
module_exit(dib_dxb_plat_driver_exit);

MODULE_AUTHOR("Michael RYU <Michael.Ryu@parrot.com>");
MODULE_DESCRIPTION("GPIO(General Purpose IO) I2C driver for DiBcom");
MODULE_LICENSE("GPL");

