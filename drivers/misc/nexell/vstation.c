/* This driver is for samsung voicestation custom driver */
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
#include <linux/i2c.h>

#include <mach/platform.h>
#include <mach/devices.h>
#include <mach/soc.h>
#include <mach/vstation.h>

#include <linux/mfd/wm8994/core.h>
#include <linux/mfd/wm8994/registers.h>
#include <linux/mfd/wm8994/pdata.h>
#include <linux/mfd/wm8994/gpio.h>


#include "wm8994-registers.h"

#define VSTATION_DBG_HEADER "[VSTATION]"

#define VSTATION_DEBUG
#ifdef VSTATION_DEBUG
#define VSTATION_DBG(args...) printk(VSTATION_DBG_HEADER ":" args)
#else
#define VSTATION_DBG(args...)
#endif

#define CHECK_TIMESTAMP     (0)
#if (CHECK_TIMESTAMP)
#define SET_SEC_VAL(ss)     (ss * 1000*1000)
#define GET_SEC_VAL(ls)     (ls / 1000*1000)

#define GET_REAL_TIME(lt) {                 \
        struct timeval tv;                  \
        do_gettimeofday(&tv);               \
        NX_ASSERT(1000*1000 > tv.tv_usec);  \
        lt  = 0;                            \
        lt  = SET_SEC_VAL(tv.tv_sec);       \
        lt += tv.tv_usec;                   \
    }
#else
#define GET_REAL_TIME(lt)   (0)
#endif


#define VSTATION_DEV_NAME "vstation"

#define VSTATION_DOUBLE_BUFFER      (1)

extern void wm8994_reg_init(void);




/* structures */
struct ring_buf {
    unsigned int   head;
    unsigned int   tail;
    unsigned char *start;
    unsigned char *end;
    unsigned long  size;
    unsigned long  chunk_size;
};

enum dma_type {
    DMA_TYPE_MPEGTS,
    DMA_TYPE_AUDIO_OUT
};

struct dma_ctx {
    enum dma_type  type;
    int ch;
    unsigned long phy_base;
    unsigned long virt_base;
    unsigned long size;
    struct dma_trans *dma_tr;
    struct device *dev;
    int irq;
};
/*
struct wm8994 {
    struct mutex io_lock;
    struct device *dev;
    void *control_data;
    int (*read_dev)(struct wm8994 *wm8994, unsigned short reg, int bytes, void *dest);
    int (*write_dev)(struct wm8994 *wm8994, unsigned short reg, int bytes, void *src);
};
*/
struct wm8994_regval {
    unsigned short reg;
    unsigned short val;
    unsigned long  delay_ms;
};

struct vstation_ctx {
    struct ring_buf read_buf;  // wm8994
    struct ring_buf write_buf; // bluetooth
    struct dma_ctx  read_dma;
    struct dma_ctx  write_dma;
    wait_queue_head_t read_wait;

    int irq_mpegts; // for check ts error

    struct wm8994 *wm8994;
    struct device *dev;

    int is_opened;
};

struct vstation_ctx *s_ctx = NULL;



// test for resume time
#define CFG_GPIO_RESUME_TEST   (PAD_GPIO_C + 0)
static inline void set_gpio_resume_set(int val)
{
    soc_gpio_set_out_value(CFG_GPIO_RESUME_TEST, val);
}

static void wm8994_set_mclk(unsigned long clock, int duty);

/* ring buffer interface */
static inline int _get_avail_read_size(struct ring_buf *buf)
{
    if (buf->head >= buf->tail) {
        return buf->head - buf->tail;
    } else {
        return buf->head + buf->size - buf->tail;
    }
}

static inline void _w_buf(struct ring_buf *buf)
{
    buf->head += buf->chunk_size;
    if (!(buf->head % buf->size)) {
        buf->head = 0;
    }
}

static inline void _r_buf(struct ring_buf *buf)
{
    buf->tail += buf->chunk_size;
    if (!(buf->tail % buf->size)) {
        buf->tail = 0;
    }
}

static inline void _init_ring_buf(struct ring_buf *buf, unsigned char *start, unsigned long size, unsigned long chunk_size)
{
    buf->head = buf->tail = 0;
    buf->start = start;
    buf->size = size;
    buf->chunk_size = chunk_size;
    buf->end = start + size;
}

static inline void _reset_ring_buf(struct ring_buf *buf)
{
    buf->head = buf->tail = 0;
    memset(buf->start,0xff,(buf->end - buf->start));
}

