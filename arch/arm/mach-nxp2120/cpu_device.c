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

#include <linux/sizes.h>
#include <linux/dma-mapping.h>

/* nexell soc headers */
#include <mach/platform.h>
#include <mach/devices.h>


/*------------------------------------------------------------------------------
 * Serial platform device
 */
#if defined(CONFIG_SERIAL_NEXELL_UART)

#if	defined(CONFIG_SERIAL_NEXELL_UART_PORT0)
static struct platform_device uart_plat_device0 = {
	.name	= UART_DEV_NAME,
	.id		= 0,
};
#endif
#if	defined(CONFIG_SERIAL_NEXELL_UART_PORT1)
static struct platform_device uart_plat_device1 = {
	.name	= UART_DEV_NAME,
	.id		= 1,
};
#endif

static struct platform_device *uart_plat_devices[] = {
#if	defined(CONFIG_SERIAL_NEXELL_UART_PORT0)
	&uart_plat_device0,
#endif
#if	defined(CONFIG_SERIAL_NEXELL_UART_PORT1)
	&uart_plat_device1,
#endif
};
#endif /* CONFIG_SERIAL_NEXELL_UART */

/*------------------------------------------------------------------------------
 * USB EHCI platform device
 */
#if defined(CONFIG_USB_EHCI_HCD) ||	defined(CONFIG_USB_EHCI_HCD_MODULE)

/*
static struct resource ehci_resources[] = {
	[0] = {
		.start  = PHY_BASEADDR_EHCI,
		.end    = PHY_BASEADDR_EHCI + 0x00000400 -1,
		.flags  = IORESOURCE_IO,
	},
	[1] = {
		.start  = IRQ_PHY_EHCI,
		.end    = IRQ_PHY_EHCI,
		.flags  = IORESOURCE_IRQ,
	},
};

static u64 ehci_dmamask = 0xffffffffUL;

static struct platform_device ehci_plat_device = {
	.name	= EHCI_DEV_NAME,
	.id		= 0,
	.dev    = {
		.dma_mask = &ehci_dmamask,
		.coherent_dma_mask = 0xffffffffUL
	},
	.num_resources  = ARRAY_SIZE(ehci_resources),
	.resource       = ehci_resources,
};
*/

static struct resource ehci_resources[] = {
	[0] = DEFINE_RES_MEM(PHY_BASEADDR_EHCI, SZ_1K),
	[1] = DEFINE_RES_IRQ(IRQ_PHY_EHCI),
};

static u64 ehci_dmamask = 0xffffffffUL;

