/*
 * (C) Copyright 2012
 * Michael RYU <Michael.Ryu@parrot.com>
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
#ifndef __DIB_DXB_CTRL_H__
#define __DIB_DXB_CTRL_H__

/*------------------------------------------------------------------------------
 * 	data
 */
struct dib_dxb_info{
	unsigned int value;		/* true, false, ...... */
};

/*------------------------------------------------------------------------------
 * 	ioctl
 */
#include "ioc_magic.h"

#if 0
enum {
	IOCTL_DXB_POWER_HIGH		= _IO(IOC_NX_MAGIC,  1),
	IOCTL_DXB_POWER_LOW		= _IO(IOC_NX_MAGIC,  2),
	IOCTL_DXB_RESET_HIGH		= _IO(IOC_NX_MAGIC,  3),
	IOCTL_DXB_RESET_LOW		= _IO(IOC_NX_MAGIC,  4),
	IOCTL_DXB_SCL_OUT		= _IO(IOC_NX_MAGIC,  5),
	IOCTL_DXB_SDA_OUT		= _IO(IOC_NX_MAGIC,  6),
	IOCTL_DXB_SDA_IN		= _IO(IOC_NX_MAGIC,  7),
};
#else
#ifndef DXB_CTRL_DEV_MAJOR
#define DXB_CTRL_DEV_MAJOR              245
#endif
#define IOCTL_DXB_POWER_HIGH            _IO(DXB_CTRL_DEV_MAJOR, 1)
#define IOCTL_DXB_POWER_LOW             _IO(DXB_CTRL_DEV_MAJOR, 2)
#define IOCTL_DXB_RESET_HIGH            _IO(DXB_CTRL_DEV_MAJOR, 3)
#define IOCTL_DXB_RESET_LOW             _IO(DXB_CTRL_DEV_MAJOR, 4)
#define IOCTL_DXB_SCL_OUT               _IO(DXB_CTRL_DEV_MAJOR, 5)
#define IOCTL_DXB_SDA_OUT               _IO(DXB_CTRL_DEV_MAJOR, 6)
#define IOCTL_DXB_SDA_IN                _IO(DXB_CTRL_DEV_MAJOR, 7)
#endif
#if defined(CONFIG_ARCH_REQUIRE_GPIOLIB)
#define ARCH_NR_GPIOS			(32*5+8)	 /* A,B,C,D,E and Alive - software */

#include <asm-generic/gpio.h>

#define gpio_to_irq			__gpio_to_irq
#define gpio_get_value			__gpio_get_value
#endif

#endif /* End of __DIB_DXB_CTRL_H__ */