/* dma interface */
static inline int _init_dma(struct dma_ctx *ctx)
{
    struct dma_trans *dma_tr = NULL;
    printk("DMA CHANNEL = %d \n",ctx->ch);
    dma_tr = soc_dma_request(ctx->ch, false);
    if (!dma_tr) {
        printk("Error: %s Failed to soc_dma_request\n", __func__);
        return -1;
    }

    if (ctx->type == DMA_TYPE_MPEGTS) {
        dma_tr->tr_type = DMA_TRANS_IO2MEM;
        dma_tr->iom.srcbase = NX_MPEGTSIF_GetPhysicalAddress();
        dma_tr->iom.src_id  = NX_MPEGTSIF_GetDMAIndex();
        dma_tr->iom.src_bit = NX_MPEGTSIF_GetDMABusWidth();
    } else if (ctx->type == DMA_TYPE_AUDIO_OUT) {
        dma_tr->tr_type = DMA_TRANS_MEM2IO;
        dma_tr->mio.dstbase = PHY_BASEADDR_AUDIO;
        dma_tr->mio.dst_id  = DMAINDEX_OF_AUDIO_MODULE_PCMOUT;
        dma_tr->mio.dst_bit = 16;
    }

    ctx->dma_tr = dma_tr;
    ctx->irq    = PB_DMA_IRQ(dma_tr->channel);
    return 0;
}

static inline void _deinit_dma(struct dma_ctx *ctx)
{
    if (ctx->dma_tr) {
        soc_dma_release(ctx->dma_tr);
        ctx->dma_tr = NULL;
    }
}

static inline void _enqueue_dma(struct dma_ctx *ctx, unsigned int offset, unsigned int length)
{
    if (ctx->type == DMA_TYPE_MPEGTS) {
        ctx->dma_tr->iom.dstbase = ctx->phy_base + offset;
        ctx->dma_tr->iom.length  = length;
    } else if (ctx->type == DMA_TYPE_AUDIO_OUT) {
        ctx->dma_tr->mio.srcbase = ctx->phy_base + offset;
        ctx->dma_tr->mio.length  = length;
    }
    soc_dma_transfer(ctx->dma_tr);
}

static inline void _stop_dma(struct dma_ctx *ctx)
{
    while (soc_dma_check_run(ctx->dma_tr)) {
        soc_dma_trans_stop(ctx->dma_tr);
        soc_dma_wait_ready(ctx->dma_tr, 1000*1000);
    }
}

static int _alloc_dma(struct dma_ctx *ctx, unsigned long size)
{
    if (!ctx->dev) {
        printk("error: dev is NULL\n");
        return -1;
    }
    ctx->dev->coherent_dma_mask = 0xffffffff;
    ctx->size = PAGE_ALIGN(size);
    ctx->virt_base = (unsigned long)dma_alloc_writecombine(ctx->dev, ctx->size, (dma_addr_t *)&ctx->phy_base, GFP_ATOMIC);
    if (!ctx->virt_base) {
        printk("Error: %s Failed to dma_alloc_writecombine\n", __func__);
        return -ENOMEM;
    }

    memset((void*)ctx->virt_base,0,ctx->size);
    return 0;
}

static void _free_dma(struct dma_ctx *ctx)
{
    if (ctx->virt_base) {
        dma_free_writecombine(ctx->dev, ctx->size, (void *)ctx->virt_base, ctx->phy_base);
        ctx->size = 0;
        ctx->phy_base = ctx->virt_base = 0;
    }
}

/* mpegts interface */
static void _init_mpegts(void)
{
    NX_MPEGTSIF_Initialize();
    NX_MPEGTSIF_SetBaseAddress((unsigned int)IO_ADDRESS(PHY_BASEADDR_MPEGTSIF));
    NX_MPEGTSIF_OpenModule();

    NX_MPEGTSIF_SetClockPClkMode(NX_PCLKMODE_ALWAYS);

    //NX_MPEGTSIF_SetClockPolarity(0);
    NX_MPEGTSIF_SetClockPolarity(1);
    NX_MPEGTSIF_SetDataPolarity(0);
    NX_MPEGTSIF_SetSyncSource(0);
    NX_MPEGTSIF_SetSyncMode(0);
    NX_MPEGTSIF_SetWordCount(0x3);
}

static void _deinit_mpegts(void)
{
    NX_MPEGTSIF_CloseModule();
}

static void _enable_mpegts(int enable)
{
    NX_MPEGTSIF_SetInterruptEnableAll(enable);
    NX_MPEGTSIF_SetEnable(enable);
}


static irqreturn_t _mpegts_data_irq(int irq, void *param)
{
    struct vstation_ctx *ctx = (struct vstation_ctx *)param;
    struct dma_ctx *dma_ctx= &ctx->read_dma;
    struct ring_buf *ring_buf = &ctx->read_buf;

    #if (CHECK_TIMESTAMP)
    {
        long long new_ts;
         GET_REAL_TIME(new_ts);
        static long long pre_ts =0;

        //if (pre_ts && (new_ts - pre_ts) > 22000)
            printk("DMA irq time :%06llu us\n",(new_ts - pre_ts));
        pre_ts = new_ts;
    }
    #endif
    #if (VSTATION_DOUBLE_BUFFER)
        //printk("%s,%d\n",__func__,dma_ctx->ch);
		soc_dma_irq_enable(dma_ctx->dma_tr,1);/* next command buffer irq*/
	#endif

    _w_buf(ring_buf);
    _enqueue_dma(dma_ctx, ring_buf->head, ring_buf->chunk_size);

    wake_up(&ctx->read_wait);

    //printk("R");
    return IRQ_HANDLED;
}

