#ifndef __NX_GPIOMON_DEV_H__
#define __NX_GPIOMON_DEV_H__

#include "ioc_magic.h"

enum {
	IOCTL_SET_MONITOR_NUM      = _IO(IOC_NX_MAGIC,  1),
	IOCTL_GET_MONITOR_VAL      = _IO(IOC_NX_MAGIC,  2),
};

#endif
