#define NAND_BLOCK_SIZE		(0x20000)

static struct mtd_partition partition_map[] = {
	{
		.name		= "uboot",
		.offset		= 0,
		.size	    = 3 * SZ_256K,
	}, {
		.name		= "uboot env",
		.offset		= MTDPART_OFS_APPEND,
		.size	    = SZ_256K,
/*	}, {
		.name		= "dtb",
		.offset		= MTDPART_OFS_APPEND,
		.size	    = SZ_256K,
*/	}, {
		.name		= "ext",
		.offset		= MTDPART_OFS_APPEND,
		.size	    = 3 * SZ_1M,
	}, {
		.name		= "env kernel",
		.offset		= MTDPART_OFS_APPEND,
		.size	    = 8 * SZ_1M,
	}, {
		.name		= "env Ramdisk",
		.offset		= MTDPART_OFS_APPEND,
		.size	    = 32 * SZ_1M,
	}, {
		.name		= "config data",
		.offset		= MTDPART_OFS_APPEND,
		.size	    = 12 * SZ_1M,
	}, {
		.name		= "user data",
		.offset		= MTDPART_OFS_APPEND,
		.size	    = 192 * SZ_1M,
	}, {
		.name 		= "app kernel",
		.offset		= MTDPART_OFS_APPEND,
		.size	    = 8 * SZ_1M,
	}, {
		.name		= "app rootfs",
		.offset		= MTDPART_OFS_APPEND,
		.size		= MTDPART_SIZ_FULL,
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

	