static irqreturn_t _mpegts_error_irq(int irq, void *param)
{
    NX_MPEGTSIF_ClearInterruptPendingAll();
    NX_MPEGTSIF_SetEnable(0);
    NX_MPEGTSIF_SetEnable(1);

    printk("E");
    return IRQ_HANDLED;
}

/* i2s interface */

#if 0
static irqreturn_t _i2s_data_irq(int irq, void *param)
{
    struct vstation_ctx *ctx = (struct vstation_ctx *)param;
    struct dma_ctx *dma_ctx= &ctx->write_dma;
    struct ring_buf *ring_buf = &ctx->write_buf;

    //printk("W:%x\n", ring_buf->tail);
    _r_buf(ring_buf);
    if (_get_avail_read_size(ring_buf) >= ring_buf->chunk_size) {
        _enqueue_dma(dma_ctx, ring_buf->tail, ring_buf->chunk_size);
    }

    return IRQ_HANDLED;
}
#endif

static int wm8994_i2c_read_device(struct wm8994 *wm8994, unsigned short reg,
        int bytes, void *dest)
{
    struct i2c_client *i2c = wm8994->control_data;
    int ret;
    u16 r = cpu_to_be16(reg);

    ret = i2c_master_send(i2c, (unsigned char *)&r, 2);
    if (ret < 0)
        return ret;
    if (ret != 2)
        return -EIO;

    ret = i2c_master_recv(i2c, dest, bytes);
    if (ret < 0)
        return ret;
    if (ret != bytes)
        return -EIO;
    return 0;
}

static int wm8994_i2c_write_device(struct wm8994 *wm8994, unsigned short reg,
        int bytes, void *src)
{
    struct i2c_client *i2c = wm8994->control_data;
    unsigned char msg[bytes + 2];
    int ret;

    reg = cpu_to_be16(reg);
    memcpy(&msg[0], &reg, 2);
    memcpy(&msg[2], src, bytes);

    ret = i2c_master_send(i2c, msg, bytes + 2);
    if (ret < 0)
        return ret;
    if (ret < bytes + 2)
        return -EIO;

    return 0;
}
#if 0
#define CFG_GPIO_LDO_EN   (PAD_GPIO_C + 6)
static inline void wm8994_ldo_control(int enable)
{
    int io = CFG_GPIO_LDO_EN;
    soc_gpio_set_io_func(io, NX_GPIO_PADFUNC_GPIO);
    soc_gpio_set_io_dir(io, 1);
    soc_gpio_set_out_value(io, enable);
}
#endif
static int wm8994_device_init(struct wm8994 *wm8994)
{
    int ret;

#if 0//ndef CONFIG_PLAT_NXP2120_WIFIAUDIO
    wm8994_ldo_control(1);
#endif


    wm8994_set_mclk(12000000, 50);
    //mdelay(50);
    mdelay(500);

    ret = wm8994_reg_read(wm8994, WM8994_SOFTWARE_RESET);
    if (ret < 0) {
        printk(KERN_ERR "Error %s: Falied to read ID register\n", __func__);
        goto err_enable;
    }

    if (ret != 0x8994) {
        printk(KERN_ERR "Error %s: Device is not a WM8994, ID is %x\n", __func__, ret);
        ret = -EINVAL;
        goto err_enable;
    }

    ret = wm8994_reg_read(wm8994, WM8994_CHIP_REVISION);
    if (ret < 0) {
        printk(KERN_ERR "Error %s: Falied to read revision register - %d\n", __func__, ret);
        goto err_enable;
    }

    switch (ret) {
        case 0:
        case 1:
            printk("Warning %s: revision %c not fully supported\n", __func__, 'A' + ret);
            break;
        default:
            printk("Info %s: revision %c\n", __func__, 'A' + ret);
            break;
    }
//wm8994_reg_init();

//    wm8994_init_default(wm8994);


    return 0;

err_enable:
#if 0//ndef CONFIG_PLAT_NXP2120_WIFIAUDIO
    wm8994_ldo_control(0);
#endif

    return ret;
}

static void wm8994_device_exit(struct wm8994 *wm8994)
{
#if 0 //ndef CONFIG_PLAT_NXP2120_WIFIAUDIO
    wm8994_ldo_control(0);
#endif
    kfree(wm8994);
}
#if 0
static struct wm8994_regval wm8994_def_val[] = {
        { 0x0000, 0x0000, 500 },
//  { 0x0102, 0x0003, 0   },
//    { 0x0817, 0x0000, 0   },
//    { 0x0102, 0x0000, 0   },

