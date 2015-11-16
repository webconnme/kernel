/*
 * (C) Copyright 2010
 * jung hyun kim, Nexell Co, <jhkim@nexell.co.kr>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>

/* nexell soc headers */
#include <mach/platform.h>
#include <mach/devices.h>

/*------------------------------------------------------------------------------
 * Network DM9000
 */
#if defined(CONFIG_DM9000) || defined(CONFIG_DM9000_MODULE)
#include <linux/dm9000.h>

static struct resource dm9000_resource[] = {
	[0] = {
		.start	= CFG_EXT_PHY_BASEADDR_ETHER,
		.end	= CFG_EXT_PHY_BASEADDR_ETHER + 3,
		.flags	= IORESOURCE_MEM
	},
	[1] = {
		.start	= CFG_EXT_PHY_BASEADDR_ETHER + 0x4,
		.end	= CFG_EXT_PHY_BASEADDR_ETHER + 0x4 + 500,
		.flags	= IORESOURCE_MEM
	},
	[2] = {
		.start	= CFG_EXT_IRQ_NUM_ETHER,
		.end	= CFG_EXT_IRQ_NUM_ETHER,
		.flags	= IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHEDGE,
	}
};

static struct dm9000_plat_data dm9000_platdata = {
	.flags		= DM9000_PLATF_8BITONLY,
};

static struct platform_device dm9000_plat_device = {
	.name			= DM9000_DEV_NAME,
	.id				= 0,
	.num_resources	= ARRAY_SIZE(dm9000_resource),
	.resource		= dm9000_resource,
	.dev			= {
		.platform_data	= &dm9000_platdata,
	}
};
#endif	/* CONFIG_DM9000 || CONFIG_DM9000_MODULE */

#if defined(CONFIG_AX88796B) || defined(CONFIG_AX88796B_MODULE)
/* AX88796B LAN via ROM interface */
static struct resource ax88796b_resources[] = {
	[0] = DEFINE_RES_MEM(CFG_EXT_PHY_BASEADDR_ETHER, SZ_256),
	[1] = DEFINE_RES_IRQ(CFG_EXT_IRQ_NUM_ETHER),
};

struct platform_device ax88796b_plat_device = {
	.name           = "ax88796b",
	.id     		= 0,	// -1
	.num_resources  = ARRAY_SIZE(ax88796b_resources),
	.resource       = ax88796b_resources,
};

#endif  /* CONFIG_AX88796B || CONFIG_AX88796B_MODULE */

/*------------------------------------------------------------------------------
 * Frame Buffer platform device
 */
#if defined(CONFIG_FB_NEXELL)

/* fb0 */
#if defined(CONFIG_FB_NEXELL_PRIMARY)
static struct fb_plat_data fb0_platdata = {
	.module		= MAIN_SCREEN_PRI == 0 ? 0 : 1,
	.layer		= CFG_DISP_LAYER_SCREEN,
	.x_res		= CFG_DISP_PRI_RESOL_WIDTH,
	.y_res		= CFG_DISP_PRI_RESOL_HEIGHT,
	.pixelbit	= CFG_DISP_SCREEN_PIXEL_BYTE * 8,
	.format		= CFG_DISP_SCREEN_RGB_FORMAT,
#ifdef CONFIG_ANDROID
	.buffers	= 2,
#else
	.buffers	= 2,
#endif
	.with_mm	= 152.4,
	.height_mm	=  91.44,
	.hs_left	= CFG_DISP_PRI_HSYNC_SYNC_WIDTH + CFG_DISP_PRI_HSYNC_BACK_PORCH,
	.hs_right	= CFG_DISP_PRI_HSYNC_FRONT_PORCH,
	.vs_upper	= CFG_DISP_PRI_VSYNC_SYNC_WIDTH + CFG_DISP_PRI_VSYNC_BACK_PORCH,
	.vs_lower	= CFG_DISP_PRI_VSYNC_FRONT_PORCH,
	.pixelclock	= CFG_DISP_PRI_PIXEL_CLOCK,
};

static struct platform_device fb0_plat_device = {
	.name	= FB_DEV_NAME,
	.id		= 0,	/* device channel */
	.dev    = {
		.coherent_dma_mask 	= 0xffffffffUL,	/* for DMA allocate */
		.platform_data		= &fb0_platdata
	},
};
#endif

