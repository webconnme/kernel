#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/seq_file.h>
#include <linux/ctype.h>

#include <mach/platform.h>

static struct proc_dir_entry *gpio_proc_entry;

static s32 atoi(char *psz_buf);

static s32 atoi(char *psz_buf)
{
	char *pch = psz_buf;
	s32 base = 0;

	while (isspace(*pch))
		pch++;

	if (*pch == '-' || *pch == '+') {
		base = 10;
		pch++;
	} else if (*pch && tolower(pch[strlen(pch) - 1]) == 'h') {
		base = 16;
	}

	return simple_strtoul(pch, NULL, base);
}

static int GetGroup(char grp)
{
	int iGroup = 0;

	// Set GPIO Group
	switch(grp)
	{
		case 'a':
		case 'A':
			iGroup = 0;
			break;

		case 'b':
		case 'B':
			iGroup = 1;
			break;

		case 'c':
		case 'C':
			iGroup = 2;
			break;

		default:
			printk(" ... Wrong Command ....\n");
			break;
	}

	return iGroup;
}

static int GetOption(char opt)
{
	int iFunc = 0;

	/**
	 * S: SetOutputValue	= (h:High, l:Low) 
	 * D: SetDirection		= (o:Output, i:Input)
	 * F: SetPadFunction	= (0:GPIO, 1:ALT1, 2:ALT2, 3:ALT3)
	 **/
	switch(opt)
	{
		case 'h':
		case 'H':
		case 'o':
		case 'O':
		case 'u':
		case 'U':
			iFunc = 1;
			break;
		case 'l':
		case 'L':
		case 'i':
		case 'I':
		case 'd':
		case 'D':
			iFunc = 0;
			break;

		case '0':
			iFunc = 0;
			break;
		case '1':
			iFunc = 1;
			break;
		case '2':
			iFunc = 2;
			break;
		case '3':
			iFunc = 3;
			break;
		case 'g':
		case 'G':
			break;
		case 'f': // Falling Edge
		case 'F':
			iFunc = 2;
			break;
		case 'r': // Rising Edge
		case 'R':
			iFunc = 3;
		default:
			printk(" ... Wrong Command ....\n");
			break;
	}

	return iFunc;
}
static ssize_t gpio_proc_write(struct file *file, const char __user *buf,
			    size_t count, loff_t *ppos)
{
	int len,i;
	int iFunc=0, iGroup=0, iBit=0;
	char buffer[256];

	for(i=0;i<count-1;i++) 
		buffer[i] = buf[i];

	buffer[i] = '\0';

	if( buffer[0] != 'h' || buffer[0] != 'H' ) {
		iFunc	= GetOption(buffer[1]);
		iGroup	= GetGroup(buffer[2]);
		// Set Bit
		iBit = atoi(&buffer[3]);
	}


	if( buffer[0] == 's' || buffer[0] == 'S' ) {
		NX_GPIO_SetOutputValue( iGroup, iBit, iFunc );
	}
	else if( buffer[0] == 'd' || buffer[0] == 'D' ) {
		NX_GPIO_SetOutputEnable( iGroup, iBit, iFunc );
	}
	else if( buffer[0] == 'f' || buffer[0] == 'F' ) {
		NX_GPIO_SetPadFunction( iGroup, iBit, iFunc );
	} 
	else if( buffer[0] == 'g' || buffer[0] == 'G' ) {
		printk("### Get GPIO%c-%d Func(%d)\n"
				"(0:GPIO, 1:ALT1, 2:ALT2, 3:ALT3)\n\n"
				"###Get GPIO%c-%d I/O Mode(%d)\n"
				"(0:Input, 1:Output)\n\n"
				"###Get GPIO%c-%d Level(%d)\n"
				"(0:Low Level, 1:High Level)\n\n"
				"###Get GPIO%c-%d Resistor(%d)\n"
				"(0:Pull-Down, 1:Pull-Up)\n\n"
				"###Get GPIO%c-%d InputValue(%d)\n"
				"(0:Low, 1:High)\n\n"
				"###Get GPIO%c-%d InterruptEnable(%d)\n"
				"(0:INT Disable, 1:INT Enable)\n\n"
				"###Get GPIO%c-%d InterruptMode(%d)\n"
				"(0:Low, 1:High, 2:Falling, 3:Rising)\n\n"
				,buffer[2],iBit,NX_GPIO_GetPadFunction( iGroup, iBit )
				,buffer[2],iBit,NX_GPIO_GetOutputEnable( iGroup, iBit )
				,buffer[2],iBit,NX_GPIO_GetOutputValue( iGroup, iBit )
				,buffer[2],iBit,NX_GPIO_GetPullUpEnable( iGroup, iBit )
				,buffer[2],iBit,NX_GPIO_GetInputValue( iGroup, iBit )
				,buffer[2],iBit,NX_GPIO_GetInterruptEnable( iGroup, iBit )
				,buffer[2],iBit,NX_GPIO_GetInterruptMode( iGroup, iBit ));
	} 
	else if( buffer[0] == 'p' || buffer[0] == 'P' ) {
		NX_GPIO_SetPullUpEnable( iGroup, iBit, iFunc );
	}
	else if( buffer[0] == 'i' || buffer[0] == 'I' ) {
		NX_GPIO_SetInterruptEnable( iGroup, iBit, iFunc );
	}
	else if( buffer[0] == 'm' || buffer[0] == 'M' ) {
		NX_GPIO_SetInterruptMode( iGroup, iBit, iFunc );
	}
	else if( buffer[0] == 'h' || buffer[0] == 'H' ) {
		printk("Usage: echo CMD > /proc/gpioset\n"
				" Arg 1. [s] : Set Output Value\n"
				"        [d] : Set Direction\n"
				"        [f] : Set PadFunction\n"
				"        [p] : Set Pull-Up&Down Resistor\n"
				"		 [i] : Set Interrupt Enable\n"
				"		 [m] : Set Interrupt Mode\n"
				"        [g] : Get PadFunction & I/O Mode & Level \n"
				"        [h] : Help\n\n"
				" Arg 2. [s][h,l]		: (h:High, l:Low)\n"
				"        [d][o,i]		: (o:OutPut, i:Input)\n"
				"        [f][0~4]		: (0:GPIO, 1:ALT1, 2:ALT2, 3:ALT3)\n"
				"        [p][u,d]		: (u:Pull-Up, d:Pull-Down)\n"
				"        [i][0,1]		: (0:INT Disable, 1:INT Enable)\n"
				"		 [m][l,h,f,r]	: (l:Low, h:High, f:Falling, r:Rising)\n"
				"        [g][g] 		: (g:Just Input!!)\n\n"
				" Arg 3. [?][?][a~c]	: (a:GPIOA, b:GPIOB, c:GPIOC)\n\n"
				" Arg 4. [?][?][?][0~31]: (GPIO Bit Number)\n\n"
				" exam: echo shb31 > /proc/gpioset\n");
	}
	else
		printk(" ... Wrong Command ....\n");

	len = count;
	return len;
}

static int gpio_proc_show(struct seq_file *m, void *v)
{
	return 0;
}

static int gpio_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, gpio_proc_show, NULL);
}

static struct file_operations procfs_ops = {
	.owner = THIS_MODULE,
	.open  = gpio_proc_open,
	.read  = seq_read,
	.write = gpio_proc_write,
};

static int __init gpio_procfs_init(void)
{
	printk("%s\n", __func__);
	gpio_proc_entry = proc_create("gpioset", S_IRUGO | S_IFREG, NULL, &procfs_ops);

	if (gpio_proc_entry)
		return 0;
	else
		return -ENOMEM;
}

static void __exit gpio_procfs_exit(void)
{
	printk("%s\n", __func__);
	proc_remove(gpio_proc_entry);
}
module_init(gpio_procfs_init);
module_exit(gpio_procfs_exit);

MODULE_AUTHOR("larche <larche@falinux.com>");
MODULE_DESCRIPTION("NXP2120 GPIO Control Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:NXP2120");
