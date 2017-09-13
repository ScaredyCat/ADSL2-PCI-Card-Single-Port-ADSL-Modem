/******************************************************************************
**
** FILE NAME    : ifx_sd_card.h
** PROJECT      : Danube
** MODULES      : SDIO
**
** DATE         : 1 Jan 2006
** AUTHOR       : TC Chen
** DESCRIPTION  : SDIO Driver
** COPYRIGHT    :       Copyright (c) 2006
**                      Infineon Technologies AG
**                      Am Campeon 1-12, 85579 Neubiberg, Germany
**
**    This program is free software; you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or
**    (at your option) any later version.
**
** HISTORY
** $Version $Date      $Author     $Comment
*******************************************************************************/

#include <linux/ioctl.h>

/* version dependencies have been confined to a separate file */

/*
 * Macros to help debugging
 */
#define SD_MEMORY_DEBUG

#undef PDEBUG			/* undef it, just in case */
#ifdef SD_MEMORY_DEBUG
#  ifdef __KERNEL__
/* This one if debugging is on, and kernel space */
#    define PDEBUG(fmt, args...) printk( KERN_DEBUG "sd_memory: " fmt, ## args)
#  else
/* This one for user space */
#    define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#  endif
#else
#  define PDEBUG(fmt, args...)	/* not debugging: nothing */
#endif

#undef PDEBUGG
#define PDEBUGG(fmt, args...)	/* nothing: it's a placeholder */

#define SD_MEMORY_DEVS 2	/* two disks */
#define SD_MEMORY_RAHEAD 2	/* two sectors */
#define SD_MEMORY_BLKNR 2048	/* two megs each */
#define SD_MEMORY_BLKSIZE 1024	/* 1k blocks */
#define SD_MEMORY_HARDSECT 512	/* 512-byte hardware sectors */

/*
 * The sd_memory device is removable: if it is left closed for more than
 * half a minute, it is removed. Thus use a usage count and a
 * kernel timer
 */

typedef struct IFXSDCard {
	int usage;
	spinlock_t lock;
	u8 *data;
#ifndef TEST_WITH_FILE
	sdio_card_driver_t card_driver;
#endif
	int slot;
	int load;
	sdio_card_t *card;
	uint8_t block_size_shift;
	uint32_t size;
	uint32_t nBlock;
} IFXSDCard_t;