static struct platform_device *fb_plat_devices[] = {
#if defined(CONFIG_FB_NEXELL_PRIMARY)
		&fb0_plat_device,
#endif
};
#endif	/* CONFIG_FB_NEXELL */

/*------------------------------------------------------------------------------
 * V4L2 platform device
 */
#if defined(CONFIG_VIDEO_NEXELL)
static struct v4l2_plat_data v4l2_platdata = {
	.in_port		= 1,
	.dpc_device		= MAIN_SCREEN_PRI,
	.hor_align		= 1,
	.ver_align		= 1,
	.buff_count		= 4,
	.skip_count		= 3,
};

static struct platform_device v4l2_plat_device = {
	.name	= V4L2_DEV_NAME,
	.id		= 0,	/* 0, 1, ... */
	.dev    = {
		.platform_data	= &v4l2_platdata,
	},
};
#endif	/* CONFIG_VIDEO_NEXELL */

/*------------------------------------------------------------------------------
 * VIP platform device
 */
#if defined(CONFIG_GRP_NEXELL_VIP) || defined(CONFIG_GRP_NEXELL_VIP_MODULE)

#if	defined(CONFIG_GRP_NEXELL_VIP_MODULE0)
static struct platform_device vip_plat_device0 = {
	.name	= VIP_DEV_NAME,
	.id		= 0,
};
#endif

#if	defined(CONFIG_GRP_NEXELL_VIP_MODULE1)
static struct platform_device vip_plat_device1 = {
	.name	= VIP_DEV_NAME,
	.id		= 1,
};
#endif

static struct platform_device *vip_plat_devices[] = {
#if	defined(CONFIG_GRP_NEXELL_VIP_MODULE0)
	&vip_plat_device0,
#endif
#if	defined(CONFIG_GRP_NEXELL_VIP_MODULE1)
	&vip_plat_device1,
#endif
};
#endif	/* CONFIG_GRP_NEXELL_VIP || CONFIG_GRP_NEXELL_VIP_MODULE */

/*------------------------------------------------------------------------------
 * SDHC platform device
 */
#if defined(CONFIG_MMC_NEXELL_SDHC)
static struct sdhc_plat_data sdhc0_platdata = {
	.io_detect		= CFG_PIO_SDHC_0_DETECT,
	.io_wprotect	= CFG_PIO_SDHC_0_WP,
};

struct platform_device sdhc0_plat_device = {
	.name	= SDHC_DEV_NAME,
	.id		= 0,	/* device channel */
	.dev    = {
		.platform_data	= &sdhc0_platdata,
	},
};
#endif	/* CONFIG_MMC_NEXELL_SDHC */

/*------------------------------------------------------------------------------
 * Touch platform device
 */
#if	defined(CONFIG_TOUCHSCREEN_NEXELL_ADC) || defined(CONFIG_TOUCHSCREEN_NEXELL_ADC_MODULE)
static struct adc_ts_plat_data ts_platdata = {
	.io =  {
		.xm_p		= CFG_PIO_TOUCH_XMON_P,
		.xm_n		= CFG_PIO_TOUCH_XMON_N,
		.ym_p		= CFG_PIO_TOUCH_YMON_P,
		.ym_n		= CFG_PIO_TOUCH_YMON_N,
		.detect		= CFG_PIO_TOUCH_PENDOWN_DETECT,
		.control	= CFG_PIO_TOUCH_PENDOWN_CON,
		.delay		= 1,
	},
	.adc = {
		.x_ch		= CFG_TOUCH_X_ADC_CH,
		.y_ch		= CFG_TOUCH_Y_ADC_CH,
		.x_min	 	= 0,
		.x_max	 	= CFG_DISP_PRI_RESOL_WIDTH,		/* default 1023 */
		.y_min	 	= 0,
		.y_max	 	= CFG_DISP_PRI_RESOL_HEIGHT,	/* deafult 1023 */
		.pointercal = CFG_TOUCH_CALIBRATION, 		/* CFG_TOUCH_CALIBRATION, { 0,0,0,0,0,0,0 } */
	},
};

static struct platform_device ts_plat_device = {
	.name	= TS_DEV_NAME,
	.id		= -1,
	.dev    = {
		.platform_data	= &ts_platdata
	},
};
#endif	/* CONFIG_TOUCHSCREEN_NEXELL_ADC || CONFIG_TOUCHSCREEN_NEXELL_ADC_MODULE */
/*------------------------------------------------------------------------------
 * Keypad platform device
 */
