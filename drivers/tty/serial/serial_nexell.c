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
#include <linux/device.h>
#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/seqlock.h>
#include <linux/platform_device.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_core.h>
#include <linux/serial_reg.h>
#include <linux/serial.h>
#include <linux/version.h>
#include <asm/irq.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>

/* nexell soc headers */
#include <mach/platform.h>
#include <mach/devices.h>
#include <mach/soc.h>

/*Hugh Res Timer */
#include <linux/hrtimer.h>
#include <linux/ktime.h>

/* Debugging stuff */
#if (0)
#define DBGOUT(msg...)		{ printk(KERN_INFO "uart: " msg); }
#else
#define DBGOUT(msg...)		do {} while (0)
#endif

#if (0)
#define DBGINTR(msg...)		printk(msg)
#else
#define DBGINTR(msg...)		do {} while (0)
#endif

/*
 * 	Uart macros & functions
 */
#define	PORT_NEXELL			76
#define	UART_TERMINAL		"ttyS"
#define UART_BAUDRATE		115200

#define	UART_TXD_INTNUM		0	/* Tx interrupt bit */
#define	UART_RXD_INTNUM		1	/* Rx interrupt bit */
#define	UART_ERR_INTNUM		2	/* Error interrupt bit */
#define	UART_MOD_INTNUM		3	/* Modem interrupt bit */

#define UART_TXD_LEVEL		0	/* empty */
#define UART_RXD_LEVEL		12	/* 12byte */

#define UART_RXD_DUMMY		(0x10000000)

#define UART_ERR_OVERRUN	(1<<0)	/* overrun error */
#define UART_ERR_PARITY		(1<<1)	/* parity error */
#define UART_ERR_FRAME		(1<<2)	/* frame error */
#define UART_ERR_BREAK		(1<<3)	/* break error */

#define UART_INT_PEND		(0x18)

#define	UART_WAIT_READY		{ volatile int cnt; for (cnt=0; cnt <0xffff; cnt++); }

#define UART_DIVISOR_MAX	64
#define UART_BRD_MAX		64

#define KBYTE				1024
#define UART_DMA_BUFFER_SIZE (32*KBYTE)

#if defined(CONFIG_SERIAL_PORT0_RX_DMA_MODE)||defined(CONFIG_SERIAL_PORT1_RX_DMA_MODE)
#define CONFIG_SERIAL_RX_DMA_MODE
#endif

 /* High Res Timer */
 #define MS_TO_NS(x)	(x * 1E6L)
/* uart ops functions */
static struct uart_ops nx_uart_ops;

static unsigned int nx_uart_ops_tx_empty(struct uart_port *port);
static void nx_uart_ops_set_mctrl(struct uart_port *port, unsigned int mctrl);
static unsigned int nx_uart_ops_get_mctrl(struct uart_port *port);
static void nx_uart_ops_stop_tx(struct uart_port *port);
static void nx_uart_ops_start_tx(struct uart_port *port);
static void nx_uart_ops_stop_rx(struct uart_port *port);
static void nx_uart_ops_enable_ms(struct uart_port *port);
static void nx_uart_ops_break_ctl(struct uart_port *port, int break_state);
static int nx_uart_ops_startup(struct uart_port *port);
static void nx_uart_ops_shutdown(struct uart_port *port);
static void nx_uart_ops_set_termios(struct uart_port *port, struct ktermios *termios, struct ktermios *old);
static const char *nx_uart_ops_type(struct uart_port *port);
static void nx_uart_ops_release_port(struct uart_port *port);
static int nx_uart_ops_request_port(struct uart_port *port);
static void nx_uart_ops_config_port(struct uart_port *port, int flags);
static int nx_uart_ops_verify_port(struct uart_port *port, struct serial_struct *ser);

/* interrupt handler */
static irqreturn_t nx_uart_irq_handler(int irq, void *dev_id);


/*
 * nexell uart struct
 */

struct nx_uart_port {
	struct uart_port	port;	/* Note : First type must be 'struct uart_port' */
	struct hrtimer hr_timer;
	ktime_t ktime;
	unsigned long delay_in_ms;

	char			*	name;
	int					count;	/* open count */
	
	int 				req_baud;
	int 				real_baud;
	int 				baud_div;
	int 				baud_brd;

/* DMA Mode data struct*/
	int 				rx_mode;
	int 				tx_mode;

	struct dma_trans *	dmatxtr;
	struct dma_trans * 	dmarxtr;
	unsigned int 		buf_size;
	unsigned int 		dma_virt;
	dma_addr_t 			dma_phy;
	
	int 				dma_irq;
	int 				dma_ch;

	unsigned int 		b_size;
	unsigned int 		w_pos;
	unsigned int 		r_pos;
	unsigned int 		r_cnt;
};

/*
 * 	uart port info
 */
