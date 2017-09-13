/*
 * Handle mapping of the flash memory access routines
 * on Danube  based devices.
 *
 * Copyright(C) 2004 peng.liu@infineon.com
 *
 * This code is GPLed
 *
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <asm/io.h>

#include <linux/init.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/cfi.h>
#include <asm/danube/danube.h>


#if defined(CONFIG_PCI) && defined(CONFIG_DANUBE_CHIP_A11) && defined(CONFIG_DANUBE_EBU_PCI_SOFTWARE_ARBITOR)
  #define EBU_PCI_SOFTWARE_ARBITOR
#endif

#ifdef EBU_PCI_SOFTWARE_ARBITOR
  #define DANUBE_PCI_REG32( addr )          (*(volatile u32 *)(addr))
#endif


#define FLASH_BANK_MAX 1
#if 0
#define DANUBE_MTD_DMSG(fmt, args...) printk( KERN_ERR  "%s: " fmt,__FUNCTION__, ## args)
#else
#define DANUBE_MTD_DMSG(fmt, args...)
#endif
#define DANUBE_MTD_EMSG(fmt, args...) printk( KERN_ERR  "%s: " fmt,__FUNCTION__, ## args)
/* trivial struct to describe partition information */
struct mtd_part_def
{
	int nums;
	unsigned char *type;
	struct mtd_partition* mtd_part;
};

/* static struct mtd_info *mymtd; */
static struct mtd_info* mtd_banks[FLASH_BANK_MAX];
static struct map_info* map_banks[FLASH_BANK_MAX];
static struct mtd_part_def part_banks[FLASH_BANK_MAX];
static unsigned long num_banks;
static unsigned long start_scan_addr;
static int probing = 0;

#define DANUBE_MTD_REG32( addr )		  (*(volatile u32 *)(addr))
static unsigned long irq_flags;
static void inline enable_ebu(void)
{
#ifdef EBU_PCI_SOFTWARE_ARBITOR
    u32 temp_buffer;

    /* drive CFRAME_N/GPIO7 to 0 */
  #if 0
    *DANUBE_GPIO_P0_OUT &= (~(1<<7));
    *DANUBE_GPIO_P0_OD |=  ((1<<7));
    *DANUBE_GPIO_P0_DIR |=  ((1<<7));
    *DANUBE_GPIO_P0_ALTSEL1 &= (~(1<<7));
    *DANUBE_GPIO_P0_ALTSEL0 &= (~(1<<7));
  #endif
    //  printk("\nenable_ebu\n");

    //Delay before enabling ebu
    for(temp_buffer=0; temp_buffer<9999;temp_buffer++);

    /* disable int/ext pci request */
    temp_buffer = DANUBE_PCI_REG32(PCI_CR_PC_ARB_REG);
    temp_buffer &= ~0x0C00;
    DANUBE_PCI_REG32(PCI_CR_PC_ARB_REG) = temp_buffer;
    temp_buffer |= 0x3300;
    DANUBE_PCI_REG32(PCI_CR_PC_ARB_REG) = temp_buffer;

    /* poll for pci bus idle */
    temp_buffer = DANUBE_PCI_REG32(PCI_CR_PC_ARB_REG);
//  printk("PCI_CR_PC_ARB_REG1: %08X",temp_buffer);
    while ((temp_buffer & 0x300000) != 0x300000) {
//      printk("PCI_CR_PC_ARB_REG: %08X",temp_buffer);
        temp_buffer = DANUBE_PCI_REG32(PCI_CR_PC_ARB_REG);
    };

    DANUBE_PCI_REG32(0xBE105400) = 0;

    /* unmask CFRAME_MASK */
    /* changing PCI's Config Space via internal path */
    /* might need to change to external path */
    DANUBE_PCI_REG32(0xBE105430) = 0x00000103;
    temp_buffer = DANUBE_PCI_REG32(0xB700006C);
    temp_buffer &= (~0x00000001);
    DANUBE_PCI_REG32(0xB700006C) = temp_buffer;
    DANUBE_PCI_REG32(0xBE105430) = 0x01000103;
    temp_buffer = DANUBE_PCI_REG32(0xB700006C); // do dummy read

  #if 0
    printk("\nwaiting ebu ownnership");
    while ((DANUBE_PCI_REG32(PCI_CR_PC_ARB_REG) & 0x400000) != 0) {
//      *DANUBE_GPIO_P0_OUT |= ((1<<7));
        printk(".");
    }
//  *DANUBE_GPIO_P0_OUT &= (~(1<<7));
    printk("done\n");
  #endif
#endif
}

