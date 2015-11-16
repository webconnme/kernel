/* This driver is for nexell nxp2120 mpeg ts interface */
#include <linux/module.h>
#include <linux/clk.h>
#include <linux/file.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/dma-mapping.h>
#include <linux/wait.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/ioctl.h>
#include <linux/kernel.h>
#include <linux/timer.h>

#include <mach/platform.h>
#include <mach/devices.h>
#include <mach/soc.h>
#include <mach/mpegts.h>
#define MPEGTS_DBG_HEADER "[MPEGTS_INTF]"

//#define NEXELL_MPEGTS_DEBUG

#ifdef NEXELL_MPEGTS_DEBUG
#define MPEGTS_DBG(args...) printk(MPEGTS_DBG_HEADER ":" args)
#else
#define MPEGTS_DBG(args...)	do{}while(0)
#endif



#define PACKET_SIZE		(CFG_MPEGTS_WORDCNT*4*16*8)
#define NUM_PACKET		(128)
#define DMA_DOUBLE_BUFFER	(1)

struct ts_ring_buf {
	unsigned int	cnt;
	unsigned int	wPos;
	unsigned int	rPos;
	unsigned char	*ts_packet[NUM_PACKET];
	unsigned char	*ts_phy_packet[NUM_PACKET];
};

struct ts_drv_context {
	void __iomem *baseaddr;
	int irq_ts_intf;
	int irq_dma;

	struct dma_trans *dma_tr;
	struct ts_ring_buf *buf;
	unsigned int dma_virt;
	dma_addr_t dma_phy;
	unsigned int buf_size;
	wait_queue_head_t wait; // read wait
	struct device *dev;
	int is_opened;
	int is_running;
};

struct ts_packet_data {
	int size;
	int num;
};

static struct ts_packet_data ts_pack;

static struct ts_drv_context *s_ctx = NULL;
static int one_sec_ticks = 100;
static inline void _w_buf(struct ts_ring_buf *buf)
{
	if(buf->cnt < ts_pack.num)
	{
		buf->wPos = (buf->wPos + 1) %ts_pack.num;
	}

}

static inline void _r_buf(struct ts_ring_buf *buf)
{
	buf->cnt --;
	buf->rPos = (buf->rPos + 1) %ts_pack.num;
}

// TODO: define in plat
#define CFG_DMA_MPEGTS 0
static inline int _init_dma(struct ts_drv_context *ctx)
{
#if !defined(CFG_DMA_MPEGTS)
#error "[NXP2120 MPEGTS] CFG_DMA_MPEGTS is not defined!!!"
#endif
	struct dma_trans *dma_tr = NULL;
	dma_tr = soc_dma_request(CFG_DMA_MPEGTS, false);

	if (!dma_tr) {
		printk(KERN_ERR "fail, request for mpegts dma channel ...\n");
		return -EINVAL;
	}

	dma_tr->tr_type = DMA_TRANS_IO2MEM;
	dma_tr->iom.srcbase = NX_MPEGTSIF_GetPhysicalAddress();
	dma_tr->iom.src_id  = NX_MPEGTSIF_GetDMAIndex();
	dma_tr->iom.src_bit = NX_MPEGTSIF_GetDMABusWidth();
//	dma_tr->iom.length  = PACKET_SIZE;
	dma_tr->iom.length  = ts_pack.size;

	ctx->dma_tr = dma_tr;
	ctx->irq_dma = PB_DMA_IRQ(ctx->dma_tr->channel);

	return 0;
}

static inline void _deinit_dma(struct ts_drv_context *ctx)
{
	if (ctx->dma_tr) {
		soc_dma_release(ctx->dma_tr);
		ctx->dma_tr = NULL;
	}
}

static inline void _enqueue_dma(struct ts_drv_context *ctx)
{
	ctx->dma_tr->iom.dstbase = (unsigned int)ctx->buf->ts_phy_packet[ctx->buf->wPos];
	soc_dma_transfer(ctx->dma_tr);
}

static inline void _stop_dma(struct ts_drv_context *ctx)
{
	int ret=0;

	soc_dma_get_length(ctx->dma_tr);
	soc_dma_trans_stop(ctx->dma_tr);
	while (soc_dma_check_run(ctx->dma_tr)) {
		ret++;
		msleep(1);
	}

	ctx->buf->cnt=0;

}