static struct nx_uart_port nx_ports[] = {
#if defined(CONFIG_SERIAL_NEXELL_UART_PORT0)
	{
		.name = "UART0",
		.port = {
			.type 		= PORT_NEXELL,
			.iotype 	= UPIO_MEM,
			.membase 	= (u_char __iomem *)IO_ADDRESS(PHY_BASEADDR_UART0),
			.mapbase 	= PHY_BASEADDR_UART0,
			.irq 		= IRQ_PHY_UART0,
			.uartclk 	= CFG_UART_DEBUG_CLKFREQ,
			.fifosize 	= 16,
			.flags 		= UPF_BOOT_AUTOCONF,
			.ops 		= &nx_uart_ops,
			.line 		= 0,
		},
#if defined (CONFIG_SERIAL_PORT0_RX_DMA_MODE)
		.rx_mode = NX_UART_OPERMODE_DMA,
		.dma_irq = PB_DMA_IRQ(CFG_DMA_UART0_RX),
		.dma_ch = CFG_DMA_UART0_RX,
		.delay_in_ms = 2L,
#else
		.rx_mode = NX_UART_OPERMODE_NORMAL,
#endif
		.tx_mode = NX_UART_OPERMODE_NORMAL,
	},
#endif
#if defined(CONFIG_SERIAL_NEXELL_UART_PORT1)
	{
		.name = "UART1",
		.port = {
			.type 		= PORT_NEXELL,
			.iotype 	= UPIO_MEM,
			.membase 	= (u_char __iomem *)IO_ADDRESS(PHY_BASEADDR_UART1),
			.mapbase 	= PHY_BASEADDR_UART1,
			.irq 		= IRQ_PHY_UART1,
			.uartclk 	= CFG_UART_DEBUG_CLKFREQ,
			.fifosize 	= 16,
			.flags 		= UPF_BOOT_AUTOCONF,
			.ops 		= &nx_uart_ops,
			.line 		= 1,
		},
#if defined (CONFIG_SERIAL_PORT1_RX_DMA_MODE)	
		.rx_mode = NX_UART_OPERMODE_DMA,
		.dma_irq = PB_DMA_IRQ(CFG_DMA_UART1_RX),
		.delay_in_ms = 2L,
		.dma_ch  = CFG_DMA_UART1_RX,
#else
		.rx_mode = NX_UART_OPERMODE_NORMAL,
#endif
		.tx_mode = NX_UART_OPERMODE_NORMAL,
	},
#endif	
#if defined(CONFIG_SERIAL_NEXELL_UART_PORT2)
	{
		.name = "UART2",
		.port = {
			.type 		= PORT_NEXELL,
			.iotype 	= UPIO_MEM,
			.membase 	= (u_char __iomem *)IO_ADDRESS(PHY_BASEADDR_UART2),
			.mapbase 	= PHY_BASEADDR_UART2,
			.irq 		= IRQ_PHY_UART2,
			.uartclk 	= CFG_UART_DEBUG_CLKFREQ,
			.fifosize 	= 16,
			.flags 		= UPF_BOOT_AUTOCONF,
			.ops 		= &nx_uart_ops,
			.line 		= 2,
		},
	},
#endif
#if defined(CONFIG_SERIAL_NEXELL_UART_PORT3)
	{
		.name = "UART3",
		.port = {
			.type 		= PORT_NEXELL,
			.iotype 	= UPIO_MEM,
			.membase 	= (u_char __iomem *)IO_ADDRESS(PHY_BASEADDR_UART3),
			.mapbase 	= PHY_BASEADDR_UART3,
			.irq 		= IRQ_PHY_UART3,
			.uartclk 	= CFG_UART_DEBUG_CLKFREQ,
			.fifosize 	= 16,
			.flags 		= UPF_BOOT_AUTOCONF,
			.ops 		= &nx_uart_ops,
			.line 		= 3,
		},
	},
#endif
#if defined(CONFIG_SERIAL_NEXELL_UART_PORT4)
	{
		.name = "UART4",
		.port = {
			.type 		= PORT_NEXELL,
			.iotype 	= UPIO_MEM,
			.membase 	= (u_char __iomem *)IO_ADDRESS(PHY_BASEADDR_UART4),
			.mapbase 	= PHY_BASEADDR_UART4,
			.irq 		= IRQ_PHY_UART4,
			.uartclk 	= CFG_UART_DEBUG_CLKFREQ,
			.fifosize 	= 16,
			.flags 		= UPF_BOOT_AUTOCONF,
			.ops 		= &nx_uart_ops,
			.line 		= 4,
		},
	},
#endif
#if defined(CONFIG_SERIAL_NEXELL_UART_PORT5)
	{
		.name = "UART5",
		.port = {
			.type 		= PORT_NEXELL,
			.iotype 	= UPIO_MEM,
			.membase 	= (u_char __iomem *)IO_ADDRESS(PHY_BASEADDR_UART5),
			.mapbase 	= PHY_BASEADDR_UART5,
			.irq 		= IRQ_PHY_UART5,
			.uartclk 	= CFG_UART_DEBUG_CLKFREQ,
			.fifosize 	= 16,
			.flags 		= UPF_BOOT_AUTOCONF,
			.ops 		= &nx_uart_ops,
			.line 		= 5,
		},
	},
#endif
};
#if 0
static void dump_register(int port)
{
	struct  NX_UART_RegisterSet * pREG= (struct NX_UART_RegisterSet *)NX_UART_GetBaseAddress(port);

	printk(" DMA Dump_Register port = %d \n", port);
	printk(" DMA LCON       : 0x%4x \r\n", pREG->LCON);
	printk(" DMA UCON       : 0x%4x \r\n", pREG->UCON);
	printk(" DMA FCON       : 0x%4x \r\n", pREG->FCON);
	printk(" DMA MCON       : 0x%4x \r\n", pREG->MCON);
	printk(" DMA TRSTATUS   : 0x%4x \r\n", pREG->TRSTATUS);
	printk(" DMA ESTATUS    : 0x%4x \r\n", pREG->ESTATUS);
	printk(" DMA MSTATUS    : 0x%4x \r\n", pREG->MSTATUS);
	printk(" DMA THB        : 0x%4x \r\n", pREG->THB);
	printk(" DMA RHB        : 0x%4x \r\n", pREG->RHB);
	printk(" DMA BRD        : 0x%4x \r\n", pREG->BRD);
	printk(" DMA TIMEOUT    : 0x%4x \r\n", pREG->TIMEOUT);
	printk(" DMA INTCON     : 0x%4x \r\n", pREG->INTCON);
	printk(" DMA CLKENB     : 0x%4x \r\n", pREG->CLKENB);
	printk(" DMA CLKGEN     : 0x%4x \r\n", pREG->CLKGEN);
}
#endif

#ifdef CONFIG_SERIAL_RX_DMA_MODE

static irqreturn_t nx_uart_dma_irq_handler(int irq, void *dev_id);
/*
//	HR-Timer Function
*/
static void trans_tty(struct nx_uart_port *uart ,int timeout )
{
	struct uart_port *port =(struct uart_port *)uart;
	struct tty_struct *tty = port->state->port.tty;
	int dmalen,i;
	char ch;
	int len = 6;
	unsigned int flag,err=0;

	if (timeout) {
		dmalen = soc_dma_get_length(uart->dmarxtr)+1;
		len = uart->dmarxtr->iom.length - dmalen - uart->r_cnt;
	}		
	else
		len = uart->dmarxtr->iom.length - uart->r_cnt;

	err  = NX_UART_GetErrorStatus(port->line);
	flag = TTY_NORMAL;

	/* error status */
	if (unlikely(err & (UART_ERR_OVERRUN |
				UART_ERR_PARITY	 |
				UART_ERR_FRAME   |
				UART_ERR_BREAK )) ) {
		printk(KERN_ERR "uart: rx error port:%d, err=%x\n", port->line, err);

		/* break error */
		if (err & UART_ERR_BREAK) {
			port->icount.brk++;
			if (uart_handle_break(port))
				goto ignore_char;
		}
		/* parity error */
		if (err & UART_ERR_PARITY)
			port->icount.parity++;

		if (err & UART_ERR_FRAME)
			port->icount.frame++;

		if (err & UART_ERR_OVERRUN)
			port->icount.overrun++;

		/*
		 * Mask off conditions which should be ignored.
		 */
		err &= port->read_status_mask;

		if (err & UART_ERR_BREAK)
			flag = TTY_BREAK;
		else if (err & UART_ERR_PARITY)
			flag = TTY_PARITY;
		else if (err & UART_ERR_FRAME)
			flag = TTY_FRAME;
	}

	for(i=0; i <len ; i++){
		flag = TTY_NORMAL;
		ch = *(char*)(uart->dma_virt + uart->r_cnt + uart->r_pos);
		uart->r_cnt++;
		port->icount.rx++;
		
		if (uart_handle_sysrq_char(port, ch))
			goto ignore_char;

		uart_insert_char(port, err, UART_LSR_OE, ch, flag);

		ignore_char:
		continue;
	}
	
	tty_flip_buffer_push(tty);
}

/*
*	HR Timer Callback Func	
*/
enum hrtimer_restart dma_hrtimer_callback( struct hrtimer *timer )
{

	struct nx_uart_port *uart =  (struct nx_uart_port *)((unsigned int)timer - sizeof(struct uart_port));
	struct uart_port *port =(struct uart_port *)uart;

	DBGINTR("%s (line:%d)\n", __func__, port->line);