static void inline disable_ebu(void)
{
#ifdef EBU_PCI_SOFTWARE_ARBITOR
    u32 temp_buffer;
    //printk("\ndisable_ebu\n");
    //Delay before enabling ebu
    for(temp_buffer=0; temp_buffer<9999;temp_buffer++);

     DANUBE_PCI_REG32(0xBE105400) = 0x2;
    /* unmask CFRAME_MASK */
    /* changing PCI's Config Space via internal path */
    /* might need to change to external path */
    DANUBE_PCI_REG32(0xbE105430) = 0x00000103;
    temp_buffer = DANUBE_PCI_REG32(0xB700006C);
    temp_buffer |= 0x00000001;
    DANUBE_PCI_REG32(0xB700006C) = temp_buffer;
    DANUBE_PCI_REG32(0xbE105430) = 0x01000103;

    /* enable int/ext pci request */
    temp_buffer = DANUBE_PCI_REG32(PCI_CR_PC_ARB_REG);
    temp_buffer &= (~0x3300);
    temp_buffer |= 0x0C00;
    DANUBE_PCI_REG32(PCI_CR_PC_ARB_REG) = temp_buffer;
#endif
}

__u8 danube_read8(struct map_info *map, unsigned long ofs)
{
	u8 temp;
	enable_ebu();
	temp = *((__u8 *)(map->map_priv_1 + ofs));
	disable_ebu();
	DANUBE_MTD_DMSG("8: [%p + %x] ==> %x \n",map->map_priv_1, ofs, temp);
	return temp;
}

__u16 danube_read16(struct map_info *map, unsigned long ofs)
{
 /* Modify the address offset to account the EBU address swap.
  * Needed only for probing.
  */
	u16 temp;
	if (probing)
		ofs ^= 2;
	enable_ebu();
	temp = *((__u16 *)(map->map_priv_1 + ofs));
	disable_ebu();
	DANUBE_MTD_DMSG("16: [%p + %x] ==> %x \n",map->map_priv_1, ofs, temp);
	return temp;
}

__u32 danube_read32(struct map_info *map, unsigned long ofs)
{
	u32 temp;
	enable_ebu();
	temp = *((__u32 *)(map->map_priv_1 + ofs));
	disable_ebu();
	DANUBE_MTD_DMSG("32: [%p + %x] ==> %x \n",map->map_priv_1, ofs, temp);
	return temp;
}

void danube_copy_from(struct map_info *map, void *to, unsigned long from, ssize_t len)
{
	DANUBE_MTD_DMSG("from:%x to:%x len:%d",from,to,len);
#ifdef CONFIG_DANUBE
//work around for EBU access that causes unaligned access exception
	u8 *p;
	u8 *to_8;

	enable_ebu();
	from = (unsigned long) (from + map->map_priv_1);
	if  ( (((unsigned long)to) & 3) == (from & 3) ) {
		memcpy_fromio(to, (void *)from, len);
	}else {
		p = (u8 *) (from);
		to_8 = (u8 *) (to);
		while(len--){
			*to_8++ = *p++;
		}
	}
	disable_ebu();
#else
	memcpy_fromio(to, (void *)(map->map_priv_1 + from), len);
#endif

}

void danube_write8(struct map_info *map, __u8 d, unsigned long adr)
{
	enable_ebu();
	*((__u8 *)(map->map_priv_1 + adr)) = d;
	disable_ebu();
	DANUBE_MTD_DMSG("8: [%p + %x] <== %x \n",map->map_priv_1, adr, d);
}

void danube_write16(struct map_info *map, __u16 d, unsigned long adr)
{
 /* Modify the address offset to account the EBU address swap.
  * Needed only for probing.
  */
	if (probing)
		adr ^= 2;
	enable_ebu();
	*((__u16 *)( map->map_priv_1 + adr)) = d;
	disable_ebu();
	DANUBE_MTD_DMSG("16: [%p + %x] <== %x \n",map->map_priv_1, adr, d);
}

void danube_write32(struct map_info *map, __u32 d, unsigned long adr)
{
	enable_ebu();
	*((__u32 *)(map->map_priv_1 + adr)) = d;
	disable_ebu();
	DANUBE_MTD_DMSG("32: [%p + %x] <== %x \n",map->map_priv_1, adr, d);
}

void danube_copy_to(struct map_info *map, unsigned long to, const void *from, ssize_t len)
{
	DANUBE_MTD_DMSG("from:%x to:%x len:%d",from,to,len);
	enable_ebu();
	memcpy_toio((void *)(map->map_priv_1 + to), from, len);
	disable_ebu();
}

