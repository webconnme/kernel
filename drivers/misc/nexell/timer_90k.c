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
#include <linux/interrupt.h>
#include <asm/uaccess.h>

/* nexell soc headers */
#include <board_devs.h>
#include <mach/platform.h>

#if (0)
#define DBGOUT(msg...)		{ printk(KERN_INFO "timer_90k: " msg); }
#else
#define DBGOUT(msg...)		do {} while (0)
#endif

/*------------------------------------------------------------------------------
 * 	data
 */
struct timer_90k_tick {
	unsigned long 	sec;
	unsigned long 	tick;	/* 0 ~ 90khz */
};

struct timer_sys_tick {
	unsigned long 	sec;
	unsigned long 	tick;	/* 0 ~ 90khz */
	unsigned long 	irq_tick;	/* 0 ~ 90khz */
	spinlock_t		lock;
};

/*------------------------------------------------------------------------------
 * 	ioctl
 */
#include <mach/ioc_magic.h>

enum {
	IOCTL_TIMER_90K_GET_TICK	= _IO(IOC_NX_MAGIC, 0),
};

#define	TICK_HZ			90000

static struct timer_sys_tick tick_cnt = { 0, };
/*------------------------------------------------------------------------------
 */
static irqreturn_t	do_irq_handler(int irq, void *desc)
{
	tick_cnt.sec++;
	tick_cnt.irq_tick = NX_TIMER_GetTimerCounter(CFG_TIMER_90K_TICK_CH);
	NX_TIMER_ClearInterruptPendingAll(CFG_TIMER_90K_TICK_CH);

	return IRQ_HANDLED;
}

static void get_tick(struct timer_90k_tick *tick)
{
	unsigned int count, value;
	int irq;
	unsigned long flags;

	DBGOUT("%s\n", __func__);

	irq = NX_TIMER_GetInterruptNumber(CFG_TIMER_90K_TICK_CH);

	/* lock */
	spin_lock_irqsave(&tick_cnt.lock, flags);

	value = NX_TIMER_GetTimerCounter(CFG_TIMER_90K_TICK_CH);
	count = value;

	/* 	count = count * 0.09; */
	count = count * ((TICK_HZ*1000)/CFG_TIMER_90K_TICK_CLKFREQ);
	count = count/1000;

	tick->sec  = tick_cnt.sec;
	tick->tick = count;

	if ( tick_cnt.irq_tick != value &&
		(CFG_TIMER_90K_TICK_CLKFREQ == value || 0 == value) ) {
		tick->sec += 1;
		tick->tick = 0;
	}

	/* unlock */
	spin_unlock_irqrestore(&tick_cnt.lock, flags);
}

static int timer_90k_ops_open(struct inode *inode, struct file *flip)
{
	DBGOUT("%s\n", __func__);
	return 0;
}

static int timer_90k_ops_release(struct inode *inode, struct file *flip)
{
	DBGOUT("%s\n", __func__);
	return 0;
}

static int timer_90k_ops_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	DBGOUT("%s(cmd:0x%x)\n", __func__, cmd);

	switch (cmd)	{
	case IOCTL_TIMER_90K_GET_TICK:
		{
			struct timer_90k_tick tick;
			if (copy_from_user(&tick, (const void __user *)arg, sizeof(struct timer_90k_tick)))
				return -EFAULT;

			get_tick(&tick);

			if (copy_to_user((void __user *)arg, (const void *)&tick, sizeof(struct timer_90k_tick)))
				return -EFAULT;
		}
		break;

	default:
		printk(KERN_ERR "%s: fail, unknown command 0x%x, \n",
			TIMER_90K_DEV_NAME, cmd);
		return -EINVAL;
	}

	DBGOUT("IoCtl (cmd:0x%x) \n\n", cmd);
	return 0;
}

struct file_operations timer_90k_ops = {
	.owner 	= THIS_MODULE,
	.open 	= timer_90k_ops_open,
	.release= timer_90k_ops_release,
	.ioctl 	= timer_90k_ops_ioctl,
};

/*--------------------------------------------------------------------------------
 * platform_driver functions
 ---------------------------------------------------------------------------------*/