    { 0x0700, 0x8101, 0   },
    { 0x0701, 0x8100, 0   },
    { 0x0702, 0x0100, 0   },
    { 0x0703, 0x0100, 0   },
    { 0x0704, 0x8100, 0   },
    { 0x0709, 0x2015, 0   },

    { 0x0039, 0x0064, 0   },
    { 0x0001, 0x3013, 50  },//for AIF2

    { 0x0003, 0x0300, 0   },
    { 0x0022, 0x0000, 0   },
    { 0x0023, 0x0100, 0   },
    { 0x0036, 0x0003, 0   },

    { 0x0004, 0x0F3C, 0   },

    { 0x0005, 0x3003, 0   },

    { 0x0601, 0x0004, 0   },
    { 0x0602, 0x0004, 0   },
    { 0x0606, 0x0002, 0   },
    { 0x0607, 0x0002, 0   },
    { 0x0608, 0x0002, 0   },
    { 0x0609, 0x0002, 0   },

    { 0x0220, 0x0005, 0   },
    { 0x0221, 0x0700, 0   },
    { 0x0222, 0x3127, 0   },
    { 0x0223, 0x0100, 0   },

    { 0x0210, 0x0083, 0   }, // for power voice: 48KHz
    { 0x0300, 0x4010, 0   },
    { 0x0302, 0x4000, 0   },
    { 0x0303, 0x0040, 0   },


    { 0x0211, 0x0083, 0   },
    { 0x0310, 0x4010, 0   },
    { 0x0312, 0x4000, 0   },
    { 0x0313, 0x0040, 0   },

    { 0x0208, 0x000e, 0   },
    { 0x0200, 0x0011, 0   },
    { 0x0204, 0x0011, 0   },
};
#endif
/*
extern struct snd_soc_codec *wm8994_codec;
extern int wm8994_write(struct snd_soc_codec *codec, unsigned int reg,
    unsigned int value);

extern unsigned int wm8994_read(struct snd_soc_codec *codec,
                unsigned int reg);
                */
#if 0
static void wm8994_init_default(struct wm8994 *wm8994)
{
    int i;
    struct wm8994_regval *regval;
    int readval;

    printk("%s\n",__func__);

    set_gpio_resume_set(0);
    for (i = 0; i < ARRAY_SIZE(wm8994_def_val); i++) {
        regval = &wm8994_def_val[i];
        wm8994_reg_write(wm8994, regval->reg, regval->val);
   // wm8994_reg_write(wm8994, regval->reg, regval->val);
        if (regval->delay_ms) {
            mdelay(regval->delay_ms);
        }
    }
    set_gpio_resume_set(1);
#if 1
    //for (i = 0; i <0x74a; i++) {
    for (i = 0; i < ARRAY_SIZE(wm8994_def_val); i++) {
        regval = &wm8994_def_val[i];
        //readval = wm8994_reg_read(wm8994, i);
        readval = wm8994_reg_read(wm8994, regval->reg);
        printk("REG 0x%x: 0x%x\n", i, readval);
    }
#endif
}
#endif
#if 0
static void wm8994_start(struct wm8994 *wm8994)
{
    wm8994_reg_write(wm8994, 0x0004, 0x0F3C);
#if 1
    wm8994_reg_write(wm8994, 0x0400, 0x01E0);
    wm8994_reg_write(wm8994, 0x0401, 0x01E0);
    wm8994_reg_write(wm8994, 0x0404, 0x01E0);
    wm8994_reg_write(wm8994, 0x0405, 0x01E0);
#else
    wm8994_reg_write(wm8994, 0x0400, 0x01EF);
    wm8994_reg_write(wm8994, 0x0401, 0x01EF);
    wm8994_reg_write(wm8994, 0x0404, 0x01EF);
    wm8994_reg_write(wm8994, 0x0405, 0x01EF);
#endif
}

static void wm8994_stop(struct wm8994 *wm8994)
{
    wm8994_reg_write(wm8994, 0x0400, 0x0100);
    wm8994_reg_write(wm8994, 0x0401, 0x0100);
    wm8994_reg_write(wm8994, 0x0404, 0x0100);
    wm8994_reg_write(wm8994, 0x0405, 0x0100);
    wm8994_reg_write(wm8994, 0x0004, 0x003C);
}
#else
extern void wm8994_start(void);
extern void wm8994_stop(void);

#endif
static void wm8994_set_mclk(unsigned long clock, int duty)
{
    soc_pwm_set_frequency(0, clock, duty);
}

