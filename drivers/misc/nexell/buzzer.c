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
#include <mach/ioc_magic.h>

#if (0)
#define DBGOUT(msg...)		{ printk(KERN_INFO "buzzer: " msg); }
#else
#define DBGOUT(msg...)		do {} while (0)
#endif

/*------------------------------------------------------------------------------
* IOCTL
*/
enum {
	IOCTL_BUZZER_SET_FREQ = _IO(IOC_NX_MAGIC,1),
	IOCTL_BUZZER_STOP = _IO(IOC_NX_MAGIC,2),
};

/*------------------------------------------------------------------------------
 * 	PWM ops
 */

static int nx_buzzer_pwm_init(void)
{
		NX_PWM_SetBaseAddress((U32)IO_ADDRESS(NX_PWM_GetPhysicalAddress()));
		NX_PWM_OpenModule();

		NX_PWM_SetClockDivisorEnable(CFALSE);
		NX_PWM_SetClockSource(0,1);// 1: PLL1
		NX_PWM_SetClockDivisor(0,64);
		NX_PWM_SetClockPClkMode(0);
		NX_PWM_SetPreScale(1,128);
		

}
static int nx_buzzer_ops_open(struct inode *inode, struct file *flip)
{
	DBGOUT("%s\n", __func__);
	nx_buzzer_pwm_init();
	
	return 0;
}

static int nx_buzzer_ops_release(struct inode *inode, struct file *flip)
{
	DBGOUT("%s\n", __func__);
	return 0;
}

static int nx_buzzer_ops_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	int duty,period;
	DBGOUT("%s(cmd:0x%x) %d \n", __func__, cmd,arg);

	switch(cmd){
		case IOCTL_BUZZER_SET_FREQ :
			
			switch(arg){
				case 700:
				period = 26;
				duty = 13;
				break;
				case 666:
				period = 27;
				duty = 13;
				break;
				case 600:
				period = 30;
				duty = 15;
				break;
				case 500:
				period = 36;
				duty = 18;
				break;
				case 400:
				period = 45;
				duty = 22;
				break;
				case 333:
				period = 54;
				duty = 27;
				break;
				case 250:
				period = 72;
				duty = 36;
				break;
				default:
				printk("Not Support Frequency %d \n",arg );
				return -1;
				break;
			}
		NX_PWM_SetPeriod(1,period);
		NX_PWM_SetDutyCycle(1,duty);
		NX_PWM_SetClockDivisorEnable(CTRUE);
		break;

		case IOCTL_BUZZER_STOP :
		NX_PWM_SetClockDivisorEnable(CFALSE);
		break;
			
	}

	DBGOUT("IoCtl (cmd:0x%x) \n\n", cmd);
	return 0;
}

struct file_operations nx_buzzer_ops = {
	.owner 	= THIS_MODULE,
	.open 	= nx_buzzer_ops_open,
	.release= nx_buzzer_ops_release,
	.ioctl 	= nx_buzzer_ops_ioctl,
};

/*--------------------------------------------------------------------------------
 * PWM platform_driver functions
 ---------------------------------------------------------------------------------*/
static int __init nx_buzzer_drv_probe(struct platform_device *pdev)
{
	DBGOUT("%s\n", __func__);

	int ret = register_chrdev(
					BUZZER_DEV_MAJOR, "buzzer", &nx_buzzer_ops);

	if (0 > ret) {
		printk(KERN_ERR "fail, register device (%s, major:%d)\n",
			BUZZER_DEV_NAME, BUZZER_DEV_MAJOR);
		return ret;
	}

	printk(KERN_INFO "%s: register major:%d\n", pdev->name, BUZZER_DEV_MAJOR);
	return 0;
}

static int nx_buzzer_drv_remove(struct platform_device *pdev)
{
	DBGOUT("%s\n", __func__);
	unregister_chrdev(BUZZER_DEV_MAJOR, "buzzer");
	return 0;
}

static int nx_buzzer_drv_suspend(struct platform_device *dev, pm_message_t state)
{
	PM_DBGOUT("+%s\n", __func__);

	soc_pwm_set_suspend();

	PM_DBGOUT("-%s\n", __func__);
	return 0;
}

static int nx_buzzer_drv_resume(struct platform_device *dev)
{
	PM_DBGOUT("+%s\n", __func__);

	soc_pwm_set_resume();

	PM_DBGOUT("-%s\n", __func__);
	return 0;
}

static struct platform_driver buzzer_plat_driver = {
	.probe		= nx_buzzer_drv_probe,
	.remove		= nx_buzzer_drv_remove,
	.suspend	= nx_buzzer_drv_suspend,
	.resume		= nx_buzzer_drv_resume,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= BUZZER_DEV_NAME,
	},
};

static int __init nx_buzzer_drv_init(void)
{
	int ret;
	DBGOUT("%s\n", __func__);
	ret = platform_driver_register(&buzzer_plat_driver);
	printk("Return Value : %d ",ret);
	return ret;
	//return platform_driver_register(&buzzer_plat_driver);
}

static void __exit nx_buzzer_drv_exit(void)
{
	DBGOUT("%s\n", __func__);
	platform_driver_unregister(&buzzer_plat_driver);
}

module_init(nx_buzzer_drv_init);
module_exit(nx_buzzer_drv_exit);

MODULE_AUTHOR("ybpark <ybpark@nexell.co.kr>");
MODULE_DESCRIPTION("Buzzer driver for the Nexell");
MODULE_LICENSE("GPL");

