#include <linux/version.h>
#include <mach/platform.h>

    .macro	addruart, rp, rv, tmp
	
		ldr	\rp, =PB_UART_PHYS_BASE
		ldr	\rv, =PB_UART_VIRT_BASE
		
	.endm

	.macro  senduart,rd,rx
	
	    str	\rd, [\rx, #PB_UART_THB]
		
	.endm

	.macro  busyuart,rd,rx
	.endm

	.macro  waituart,rd,rx
1001:
		ldr	\rd, [\rx, #PB_UART_TRSTATUS]
		tst	\rd, #PB_UART_FIFO_EMPTY
		beq	1001b
	
	.endm