static inline int _init_buf(struct ts_drv_context *ctx, int packet_num,int packet_size)
{
	int ret = 0;
	struct ts_ring_buf *buf = NULL;
	int i;

	ctx->dev->coherent_dma_mask = 0xffffffff;
	ctx->buf_size = PAGE_ALIGN(packet_num * packet_size);
	ctx->dma_virt = (unsigned int)dma_alloc_writecombine(ctx->dev, ctx->buf_size, &ctx->dma_phy, GFP_ATOMIC);
	if (!ctx->dma_virt) {
		printk(KERN_ERR "can't alloc packet buffer...\n");
		return -ENOMEM;
	}

	buf = (struct ts_ring_buf *)kzalloc(sizeof(struct ts_ring_buf), GFP_KERNEL);
	if (!buf) {
		printk(KERN_ERR "fail, no memory for struct ts_ring_buf ...\n");
		ret = -ENOMEM;
		goto fail_ring_buf;
	}

	printk( " %s pakcet_size = %d, packet_num=%d\n",__func__, packet_size, packet_num);

	for(i=0;i<packet_num;i++)
	{
		buf->ts_packet[i] = (unsigned char *)ctx->dma_virt + i*packet_size;
		buf->ts_phy_packet[i] = (unsigned char*)(ctx->dma_phy + i*packet_size);
	}
	buf->wPos = 0;
	buf->rPos = 0;
	buf->cnt = 0;
	ctx->buf = buf;

	ctx->dma_tr->iom.length  = ts_pack.size;
	return 0;

fail_ring_buf:
	dma_free_coherent(ctx->dev, ctx->buf_size, (void *)ctx->dma_virt, ctx->dma_phy);
	ctx->buf_size = 0;
	ctx->dma_virt = 0;
	ctx->dma_phy = 0;
	return ret;
}

static inline void _deinit_buf(struct ts_drv_context *ctx)
{
	if (ctx->buf) {
		kfree(ctx->buf);
		ctx->buf = NULL;
	}

	if (ctx->dma_virt) {
		dma_free_coherent(ctx->dev, ctx->buf_size, (void *)ctx->dma_virt, ctx->dma_phy);
		ctx->buf_size = 0;
		ctx->dma_virt = 0;
		ctx->dma_phy = 0;
	}
}

static int _init_context(struct ts_drv_context *ctx)
{
	int ret;
	ts_pack.size = PACKET_SIZE;
	ts_pack.num = NUM_PACKET;
	ret = _init_dma(ctx);
	if (ret) return ret;
	ret = _init_buf(ctx, NUM_PACKET,PACKET_SIZE);
	if (ret) {
		goto fail_init_buf;
	}
	/*set pad TS Interface */
	soc_gpio_set_io_func(PAD_GPIO_B+13,NX_GPIO_PADFUNC_2); // ERROR
	soc_gpio_set_io_func(PAD_GPIO_B+14,NX_GPIO_PADFUNC_2);
	soc_gpio_set_io_func(PAD_GPIO_B+15,NX_GPIO_PADFUNC_2);
	soc_gpio_set_io_func(PAD_GPIO_B+16,NX_GPIO_PADFUNC_2);
	soc_gpio_set_io_func(PAD_GPIO_B+17,NX_GPIO_PADFUNC_2);

	ctx->baseaddr = (void __iomem *)IO_ADDRESS(PHY_BASEADDR_MPEGTSIF);
	init_waitqueue_head(&ctx->wait);
	return 0;

fail_init_buf:
	_deinit_dma(ctx);
	return ret;
}

static void _deinit_context(struct ts_drv_context *ctx)
{
	wake_up(&ctx->wait);
	_stop_dma(ctx);
	_deinit_buf(ctx);
	_deinit_dma(ctx);
}

static int _init_device(struct mpegts_plat_data *c)
{
	NX_MPEGTSIF_Initialize();
	NX_MPEGTSIF_SetBaseAddress((unsigned int)s_ctx->baseaddr);
	NX_MPEGTSIF_OpenModule();

	NX_MPEGTSIF_SetClockPClkMode(NX_PCLKMODE_ALWAYS);

	NX_MPEGTSIF_SetClockPolarity(c->clock_pol);
	NX_MPEGTSIF_SetDataPolarity(c->data_pol);
	NX_MPEGTSIF_SetSyncSource(c->sync_source);
	NX_MPEGTSIF_SetSyncMode(c->sync_mode);
	NX_MPEGTSIF_SetWordCount(c->word_cnt);


	return 0;
}

static void _deinit_device(void)
{
	NX_MPEGTSIF_CloseModule();
}

static int _enable_device(void)
{
	NX_MPEGTSIF_SetEnable(CTRUE);

	return 0;
}

#if (0)
static void  _mpegts_clear(void)
{

	NX_MPEGTSIF_SetDataPolarity(1);
	msleep(1);
	//NX_MPEGTSIF_SetDataPolarity(0);

}
#endif