	trans_tty(uart, 1);
	NX_UART_SetInterruptEnable(port->line, UART_RXD_INTNUM, CTRUE);	
 	return HRTIMER_NORESTART;
}

static int init_rx_dma(struct nx_uart_port *uart)
{
	struct dma_trans *dma_tr = NULL;

	DBGOUT("%s\n", __func__);

	dma_tr = soc_dma_request(uart->dma_ch,false);

	if (!dma_tr) {
		printk(KERN_ERR "fail, request for uart dma channel uart channel %d ...\n",uart->port.line);
		return -EINVAL;
	}
	dma_tr->channel = uart->dma_ch;
	dma_tr->tr_type = DMA_TRANS_IO2MEM;
	dma_tr->iom.srcbase = NX_UART_GetPhysicalAddress(uart->port.line);
	dma_tr->iom.src_id  = NX_UART_GetDMAIndex_Rx(uart->port.line);
	dma_tr->iom.src_bit = NX_UART_GetDMABusWidth(uart->port.line);
	//dma_tr->iom.length  = 32*1024/256;
	dma_tr->iom.length  = 128;
	uart->b_size = dma_tr->iom.length;
	uart->dmarxtr = dma_tr;
	
	DBGOUT("%s (Buf length:%d)\n", __func__, dma_tr->iom.length);

	return 0;	
}

static inline void deinit_rx_dma(struct nx_uart_port *uart)
{
	DBGOUT("%s\n", __func__);

	if (uart->dmarxtr) {
		soc_dma_release(uart->dmarxtr);
		uart->dmarxtr = NULL;
	}
	uart->w_pos = 0;
	uart->r_pos = 0;
	uart->r_cnt = 0;
}

static int init_dma_buffer(struct nx_uart_port *uart)
{
	
	uart->port.dev->coherent_dma_mask = 0xffffffff;
	uart->buf_size = PAGE_ALIGN(UART_DMA_BUFFER_SIZE);
	
	uart->dma_virt = (unsigned int)dma_alloc_writecombine(uart->port.dev, uart->buf_size, &uart->dma_phy, GFP_ATOMIC);
	if (!uart->dma_virt) {
		printk(KERN_ERR "can't alloc dma buffer...\n");
		return -ENOMEM;
	}
		
	uart->w_pos = 0;
	uart->r_pos = 0;
	uart->r_cnt = 0;
	
	return 0;
}

static inline void deinit_dma_buf(struct nx_uart_port *uart)
{

	if (uart->dma_virt) {
		dma_free_coherent(uart->port.dev, uart->buf_size, (void *)uart->dma_virt, uart->dma_phy);
	}
}

static void enqueue_dma(struct nx_uart_port *uart)
{
	uart->dmarxtr->iom.dstbase=(unsigned int)uart->dma_phy + uart->w_pos;
	soc_dma_transfer(uart->dmarxtr);
}

static inline void _stop_dma(struct nx_uart_port *uart)
{
	int ret=0;

//	soc_dma_get_length(uart->dmarxtr);
	soc_dma_trans_stop(uart->dmarxtr);
	while (soc_dma_check_run(uart->dmarxtr)) {
		ret++ ;
		msleep(1);
	}
}

static inline void _w_buf(struct nx_uart_port *buf)
{
	buf->w_pos+=buf->b_size;

	if((UART_DMA_BUFFER_SIZE - buf->w_pos) <= buf->b_size)
		buf->w_pos = 0;
}

static inline void _r_buf(struct nx_uart_port *buf)
{
	buf->r_pos+=buf->b_size;
	if((UART_DMA_BUFFER_SIZE - buf->r_pos) <= buf->b_size)
		buf->r_pos = 0;
}

static int setup_dma(struct nx_uart_port *uart)
{
	struct uart_port *port = (struct uart_port *)uart;
	int ret;

	DBGOUT("%s (port:%d)\n", __func__, port);

	if(uart->rx_mode == NX_UART_OPERMODE_DMA) {
		init_rx_dma    (uart);
		init_dma_buffer(uart);

		ret = request_irq(uart->dma_irq, nx_uart_dma_irq_handler, IRQF_DISABLED, UART_DEV_NAME, port);
		if ( ret!=0 ) {
			dev_err(port->dev, "unable to grab irq%d\n",uart->dma_irq);
			return ret;
		}

		soc_dma_set_mode(uart->dmarxtr,DMA_MODE_BUFFER);

		if(soc_dma_check_run(uart->dmarxtr)) {
			dev_err(port->dev, "uart dma (%d) is running !!!\n",uart->dma_irq);
			return -1;
		}

		/* Timer Setting */

		uart->ktime = ktime_set( 0, MS_TO_NS(uart->delay_in_ms) );
		hrtimer_init( &uart->hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL );
		uart->hr_timer.function = &dma_hrtimer_callback;

		DBGOUT( "Starting timer to fire in %ldms (%ld)\n", uart->delay_in_ms, jiffies );
		
		enqueue_dma(uart);
		_w_buf(uart);
		enqueue_dma(uart);	// for command buffer
		
	}
	return 0;
}

#endif

static void init_uart_port(void)
{
	int i = 0;
	int array = NX_UART_GetNumberOfModule();	

	DBGOUT("%s (array:%d)\n", __func__, array);
	
	NX_DMA_Initialize();

	for (i=0; i < array; i++) {
		NX_UART_SetBaseAddress(i, (U32)IO_ADDRESS(NX_UART_GetPhysicalAddress(i)));
		/* Disable debug uart port clock */
#if !(PM_DBGOUT_ON) && !defined(CONFIG_DEBUG_LL_UART)
		NX_UART_OpenModule(i);
		NX_UART_SetClockDivisorEnable(i, CFALSE);
		NX_UART_SetClockPClkMode(i, NX_PCLKMODE_DYNAMIC);
#endif
	
	}
}


