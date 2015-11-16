#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/file.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/ioctl.h>

#include <mach/platform.h>
#include <mach/devices.h>
#include <mach/soc.h>
#include <mach/gpiomon.h>

/* debugging macro */
#define GPIOMON_DBG_HEADER "[GPIOMON]"

//#define GPIOMON_DEBUG
#ifdef GPIOMON_DEBUG
#define GPIOMON_DBG(args...) printk(GPIOMON_DBG_HEADER ":" args)
#else
#define GPIOMON_DBG(args...)
#endif

#define GPIOMON_DEV_NAME "gpiomon"


// static variables
static int s_open_count = 0;
static int s_current_mon_num = -1;
static int s_current_mon_val = 0;
static wait_queue_head_t read_wait;
static int irq_num = -1;

static int key_val[32] = {0};
static int key_num = 0;

static irqreturn_t gpio_irq_handler(int irq, void *dev_id)
{
	int read_pio = irq - IRQ_GPIO_START;
	int i;
	//printk("%s, irq = %d, id = %d\n",__func__,irq, read_pio);
	int read_val,mask;

	read_val = soc_gpio_get_in_value(read_pio);
	
	if (read_val == 0) {
		soc_gpio_set_int_mode(read_pio, 1); // High level detect
	} else {
		soc_gpio_set_int_mode(read_pio, 0); // Low level detect
	}
	

	for (i=0;i<key_num;i++)
	{
		if(key_val[i] == read_pio){
		break;
		}
	}
	
	mask = 1<<i;

	if(!read_val){
		mask = ~mask;
		s_current_mon_val = s_current_mon_val & mask;	
	}
	else{
			s_current_mon_val = s_current_mon_val | mask;	
	}
	
	

	
	wake_up(&read_wait);

	return IRQ_HANDLED;
}

static void release_monitor_num(void)
{
	if (irq_num >= 0) {
		free_irq(irq_num, NULL);
		irq_num = -1;
		soc_gpio_set_int_enable(s_current_mon_num, 0);
	}

	s_current_mon_num = -1;
}

static int assign_monitor_num(int num)
{
	int ret;
	int read;

		
	key_val[key_num] = num;
	
	

	soc_gpio_set_io_func(num, NX_GPIO_PADFUNC_GPIO);
	soc_gpio_set_io_dir(num, 0);
	soc_gpio_clr_int_pend(num);
	read = soc_gpio_get_in_value(num);
	if (read == 0) {
		soc_gpio_set_int_mode(num, 1); // High level detect
	} else {
		soc_gpio_set_int_mode(num, 2); // Low level detect
	}
	s_current_mon_val = s_current_mon_val + (read << key_num);

	irq_num = PB_PIO_IRQ(num);

	ret = request_irq(irq_num, gpio_irq_handler, IRQF_DISABLED, GPIOMON_DEV_NAME, NULL);
	if (ret) {
		printk("Error %s: Failed to request_irq(%d)\n", __func__, irq_num);
		return ret;
	}

	s_current_mon_num = num;
	key_num++;
	
	return 0;
}

static int gpiomon_open(struct inode *inode, struct file *filp)
{
	GPIOMON_DBG("%s entered\n", __func__);
	if (s_open_count > 0) {
		printk("Error %s: already opened\n", __func__);
		return -1;
	}
	init_waitqueue_head(&read_wait);
	s_open_count = 1;
	return 0;
}

static int gpiomon_close(struct inode *inode, struct file *filp)
{
	int i;
	GPIOMON_DBG("%s entered\n", __func__);
	if (s_open_count > 0) {
		s_open_count = 0;
	}

	for (i=0;i<key_num;i++)
	{
		free_irq(PB_PIO_IRQ(key_val[i]), NULL);	
		key_val[i] = NULL;
	}
	key_num=0;
	s_current_mon_val=0;
	return 0;
}

static int gpiomon_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret;
	switch (cmd) {
	case IOCTL_SET_MONITOR_NUM:
		{
			int num;
			printk("%s %d \n",__func__,num);
			if (copy_from_user((void *)&num, (void __user *)arg, sizeof(int))) {
				printk("Error %s: copy_from_user()\n", __func__);
				return -EFAULT;
			}
			if (s_current_mon_num != num) {
				//release_monitor_num();
				ret = assign_monitor_num(num);
				if (ret < 0) {
					printk("Error %s: Failed to assign_monitor_num: %d\n", __func__, num);
					return -EINVAL;
				}
			}
		}
		break;
	case IOCTL_GET_MONITOR_VAL:
		{
			int val;
			int i;
			//if (copy_from_user((void *)&val, (void __user *)arg, sizeof(int))) {
			if (copy_from_user((void *)&val, (void __user *)arg, sizeof(int))) {
				printk("Error %s: copy_from_user()\n", __func__);
				return -EFAULT;
			}
			if (val == s_current_mon_val) {
				
					interruptible_sleep_on(&read_wait);
				}


			if (copy_to_user((void __user *)arg, &s_current_mon_val, sizeof(int))) {
				printk("Error %s: copy_to_user()\n", __func__);
				return -EFAULT;
			}
		}
		break;
	default:
		printk("Error: invalid command(0x%x)\n", cmd);
		return -EINVAL;
		break;
	}

	return 0;
}

static ssize_t gpiomon_read(struct file *filp, char *buf, size_t count, loff_t *pos)
{
	if (s_current_mon_num < 0) {
		printk("Error: you must set monitoring gpio num!!!: IOCTL_SET_MONITOR_NUM\n");
		return -1;
	}

	if (copy_to_user(buf, &s_current_mon_val, sizeof(int))) {
		printk("Error: copy_to_user\n");
		return -EFAULT;
	}

	return sizeof(int);
}

static struct file_operations gpiomon_fops = {
	.owner      = THIS_MODULE,
	.open       = gpiomon_open,
	.release    = gpiomon_close,
	.ioctl      = gpiomon_ioctl,
	.read       = gpiomon_read,
};

static struct miscdevice gpiomon_misc_device = {
	.minor      = MISC_DYNAMIC_MINOR,
	.name       = GPIOMON_DEV_NAME,
	.fops       = &gpiomon_fops,
};

static int __init gpiomon_init(void)
{
	int ret;
	GPIOMON_DBG("%s\n", __func__);

	ret = misc_register(&gpiomon_misc_device);
	if (ret) {
		printk("Error %s: Failed to misc_register\n", __func__);
		return ret;
	}

	return ret;
}

static void __exit gpiomon_exit(void)
{
	GPIOMON_DBG("%s\n", __func__);
}

module_init(gpiomon_init);
module_exit(gpiomon_exit);

MODULE_DESCRIPTION("Nexell Generic GPIO Monitor driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nexell swpark@nexell.co.kr");
