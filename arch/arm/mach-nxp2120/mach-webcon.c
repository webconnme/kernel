/* linux/arch/arm/mach-nxp2120/mach-webcon.c
 *
 * Copyright 2015 FALINUX,Co.,Ltd.
 *	David You <frog@falinux.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/vmalloc.h>

#include <asm/setup.h>
#include <asm/pgtable.h>

#include <asm/mach/arch.h>
#include <asm/mach-types.h>

#include <mach/cfg_mem.h>	

/* nexell soc headers */
#include <mach/platform.h>
#include <mach/map_desc.h>
#include <mach/system.h>	

extern void             cpu_base_init(void);
extern void __init 		cpu_init_irq(void);
extern void __init 		cpu_timer_init(void);

extern void __init 		cpu_soc_init(void);
extern void __init 		cpu_device(void);
extern void 	 		board_init(void);		
extern void __init 		board_device(void);

#define TRACE_LINE   early_print(">>TRACE %s:%d\n", __FUNCTION__,__LINE__ );

// #define DEBUG 1

void	webcon_fixup(struct tag *tag, char **cmdline, struct meminfo *meminfo){
	
#ifdef DEBUG

	int nr_bank;

	{TRACE_LINE}
	
	printk(">> ATAG START ADDRESS = %p\n", tag );
	printk(">> CMDLINE = [%s]\n", *cmdline );
	
	printk( ">> meminfo.nr_banks = %d\n", meminfo->nr_banks );
	for( nr_bank = 0; nr_bank < meminfo->nr_banks; nr_bank++ )
	{
		printk( ">>   meminfo->bank[%d].start   = %08X\n", nr_bank, (int) meminfo->bank[nr_bank].start   );
		printk( ">>   meminfo->bank[%d].size    = %08X\n"  , nr_bank, (int) meminfo->bank[nr_bank].size  );
		printk( ">>   meminfo->bank[%d].highmem = %d\n"  , nr_bank, (int) meminfo->bank[nr_bank].highmem );
	}
#endif // DEBUG

    meminfo->nr_banks     	= 1;
	meminfo->bank[0].start 	= CFG_MEM_PHY_SYSTEM_BASE;
#ifndef CFG_MEM_PHY_DMAZONE_SIZE
    meminfo->bank[0].size	= CFG_MEM_PHY_SYSTEM_SIZE;
#else
	meminfo->bank[0].size	= CFG_MEM_PHY_SYSTEM_SIZE + CFG_MEM_PHY_DMAZONE_SIZE;
#endif  // CFG_MEM_PHY_DMAZONE_SIZE
	
#ifdef DEBUG

	{TRACE_LINE}
	
	printk( ">> meminfo.nr_banks = %d\n", meminfo->nr_banks );
	for( nr_bank = 0; nr_bank < meminfo->nr_banks; nr_bank++ )
	{
		printk( ">>   meminfo->bank[%d].start   = %08X\n", nr_bank, (int) meminfo->bank[nr_bank].start   );
		printk( ">>   meminfo->bank[%d].size    = %08X\n"  , nr_bank, (int) meminfo->bank[nr_bank].size  );
		printk( ">>   meminfo->bank[%d].highmem = %d\n"  , nr_bank, (int) meminfo->bank[nr_bank].highmem );
	}
	
#endif // DEBUG

}

void	webcon_reserve(void){
	
#ifdef DEBUG

    {TRACE_LINE}
	
#endif // DEBUG

}

void	webcon_map_io(void){
#ifdef DEBUG

    {TRACE_LINE}
	
#endif // DEBUG

	iotable_init(cpu_iomap_desc, ARRAY_SIZE(cpu_iomap_desc));

}
	
void	webcon_init_early(void){
#ifdef DEBUG

    {TRACE_LINE}
	
#endif // DEBUG

	cpu_base_init();
}
	
void	webcon_init_irq(void){
#ifdef DEBUG

    {TRACE_LINE}
	
#endif // DEBUG

	cpu_init_irq();
}

void	webcon_init_time(void){
#ifdef DEBUG

	{TRACE_LINE}
	
#endif // DEBUG
	
	cpu_timer_init();
}
	
void	webcon_init_machine(void){
#ifdef DEBUG

	{TRACE_LINE}
	
#endif // DEBUG
	
    board_init();
    cpu_soc_init(); 
		
    cpu_device();
    board_device();
}	

void	webcon_init_late(void){

#ifdef DEBUG
	{TRACE_LINE}
#endif // DEBUG

}

void	webcon_restart(char mode, const char *cmd){
#ifdef DEBUG

	printk(">>TRACE %s:%d\n", __FUNCTION__,__LINE__ );
	
#endif // DEBUG
	
	arch_reset(mode,cmd);
	
}

MACHINE_START(WEBCON, "WEBCON")
	.atag_offset = 0x100,

    .fixup         = webcon_fixup        ,
    .reserve       = webcon_reserve      ,
    .map_io        = webcon_map_io       ,
    .init_early    = webcon_init_early   ,
    .init_irq      = webcon_init_irq     ,
    .init_time     = webcon_init_time    ,
    .init_machine  = webcon_init_machine ,
    .init_late     = webcon_init_late    ,
    .restart       = webcon_restart      ,
	
MACHINE_END