#if defined(CONFIG_KEYBOARD_NEXELL_KEY) || defined(CONFIG_KEYBOARD_NEXELL_KEY_MODULE)

#include <linux/input.h>

static unsigned int  button_gpio[] = CFG_KEYPAD_KEY_BUTTON;
static unsigned int  button_code[] = CFG_KEYPAD_KEY_CODE;

struct key_plat_data key_platdata = {
	.bt_count	= ARRAY_SIZE(button_gpio),
	.bt_io		= button_gpio,
	.bt_code	= button_code,
};

static struct platform_device key_plat_device = {
	.name	= KEYPAD_DEV_NAME,
	.id		= -1,
	.dev    = {
		.platform_data	= &key_platdata
	},
};
#endif	/* CONFIG_KEYBOARD_NEXELL_KEY || CONFIG_KEYBOARD_NEXELL_KEY_MODULE */

/*------------------------------------------------------------------------------
 * SPI platform device
 */


#if defined(CONFIG_SPI_NEXELL) || defined(CONFIG_SPI_NEXELL_MODULE)
static struct spi_io_cs spi_board_cs [] = {
	{CFG_PIO_SPI0_FRAME, NX_GPIO_PADFUNC_GPIO}
};
static struct spi_plat_data spi_plat_data = {
	.port = 0,
	.irq = IRQ_PHY_SSPSPI0,
	.io_clk = CFG_PIO_SPI0_CLOCK,
	.io_rx = CFG_PIO_SPI0_RX,
	.io_tx = CFG_PIO_SPI0_TX,
	.io_cs = spi_board_cs,
	.cs_num = ARRAY_SIZE(spi_board_cs),
	.format = NX_SSPSPI_FORMAT_A,

	.clk_src = CFG_SPI_CLK_SRC,
	.clk_divisor = CFG_SPI_CLOCK_DIVISOR,
	.clk_divcount = CFG_SPI_CLOCK_DIVCOUNT,
	
	.dma_tx_channel = CFG_DMA_SPI_TX,
	.dma_rx_channel = CFG_DMA_SPI_RX,
};
static struct platform_device spi_plat_device = {
	.name	= SPI_DEV_NAME,
	.id		= 0,
	.dev	= {
		.platform_data = &spi_plat_data
	},
};
#endif	/* CONFIG_SPI_NEXELL || CONFIG_SPI_NEXELL_MODULE */

#if defined(CONFIG_SPI_SPIDEV) || defined(CONFIG_SPI_SPIDEV_MODULE)

#include <linux/spi/spi.h>

static struct spi_board_info spi_plat_board[] __initdata = {
	[0] = {
		.modalias 		 = "spidev",	/* fixup */
		.max_speed_hz 	 = 3125000,     /* max spi clock (SCK) speed in HZ */
		.bus_num 		 = 0,			/* Note> set bus num, must be smaller than ARRAY_SIZE(spi_plat_device) */
		.chip_select 	 = 0,			/* Note> set chip select num, must be smaller than spi cs_num */
	},
};
#endif

/*------------------------------------------------------------------------------
 * Three-Axis sensor(LIS3XXX) platform device
 */
#if defined(CONFIG_SENSORS_LIS3_I2C) || defined(CONFIG_SENSORS_LIS3_I2C_MODULE)

#include <linux/lis3lv02d.h>
#include <linux/i2c.h>

static struct lis3lv02d_platform_data lis3_platdata = {
	.click_flags 	= LIS3_CLICK_SINGLE_X | LIS3_CLICK_SINGLE_Y | LIS3_CLICK_SINGLE_Z,	/* reserved in LIS302DL */
	.irq_cfg		= LIS3_IRQ1_CLICK | LIS3_IRQ2_CLICK,

	.wakeup_flags	= LIS3_WAKEUP_X_LO | LIS3_WAKEUP_X_HI |
			  		  LIS3_WAKEUP_Y_LO | LIS3_WAKEUP_Y_HI |
			  		  LIS3_WAKEUP_Z_LO | LIS3_WAKEUP_Z_HI,
	.wakeup_thresh	= 10,
	.click_thresh_x = 10,	/* reserved in LIS302DL */
	.click_thresh_y = 10,	/* reserved in LIS302DL */
	.click_thresh_z = 10,	/* reserved in LIS302DL */
};