/* application interface */
static int init_read_channel(struct vstation_ctx *ctx)
{
    int ret;
    struct dma_ctx *dma_ctx;
    struct ring_buf *ring_buf;
    unsigned long chunk_size;
    unsigned long chunk_count;

    VSTATION_DBG("%s\n", __func__);

    chunk_size = READ_CHANNEL_CHUNKSIZE;
    chunk_count = READ_CHANNEL_CHUNKCOUNT;

    // init mpegts
    _init_mpegts();

    // init read dma
    dma_ctx = &ctx->read_dma;
    memset(dma_ctx, 0, sizeof(struct dma_ctx));
    dma_ctx->type = DMA_TYPE_MPEGTS;
    dma_ctx->ch = 4;
    ret = _init_dma(dma_ctx);
    if (ret < 0) {
        return ret;
    }

    // alloc read dma buf
    dma_ctx->dev = ctx->dev;
    ret = _alloc_dma(dma_ctx, chunk_size*chunk_count);
    if (ret < 0) {
        return ret;
    }

    // init read ring buf
    ring_buf = &s_ctx->read_buf;
    _init_ring_buf(ring_buf, (unsigned char *)dma_ctx->virt_base, dma_ctx->size, chunk_size);

    // init wait queue
    init_waitqueue_head(&ctx->read_wait);

    // data irq
    ret = request_irq(dma_ctx->irq, _mpegts_data_irq, IRQF_DISABLED, VSTATION_DEV_NAME, ctx);
    if (ret) {
        printk("Error: %s Failed to request_irq\n", __func__);
        return ret;
    }

    // error irq
    ret = request_irq(NX_MPEGTSIF_GetInterruptNumber(), _mpegts_error_irq, IRQF_DISABLED, VSTATION_DEV_NAME, ctx);
    if (ret) {
        printk("Error: %s Failed to request_irq\n", __func__);
        return ret;
    }
    //init
    //wm8994_reg_init();
    return 0;
}

static int start_read_channel(struct vstation_ctx *ctx)
{
    struct dma_ctx *dma_ctx = &ctx->read_dma;
    struct ring_buf *ring_buf = &ctx->read_buf;
	int ret =0;
    VSTATION_DBG("%s entered\n", __func__);

    // reset ring buf
    _reset_ring_buf(ring_buf);
    memset((void*)dma_ctx->virt_base,0xff,dma_ctx->size );
 //   _stop_dma(dma_ctx);
#if (VSTATION_DOUBLE_BUFFER)
	 ret = soc_dma_set_mode(dma_ctx->dma_tr,DMA_MODE_BUFFER);
	 if(0 != ret)
	 {
	 	printk("command Mode set fail\n");
	    ret = -1;
	 }

     ret = soc_dma_get_mode(dma_ctx->dma_tr);
    printk("%s mode = %d \n",__func__,ret);
    // printk("%s type = %d \n",__func__,dma_ctx->type);
    _enqueue_dma(dma_ctx, ring_buf->head, ring_buf->chunk_size);
    _w_buf(ring_buf);
    _enqueue_dma(dma_ctx, ring_buf->head, ring_buf->chunk_size);
    //_w_buf(ring_buf);
#else
    // enqueue dma
    _enqueue_dma(dma_ctx, ring_buf->head, ring_buf->chunk_size);
    _w_buf(ring_buf);
#endif

	// enable read interface
    _enable_mpegts(1);

    // start wm8994_start
    //wm8994(ctx->wm8994);
    wm8994_start();
    VSTATION_DBG("%s exit\n", __func__);

    return 0;
}
static void stop_read_channel(struct vstation_ctx *ctx)
{
    VSTATION_DBG("%s entered\n", __func__);
    //wm8994_set_mclk(12000000, 100); // mclock disable
    _enable_mpegts(0);
    VSTATION_DBG("%s exit\n", __func__);
}

static void deinit_read_channel(struct vstation_ctx *ctx)
{
    struct dma_ctx *dma_ctx = &ctx->read_dma;

    _stop_dma(dma_ctx);

    free_irq(NX_MPEGTSIF_GetInterruptNumber(), ctx);
    free_irq(dma_ctx->irq, ctx);
    dma_ctx->irq = -1;

    _free_dma(dma_ctx);
    _deinit_dma(dma_ctx);

    stop_read_channel(ctx);

    _deinit_mpegts();
}

static int init_write_channel(struct vstation_ctx *ctx)
{
    int ret;
    struct dma_ctx *dma_ctx;
    struct ring_buf *ring_buf;
    unsigned long chunk_size;
    unsigned long chunk_count;

    VSTATION_DBG("%s\n", __func__);

    chunk_size = WRITE_CHANNEL_CHUNKSIZE;
    chunk_count = WRITE_CHANNEL_CHUNKCOUNT;

    // init i2s
    //i2s_init_device();

    // init write dma
    dma_ctx = &ctx->write_dma;
    memset(dma_ctx, 0, sizeof(struct dma_ctx));
    dma_ctx->type = DMA_TYPE_AUDIO_OUT;
    dma_ctx->ch = 3;
    ret = _init_dma(dma_ctx);
    if (ret < 0) {
        return ret;
    }

    // alloc write dma buf
    dma_ctx->dev = ctx->dev;
    ret = _alloc_dma(dma_ctx, chunk_size*chunk_count);
    if (ret < 0) {
        return ret;
    }

    // init write ring buf
    ring_buf = &s_ctx->write_buf;
    _init_ring_buf(ring_buf, (unsigned char *)dma_ctx->virt_base, dma_ctx->size, chunk_size);

    // data irq
//    ret = request_irq(dma_ctx->irq, _i2s_data_irq, IRQF_DISABLED, VSTATION_DEV_NAME, ctx);
    if (ret) {
        printk("Error: %s Failed to request_irq\n", __func__);
        return ret;
    }

    return 0;
}