static int _disable_device(void)
{
	NX_MPEGTSIF_SetDataPolarity(0);
	NX_MPEGTSIF_SetEnable(CFALSE);
	//NX_MPEGTSIF_SetDataPolarity(1);
	msleep(1);

	NX_MPEGTSIF_SetDataPolarity(0);

	return 0;
}


/* application interface */
static int mpegts_open(struct inode *inode, struct file *filp)
{
	int ret = 0;

	MPEGTS_DBG("[MPEG-TS] open ++\n");
	if (s_ctx->is_opened) {
		printk("MPEG TS Device is already opened!!!\n");
		return -EBUSY;
	}
	s_ctx->is_opened = 1;
	MPEGTS_DBG("[MPEG-TS] open --\n");
	return ret;
}

static int mpegts_close(struct inode *inode, struct file *filp)
{
	int ret = 0;


	MPEGTS_DBG("[MPEG-TS] close ++\n");
	if (!s_ctx->is_opened) {
		printk("MPEG TS Device is not opened!!!\n");
		return -EINVAL;
	}

	if( s_ctx->is_running )
	{
		s_ctx->is_running = 0;
		_disable_device();
		_stop_dma(s_ctx);
	}
	s_ctx->is_opened = 0;

	MPEGTS_DBG("[MPEG-TS] close --\n");
	return ret;
}

static int mpegts_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = -1;



	switch(cmd)
	{
		case IOCTL_MPEGTS_RUN:
			if( s_ctx->is_running )
				break;
			s_ctx->buf->cnt=0;
			s_ctx->buf->wPos=0;
			s_ctx->buf->rPos=0;

			//dbg_flag = 1;

#if (DMA_DOUBLE_BUFFER)
#if 1
			ret = soc_dma_set_mode(s_ctx->dma_tr,DMA_MODE_BUFFER);
			if(0 != ret)
			{
				printk("command Mode set fail\n");
				ret = -1;
			}

			_enqueue_dma(s_ctx);
			_w_buf(s_ctx->buf);
			_enqueue_dma(s_ctx);
#endif

#else
			_enqueue_dma(s_ctx);
#endif
			msleep(1);
			ret = _enable_device();

			ret = 0;
			if( ret == 0 ){
				s_ctx->is_running = 1;
				ret = 0;
			}else{
				s_ctx->is_running = 0;
				_stop_dma(s_ctx);
			}

			break;
		case IOCTL_MPEGTS_STOP:
			if( s_ctx->is_running )
			{
				s_ctx->is_running = 0;

				ret = _disable_device();
				_stop_dma(s_ctx);

				s_ctx->is_running = 0;
			}
			ret =0;
			break;


			case IOCTL_MPEGTS_CON_BUF:
			{
			//	struct ts_packet_data param;

				if(copy_from_user(&ts_pack,(const void __user *)arg,sizeof (ts_pack)))
					return false;
				if(ts_pack.num > NUM_PACKET)
				{
					printk("Error : Buffer Number not over 128\n");
					return false;
				}

				_deinit_buf(s_ctx);
				_init_buf(s_ctx,ts_pack.num,ts_pack.size);
				ret=0;

			}
				break;
			
			case IOCTL_MPEGTS_READ_BUF_STATUS:
			{
				if(copy_to_user((void __user *)arg,(const void*)&ts_pack,sizeof (ts_pack)))
				return false;
			}

		default:
			break;
	}

	return ret;
}

static ssize_t mpegts_read(struct file *filp, char *buf, size_t count, loff_t *pos)
{
	int ret=0;
	struct ts_ring_buf *rbuf = s_ctx->buf;

	if( !s_ctx->is_running ){
		printk(KERN_ERR "Error: Invalid operation.(is not running)\n");
		return -1;
	}

	if ( ts_pack.size> count )  {
		printk(KERN_ERR "Error: read size must be lager than 188 \n");
		return -1;
	}

	if(rbuf->cnt <= 0){
		ret = interruptible_sleep_on_timeout(&s_ctx->wait, one_sec_ticks);
		if(0 == ret ){
			MPEGTS_DBG("time out \n");

			return 0;
		}
	}

	if(rbuf->cnt == 0){
		MPEGTS_DBG("=============> overflow ???\n");
		return 0;
	}

	if(copy_to_user(buf, rbuf->ts_packet[rbuf->rPos], ts_pack.size)) {
		return -EFAULT;
	}

	if( buf[0] != 0x47 ){
		printk("Warn : TS Data Sync Fail(0x%02x)\n", buf[0] );
	}

	_r_buf(s_ctx->buf);
	//return PACKET_SIZE;
	return ts_pack.size;
}