#define	LIS3_I2C_BUS		0

static struct i2c_board_info lis3_i2c_boardinfo = {
	.type			= "lis3lv02d",
	.addr			= 0x3A>>1,
	.platform_data	= &lis3_platdata,
	.irq			= PB_PIO_IRQ(PAD_GPIO_C + 10)
};
#endif

/*------------------------------------------------------------------------------
 * PCF857X I2C device
 */
#if defined(CONFIG_GPIO_PCF857X)
#include <linux/i2c/pcf857x.h>
#include <mach/nxp2120.h>

#define PCF857X_I2C_BUS		0
static struct pcf857x_platform_data pcf_platdata = {
	.gpio_base		= PHY_BASEADDR_GPIO,
	.n_latch		= (1<<7) | (1<<6) | (1<<5) | (1<<4) | (1<<3) | (1<<2) | (1<<1) | (1<<0),
};

static struct i2c_board_info pcf857x_i2c_boardinfo = {
		I2C_BOARD_INFO("pcf8574", 0x20),
		.platform_data	= &pcf_platdata,
		.irq			= PB_PIO_IRQ(PAD_GPIO_A + 28)
};
#endif

/*------------------------------------------------------------------------------
 * MPEGTSIF RAONTECH ISDBT Module i2c device
 */
#if defined(CONFIG_MISC_NEXELL_MPEGTS) && defined(CONFIG_RAONTV_MTV818)
//#define RAONTV_I2C_BUS		0
#define RAONTV_I2C_BUS		1
static struct i2c_board_info raontv_i2c_boardinfo[] = {
	{
		I2C_BOARD_INFO("mtvi2c", 0x86>>1),
	},
};
#endif

/*------------------------------------------------------------------------------
 * V4L2 platform device
 */
#if defined(CONFIG_VIDEO_V4L2) || defined(CONFIG_VIDEO_V4L2_MODULE)

#include <linux/i2c.h>
#include <media/soc_camera.h>
#include <media/ov772x.h>
#include <media/nxp-camera-platformdata.h>

static struct i2c_board_info nxp2120_dtk_i2c_cam_info[] = {
	{
		I2C_BOARD_INFO("ov772x", 0x42>>1),
	},
};

static struct ov772x_camera_info ov7725_info = {
	.buswidth	= SOCAM_DATAWIDTH_8,
	.flags		= 0,
	.edgectrl	= OV772X_AUTO_EDGECTRL(0x3, 0),
	.link = {
		.bus_id		= 0,
		.board_info	= &nxp2120_dtk_i2c_cam_info[0],
		.i2c_adapter_id	= 0,
		.module_name	= "ov772x",
	},
};

static struct platform_device nxp_soc_camera = {
	.name       = "soc-camera-pdrv",
	.id         = -1,
	.dev        = {
		.platform_data = &ov7725_info.link,
	},
};

static struct nxp_camera_platform_data ov772x_camera_data = { 
	.video_type     = 0,
	.interlace      = 0,
	.external_sync  = 0,
	.h_active       = 640,
	.h_frontporch   = 7,
	.h_syncwidth    = 1,
	.h_backporch    = 1,
	.v_active       = 480,
	.v_frontporch   = 1,
	.v_syncwidth    = 1,
	.v_backporch    = 1,
	.data_order     = 2,
	.clock_invert   = 0,
	.framepersec    = 30, 
	.ext_field_sig  = 0,
	.ext_field_sel  = 0,
	.ext_dvalid_sig = 0,
	.ext_dvalid_pol = 0,
	.field_invert   = 0 
};

static struct platform_device nxp_camera_dev = { 
	.name     = "nxp-camera",
	.id       = 0, 
	.dev      = { 
		.platform_data = &ov772x_camera_data,
	},  
};

#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
/*------------------------------------------------------------------------------
 * ASoC (PS8738_8106) platform device
 */
#if defined(CONFIG_SND_CODEC_PS8738_8106) || defined(CONFIG_SND_CODEC_PS8738_8106_MODULE)
#include <linux/i2c.h>

#define	PS8XXX_I2C_BUS		(0)

static struct i2c_board_info __initdata ps8xxx_i2c_bdi[] = {
	{
		.type	= "ps8738",
		.addr	= (0x38),
	},
	{
		.type	= "ps8106",
		.addr	= (0x3A),				/* ps8106: depend SA0 level */
	}
};
#endif