static int __init timer_90k_drv_probe(struct platform_device *pdev)
{
	int irq = -1, ret;

	/* timer hw */
	NX_TIMER_Initialize();
	NX_TIMER_SetBaseAddress(CFG_TIMER_90K_TICK_CH,
				(U32)IO_ADDRESS(NX_TIMER_GetPhysicalAddress(CFG_TIMER_90K_TICK_CH)));
	NX_TIMER_OpenModule(CFG_TIMER_90K_TICK_CH);

	NX_TIMER_SetClockDivisorEnable(CFG_TIMER_90K_TICK_CH, CFALSE);
	NX_TIMER_SetClockSource (CFG_TIMER_90K_TICK_CH, 0, CFG_TIMER_90K_TICK_CLKSRC);
	NX_TIMER_SetClockDivisor(CFG_TIMER_90K_TICK_CH, 0, CFG_TIMER_90K_TICK_CLKDIV);
	NX_TIMER_SetClockPClkMode(CFG_TIMER_90K_TICK_CH, NX_PCLKMODE_ALWAYS);

	NX_TIMER_SetClockDivisorEnable(CFG_TIMER_90K_TICK_CH, CTRUE);
	NX_TIMER_Stop(CFG_TIMER_90K_TICK_CH);

	NX_TIMER_SetWatchDogEnable(CFG_TIMER_90K_TICK_CH, CFALSE);
	NX_TIMER_SetInterruptEnableAll(CFG_TIMER_90K_TICK_CH, CFALSE);
	NX_TIMER_SetTClkDivider(CFG_TIMER_90K_TICK_CH, NX_TIMER_CLOCK_TCLK);
	NX_TIMER_ClearInterruptPendingAll(CFG_TIMER_90K_TICK_CH);

	NX_TIMER_SetTimerCounter(CFG_TIMER_90K_TICK_CH, 0);
	NX_TIMER_SetMatchCounter(CFG_TIMER_90K_TICK_CH, CFG_TIMER_90K_TICK_CLKFREQ);

	irq = NX_TIMER_GetInterruptNumber(CFG_TIMER_90K_TICK_CH);

	/* register driver */
	ret = register_chrdev(TIMER_90K_DEV_MAJOR, "timer(90khz)", &timer_90k_ops);
	if (0 > ret) {
		printk(KERN_ERR "fail, register device (%s, major:%d)\n",
			TIMER_90K_DEV_NAME, TIMER_90K_DEV_MAJOR);
		return ret;
	}

	/* register interrupt */
	ret = request_irq(irq, &do_irq_handler, IRQF_DISABLED, TIMER_90K_DEV_NAME, NULL);
	if (ret) {
		printk(KERN_ERR "Error, request_irq(irq:%d)\n", irq);
		unregister_chrdev(TIMER_90K_DEV_MAJOR, TIMER_90K_DEV_NAME);
		return ret;
	}
	NX_TIMER_SetInterruptEnableAll(CFG_TIMER_90K_TICK_CH, CTRUE);
	NX_TIMER_Run(CFG_TIMER_90K_TICK_CH);

	spin_lock_init(&tick_cnt.lock);

	printk(KERN_INFO "%s: register major:%d, irq:%d\n",
		pdev->name, TIMER_90K_DEV_MAJOR, irq);
	return 0;
}

static int timer_90k_drv_remove(struct platform_device *pdev)
{
	int irq;
	DBGOUT("%s\n", __func__);

	irq = NX_TIMER_GetInterruptNumber(CFG_TIMER_90K_TICK_CH);

	NX_TIMER_SetInterruptEnableAll(CFG_TIMER_90K_TICK_CH, CFALSE);
	NX_TIMER_Stop(CFG_TIMER_90K_TICK_CH);
	NX_TIMER_SetClockDivisorEnable(CFG_TIMER_90K_TICK_CH, CFALSE);					// Disable Timer Clock

	free_irq(irq, NULL);
	unregister_chrdev(TIMER_90K_DEV_MAJOR, "timer(90khz)");
	return 0;
}

static unsigned int saved_count;
static int timer_90k_drv_suspend(struct platform_device *dev, pm_message_t state)
{
	PM_DBGOUT("+%s\n", __func__);

	saved_count = NX_TIMER_GetTimerCounter(CFG_TIMER_90K_TICK_CH);

	PM_DBGOUT("-%s\n", __func__);
	return 0;
}

static int timer_90k_drv_resume(struct platform_device *dev)
{
	u_long flags;

	PM_DBGOUT("+%s\n", __func__);

	if (NX_TIMER_GetClockDivisorEnable(CFG_TIMER_90K_TICK_CH))
		return 0;

	local_irq_save(flags);

	NX_TIMER_SetClockDivisorEnable(CFG_TIMER_90K_TICK_CH, CFALSE);
	NX_TIMER_SetClockSource (CFG_TIMER_90K_TICK_CH, 0, CFG_TIMER_90K_TICK_CLKSRC);
	NX_TIMER_SetClockDivisor(CFG_TIMER_90K_TICK_CH, 0, CFG_TIMER_90K_TICK_CLKDIV);
	NX_TIMER_SetClockPClkMode(CFG_TIMER_90K_TICK_CH, NX_PCLKMODE_ALWAYS);

	NX_TIMER_SetClockDivisorEnable(CFG_TIMER_90K_TICK_CH, CTRUE);
	NX_TIMER_Stop(CFG_TIMER_90K_TICK_CH);

	NX_TIMER_SetWatchDogEnable(CFG_TIMER_90K_TICK_CH, CFALSE);
	NX_TIMER_SetInterruptEnableAll(CFG_TIMER_90K_TICK_CH, CFALSE);
	NX_TIMER_SetTClkDivider(CFG_TIMER_90K_TICK_CH, NX_TIMER_CLOCK_TCLK);
	NX_TIMER_ClearInterruptPendingAll(CFG_TIMER_90K_TICK_CH);

	NX_TIMER_SetTimerCounter(CFG_TIMER_90K_TICK_CH, saved_count);
	NX_TIMER_SetMatchCounter(CFG_TIMER_90K_TICK_CH, CFG_TIMER_90K_TICK_CLKFREQ);


	NX_TIMER_SetInterruptEnableAll(CFG_TIMER_90K_TICK_CH, CTRUE);
	NX_TIMER_Run(CFG_TIMER_90K_TICK_CH);

	local_irq_restore(flags);

	PM_DBGOUT("-%s\n", __func__);
	return 0;
}

static struct platform_driver pwm_plat_driver = {
	.probe		= timer_90k_drv_probe,
	.remove		= timer_90k_drv_remove,
	.suspend	= timer_90k_drv_suspend,
	.resume		= timer_90k_drv_resume,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= TIMER_90K_DEV_NAME,
	},
};

static int __init timer_90k_drv_init(void)
{
	DBGOUT("%s\n", __func__);
	return platform_driver_register(&pwm_plat_driver);
}

static void __exit timer_90k_drv_exit(void)
{
	DBGOUT("%s\n", __func__);
	platform_driver_unregister(&pwm_plat_driver);
}

module_init(timer_90k_drv_init);
module_exit(timer_90k_drv_exit);

MODULE_AUTHOR("jhkim <jhkin@nexell.co.kr>");
MODULE_DESCRIPTION("90Khz timer driver for the Nexell");
MODULE_LICENSE("GPL");