struct platform_device ehci_plat_device = {
	.name		= EHCI_DEV_NAME,
	.id		= -1,
	.num_resources	= ARRAY_SIZE(ehci_resources),
	.resource	= ehci_resources,
	.dev		= {
		.dma_mask		=  &ehci_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};

#endif	/* CONFIG_USB_EHCI_HCD || CONFIG_USB_EHCI_HCD_MODULE */

/*------------------------------------------------------------------------------
 * USB OHCI platform device
 */
#if defined(CONFIG_USB_OHCI_HCD) ||	defined(CONFIG_USB_OHCI_HCD_MODULE)

static struct resource ohci_resources[] = {
	[0] = {
		.start  = PHY_BASEADDR_OHCI,
		.end    = PHY_BASEADDR_OHCI + 0x00000400 - 1,
		.flags  = IORESOURCE_IO,
	},
	[1] = {
		.start  = IRQ_PHY_OHCI,
		.end    = IRQ_PHY_OHCI,
		.flags  = IORESOURCE_IRQ,
	},
};

static u64 ohci_dmamask = 0xffffffffUL;

static struct platform_device ohci_plat_device = {
	.name	= OHCI_DEV_NAME,
	.id		= 0,
	.dev    = {
		.dma_mask = &ohci_dmamask,
		.coherent_dma_mask = 0xffffffffUL
	},
	.num_resources  = ARRAY_SIZE(ohci_resources),
	.resource       = ohci_resources,
};

#endif	/* CONFIG_USB_OHCI_HCD || CONFIG_USB_OHCI_HCD_MODULE */

/*------------------------------------------------------------------------------
 * USB Gadget platform device
 */
#if defined(CONFIG_USB_GADGET_NEXELL)

static struct resource gadget_resources[] = {
	[0] = {
		.start  = PHY_BASEADDR_UDC,
		.end    = PHY_BASEADDR_UDC + 0x00000C00,
		.flags  = IORESOURCE_IO,
	},
	[1] = {
		.start  = IRQ_PHY_UDC,
		.end    = IRQ_PHY_UDC,
		.flags  = IORESOURCE_IRQ,
	},
};

static u64 gadget_dmamask = 0xffffffffUL;

static struct platform_device gadget_plat_device = {
	.name	= GADGET_DEV_NAME,
	.id		= -1,
	.dev    = {
		.dma_mask = &gadget_dmamask,
		.coherent_dma_mask = 0xffffffffUL
	},
	.num_resources  = ARRAY_SIZE(gadget_resources),
	.resource       = gadget_resources,
};
EXPORT_SYMBOL(gadget_plat_device);

/* android usb function platform device */
#if defined(CONFIG_USB_ANDROID)

#include <linux/usb/android_composite.h>
static char *usb_functions[] = { "usb_mass_storage" };
//static char *usb_functions_adb[] = { "usb_mass_storage", "adb" };
static char *usb_functions_adb[] = { "adb" };

static struct android_usb_product usb_products[] = {
#if 0
	{
		.product_id     = 0x0d01,
		.num_functions  = ARRAY_SIZE(usb_functions),
		.functions      = usb_functions,
	},
#endif
	{
		.product_id     = 0x0d02,
		.num_functions  = ARRAY_SIZE(usb_functions_adb),
		.functions      = usb_functions_adb,
	},
};

/* standard android USB platform data */
static struct android_usb_platform_data andusb_plat = {
	.vendor_id              = 0x18d1,
	.product_id             = 0x0d02,
	.manufacturer_name      = "NEXELL",
	.product_name           = "nxp2120",
	.serial_number          = NULL,
	.num_products = ARRAY_SIZE(usb_products),
	.products = usb_products,
    // psw0523 fix for create3_2
	.num_functions = ARRAY_SIZE(usb_functions_adb),
	//num_functions = 1,
	.functions = usb_functions_adb,
};

static struct platform_device androidusb_device = {
	.name   = "android_usb",
	.id     = -1,
	.dev    = {
		.platform_data  = &andusb_plat,
	},
};

/* mass storage device */
#if defined(CONFIG_USB_ANDROID_MASS_STORAGE)
static struct usb_mass_storage_platform_data mass_storage_pdata = {
	.nluns		= 1,
	.vendor		= "CREATE",
	.product	= "Android MID",
	.release	= 0x0100,
};

static struct platform_device usb_mass_storage_device = {
	.name	= "usb_mass_storage",
	.id	= -1,
	.dev	= {
		.platform_data = &mass_storage_pdata,
	},
};
#endif /* CONFIG_USB_ANDROID_MASS_STORAGE */

#endif /* CONFIG_USB_ANDROID */

#endif	/* CONFIG_USB_GADGET_NEXELL */

/*------------------------------------------------------------------------------
 * I2C Bus platform device
 */
#if defined(CONFIG_I2C_NEXELL)

#if	defined(CONFIG_I2C_NEXELL_PORT0)
static struct i2c_plat_data i2c0_platdata = {
	.port		= 0,
	.irq		= IRQ_PHY_I2C0,
	.io_scl		= CFG_PIO_I2C0_SCL,
	.io_sda		= CFG_PIO_I2C0_SDA,
	.clksrc 	= CFG_I2C0_CLOCK_SOURCE,
	.clkscale 	= CFG_I2C0_CLOCK_SCALER,
	.cntmax 	= CFG_I2C0_CLOCK_DELAY,
};

static struct platform_device i2c_plat_device0 = {
	.name	= I2C_DEV_NAME,
	.id		= 0,
	.dev    = {
		.platform_data	= &i2c0_platdata
	},
};
#endif
#if	defined(CONFIG_I2C_NEXELL_PORT1)
static struct i2c_plat_data i2c1_platdata = {
	.port		= 1,
	.irq		= IRQ_PHY_I2C1,
	.io_scl		= CFG_PIO_I2C1_SCL,
	.io_sda		= CFG_PIO_I2C1_SDA,
	.clksrc 	= CFG_I2C1_CLOCK_SOURCE,
	.clkscale 	= CFG_I2C1_CLOCK_SCALER,
	.cntmax 	= CFG_I2C1_CLOCK_DELAY,
};

static struct platform_device i2c_plat_device1 = {
	.name	= I2C_DEV_NAME,
	.id		= 1,
	.dev    = {
		.platform_data	= &i2c1_platdata
	},
};
#endif

static struct platform_device *i2c_plat_devices[] = {
#if	defined(CONFIG_I2C_NEXELL_PORT0)
	&i2c_plat_device0,
#endif
#if	defined(CONFIG_I2C_NEXELL_PORT1)
	&i2c_plat_device1,
#endif
};

#endif	/* CONFIG_I2C_NEXELL */

/*------------------------------------------------------------------------------
 * RTC (Real Time Clock) platform device
 */
#if defined(CONFIG_RTC_DRV_NEXELL)
static struct platform_device rtc_plat_device = {
	.name	= RTC_DEV_NAME,
	.id		= 0,
};
#endif	/* CONFIG_RTC_DRV_NEXELL */

/*------------------------------------------------------------------------------
 * WatchDog platform device
 */
#if defined(CONFIG_NEXELL_WATCHDOG)
static struct platform_device wdt_plat_device = {
	.name	= WDT_DEV_NAME,
	.id		= -1,
};
#endif /* CONFIG_NEXELL_WATCHDOG */

/*------------------------------------------------------------------------------
 * Graphic VMEM platform device
 */
#if defined(CONFIG_GRP_NEXELL_VMEM)

#if defined(CONFIG_GRP_NEXELL_VMEM_KILLER)
static char * task_name[] = CFG_VMEM_KILLER_RESERVE_TASK;

struct vmem_plat_data vmem_platdata = {
	.resv_task_num	= ARRAY_SIZE(task_name),
	.resv_task_name	= task_name,
};
#endif

static struct platform_device vmem_plat_device = {
	.name	= VMEM_DEV_NAME,
	.id		= 0,
#if defined(CONFIG_GRP_NEXELL_VMEM_KILLER)
	.dev    = {
		.platform_data	= &vmem_platdata
	},
#endif
};
#endif 	/* CONFIG_GRP_NEXELL_VMEM && CONFIG_GRP_NEXELL_VMEM_KILLER */

/*------------------------------------------------------------------------------
 * Graphic DPC platform device
 */
#if defined(CONFIG_GRP_NEXELL_DPC)
static struct platform_device dpc_plat_device = {
	.name	= DPC_DEV_NAME,
	.id		= 0,
};
#endif	/* CONFIG_GRP_NEXELL_DPC */

/*------------------------------------------------------------------------------
 * Fine Scaler
 */
#if defined(CONFIG_NEXELL_FINESCALER)
static struct platform_device finescaler_plat_device = {
	.name	= SCALER_DEV_NAME,
	.id		= 0,
};
#endif	/* CONFIG_NEXELL_FINESCALER */

/*------------------------------------------------------------------------------
 * ALSA PCM/I2S device
 */
#if defined(CONFIG_SND_NEXELL_SOC_I2S) || defined(CONFIG_SND_NEXELL_SOC_I2S_MODULE)
static struct i2s_plat_data i2s_platdata = {
	.master			= CFG_AUDIO_I2S_MASTER_MODE,
	.syncbit		= CFG_AUDIO_I2S_SYNC_PERIOD,
	.pcmoutw		= CFG_AUDIO_I2S_PCM_OUT_WIDTH,
	.pcminw			= CFG_AUDIO_I2S_PCM_IN_WIDTH,
	.transmode		= CFG_AUDIO_I2S_TRANS_MODE,
	.samplerate		= CFG_AUDIO_I2S_SAMPLE_RATES,