static int start_write_channel(struct vstation_ctx *ctx)
{
    struct ring_buf *ring_buf = &ctx->read_buf;
    VSTATION_DBG("%s\n",__func__);
    _reset_ring_buf(ring_buf);
    //i2s_start_device();
    return 0;
}

static void stop_write_channel(struct vstation_ctx *ctx)
{
    VSTATION_DBG("%s\n",__func__);
    //i2s_stop_device();
}

static void deinit_write_channel(struct vstation_ctx *ctx)
{
    struct dma_ctx *dma_ctx = &ctx->write_dma;

    _stop_dma(dma_ctx);

    //free_irq(dma_ctx->irq, ctx);
    dma_ctx->irq = -1;

    _free_dma(dma_ctx);
    _deinit_dma(dma_ctx);

    //i2s_stop_device();
    //i2s_exit_device();
}

#define CFG_GPIO_CHECK_2121   (PAD_GPIO_B + 15)
static int check_2121(void)
{
    int io = CFG_GPIO_CHECK_2121;
    soc_gpio_set_io_func(io, NX_GPIO_PADFUNC_GPIO);
    soc_gpio_set_io_dir(io, 0);
    return soc_gpio_get_in_value(io) == 0;
}

static int vstation_open(struct inode *inode, struct file *filp)
{
    int ret;

    VSTATION_DBG("%s\n", __func__);

    if (!check_2121()) {
        printk("Fatal Error: %s This chip is not nxp2121\n", __func__);
        return -1;
    }

    if (s_ctx->is_opened) {
        printk("Error: %s VStation Device already opened\n", __func__);
        return -EBUSY;
    }

    ret = init_read_channel(s_ctx);
    if (ret < 0) {
        printk("Error: %s Failed to init_read_channel\n", __func__);
        return ret;
    }


    ret = init_write_channel(s_ctx);
    if (ret < 0) {
        printk("Error: %s Failed to init_write_channel\n", __func__);
        return ret;
    }

    filp->private_data = s_ctx;

    s_ctx->is_opened = 1;
    return 0;
}

static int vstation_close(struct inode *inode, struct file *filp)
{
    struct vstation_ctx *ctx = (struct vstation_ctx *)filp->private_data;

    VSTATION_DBG("%s\n", __func__);

    if (!s_ctx->is_opened) {
        printk("Error: %s not opened\n", __func__);
        return -1;
    }

    deinit_write_channel(ctx);
    deinit_read_channel(ctx);

    filp->private_data = NULL;
    s_ctx->is_opened = 0;
    return 0;
}

static int vstation_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct vstation_ctx *ctx = (struct vstation_ctx *)filp->private_data;

    //VSTATION_DBG("%s\n", __func__);

    switch (cmd) {
        case IOCTL_VSTATION_START_READ_CHANNEL:
        printk("IOCTL_VSTATION_START_READ_CHANNEL\n");
            // test for resume time
            set_gpio_resume_set(0);
            start_read_channel(ctx);
            break;
        case IOCTL_VSTATION_STOP_READ_CHANNEL:
            stop_read_channel(ctx);
            break;
        case IOCTL_VSTATION_GET_READ_OFFSET:
            {
                int ret;
                struct ring_buf *ring_buf = &ctx->read_buf;
//				printk("get avail = %4x\n",_get_avail_read_size(ring_buf));//bok test
                //if (_get_avail_read_size(ring_buf) < ring_buf->chunk_size) {
            #if (VSTATION_DOUBLE_BUFFER)
                if (_get_avail_read_size(ring_buf) >1 ) {
            #else
                if (_get_avail_read_size(ring_buf) < ring_buf->chunk_size) {
            #endif
#if 1
                    ret = interruptible_sleep_on_timeout(&ctx->read_wait, 1000); // org
                    if (ret == 0) {
                        // timeout
                        printk("Error: %s wait read channel timeout\n", __func__);
                        return -1;
                    }
#else
                    interruptible_sleep_on(&ctx->read_wait);
#endif
                }
                else
                    return -1;
//				printk("tail = %4x\n",ring_buf->tail);
                if (copy_to_user((void __user *)arg, (void *)&ring_buf->tail, sizeof(unsigned int))) {
                    return -EFAULT;
                }
                _r_buf(ring_buf);
            }
            break;
        case IOCTL_VSTATION_START_WRITE_CHANNEL:
            start_write_channel(ctx);
            break;
        case IOCTL_VSTATION_STOP_WRITE_CHANNEL:
            stop_write_channel(ctx);
            break;
        case IOCTL_VSTATION_SET_WRITE_OFFSET:
            {
                struct dma_ctx *dma_ctx = &ctx->write_dma;
                struct ring_buf *ring_buf = &ctx->write_buf;
                _w_buf(ring_buf);

                if (!soc_dma_check_run(dma_ctx->dma_tr)) {
                    _enqueue_dma(dma_ctx, ring_buf->tail, ring_buf->chunk_size);
                }
            }
            break;
    }

    return 0;
}