static void setup_uart_port(unsigned int port)
{
	DBGOUT("%s (port:%d)\n", __func__, port);
	
	/* Select module & initialize */
	NX_UART_OpenModule(port);

	/* Clock & Interrupt Disable */
	NX_UART_SetClockDivisorEnable(port, CFALSE);
	NX_UART_SetClockPClkMode(port, NX_PCLKMODE_DYNAMIC);

	NX_UART_ClearInterruptPendingAll(port);
   	NX_UART_SetInterruptEnableAll(port, CFALSE);

	// UART Mode : Tx, Rx Only
   	NX_UART_SetSIRMode(port, CFALSE);
	NX_UART_SetLoopBackMode(port, CFALSE);
	NX_UART_SetAutoFlowControl(port, CFALSE);
	NX_UART_SetHalfChannelEnable(port, CTRUE);

    NX_UART_SetSCTxEnb(port, CFALSE);
    NX_UART_SetSCRxEnb(port, CTRUE);

	// Frame Configuration : Data 8 - Parity 0 - Stop 1
	NX_UART_SetFrameConfiguration (port, NX_UART_PARITYMODE_NONE, 8, 1);

	// Tx Rx Operation : Default Polling
  	NX_UART_SetTxIRQType(port, NX_UART_IRQTYPE_PULSE);
   	

   	NX_UART_SetTxOperationMode(port, NX_UART_OPERMODE_NORMAL);    	// Interrupt or Polling
  	//NX_UART_SetTxOperationMode(port, nx_ports[port].tx_mode);
   	NX_UART_SetRxIRQType(port, NX_UART_IRQTYPE_LEVEL);

   	
   	NX_UART_SetRxOperationMode(port, nx_ports[port].rx_mode);   	
   	NX_UART_SetIntEnbWhenExceptionOccur(port, CFALSE);

	NX_UART_SetSYNCPendClear(port);

	/* FCON: fifo control */
	NX_UART_SetTxFIFOTriggerLevel(port, UART_TXD_LEVEL);			// Tx empty irq
    NX_UART_ResetTxFIFO(port);

	if(NX_UART_OPERMODE_DMA == nx_ports[port].rx_mode )
		NX_UART_SetRxFIFOTriggerLevel(port, 0);						// Rx full  irq	
	else
		NX_UART_SetRxFIFOTriggerLevel(port, UART_RXD_LEVEL);		// Rx full  irq

    NX_UART_ResetRxFIFO(port);
    NX_UART_SetFIFOEnb(port, CFALSE);

	/* MCON: modem control - skip */
    NX_UART_SetDTR(port, CFALSE);
    NX_UART_SetRTS(port, CFALSE);

	/* BRDn: baud rate divisor */
	NX_UART_SetBRD(port, NX_UART_MakeBRD(CFG_UART_DEBUG_BAUDRATE, CFG_UART_DEBUG_CLKFREQ));

	/* TIMEOUTREG: receive timeout */

	NX_UART_SetRxTimeOutEnb(port, CTRUE);							// NEED Enable for RX interrupt.
   	
   	NX_UART_SetRxTimeOut(port, 0x1F);	
	if(NX_UART_OPERMODE_DMA == nx_ports[port].rx_mode ) {
		NX_UART_SetRxTimeOutEnb(port, CFALSE);					// NEED Enable for RX interrupt.
    }
    	
	/* INTSTATUSREG: Interrupt status */
    NX_UART_ClearInterruptPendingAll(port);

	/* UARTCLKGEN, UARTCLKENB */
	NX_UART_SetClockSource  (port, 0, CFG_UART_DEBUG_CLKSRC);
	NX_UART_SetClockDivisor (port, 0, CFG_UART_DEBUG_CLKDIV);
	NX_UART_SetClockDivisorEnable(port, CTRUE);
	NX_UART_SetClockPClkMode(port, NX_PCLKMODE_ALWAYS);

	UART_WAIT_READY;

}


/*---------------------------------------------------------------------------------------------
 * 	Uart ops functions
 --------------------------------------------------------------------------------------------*/

/*
 * return 0; tx buffer not empty
 * return 1: tx buffer empty
 */
static unsigned int nx_uart_ops_tx_empty(struct uart_port *port)
{
	unsigned long flags;
	unsigned int ret=0;
	DBGOUT("%s (line:%d)\n", __func__, port->line);

	/* disable irq */
	spin_lock_irqsave(&port->lock,flags);

	/* buffer empty */
	if ( (NX_UART_GetTxRxStatus(port->line) & NX_UART_TRANSMITTER_EMPTY) ) {
		ret = TIOCSER_TEMT;
	}

	/* enable irq */
	spin_unlock_irqrestore(&port->lock, flags);
	return ret;
}

/* modem control */
static void nx_uart_ops_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
	DBGOUT("%s (line:%d)\n", __func__, port->line);

	/* not implementation */
}

/* modem control */
static unsigned int nx_uart_ops_get_mctrl(struct uart_port *port)
{
	unsigned int ret = (TIOCM_CTS | TIOCM_DSR | TIOCM_CAR);
	DBGOUT("nx_uart_ops_get_mctrl (line:%d)\n", port->line);

	/* not implementation */
	return ret;
}

static void nx_uart_ops_stop_tx(struct uart_port *port)
{
	DBGINTR("%s (line:%d)\n", __func__, port->line);

	if (NX_UART_GetInterruptEnable(port->line, UART_TXD_INTNUM))
		NX_UART_SetInterruptEnable(port->line, UART_TXD_INTNUM, CFALSE);
}

static void nx_uart_ops_start_tx(struct uart_port *port)
{
	DBGINTR("%s (line:%d)\n", __func__, port->line);

	if ( ! NX_UART_GetInterruptEnable(port->line, UART_TXD_INTNUM))
		NX_UART_SetInterruptEnable(port->line, UART_TXD_INTNUM, CTRUE);
}

static void nx_uart_ops_stop_rx(struct uart_port *port)
{
	DBGOUT("%s (line:%d)\n", __func__, port->line);

	if (NX_UART_GetInterruptEnable(port->line, UART_RXD_INTNUM))
		NX_UART_SetInterruptEnable(port->line, UART_RXD_INTNUM, CFALSE);
}

static void nx_uart_ops_enable_ms(struct uart_port *port)
{
	DBGOUT("%s (line:%d)\n", __func__, port->line);
	/* not implementation */
}


static void nx_uart_ops_break_ctl(struct uart_port *port, int break_state)
{
	unsigned long flags;
	DBGOUT("%s (line:%d)\n", __func__, port->line);

	/* disable irq */
	spin_lock_irqsave(&port->lock, flags);

	/* not implementation */

	/* enable irq */
	spin_unlock_irqrestore(&port->lock, flags);
}


static int nx_uart_ops_startup(struct uart_port *port)
{
	struct nx_uart_port *uart = (struct nx_uart_port *)port;
	int ret;
	unsigned long flags;

	DBGOUT("%s (line:%d, count:%d)\n", __func__, port->line, uart->count);

	ret = request_irq(port->irq, nx_uart_irq_handler, IRQF_DISABLED, UART_DEV_NAME, port);
	if ( ret!=0 ) {
		dev_err(port->dev, "unable to grab irq%d\n",port->irq);
		return ret;
	}

#ifdef CONFIG_SERIAL_RX_DMA_MODE
	if(uart->rx_mode == NX_UART_OPERMODE_DMA)
	setup_dma(uart);
#endif

	/* disable irq */
	spin_lock_irqsave(&port->lock, flags);

	if(0 == uart->count)
		setup_uart_port(port->line);

	uart->count++;

	/* enable irq */
	spin_unlock_irqrestore(&port->lock, flags);
	return 0;
}

static void nx_uart_ops_shutdown(struct uart_port *port)
{
	struct nx_uart_port *uart = (struct nx_uart_port *)port;
	unsigned long flags;

	DBGOUT("%s (line:%d, count:%d)\n", __func__, port->line, uart->count);

	/* disable irq */
	spin_lock_irqsave(&port->lock,flags);

   	NX_UART_SetInterruptEnableAll(port->line, CFALSE);
    NX_UART_ClearInterruptPendingAll(port->line);

	uart->count--;
	if (0 == uart->count) {
    	NX_UART_SetSYNCPendClear(port->line);
		NX_UART_SetClockDivisorEnable(port->line, CFALSE);
	}

	if (0 >	uart->count)
		uart->count = 0;

	/* enable irq */
	spin_unlock_irqrestore(&port->lock,flags);

	/* release irq */
	free_irq(port->irq, port);

#ifdef CONFIG_SERIAL_RX_DMA_MODE
	if(NX_UART_OPERMODE_DMA == uart->rx_mode ) {
		_stop_dma(uart);
		deinit_rx_dma(uart);

		hrtimer_cancel( &uart->hr_timer );
		
		free_irq(uart->dma_irq, port);
	}
#endif
}