struct map_info danube_map = {
	name:		"DANUBE",
	size:	  0x2000,
	buswidth:	2,
	read8:		danube_read8,
	read16:		danube_read16,
	read32:		danube_read32,
	copy_from:	danube_copy_from,
	write8:		danube_write8,
	write16:	danube_write16,
	write32:	danube_write32,
	copy_to:	danube_copy_to
};

/*
 * Here are partition information for all known Danube series devices.
 * See include/linux/mtd/partitions.h for definition of the mtd_partition
 * structure.
 */

#ifdef CONFIG_MTD_PARTITIONS

/* partition definition for first flash bank
 * also ref. to "drivers/char/flash_config.c"
 */
 
static struct mtd_partition danube_partitions[] = {
        {
          name:         "U-Boot",       /* U-Boot firmware */
          offset:       0x00000000,
          size:         0x00030000,     /* 192k  */
/*        mask_flags:   MTD_WRITEABLE,     force read-only */
        },
        {
          name:         "Environment",  /* U-Boot parameter */
          offset:       0x00030000,
          size:         0x00020000,     /* 128k */
/*        mask_flags:   MTD_WRITEABLE,     force read-only */
        },
        {
          name:         "rootfs",       /* default root filesystem */
          offset:       0x00050000,
          size:         0x00230000,     /* 2M + 4x128k */
/*        mask_flags:   MTD_WRITEABLE,     force read-only */
        },
        {
          name:         "Linux",        /* default kernel image */
          offset:       0x00280000,
          size:         0x00120000,     /* 128k x 7 + 64k */
/*        mask_flags:   MTD_WRITEABLE,     force read-only */
        },
        {
          name:         "firmware",     /* fireware */
          offset:       0x003a0000,
          size:         0x00040000,     /* 256K */
/*        mask_flags:   MTD_WRITEABLE,     force read-only */
        },
        {
          name:         "User-Data",    /* raw write configuration place */
          offset:       0x003e0000,
          size:         0x00010000,     /* 128K */
/*        mask_flags:   MTD_WRITEABLE,     force read-only */
        },
//6080401:hsumc add voip partition
        {
          name:         "voip-Data",    /* raw write configuration place */
          offset:       0x003f0000,
          size:         0x00010000,
/*        mask_flags:   MTD_WRITEABLE,     force read-only */
        }
};

#endif	/* CONFIG_MTD_PARTITIONS */

#define NB_OF(x)  (sizeof(x)/sizeof(x[0]))

unsigned long flash_start, flash_size;