/* VMA operations */
static void vstation_vm_open(struct vm_area_struct *vma)
{
    VSTATION_DBG("%s\n", __func__);
}

static void vstation_vm_close(struct vm_area_struct *vma)
{
    VSTATION_DBG("%s\n", __func__);
}

static const struct vm_operations_struct vstation_vm_ops = {
    .open      = vstation_vm_open,
    .close     = vstation_vm_close,
};

// offset 0: read channel
// offset 4096: write channel
static int vstation_mmap(struct file *filp, struct vm_area_struct *vma)
{
    unsigned long offset, size;
    struct vstation_ctx *ctx = (struct vstation_ctx *)filp->private_data;
    struct dma_ctx *dma_ctx;
    int ret = 0;

    VSTATION_DBG("%s\n", __func__);

    offset = vma->vm_pgoff << PAGE_SHIFT;
    size = vma->vm_end - vma->vm_start;

    switch (offset) {
        case 0: // read channel
            VSTATION_DBG("%s: read channel\n", __func__);
            dma_ctx = &ctx->read_dma;
            break;
        case 4096: // write channel
            VSTATION_DBG("%s: write channel\n", __func__);
            dma_ctx = &ctx->write_dma;
            break;
        default:
            printk("Error: %s invalid offset 0x%lx\n", __func__, offset);
            return -1;
    }

    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
    ret = remap_pfn_range(vma, vma->vm_start, dma_ctx->phy_base >> PAGE_SHIFT, size, vma->vm_page_prot);
    if (ret) {
        printk("Error: %s failed to remap_pfn_range 0x%lx\n", __func__, dma_ctx->phy_base);
        return ret;
    }
    vma->vm_ops = &vstation_vm_ops;
    vma->vm_flags |= VM_DONTEXPAND;
    vma->vm_private_data = dma_ctx;
    vstation_vm_open(vma);

    return 0;
}

static struct file_operations vstation_fops = {
    .owner      = THIS_MODULE,
    .open       = vstation_open,
    .release    = vstation_close,
    .ioctl      = vstation_ioctl,
    .mmap       = vstation_mmap,
};

static struct miscdevice vstation_misc_device = {
    .minor      = MISC_DYNAMIC_MINOR,
    .name       = VSTATION_DEV_NAME,
    .fops       = &vstation_fops,
};


static int wm8994_i2c_probe(struct i2c_client *i2c, const struct i2c_device_id *id)
{
    struct wm8994 *wm8994;
    int ret;

    wm8994 = kzalloc(sizeof(struct wm8994), GFP_KERNEL);
    if (!wm8994) {
        kfree(i2c);
        printk(KERN_ERR "Error %s: wm8994 alloc failed\n", __func__);
        return -ENOMEM;
    }

    i2c_set_clientdata(i2c, wm8994);
    wm8994->dev = &i2c->dev;
    wm8994->control_data = i2c;
    wm8994->read_dev = wm8994_i2c_read_device;
    wm8994->write_dev = wm8994_i2c_write_device;

    mutex_init(&wm8994->io_lock);

    ret =  wm8994_device_init(wm8994);
    if (ret) {
        return ret;
    }

    s_ctx->wm8994 = wm8994;
    return 0;
}

static int wm8994_i2c_remove(struct i2c_client *i2c)
{
    struct wm8994 *wm8994 = i2c_get_clientdata(i2c);
    wm8994_device_exit(wm8994);
    return 0;
}

#ifdef CONFIG_PM
static int wm8994_i2c_suspend(struct i2c_client *client, pm_message_t mesg)
{
//    struct wm8994 *wm8994 = (struct wm8994 *)i2c_get_clientdata(client);
    VSTATION_DBG("%s entered\n", __func__);
    //_enable_mpegts(0);
    //wm8994_stop(wm8994);
    //wm8994_stop();
    //wm8994_set_mclk(12000000, 100); // mclock disable
    soc_pwm_set_suspend();
#if 0// ndef CONFIG_PLAT_NXP2120_WIFIAUDIO
    wm8994_ldo_control(0);
#endif
    VSTATION_DBG("%s exit\n", __func__);
    return 0;
}