static void nx_uart_ops_set_termios(struct uart_port *port, struct ktermios *termios, struct ktermios *old)
{
	struct nx_uart_port *uart = (struct nx_uart_port *)port;
	unsigned int data, parity, stop;
	//unsigned int baud, quot;
	unsigned int baud;
	unsigned long flags;

	int divisor , brd;
	int baudrate , tmp_baud, err;
	int c_div = 0,c_brd=0,c_baud=0;
	
	DBGOUT("%s (line:%d)\n", __func__, port->line);

	/* Data Bit */
	switch (termios->c_cflag & CSIZE) {
	case CS5:
		data =  5;
		break;
	case CS6:
		data =  6;
		break;
	case CS7:
		data =  7;
		break;
	default:
	case CS8:
		data =  8;
		break;
	}

	/* Parity Bit */
	if (termios->c_cflag & PARENB) {
		if (termios->c_cflag & PARODD)
			parity = NX_UART_PARITYMODE_ODD;
		else
			parity = NX_UART_PARITYMODE_EVEN;
	} else {
		parity = NX_UART_PARITYMODE_NONE;
	}

	/* Stop Bit */
	if (termios->c_cflag & CSTOPB)
		stop = 2;
	else
		stop = 1;
	
	/* Divisor Setting */
	if(termios->c_ispeed == 0)
		baudrate = CFG_UART_DEBUG_BAUDRATE;
	else
		baudrate = termios->c_ispeed;

	err = baudrate;

	for(divisor = UART_DIVISOR_MAX; divisor > 0  ; divisor-- ) {
		for(brd = UART_BRD_MAX ;brd > 0; brd--) {
			
			tmp_baud = CFG_SYS_PLL1_FREQ/(divisor*16*brd);

			if( abs(baudrate - tmp_baud ) < err) {
				err = abs(baudrate - tmp_baud);
				c_div = divisor;
				c_brd = brd;
				c_baud = tmp_baud;	
			}
		}
	}

	/* Baudrate */
	if (c_div == 0)
		baud = uart_get_baud_rate(port, termios, old, 0, CFG_SYS_PLL1_FREQ/CFG_UART_DEBUG_CLKDIV/16);
	else
		baud = uart_get_baud_rate(port, termios, old, 0, CFG_SYS_PLL1_FREQ/c_div/16);
	

	//quot = uart_get_divisor(port, baud);
	if(baud == baudrate) {
		uart->req_baud	= baudrate;
		uart->real_baud	= c_baud;
		uart->baud_div	= c_div;
		uart->baud_brd	= c_brd;
	} else {
		uart->req_baud	= baudrate;
		c_div = CFG_UART_DEBUG_CLKDIV;
		uart->req_baud	= baudrate;
		uart->baud_div	= c_div;
		uart->baud_brd  = NX_UART_MakeBRD(baud, (CFG_SYS_PLL1_FREQ/uart->baud_div));
		uart->real_baud = CFG_SYS_PLL1_FREQ/(uart->baud_div*16*uart->baud_brd);
	}

	/* disable irq */
	spin_lock_irqsave(&port->lock, flags);

	/* Update the per-port timeout */
	uart_update_timeout(port, termios->c_cflag, baud);

	/*
	 * Which character status flags are we interested in?
	 */
	port->read_status_mask = UART_ERR_OVERRUN;
	if (termios->c_iflag & INPCK)
		port->read_status_mask |= UART_ERR_FRAME | UART_ERR_PARITY;

	/*
	 * Which character status flags should we ignore?
	 */
	port->ignore_status_mask = 0;
	if (termios->c_iflag & IGNPAR)
		port->ignore_status_mask |= UART_ERR_OVERRUN;
	if (termios->c_iflag & IGNBRK && termios->c_iflag & IGNPAR)
		port->ignore_status_mask |= UART_ERR_FRAME;

	/*
	 * Ignore all characters if CREAD is not set.
	 */
	if ((termios->c_cflag & CREAD) == 0)
		port->ignore_status_mask |= UART_RXD_DUMMY;

	DBGOUT("%s (baud:%d, parity:%d, data:%d, stop:%d)\n", __func__, baud , parity, data, stop);
#ifdef CONFIG_SERIAL_RX_DMA_MODE		
	
	if(uart->rx_mode == NX_UART_OPERMODE_DMA) {
		if(uart->dmarxtr != NULL) {	
			
				nx_uart_ops_stop_rx(port);
				_stop_dma(uart);

				uart->dmarxtr->iom.length  = uart->real_baud/1000 ;
				uart->b_size = uart->dmarxtr->iom.length;
				uart->w_pos = 0;
				uart->r_pos = 0;
				uart->r_cnt = 0;

				DBGOUT("baudrate = %d Size = %d ",
						uart->real_baud, uart->b_size);
				
				soc_dma_set_mode(uart->dmarxtr,DMA_MODE_BUFFER);
				enqueue_dma(uart);
				_w_buf(uart);
				enqueue_dma(uart);
		}
	}	
	
#endif

	// set nexell uart register
	NX_UART_SetClockDivisorEnable(port->line, CFALSE);

	port->uartclk = CFG_SYS_PLL1_FREQ/c_div;
	NX_UART_SetClockDivisor (port->line, 0, c_div);
	
	NX_UART_SetClockPClkMode(port->line, NX_PCLKMODE_DYNAMIC);
   	NX_UART_SetInterruptEnableAll(port->line, CFALSE);

	NX_UART_SetFrameConfiguration(port->line, parity, data, stop);

	NX_UART_SetBRD(port->line, NX_UART_MakeBRD(baud, (CFG_SYS_PLL1_FREQ/c_div)));
	//NX_UART_SetBRD(port->line, NX_UART_MakeBRD(baud, (CFG_UART_DEBUG_CLKFREQ)));
   	NX_UART_ResetTxFIFO(port->line);
   	NX_UART_ResetRxFIFO(port->line);
   	NX_UART_SetFIFOEnb(port->line, CTRUE);

	// Enable RX Interrupt
	NX_UART_SetInterruptEnable(port->line, UART_MOD_INTNUM, CFALSE);
	NX_UART_SetInterruptEnable(port->line, UART_ERR_INTNUM, CFALSE);
	NX_UART_SetInterruptEnable(port->line, UART_TXD_INTNUM, CFALSE);
	NX_UART_SetInterruptEnable(port->line, UART_RXD_INTNUM, CTRUE);	

   	NX_UART_ClearInterruptPendingAll(port->line);
   	NX_UART_SetSYNCPendClear(port->line);

	//NX_UART_SetInterruptEnableAll(port->line, CTRUE);
	NX_UART_SetClockDivisorEnable(port->line, CTRUE);
	NX_UART_SetClockPClkMode(port->line, NX_PCLKMODE_ALWAYS);

	UART_WAIT_READY;

	/* enable irq */
	spin_unlock_irqrestore(&port->lock, flags);
	
}