#endif	/* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37) */

#if defined(CONFIG_MISC_NEXELL_DIBDXB)
static struct platform_device dib_dxb_plat_device = {
	.name	= DXB_DEV_NAME,
	.id	= 0,
};
#endif
/*------------------------------------------------------------------------------
 * register board platform devices
 */
void __init board_device(void)
{
#if defined(CONFIG_DM9000) || defined(CONFIG_DM9000_MODULE)
	printk("plat: add device dm9000 net\n");
	platform_device_register(&dm9000_plat_device);
#endif

#if defined(CONFIG_AX88796B) || defined(CONFIG_AX88796B_MODULE)
	printk("plat: add device ax88796b net\n");
	platform_device_register(&ax88796b_plat_device);
#endif

#if defined(CONFIG_FB_NEXELL)
	printk("plat: add device frame buffer\n");
	platform_add_devices(fb_plat_devices, ARRAY_SIZE(fb_plat_devices));
#endif

#if defined(CONFIG_VIDEO_NEXELL)
	printk("plat: add device v4l2 camera\n");
	platform_device_register(&v4l2_plat_device);
#endif

#if defined(CONFIG_GRP_NEXELL_VIP) || defined(CONFIG_GRP_NEXELL_VIP_MODULE)
	printk("plat: add device vip\n");
	platform_add_devices(vip_plat_devices, ARRAY_SIZE(vip_plat_devices));
#endif

#if defined(CONFIG_MMC_NEXELL_SDHC)
	printk("plat: add device sdhc\n");
	platform_device_register(&sdhc0_plat_device);
#endif

#if defined(CONFIG_TOUCHSCREEN_NEXELL_ADC) || defined(CONFIG_TOUCHSCREEN_NEXELL_ADC_MODULE)
	printk("plat: add device touchscreen\n");
	platform_device_register(&ts_plat_device);
#endif

#if defined(CONFIG_KEYBOARD_NEXELL_KEY) || defined(CONFIG_KEYBOARD_NEXELL_KEY_MODULE)
	printk("plat: add device keypad\n");
	platform_device_register(&key_plat_device);
#endif


#if defined(CONFIG_SPI_NEXELL) || defined(CONFIG_SPI_NEXELL_MODULE)
	printk("plat: add device spi\n");
	platform_device_register(&spi_plat_device);
#endif

#if defined(CONFIG_SPI_SPIDEV) || defined(CONFIG_SPI_SPIDEV_MODULE)
	spi_register_board_info(spi_plat_board, ARRAY_SIZE(spi_plat_board));
	printk("plat: register spidev\n");
#endif

#if defined(CONFIG_MISC_NEXELL_DIBDXB)
	printk("plat: add device misc dxb\n");
	platform_device_register(&dib_dxb_plat_device);
#endif

#if defined(CONFIG_SENSORS_LIS3_I2C) || defined(CONFIG_SENSORS_LIS3_I2C_MODULE)
	printk("plat: register three-axis sensor(LIS3XXX)\n");
	i2c_register_board_info(LIS3_I2C_BUS, &lis3_i2c_boardinfo, 1);
#endif

#if defined(CONFIG_MISC_NEXELL_MPEGTS) && defined(CONFIG_RAONTV_MTV818)
	printk("plat: register raontv isdbt module(MTV818)\n");
	i2c_register_board_info(RAONTV_I2C_BUS, raontv_i2c_boardinfo, 1);
#endif

#if defined(CONFIG_GPIO_PCF857X)
	printk("plat: register pcf857x \n");
	i2c_register_board_info(PCF857X_I2C_BUS, &pcf857x_i2c_boardinfo, 1);
#endif

#if defined(CONFIG_VIDEO_V4L2) || defined(CONFIG_VIDEO_V4L2_MODULE)
	printk("plat: add v4l2 soc camera\n");
	platform_device_register(&nxp_soc_camera);
	printk("plat: add v4l2 nxp camera\n");
	platform_device_register(&nxp_camera_dev);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
#if defined(CONFIG_SND_CODEC_PS8738_8106) || defined(CONFIG_SND_CODEC_PS8738_8106_MODULE)
	printk("plat: register asoc-ps8xxx\n");
	i2c_register_board_info(PS8XXX_I2C_BUS, ps8xxx_i2c_bdi, ARRAY_SIZE(ps8xxx_i2c_bdi));
#endif
#endif
}