	.clksrc0		= CFG_AUDIO_I2S_CLK_SRC_0,
	.clkdiv0		= CFG_AUDIO_I2S_CLK_DIV_0,
	.clkinv0		= CFG_AUDIO_I2S_CLK_INV_0,
	.clksrc1		= CFG_AUDIO_I2S_CLK_SRC_1,
	.clkdiv1		= CFG_AUDIO_I2S_CLK_DIV_1,
	.clkinv1		= CFG_AUDIO_I2S_CLK_INV_1,

	.clkoutinv 		= CFG_AUDIO_I2S_CLK_OUT_INV,
	.play_channel	= CFG_DMA_AUDIO_PLAY,
	.capt_channel	= CFG_DMA_AUDIO_REC,
};

static struct platform_device i2s_plat_device = {
	.name	= I2S_DEV_NAME,
	.id		= -1,
	.dev    = {
		.platform_data	= &i2s_platdata
	},
};
#endif /* CONFIG_SND_NEXELL_SOC_I2S || CONFIG_SND_NEXELL_SOC_I2S_MODULE */

#if defined(CONFIG_SND_NEXELL_SOC_SPDIF) || defined(CONFIG_SND_NEXELL_SOC_SPDIF_MODULE)
static struct spdif_plat_data spdif_platdata = {
	.capt_channel	= CFG_DMA_SPI_RX,
	.irq_num		= IRQ_PHY_SPDIF,
};

static struct platform_device spdif_plat_device = {
	.name	= SPDIF_DEV_NAME,
	.id		= -1,
	.dev    = {
		.platform_data	= &spdif_platdata
	},
};
#endif /* CONFIG_SND_NEXELL_SOC_SPDIF || CONFIG_SND_NEXELL_SOC_SPDIF_MODULE */

/*------------------------------------------------------------------------------
 * MISC ADC platform device
 */
#if defined(CONFIG_MISC_NEXELL_ADC)
static struct platform_device adc_plat_device = {
	.name	= ADC_DEV_NAME,
	.id		= 0,
};
#endif	/* CONFIG_MISC_NEXELL_ADC */

/*------------------------------------------------------------------------------
 * MISC PWM platform device
 */
#if defined(CONFIG_HAVE_PWM)
static struct platform_device pwm_plat_device = {
	.name	= PWM_DEV_NAME,
	.id		= 0,
};
#endif	/* CONFIG_HAVE_PWM */

/*------------------------------------------------------------------------------
 * MISC SPI platform device
 */
#if defined(CONFIG_MISC_NEXELL_SPI)
static struct platform_device spi_plat_misc_dev = {
	.name	= SPI_MISC_DEV_NAME,
	.id		= 0,
};
#endif	/* CONFIG_MISC_NEXELL_SPI */

/*------------------------------------------------------------------------------
 * MISC GPIO platform device
 */
#if defined(CONFIG_MISC_NEXELL_GPIO)
static struct platform_device gpio_plat_device = {
	.name	= GPIO_DEV_NAME,
	.id		= 0,
};
#endif	/* CONFIG_MISC_NEXELL_GPIO */

/*------------------------------------------------------------------------------
 * MISC MPEGTS platform device
 */
#if defined(CONFIG_MISC_NEXELL_MPEGTS)
static struct mpegts_plat_data mpegts_plat_data = {
	.clock_pol	 = CFG_MPEGTS_CLOCKPOL,
	.data_pol	 = CFG_MPEGTS_DATAPOL,
	.sync_source = CFG_MPEGTS_SYNC,
	.sync_mode	 = CFG_MPEGTS_SYNCMODE,
	.word_cnt	 = CFG_MPEGTS_WORDCNT
};

static struct platform_device mpegts_plat_device = {
	.name	= MPEGTS_DEV_NAME,
	.id		= 0,
	.dev	= {
		.platform_data = &mpegts_plat_data,
	},
};
#endif	/* CONFIG_MISC_NEXELL_MPEGTS */

/*------------------------------------------------------------------------------
 * register cpu platform devices
 */
void __init cpu_device(void)
{
#if defined(CONFIG_SERIAL_NEXELL_UART)
	printk("plat: add device serial (array:%d)\n", ARRAY_SIZE(uart_plat_devices));
	platform_add_devices(uart_plat_devices, ARRAY_SIZE(uart_plat_devices));
#endif

#if defined(CONFIG_USB_EHCI_HCD) ||	defined(CONFIG_USB_EHCI_HCD_MODULE)
	printk("plat: add device usb ehci\n");
	platform_device_register(&ehci_plat_device);
#endif

#if defined(CONFIG_USB_OHCI_HCD) ||	defined(CONFIG_USB_OHCI_HCD_MODULE)
	printk("plat: add device usb ohci\n");
	platform_device_register(&ohci_plat_device);
#endif

#if defined(CONFIG_USB_GADGET_NEXELL)
	printk("plat: add device usb gadget\n");
	platform_device_register(&gadget_plat_device);
#endif

#if defined(CONFIG_USB_ANDROID)
	printk("plat: add device usb android\n");
	platform_device_register(&androidusb_device);
#endif

#if defined(CONFIG_USB_ANDROID_MASS_STORAGE)
	printk("plat: add device usb mass storage\n");
	platform_device_register(&usb_mass_storage_device);
#endif

#if defined(CONFIG_I2C_NEXELL)
	printk("plat: add device i2c bus (array:%d) \n", ARRAY_SIZE(i2c_plat_devices));
	platform_add_devices(i2c_plat_devices, ARRAY_SIZE(i2c_plat_devices));
#endif

#if defined(CONFIG_RTC_DRV_NEXELL)
	printk("plat: add device Real Time Clock  \n");
	platform_device_register(&rtc_plat_device);
#endif

#if defined(CONFIG_NEXELL_WATCHDOG)
	printk("plat: add device watchdog\n");
	platform_device_register(&wdt_plat_device);
#endif

/* Graphic */
#if defined(CONFIG_GRP_NEXELL_VMEM)
	printk("plat: add device graphic memory allocator\n");
	platform_device_register(&vmem_plat_device);
#endif

#if defined(CONFIG_GRP_NEXELL_DPC)
	printk("plat: add device display control\n");
	platform_device_register(&dpc_plat_device);
#endif

#if defined(CONFIG_NEXELL_FINESCALER)
	printk("plat: add device finescaler\n");
	platform_device_register(&finescaler_plat_device);
#endif

#if defined(CONFIG_SND_NEXELL_SOC_I2S) || defined(CONFIG_SND_NEXELL_SOC_I2S_MODULE)
	printk("plat: add device asoc-i2s \n");
	platform_device_register(&i2s_plat_device);
#endif
#if defined(CONFIG_SND_NEXELL_SOC_SPDIF) || defined(CONFIG_SND_NEXELL_SOC_SPDIF_MODULE)
	printk("plat: add device asoc-spdif \n");
	platform_device_register(&spdif_plat_device);
#endif

/* MISC */
#if defined(CONFIG_MISC_NEXELL_ADC)
	printk("plat: add device misc adc\n");
	platform_device_register(&adc_plat_device);
#endif

#if defined(CONFIG_HAVE_PWM)
	printk("plat: add device generic pwm\n");
	platform_device_register(&pwm_plat_device);
#endif

#if defined(CONFIG_MISC_NEXELL_SPI)
	printk("plat: add device misc spi\n");
	platform_device_register(&spi_plat_misc_dev);
#endif

#if defined(CONFIG_MISC_NEXELL_GPIO)
	printk("plat: add device misc gpio\n");
	platform_device_register(&gpio_plat_device);
#endif

#if defined(CONFIG_MISC_NEXELL_MPEGTS)
	printk("plat: add device misc mpegts\n");
	platform_device_register(&mpegts_plat_device);
#endif
}
