/*
 *  linux/arch/arm/kernel/sys_arm.c
 *
 *  Copyright (C) People who wrote linux/arch/i386/kernel/sys_i386.c
 *  Copyright (C) 1995, 1996 Russell King.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  This file contains various random system calls that
 *  have a non-standard calling sequence on the Linux/arm
 *  platform.
 */
#include <linux/export.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/sem.h>
#include <linux/msg.h>
#include <linux/shm.h>
#include <linux/stat.h>
#include <linux/syscalls.h>
#include <linux/mman.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/ipc.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

/*
 * Since loff_t is a 64 bit type we avoid a lot of ABI hassle
 * with a different argument ordering.
 */
asmlinkage long sys_arm_fadvise64_64(int fd, int advice,
				     loff_t offset, loff_t len)
{
	return sys_fadvise64_64(fd, offset, len, advice);
}

#define    ZEROBOOT_NONE          0   // 아무것도 하지 않는다. 내부적으로 시험용으로 사용한다. 
#define    ZEROBOOT_SNAPSHOT      1   // 스냅샷을 시작한다. 
#define    ZEROBOOT_GET_STATE     2   // 제로부트 상태를 얻는다. 

asmlinkage void * sys_zeroboot_syscall( int cmd, __user void *data )
{
	long  str_size;
	char *kstr;
	
	printk( "SYSTEM CALL ZEROBOOT cmd = %d\n", cmd );
	switch( cmd ){
	case ZEROBOOT_NONE      : printk( "    ZEROBOOT_NONE\n"  ); 
               	
                               str_size = strlen_user( (char *) data );	
							   kstr = kmalloc( str_size, GFP_KERNEL );
    	                       copy_from_user ( kstr, (char *) data, str_size);
		                       printk( "data = [%s]\n", kstr );
		                       kfree( kstr );
	
	                           return (void *) str_size;
							   
    case ZEROBOOT_SNAPSHOT  : printk( "    ZEROBOOT_SNAPSHOT\n"  ); break;
    case ZEROBOOT_GET_STATE : printk( "    ZEROBOOT_GET_STATE\n" ); break;
	default                 : printk( "    UNKNOW CMD\n" );         break;
	    
	}
	
	return (void *) 0;
}

