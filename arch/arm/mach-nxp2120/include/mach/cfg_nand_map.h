#define NAND_BLOCK_SIZE		(0x20000)

static struct mtd_partition partition_map[] = {
	{
		.name		= "uboot",
		.offset		= 0,
		.size	    = 2 * SZ_1M,
	}, {
		.name		= "kernel",
		.offset		= MTDPART_OFS_APPEND,
		.size	    = 8 * SZ_1M,
	}, {
		.name		= "Ramdisk",
		.offset		= MTDPART_OFS_APPEND,
		.size	    = 16 * SZ_1M,
	}, {
		.name		= "App",
		.offset		= MTDPART_OFS_APPEND,
		.size	    = 32 * SZ_1M,
	}, {
		.name		= "user data",
		.offset		= MTDPART_OFS_APPEND,
		.size	    = 6 * SZ_1M,
	},
};

/*
SZ_512K
    0000 0000 :   80000 (512K)  : bootloader
    0008 0000 :   40000 (256K)  : uboot env
    000C 0000 :   40000 (256K)  : dtb
    0010 0000 :  400000 (  4M)  : kernel
    0050 0000 :  B00000 ( 11M)  : ramdisk
    
    0100 0000 : rest            : mtd
	
*/

	