static const char *nx_uart_ops_type(struct uart_port *port)
{
	struct nx_uart_port *uart = (struct nx_uart_port *)port;
	DBGOUT("%s (line:%d)\n", __func__, port->line);

	return uart->name;
} 

static void nx_uart_ops_release_port(struct uart_port *port)
{
	DBGOUT("%s (line:%d)\n", __func__, port->line);
}

static int nx_uart_ops_request_port(struct uart_port *port)
{
	DBGOUT("%s (line:%d)\n", __func__, port->line);
	return 0;
}

static void nx_uart_ops_config_port(struct uart_port *port, int flags)
{
	DBGOUT("%s (line:%d, type:%s)\n", __func__, port->line, (char *)port->type);
	port->type = PORT_NEXELL;
}

static int nx_uart_ops_verify_port(struct uart_port *port, struct serial_struct *ser)
{
	DBGOUT("%s (line:%d)\n", __func__, port->line);

	/* we don't want the core code to modify any port params */
	return -EINVAL;
}

static struct uart_ops nx_uart_ops = {
	.tx_empty		= nx_uart_ops_tx_empty,
	.set_mctrl		= nx_uart_ops_set_mctrl,
	.get_mctrl		= nx_uart_ops_get_mctrl,
	.stop_tx		= nx_uart_ops_stop_tx,
	.start_tx		= nx_uart_ops_start_tx,
	.stop_rx		= nx_uart_ops_stop_rx,
	.enable_ms		= nx_uart_ops_enable_ms,
	.break_ctl		= nx_uart_ops_break_ctl,
	.startup		= nx_uart_ops_startup,
	.shutdown		= nx_uart_ops_shutdown,
	.set_termios	= nx_uart_ops_set_termios,
	.type			= nx_uart_ops_type,
	.release_port	= nx_uart_ops_release_port,
	.request_port	= nx_uart_ops_request_port,
	.config_port	= nx_uart_ops_config_port,
	.verify_port	= nx_uart_ops_verify_port,
};


#ifdef CONFIG_SERIAL_RX_DMA_MODE		
/*---------------------------------------------------------------------------------------------
 * 	Uart DMA interrupt functions
 --------------------------------------------------------------------------------------------*/
static void nx_uart_rx_dma_irq_handler(struct nx_uart_port * uart, int timeout)
{
	struct uart_port *port = (struct uart_port *)uart;
	hrtimer_start( &uart->hr_timer, uart->ktime, HRTIMER_MODE_REL );

	NX_UART_SetInterruptEnable(port->line, UART_RXD_INTNUM, CFALSE);	
}

static irqreturn_t nx_uart_dma_irq_handler(int irq, void *param)
{
	struct nx_uart_port *uart = (struct nx_uart_port *)param;
	struct uart_port *port = (struct uart_port *)uart;

	hrtimer_start_expires( &uart->hr_timer, HRTIMER_MODE_REL );	
	soc_dma_irq_enable(uart->dmarxtr, 1);/* next command buffer irq*/
	_w_buf(uart);
	enqueue_dma(uart);
		
	trans_tty(uart,0);
	uart->r_cnt=0;
	NX_UART_SetInterruptEnable(port->line, UART_RXD_INTNUM, CTRUE);	
	_r_buf(uart);
		
	return IRQ_HANDLED;
}
#endif
/*---------------------------------------------------------------------------------------------
 * 	Uart interrupt functions
 --------------------------------------------------------------------------------------------*/
static void nx_uart_rx_irq_handler(struct uart_port *port)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)
	struct tty_struct *tty = port->state->port.tty;
#else
	struct tty_struct *tty = port->info->port.tty;
#endif
	unsigned int ch, flag;
	unsigned int err;
	int max_count = 256;

	DBGINTR(" rxirq...");
	
	while (max_count-- > 0) {

		if (NX_UART_GetRxFIFOCount(port->line)== 0)
			break;

		err  = NX_UART_GetErrorStatus(port->line);
		ch   = NX_UART_GetByte(port->line);
		flag = TTY_NORMAL;
		port->icount.rx++;
		
		/* error status */
		if (unlikely(err & (UART_ERR_OVERRUN |
					UART_ERR_PARITY	 |
					UART_ERR_FRAME   |
					UART_ERR_BREAK )) ) {
			printk(KERN_ERR "uart: rx error port:%d, ch=%c, err=%x\n", port->line, ch, err);

			/* break error */
			if (err & UART_ERR_BREAK) {
				port->icount.brk++;
				if (uart_handle_break(port))
					goto ignore_char;
			}
			/* parity error */
			if (err & UART_ERR_PARITY)
				port->icount.parity++;

			if (err & UART_ERR_FRAME)
				port->icount.frame++;

			if (err & UART_ERR_OVERRUN)
				port->icount.overrun++;

			/*
			 * Mask off conditions which should be ignored.
			 */
			err &= port->read_status_mask;

			if (err & UART_ERR_BREAK)
				flag = TTY_BREAK;
			else if (err & UART_ERR_PARITY)
				flag = TTY_PARITY;
			else if (err & UART_ERR_FRAME)
				flag = TTY_FRAME;
		}

		if (uart_handle_sysrq_char(port, ch))
			goto ignore_char;

		uart_insert_char(port, err, UART_LSR_OE, ch, flag);

	ignore_char:
		continue;
	}
//	tty_flip_buffer_push(tty); @FALINUX
	tty_flip_buffer_push(&port->state->port);
}

static void nx_uart_tx_irq_handler(struct uart_port *port)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)
	struct circ_buf *xmit = &port->state->xmit;
#else
	struct circ_buf *xmit = &port->info->xmit;
#endif
	DBGINTR(" txirq[%d]", port->line);

	if (port->x_char) {
		NX_UART_SendByte(port->line, port->x_char);
		port->icount.tx++;
		port->x_char=0;
		return;
	}

	if (uart_circ_empty(xmit) || uart_tx_stopped(port)) {
		nx_uart_ops_stop_tx(port);
		return;
	}

	do {
		NX_UART_SendByte(port->line, xmit->buf[xmit->tail]);
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		port->icount.tx++;
		if (uart_circ_empty(xmit))
			break;
	} while (NX_UART_GetTxFIFOCount(port->line) < port->fifosize);


	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(port);

	if (uart_circ_empty(xmit))
		nx_uart_ops_stop_tx(port);

	DBGINTR(" txe...");
}

static void nx_uart_err_irq_handler(struct uart_port *port)
{
	printk(KERN_ERR "%s (line:%d): uart error irq, not implement \n", __func__, port->line);
}

static void nx_uart_err_mod_handler(struct uart_port *port)
{
	printk(KERN_ERR "%s (line:%d): uart modem irq, not implement \n", __func__, port->line);
}