static int wm8994_i2c_resume(struct i2c_client *client)
{
#if 1
    int ret;
    struct wm8994 *wm8994 = (struct wm8994 *)i2c_get_clientdata(client);
    VSTATION_DBG("%s entered\n", __func__);
    soc_pwm_set_resume();
    ret = wm8994_device_init(wm8994);
    _init_mpegts();
    VSTATION_DBG("%s exit(ret: %d)\n", __func__, ret);
    return ret;
#else
    VSTATION_DBG("%s entered\n", __func__);
#if 0 //ndef CONFIG_PLAT_NXP2120_WIFIAUDIO
    wm8994_ldo_control(1);
#endif
    soc_pwm_set_resume();
    wm8994_set_mclk(12000000, 50);
    VSTATION_DBG("%s exit\n", __func__);
    return 0;
#endif
}
#else
#define wm8994_i2c_suspend NULL
#define wm8994_i2c_resume NULL
#endif

static const struct i2c_device_id wm8994_i2c_id[] = {
    { "wm8994", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, wm8994_i2c_id);

static struct i2c_driver wm8994_i2c_driver = {
    .driver     = {
        .name   = "wm8994",
        .owner  = THIS_MODULE,
    },
    .probe      = wm8994_i2c_probe,
    .remove     = wm8994_i2c_remove,
    .suspend    = wm8994_i2c_suspend,
    .resume     = wm8994_i2c_resume,
    .id_table   = wm8994_i2c_id,
};

/* wm8994 codec i2c device */
#if 0
#define WM8994_I2C_BUS 0
static struct i2c_board_info wm8994_i2c_boardinfo[] = {
    {
        I2C_BOARD_INFO("wm8994", 0x34>>1),
    },
};
#endif
#if 0
/* NFC gpio setting */
#define CFG_GPIO_NFC_LDO_EN   (PAD_GPIO_C + 5) // low to high
#define CFG_GPIO_NFC_DOWNLOAD (PAD_GPIO_A + 7) // high to low
#define CFG_GPIO_NFC_VEN      (PAD_GPIO_A + 6) // low to high
static inline void set_nfc_gpio_default(void)
{
    soc_gpio_set_out_value(CFG_GPIO_NFC_LDO_EN, 1);
    soc_gpio_set_out_value(CFG_GPIO_NFC_DOWNLOAD, 0);
    mdelay(100); // 100ms delay
    soc_gpio_set_out_value(CFG_GPIO_NFC_VEN, 1);
}

#endif
/* platform driver */
static int vstation_probe(struct platform_device *pdev)
{
    int ret;
    VSTATION_DBG("%s\n", __func__);

#if 0
    i2c_register_board_info(WM8994_I2C_BUS, wm8994_i2c_boardinfo, 1);
    ret = i2c_add_driver(&wm8994_i2c_driver);
    if (ret != 0) {
       printk("Error %s: Failed to i2c_add_driver wm8994\n", __func__);
        return ret;
    }
#endif
    ret = misc_register(&vstation_misc_device);
    if (ret) {
        printk("Error %s: Failed to misc_register\n", __func__);
        i2c_del_driver(&wm8994_i2c_driver);
        return ret;
    }

    s_ctx = (struct vstation_ctx *)kzalloc(sizeof(struct vstation_ctx), GFP_KERNEL);
    if (!s_ctx) {
        printk("Error: %s Failed to kzalloc vstation_ctx\n", __func__);
        return -ENOMEM;
    }

    s_ctx->dev = &pdev->dev;

   wm8994_set_mclk(12000000, 50);
   //wm8994_reg_init();
#if 0 //ndef CONFIG_PLAT_NXP2120_WIFIAUDIO
    set_nfc_gpio_default();
#endif

    return ret;
}

static int vstation_remove(struct platform_device *pdev)
{
    VSTATION_DBG("%s\n", __func__);
    i2c_del_driver(&wm8994_i2c_driver);
    return 0;
}

static struct platform_driver vstation_plat_driver = {
    .probe      = vstation_probe,
    .remove     = vstation_remove,
    .driver     = {
        .owner  = THIS_MODULE,
        .name   = VSTATION_DEV_NAME,
    },
};

/* platform device */
static struct platform_device vstation_plat_device = {
    .name       = VSTATION_DEV_NAME,
    .id         = 0,
};

/* entry point */
static int __init vstation_init(void)
{
    VSTATION_DBG("%s\n", __func__);

    platform_device_register(&vstation_plat_device);
    return platform_driver_register(&vstation_plat_driver);
}

static void __exit vstation_exit(void)
{
    VSTATION_DBG("%s\n", __func__);
    platform_driver_unregister(&vstation_plat_driver);
}

module_init(vstation_init);
module_exit(vstation_exit);

MODULE_DESCRIPTION("Nexell VSTATION driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nexell swpark@nexell.co.kr");
MODULE_ALIAS("platform:nexell-vstation");

