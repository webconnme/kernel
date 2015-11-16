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
#if defined(CONFIG_PM)

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/suspend.h>
#include <asm/uaccess.h>

/* nexell soc headers */
#include <mach/devices.h>
#include <mach/pmi.h>

#if (0)
#define DBGOUT(msg...)		{ printk(KERN_INFO "pmi: " msg); }
#else
#define DBGOUT(msg...)		do {} while (0)
#endif

extern int cpu_get_wakeup_source(void);

/*------------------------------------------------------------------------------
 * 	PMI ops
 */
static int nx_pmi_ops_open(struct inode *inode, struct file *flip)
{
	DBGOUT("%s\n", __func__);
	return 0;
}

static int nx_pmi_ops_release(struct inode *inode, struct file *flip)
{
	DBGOUT("%s\n", __func__);
	return 0;
}

static int nx_pmi_ops_ioctl( struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg )
{
	int ret = 0;
	DBGOUT("%s(cmd:0x%x)\n", __func__, cmd);

	switch (cmd)	{
	case IOCTL_PMI_SET_SUSPEND_MEM:
		pm_suspend(PM_SUSPEND_MEM);
		break;

	case IOCTL_PMI_GET_WAKEUP_SRC:
		{
			int src = cpu_get_wakeup_source();
			if (copy_to_user((void __user*)arg, (const void *)&src, sizeof(src)))
				ret = -EFAULT;
		}
		break;

	default:
		printk(KERN_ERR "%s: fail, unknown command 0x%x, \n",
			PMI_DEV_NAME, cmd);
		return -EINVAL;
	}

	DBGOUT("IoCtl (cmd:0x%x) \n\n", cmd);
	return ret;
}

struct file_operations nx_pmi_ops = {
	.owner 	= THIS_MODULE,
	.open 	= nx_pmi_ops_open,
	.release= nx_pmi_ops_release,
	.ioctl 	= nx_pmi_ops_ioctl,
};

static int __init nx_pmi_drv_init(void)
{
	int ret = register_chrdev(
					PMI_DEV_MAJOR, "PMI(Power manager interface)", &nx_pmi_ops
					);

	if (0 > ret) {
		printk(KERN_ERR "fail, register device (%s, major:%d)\n",
			PMI_DEV_NAME, PMI_DEV_MAJOR);
		return ret;
	}

	printk(KERN_INFO "%s: register major:%d\n", PMI_DEV_NAME, PMI_DEV_MAJOR);
	return 0;
}

static void __exit nx_pmi_drv_exit(void)
{
	DBGOUT("%s\n", __func__);
	unregister_chrdev(PMI_DEV_MAJOR, PMI_DEV_NAME);
}

module_init(nx_pmi_drv_init);
module_exit(nx_pmi_drv_exit);

MODULE_AUTHOR("jhkim <jhkin@nexell.co.kr>");
MODULE_DESCRIPTION("PMI(Power manager interface) driver for the Nexell");
MODULE_LICENSE("GPL");

#endif	/* CONFIG_PM */