static irqreturn_t nx_uart_irq_handler(int irq, void *dev_id)
{
	struct uart_port *port = dev_id;
#ifdef CONFIG_SERIAL_RX_DMA_MODE		
	struct nx_uart_port *uart = dev_id;
#endif	
	unsigned int ctrl, pend, mask;
	DBGINTR("uart irq(port:%d, irq:%d)", port->line, irq);


	/* get pend and mask num */
	ctrl = (*(unsigned short *)(port->membase + UART_INT_PEND)  & 0xff);
	mask = ((ctrl & 0xf0) >> 4);
	pend = ( ctrl & 0x0f);
	
	if ( pend & (1 << UART_RXD_INTNUM) ) {	// 1

#ifdef CONFIG_SERIAL_RX_DMA_MODE		
		if(uart->rx_mode == NX_UART_OPERMODE_DMA)
			nx_uart_rx_dma_irq_handler(uart,1);	
		else	
#endif
			nx_uart_rx_irq_handler(port);
		
		/* clear pend */
  			NX_UART_ClearInterruptPending(port->line, UART_RXD_INTNUM);
			NX_UART_SetSYNCPendClear(port->line);
	}

	if ( pend & (1 << UART_TXD_INTNUM) ) {	// 0

		nx_uart_tx_irq_handler(port);

		/* clear pend */
		NX_UART_ClearInterruptPending(port->line, UART_TXD_INTNUM);
		NX_UART_SetSYNCPendClear(port->line);
	}

	if ( pend & (1 << UART_ERR_INTNUM) ) { 	// 2

		nx_uart_err_irq_handler(port);

		/* clear pend */
		NX_UART_ClearInterruptPending(port->line, UART_ERR_INTNUM);
		NX_UART_SetSYNCPendClear(port->line);
	}

	if ( pend & (1 << UART_MOD_INTNUM) ) { 	// 3

		if (mask & (1 << UART_MOD_INTNUM))
			nx_uart_err_mod_handler(port);

		/* clear pend */
		NX_UART_ClearInterruptPending(port->line, UART_MOD_INTNUM);
		NX_UART_SetSYNCPendClear(port->line);
	}

	DBGINTR(" exit\n");
	return IRQ_HANDLED;
}


/*---------------------------------------------------------------------------------------------
 * 	Uart console functions
 --------------------------------------------------------------------------------------------*/
static void nx_uart_putchar(struct uart_port *port, int ch)
{
	while ( !(NX_UART_GetTxRxStatus(port->line) & NX_UART_TX_BUFFER_EMPTY) ) {
		/* busy wait */;
	}
	NX_UART_SendByte(port->line, (U8)ch);
}

static void nx_console_write(struct console *co, const char *s, unsigned int count)
{
	struct uart_port *port = &nx_ports[co->index].port;
	uart_console_write(port, s, count, nx_uart_putchar);
}

static int __init nx_console_setup(struct console *co, char *options)
{
	struct nx_uart_port *uart;
	struct uart_port *port;

	int baud   =  UART_BAUDRATE;
	int bits   =  8;
	int parity = 'n';
	int flow   = 'n';

	DBGOUT("%s (id:%d, opt:%s)\n", __func__, (int)co->index, options);

	if (co->index == -1 || co->index >= ARRAY_SIZE(nx_ports))
		co->index = 0;

	uart = &nx_ports[co->index];
	port = &uart->port;

	/* uart module initialize */
	setup_uart_port(co->index);

	uart->count++;

	if (options)
		uart_parse_options(options, &baud, &parity, &bits, &flow);

	DBGOUT("console_setup (options:%s, baud:%d, parity:%d, bits:%d, flow:0x%x)\n",
		options, baud, parity, bits, flow);

	return uart_set_options(port, co, baud, parity, bits, flow);
}

static struct uart_driver uart_drv;

static struct console nx_console = {
	.name		= UART_TERMINAL,
	.write		= nx_console_write,
	.device		= uart_console_device,
	.setup		= nx_console_setup,
	.flags		= CON_PRINTBUFFER,
	.index		= -1,			/* don't modify */
	.data		= &uart_drv,
};

static int __init nx_console_init(void)
{
	DBGOUT("%s\n", __func__);

	init_uart_port();
	register_console(&nx_console);
	return 0;
}
console_initcall(nx_console_init);

/*---------------------------------------------------------------------------------------------
 * 	Uart platform driver functions
 --------------------------------------------------------------------------------------------*/
/*
 * #> cat /sys/console/baudrate.n
 */
static ssize_t baud_show(struct kobject *kobj, struct kobj_attribute *kattr,
				char *buf)
{		
	struct attribute *at = &kattr->attr;
	const char *c;
	char * s = buf;
	int line = 0;	

	c = &at->name[strlen("baud.")];
	line = simple_strtoul(c, NULL, 10);

	printk("uart[%d] request=%d,real=%d,dividor=%d,brd=%d \n",
		line, nx_ports[line].req_baud, nx_ports[line].real_baud, 
		nx_ports[line].baud_div, nx_ports[line].baud_brd);

	s += sprintf(s, "%d,%d,%d,%d,%d \n",
			line, nx_ports[line].req_baud, nx_ports[line].real_baud, 
			nx_ports[line].baud_div, nx_ports[line].baud_brd);	

	if (s != buf)
		*(s-1) = '\n';

	return (s-buf);	// read size
}

#if defined(CONFIG_SERIAL_NEXELL_UART_PORT0)
static struct kobj_attribute uart0_attr = __ATTR(baud.0, 0644, baud_show, NULL);
#endif

#if defined(CONFIG_SERIAL_NEXELL_UART_PORT1)
static struct kobj_attribute uart1_attr = __ATTR(baud.1, 0644, baud_show, NULL);
#endif

#if defined(CONFIG_SERIAL_NEXELL_UART_PORT2)
static struct kobj_attribute uart2_attr = __ATTR(baud.2, 0644, baud_show, NULL);
#endif

#if defined(CONFIG_SERIAL_NEXELL_UART_PORT3)
static struct kobj_attribute uart3_attr = __ATTR(baud.3, 0644, baud_show, NULL);
#endif

#if defined(CONFIG_SERIAL_NEXELL_UART_PORT4)
static struct kobj_attribute uart4_attr = __ATTR(baud.4, 0644, baud_show, NULL);
#endif

#if defined(CONFIG_SERIAL_NEXELL_UART_PORT5)
static struct kobj_attribute uart5_attr = __ATTR(baud.5, 0644, baud_show, NULL);
#endif


static struct attribute * p[] = {
#if defined(CONFIG_SERIAL_NEXELL_UART_PORT0)
	&uart0_attr.attr, NULL,
#endif

#if defined(CONFIG_SERIAL_NEXELL_UART_PORT1)
	&uart1_attr.attr, NULL,
#endif

#if defined(CONFIG_SERIAL_NEXELL_UART_PORT2)
	&uart2_attr.attr, NULL,
#endif

#if defined(CONFIG_SERIAL_NEXELL_UART_PORT3)
	&uart3_attr.attr, NULL,
#endif

#if defined(CONFIG_SERIAL_NEXELL_UART_PORT4)
	&uart4_attr.attr, NULL,
#endif

#if defined(CONFIG_SERIAL_NEXELL_UART_PORT5)
	&uart5_attr.attr, NULL,
#endif
};

