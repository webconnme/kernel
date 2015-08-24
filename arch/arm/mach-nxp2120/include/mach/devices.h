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
#ifndef __DEVICES_H__
#define __DEVICES_H__

/* cpu */
#define UART_DEV_NAME 			"nx-uart"
#define	FB_DEV_NAME				"nx-fb"
#define	EHCI_DEV_NAME			"nx-ehci"
#define	OHCI_DEV_NAME			"nx-ohci"
#define	GADGET_DEV_NAME			"nx-gadget"
#define	I2C_DEV_NAME			"nx-i2c"
#define	SPI_DEV_NAME			"nx-spi"
#define	EHCI_DEV_NAME			"nx-ehci"
#define	OHCI_DEV_NAME			"nx-ohci"
#define	NAND_DEV_NAME			"nx-nand"
#define	KEYPAD_DEV_NAME			"nx-keypad"
#define	TS_DEV_NAME				"nx-ts"			/* Touch */
#define	RTC_DEV_NAME			"nx-rtc"
#define	SDHC_DEV_NAME			"nx-sdhc"
#define	V4L2_DEV_NAME			"nx-v4l2"		/* with vip */
#define	WDT_DEV_NAME			"nx-wdt"		/* watchdog */
#define I2S_DEV_NAME            "nx-i2s"
#define SPDIF_DEV_NAME          "nx-spdif"
#define PCM_DEV_NAME            "nx-pcm"
#define TOUCHKEY_DEV_NAME		"nx-touchkey"
#define ADCKEY_DEV_NAME		"nx-adckey"
/* graphic */
#define VMEM_DEV_MAJOR			240
#define VMEM_DEV_NAME			"vmem"

#define OGL_DEV_MAJOR			249
#define OGL_DEV_NAME			"ogl"

#define DPC_DEV_MAJOR			241
#define DPC_DEV_NAME			"dpc"

#define VIP_DEV_MAJOR			261
#define	VIP_DEV_NAME			"vip"

#define VIP0_DEV_MINOR			0
#define	VIP0_DEV_NAME			"vip0"

#define VIP1_DEV_MINOR			1
#define	VIP1_DEV_NAME			"vip1"

#define SCALER_DEV_MAJOR		262
#define SCALER_DEV_NAME			"scaler"

/* board */
#define DM9000_DEV_NAME 		"dm9000"

/* misc */
#define PWM_DEV_MAJOR			243
#define	PWM_DEV_NAME			"pwm"

#define BUZZER_DEV_MAJOR		246
#define	BUZZER_DEV_NAME			"buzzer"

#define GPIO_DEV_MAJOR			244
#define GPIO_DEV_NAME			"gpio"

#define ADC_DEV_MAJOR			250
#define	ADC_DEV_NAME			"adc"

#define DXB_DEV_MAJOR			245
#define DXB_DEV_NAME			"dxb"

#define SPI_MISC_DEV_MAJOR		252
#define SPI_MISC_DEV_NAME		"spi"

#define PMI_DEV_MAJOR			253
#define PMI_DEV_NAME			"pmi"

#define MPEGTS_DEV_NAME			"mpegts"

/*--------------------------------------------------------------------
 *	Frame buffer platform data
 */
struct fb_plat_data {
	int 	module;		/* 0: primary, 1: secondary */
	int 	layer;		/* RGB 0, 1, 2 */
	int		x_res;		/* x resolution */
	int		y_res;		/* y resolution */
	int		pixelbit;	/* bit per pixel */
	u_int	format;		/* RGB format */
	int		buffers;	/* set 2 when support pan, default 1 */
	int		with_mm;	/* with (mm), default 0 */
	int		height_mm;	/* height (mm), default 0 */
	/* for pixel clock, default 0 */
	u_int	hs_left;	/* back porch + sync width */
	u_int	hs_right;	/* front porch */
	u_int	vs_upper;	/* back porch + sync width */
	u_int	vs_lower;	/* front porch */
	u_int	pixelclock;
};

/*--------------------------------------------------------------------
 *	I2C platform data
 */
struct i2c_plat_data {
	int 	port;
	int 	irq;
	u_int	io_scl;		/* scl pad */
	u_int	io_sda;		/* scl pad */
	u_int	clksrc;		/* GPIO: 0, I2C hw: 16 or 256 */
	u_int	clkscale;
	u_int	cntmax;		/* Qyab\rter period count max register */
	int		polling;	/* wait ack method */
};

/*--------------------------------------------------------------------
 *	SPI platform data
 */
#define	MAX_SPI_CS_NUM			(4)

