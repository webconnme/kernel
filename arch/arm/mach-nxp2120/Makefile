# Machine support

obj-y += nxp2120.o
obj-y += lldebug.o
obj-y += irq.o
obj-y += timer.o

obj-y += soc/
obj-y += prototype/

obj-y += board.o
obj-y += cpu_device.o
obj-y += board_device.o

obj-$(CONFIG_PM) 				+= pm.o sleep.o

obj-$(CONFIG_MACH_WEBCON)		+= mach-webcon.o
obj-$(CONFIG_ARCH_REQUIRE_GPIOLIB) += gpio.o