int __init init_danube_mtd(void)
{
	int idx = 0, ret = 0;
	unsigned long mtd_size = 0;

	/* setting EBU */
	*DANUBE_EBU_BUSCON0 = 0x1d7ff;
	/* GPIO: address23 */
	//*DANUBE_GPIO_P1_DIR = ((*DANUBE_GPIO_P1_DIR) | 0x80); /* P1.7 output*/
	//*DANUBE_GPIO_P1_ALTSEL0 = ((*DANUBE_GPIO_P1_ALTSEL0) & (~0x80)); /* P1.7 ALT:00*/
	//*DANUBE_GPIO_P1_ALTSEL1 = ((*DANUBE_GPIO_P1_ALTSEL1) & (~0x80));

	/* request maximum flash size address space */
	flash_start &= (~0xa0000000);
	start_scan_addr = (unsigned long)ioremap(flash_start, flash_size);
	if (!start_scan_addr) {
		DANUBE_MTD_EMSG ("Failed to ioremap address:0x%x\n",flash_start);
		return -EIO;
	}
	DANUBE_MTD_EMSG("start_scan_addr: %8x\n",start_scan_addr);

	for(idx = 0 ; idx < FLASH_BANK_MAX ; idx++) {
		if(mtd_size >= flash_size)
			break;

		printk ("%s: chip probing count %d\n",
			__FUNCTION__, idx);

		map_banks[idx] = (struct map_info *)kmalloc(sizeof(struct map_info), GFP_KERNEL);
		if(map_banks[idx] == NULL) {
			ret = -ENOMEM;
			goto error_mem;
		}
		memset((void *)map_banks[idx], 0, sizeof(struct map_info));
		map_banks[idx]->name = (char *)kmalloc(16, GFP_KERNEL);
		if(map_banks[idx]->name == NULL) {
			ret = -ENOMEM;
			goto error_mem;
		}
		memset((void *)map_banks[idx]->name, 0, 16);

		sprintf(map_banks[idx]->name, "Danube Bank %d", idx);
		map_banks[idx]->buswidth = 2;
		map_banks[idx]->read8 = danube_read8;
		map_banks[idx]->read16 = danube_read16;
		map_banks[idx]->read32 = danube_read32;
		map_banks[idx]->copy_from = danube_copy_from;
		map_banks[idx]->write8 = danube_write8;
		map_banks[idx]->write16 = danube_write16;
		map_banks[idx]->write32 = danube_write32;
		map_banks[idx]->copy_to = danube_copy_to;
		map_banks[idx]->map_priv_1 = start_scan_addr;
    /* make sure probing works */
		map_banks[idx]->size = 0x2000;
		printk("Danube: probing address:%x\n",map_banks[idx]->map_priv_1);

		/* start to probe flash chips */
		probing = 1;
		mtd_banks[idx] = do_map_probe("cfi_probe", map_banks[idx]);
		probing = 0;
		if(mtd_banks[idx]) {
			struct cfi_private *cfi;

			mtd_banks[idx]->module = THIS_MODULE;
			mtd_size += mtd_banks[idx]->size;
			num_banks++;
			cfi = (struct cfi_private *)map_banks[idx]->fldrv_priv;
			/* Swap the address for correct access via EBU */
			cfi->addr_unlock1 ^= 2;
			cfi->addr_unlock2 ^= 2;
			printk("%s: bank%ld, name:%s, size:%dbytes \n", __FUNCTION__, num_banks,
			mtd_banks[idx]->name, mtd_banks[idx]->size);
		}
	}

	/* no supported flash chips found */
	if(!num_banks)
	{
		printk("Danube: No support flash chips found!\n");
		ret = -ENXIO;
		goto error_mem;
	}

#ifdef CONFIG_MTD_PARTITIONS
	/*
	 * Select Static partition definitions
	 */
	part_banks[0].mtd_part = danube_partitions;
	part_banks[0].type = "static image";
	part_banks[0].nums = NB_OF(danube_partitions);
/*
	part_banks[1].mtd_part = danube_fs_partitions;
	part_banks[1].type = "static file system";
	part_banks[1].nums = NB_OF(danube_fs_partitions);
*/

	for(idx = 0; idx < num_banks ; idx++) {
#ifdef CONFIG_MTD_CMDLINE_PARTS
		sprintf(mtdid, "%d", idx);
		n = parse_cmdline_partitions(mtd_banks[idx],
					     &part_banks[idx].mtd_part,
					     mtdid);
		DANUBE_MTD_DMSG ("%d command line partitions on bank %s\n",n, mtdid);
		if (n > 0) {
			part_banks[idx].type = "command line";
			part_banks[idx].nums = n;
		}
#endif
		if (part_banks[idx].nums == 0) {
			printk (KERN_NOTICE "DANUBE flash%d: "
				"no partition info available, "
				"registering whole flash at once\n", idx);
			add_mtd_device(mtd_banks[idx]);
		} else {
			printk (KERN_NOTICE "DANUBE flash%d: "
				"Using %s partition definition\n",
				idx, part_banks[idx].type);
			add_mtd_partitions(mtd_banks[idx],
					   part_banks[idx].mtd_part,
					   part_banks[idx].nums);
		}
	}
#else
	printk (KERN_NOTICE "DANUBE flash: registering %d whole "
		"flash banks at once\n", num_banks);
	for(idx = 0 ; idx < num_banks ; idx++)
		add_mtd_device(mtd_banks[idx]);
#endif
	return 0;
error_mem:
	for(idx = 0 ; idx < FLASH_BANK_MAX ; idx++) {
		if(map_banks[idx] != NULL) {
			if(map_banks[idx]->name != NULL) {
				kfree(map_banks[idx]->name);
				map_banks[idx]->name = NULL;
			}
			kfree(map_banks[idx]);
			map_banks[idx] = NULL;
		}
	}
error:
	iounmap((void *)start_scan_addr);
	return ret;
}

static void __exit cleanup_danube_mtd(void)
{
	unsigned int idx = 0;
	for(idx = 0 ; idx < num_banks ; idx++)
	{
		/* destroy mtd_info previously allocated */
		if (mtd_banks[idx]) {
			del_mtd_partitions(mtd_banks[idx]);
			map_destroy(mtd_banks[idx]);
		}
		/* release map_info not used anymore */
		kfree(map_banks[idx]->name);
		kfree(map_banks[idx]);
	}
	if (start_scan_addr) {
		iounmap((void *)start_scan_addr);
		start_scan_addr = 0;
	}
}

module_init(init_danube_mtd);
module_exit(cleanup_danube_mtd);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("peng.liu@infineon.com");
MODULE_DESCRIPTION("MTD map driver for DANUBE boards");