static struct file_operations mpegts_fops = {
	.owner		= THIS_MODULE,
	.open		= mpegts_open,
	.release	= mpegts_close,
	.ioctl		= mpegts_ioctl
,	.read		= mpegts_read,
};

static struct miscdevice mpegts_misc_device = {
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= MPEGTS_DEV_NAME,
	.fops		= &mpegts_fops,
};

/* irq handler */
static irqreturn_t mpegts_irq(int irq, void *param)
{
	struct ts_drv_context *ctx = (struct ts_drv_context *)param;
	struct ts_ring_buf *buf = ctx->buf;

	#if (DMA_DOUBLE_BUFFER)

	if(s_ctx->is_running != 0)
	soc_dma_irq_enable(ctx->dma_tr,1);/* next command buffer irq*/
	#endif

	buf->cnt++;

#if 0
	{
		//struct task_struct *cur = NULL;
	    //struct task_struct *pre;
		unsigned int new_time;
		static int old_time = 0;
		new_time = jiffies;
		if( old_time != 0 && (new_time-old_time)>10 ){
			printk("WARN : IRQ Interval too big (%d)\n",new_time-old_time);
#if 0
	    	cur = get_current();
			pre = list_entry(cur->tasks.prev, struct task_struct, tasks);

	    	printk(" proc : %s prev : %s \n", cur->comm, pre->comm);
			printk(" Current  Pid: %4d cur Tid : %4d \n", task_tgid_vnr(cur),task_pid_nr(cur));
			printk(" prev  Pid: %4d prev Tid : %4d \n\n", task_tgid_vnr(pre),task_pid_nr(pre));
#endif
		}
		old_time = new_time;
	}
	if( *buf->ts_packet[buf->wPos] != 0x47 ){
		printk("TS sync data fail. !!! (0x%02x)\n", *buf->ts_packet[buf->wPos]);
	}
#endif

	if(buf->cnt >= ts_pack.num-1) //Check overflow
	{
		buf->cnt--;
		MPEGTS_DBG("overflow\n");
	}
	else
	{
		buf->wPos = (buf->wPos + 1) %ts_pack.num;
	}
	if(s_ctx->is_running != 0)
	_enqueue_dma(ctx);

	wake_up(&ctx->wait);
	return IRQ_HANDLED;
}

/* register/remove driver */
static int nexell_mpegts_probe(struct platform_device *pdev)
{
	int ret;
	struct mpegts_plat_data *plat = pdev->dev.platform_data;
	one_sec_ticks = msecs_to_jiffies( 1000 );

	s_ctx = (struct ts_drv_context *)kzalloc(sizeof(struct ts_drv_context), GFP_KERNEL);
	if (!s_ctx) {
		return -ENOMEM;
	}

	s_ctx->dev = &pdev->dev;
	ret = _init_context(s_ctx);
	if (ret) {
		goto fail_init_context;
	}

	ret = _init_device(plat);
	if (ret) {
		goto fail_init_device;
	}

	ret = request_irq(s_ctx->irq_dma, mpegts_irq, IRQF_DISABLED, MPEGTS_DEV_NAME, s_ctx);
	if (ret) {
		goto fail_irq;
	}

	ret = misc_register(&mpegts_misc_device);
	if (ret) {
		goto fail_misc_register;
	}

	return 0;
fail_misc_register:
	free_irq(s_ctx->irq_dma, s_ctx);

fail_irq:
	_deinit_device();

fail_init_device:
	_deinit_context(s_ctx);

fail_init_context:
	kfree(s_ctx);
	s_ctx = NULL;

	return ret;
}

static int nexell_mpegts_remove(struct platform_device *pdev)
{

	free_irq(s_ctx->irq_dma, s_ctx);

	_deinit_device();
	_deinit_context(s_ctx);
	kfree(s_ctx);
	s_ctx = NULL;

	return 0;
}

static struct platform_driver nexell_mpegts_driver = {
	.probe		= nexell_mpegts_probe,
	.remove		= nexell_mpegts_remove,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= MPEGTS_DEV_NAME,
	},
};

static int __init nexell_mpegts_init(void)
{
	return platform_driver_register(&nexell_mpegts_driver);
}

static void __exit nexell_mpegts_exit(void)
{
	MPEGTS_DBG("%s\n", __func__);
	return platform_driver_unregister(&nexell_mpegts_driver);
}

module_init(nexell_mpegts_init);
module_exit(nexell_mpegts_exit);

MODULE_DESCRIPTION("Nexell MPEGTS Interface driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nexell swpark@nexell.co.kr");
MODULE_ALIAS("platform:nexell-mpegts");