static struct attribute_group port_group[] = {
#if defined(CONFIG_SERIAL_NEXELL_UART_PORT0)
	{ 
		.attrs = &p[0 * 2],
	},
#endif

#if defined(CONFIG_SERIAL_NEXELL_UART_PORT1)
	{ 
		.attrs = &p[1 * 2],
	},
#endif

#if defined(CONFIG_SERIAL_NEXELL_UART_PORT2)
	{ 
		.attrs = &p[2 * 2],
	},
#endif

#if defined(CONFIG_SERIAL_NEXELL_UART_PORT3)	
	{ 
		.attrs = &p[3 * 2],
	},
#endif

#if defined(CONFIG_SERIAL_NEXELL_UART_PORT4)
	{ 
		.attrs = &p[4 * 2],
	},
#endif

#if defined(CONFIG_SERIAL_NEXELL_UART_PORT5)
	{ 
		.attrs = &p[4 * 2],
	},
#endif

};


static int nx_uart_drv_probe(struct platform_device *pdev)
{
	struct nx_uart_port *uart = ( struct nx_uart_port *)&nx_ports[pdev->id].port;
	struct uart_port *port = &uart->port;
	struct kobject *kobj = &pdev->dev.kobj;
	int line = port->line;
	int ret;

	DBGOUT("%s (id:%d)\n", __func__, pdev->id);

	port->dev = &pdev->dev;

	uart_add_one_port(&uart_drv, port);

	/* 
	 * uart config attribute 
	 * : sys/devices/platform/nx-uart.n/baud.n
	 * n is port num
	 */
	ret = sysfs_create_group(kobj, &port_group[line]);
	if (0 > ret) {
		printk(KERN_ERR "Fail, %s create sysfs attribute ...\n", pdev->name);
		return ret;
	}	

	platform_set_drvdata(pdev, uart);	
	return 0;
}

static int nx_uart_drv_remove(struct platform_device *pdev)
{
	struct nx_uart_port *uart = platform_get_drvdata(pdev);
	struct uart_port *port = &uart->port;

	DBGOUT("%s (id:%d)\n", __func__, pdev->id);

	platform_set_drvdata(pdev, NULL);
	if (port) {
		uart_remove_one_port(&uart_drv, port);
	}
	return 0;
}

static int nx_uart_drv_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct nx_uart_port *uart = platform_get_drvdata(pdev);
	struct uart_port *port = &uart->port;
	PM_DBGOUT("+%s (line:%d)\n", __func__, port->line);

	if (port) {
		uart_suspend_port(&uart_drv, port);

		NX_UART_ResetTxFIFO(port->line);
		NX_UART_ResetRxFIFO(port->line);
	}

	PM_DBGOUT("-%s\n", __func__);
	return 0;
}

static int nx_uart_drv_resume(struct platform_device *pdev)
{
	struct nx_uart_port *uart = platform_get_drvdata(pdev);
	struct uart_port *port = &uart->port;
	PM_DBGOUT("+%s (line:%d)\n", __func__, port->line);

	if (port) {
		uart_resume_port(&uart_drv, port);
	}

	PM_DBGOUT("-%s\n", __func__);
	return 0;
}

static struct platform_driver uart_plat_drv = {
	.probe      = nx_uart_drv_probe,
	.remove     = nx_uart_drv_remove,
	.suspend	= nx_uart_drv_suspend,
	.resume		= nx_uart_drv_resume,
	.driver		= {
		.name   = UART_DEV_NAME,
	},
};

/*---------------------------------------------------------------------------------------------
 * 	Uart module init/exit functions
 --------------------------------------------------------------------------------------------*/
static struct uart_driver uart_drv = {
	.owner          = THIS_MODULE,
	.driver_name    = UART_DEV_NAME,
	.dev_name       = UART_TERMINAL,
	.major          = TTY_MAJOR,
	.minor          = 64,
	.nr             = ARRAY_SIZE(nx_ports),
	.cons           = &nx_console,
};

/*
 * #> cat /sys/console/serial
 */
static int serial_line   = 0;
static int serial_enable = 1;

static ssize_t serial_show(struct kobject *kobj, struct kobj_attribute *attr,
				char *buf)
{
	char *s = buf;
	DBGOUT("%s:\n", __func__);

	s += sprintf(s, "%d %s\n", serial_line, serial_enable ? "on" : "off");
	if (s != buf)
		*(s-1) = '\n';

	return (s - buf);
}

/*
 * #> echo 0 on  > /sys/console/serial
 * #> echo 0 off > /sys/console/serial
 */
static ssize_t serial_store(struct kobject *kobj, struct kobj_attribute *attr,
				const char *buf, size_t n)
{
	struct uart_port *port = NULL;
	char *ps, *pe;
	int len, line;

	ps = memchr(buf,  ' ', n);
	pe = memchr(buf, '\n', n);

	/* check argu */
	if (NULL == ps || NULL == pe)
		return n;

	/* get line num */
	line = simple_strtoul(buf, NULL, 10);
	if (line >= ARRAY_SIZE(nx_ports) || 0 > line)
		return n;

	ps  = memchr(buf, 'o', n);
	len = pe - ps;

	port = &nx_ports[line].port;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)
	if (port->state->port.tty != get_current_tty())
		return n;
#else
	if (port->info->port.tty != get_current_tty())
		return n;
#endif

	/* start serial */
	if (len == 2 && !strncmp(ps, "on", len)) {
		serial_line   = line;
		serial_enable = 1;

		console_start(&nx_console);		// register_console(&nx_console);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)
		tty_ldisc_flush(port->state->port.tty);
		start_tty(port->state->port.tty);
#else
		tty_ldisc_flush(port->info->port.tty);
		start_tty(port->info->port.tty);
#endif
	}

	/* stop serial */
	if (len == 3 && !strncmp(ps, "off", len)) {
		serial_line   = line;
		serial_enable = 0;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)
		stop_tty(port->state->port.tty);
#else
		stop_tty(port->info->port.tty);
#endif
		console_stop(&nx_console);		// unregister_console(&nx_console);
	}

	return n;
}

static struct kobj_attribute serial_attr =
			__ATTR(serial, 0644, serial_show, serial_store);


static struct attribute * g[] = {
	&serial_attr.attr,
	NULL,
};

static struct attribute_group attr_group = {
	.attrs = g,
};

static int __init nx_serial_drv_init(void)
{
	struct kobject * kobj;		
	int ret;

	DBGOUT("%s\n", __func__);

	ret = uart_register_driver(&uart_drv);
	if (ret) {
		printk(KERN_ERR "serial: failed to register uart device (%d) \n", ret);
		return ret;
	}

	ret = platform_driver_register(&uart_plat_drv);
	if (ret != 0) {
		printk(KERN_ERR "serial: failed to register platform driver (%d) \n", ret);
		uart_unregister_driver(&uart_drv);
	}
	
	kobj = kobject_create_and_add("console", NULL);
	if (! kobj)
		return -ENOMEM;

	return sysfs_create_group(kobj, &attr_group);
}

static void __exit nx_serial_drv_exit(void)
{
	DBGOUT("%s\n", __func__);
	platform_driver_unregister(&uart_plat_drv);
	uart_unregister_driver(&uart_drv);
}

module_init(nx_serial_drv_init);
module_exit(nx_serial_drv_exit);

MODULE_AUTHOR("jhkim <nexell.co.kr>");
MODULE_DESCRIPTION("Serial driver for Nexell");
MODULE_LICENSE("GPL");