struct spi_io_cs {
	unsigned int 	pad;
	unsigned int 	func;
};

struct spi_plat_data {
	int 				port;
	int 				irq;
	int					io_clk;					/* clock pad */
	int					io_rx;					/* tx pad */
	int					io_tx;					/* rx pad */
	struct spi_io_cs  * io_cs;
	int					cs_num;
	int					format;

	int					clk_src;
	int					clk_divisor;
	int					clk_divcount;

	/*DMA Channel */
	int					dma_tx_channel;
	int					dma_rx_channel;
};

/*--------------------------------------------------------------------
 *	SDHC platform data
 */
struct sdhc_plat_data {
	int io_detect;
	int io_wprotect;
};

/*--------------------------------------------------------------------
 *	Touch platform data
 */
struct adc_ts_plat_data {
	struct {
		int		xm_p;		/* xmon_p */
		int		xm_n;		/* xmon_n */
		int		ym_p;		/* ymon_p */
		int		ym_n;		/* ymon_n */
		int		detect;		/* detect pen down */
		int		control;	/* detect pen control */
		int 	delay;		/* x, y detect delay time (msec) */
		int		change_xy;
	} io;
	struct {
		int		x_ch;		/* adc x channel */
		int		y_ch;		/* adc y channel */
		int 	x_min;
		int 	x_max;
		int 	y_min;
		int 	y_max;
		int		pointercal[10]; /* calibration value (tslib) */
	} adc;
};

/*--------------------------------------------------------------------
 *	Keypad platform data
 */
struct key_plat_data {
	int				   bt_count;
	unsigned int  	 * bt_io;
	unsigned int  	 * bt_code;
	unsigned int  	 * bt_long;			/* long press action */
	unsigned int  	 * bt_code_type;	/* short key type 0=short key, 1 = long key */
	unsigned int  	 * bt_long_type;	/* long  key type 0=short key, 1 = long key */

	int				   bt_delay;		/* short key delay */
	int				   bt_long_delay;	/* long  key delay */
	struct input_dev * bt_device;
};

/*--------------------------------------------------------------------
 *	VMem platform data
 */
#if defined(CONFIG_GRP_NEXELL_VMEM_KILLER)
struct vmem_plat_data {
	int		resv_task_num;
	char **	resv_task_name;
};
#endif

/*--------------------------------------------------------------------
 *	v4l2 platfrom data
 */
struct v4l2_plat_data {
	int		in_port;		/* vip input port 0, 1 */
	int		dpc_device;		/* display out module */
	int		hor_align;
	int		ver_align;
	int		buff_count;		/* alloc buffer count */
	int		skip_count;		/* first frame skip count */
};

/*--------------------------------------------------------------------
 *  I2S platform data
 */
struct i2s_plat_data {
    int     master;         /* 1=master, 0=slave */
    int     syncbit;
    int     pcmoutw;
    int     pcminw;
    int     transmode;     /* 0 = I2S, 2 = Left-Justified, 3 = Right-Justified  */
    int     samplerate;    /* 48000, 44100 */

    int     clksrc0;       /* source clock 0  select */
    int     clkdiv0;       /* clock 0 dividor */
    int     clkinv0;       /* clock 0 invertor */
    int     clksrc1;       /* source clock 1 select */
    int     clkdiv1;       /* clock 1 dividor */
    int     clkinv1;       /* clock 1 invertor */
    u_int   clkoutinv;     /* clock out invert */

	int		play_channel;  /* Play DMA channel */
	int		capt_channel;  /* Capture DMA channel */
};

/*--------------------------------------------------------------------
 *  SPDIF platform data
 */
struct spdif_plat_data {
	int		capt_channel;  /* Capture DMA channel */
	int		irq_num;
};

/*--------------------------------------------------------------------
 *  MPEGTSIF platform data
 */
struct mpegts_plat_data {
	unsigned char clock_pol; // NX_MPEGTSIF_CLOCKPOL_FALLING, NX_MPEGTSIF_CLOCKPOL_RISING
	unsigned char data_pol;  // NX_MPEGTSIF_DATAPOL_HIGH, NX_MPEGTSIF_DATAPOL_LOW
	unsigned char sync_source; // NX_MPEGTSIF_SYNC_EXTERNAL, NX_MPEGTSIF_SYNC_INTERNAL
	unsigned char sync_mode; // NX_MPEGTSIF_SYNCMODE_BIT, NX_MPEGTSIF_SYNCMODE_BYTE
	unsigned int  word_cnt; // 1 ~ 64
};


#endif	/* __DEVICES_H__ */

