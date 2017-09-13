/******************************************************************************
**
** FILE NAME    : ifxdeu-dma.h
** PROJECT      : Danube
** MODULES     	: crypto
**
** DATE         : 23 Oct 2006
** AUTHOR       : Lee Yao Chye
** DESCRIPTION  : Support DEU driver in DMA mode.
** COPYRIGHT    : 	Copyright (c) 2006
**			Infineon Technologies AG
**			Am Campeon 1-12, 85579 Neubiberg, Germany
**
**    This program is free software; you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or
**    (at your option) any later version.
**
** HISTORY
** $Date        $Author  $Comment
*******************************************************************************/

/************************************************************************
 * Cryptographic API.
 *
 * ---------------------------------------------------------------------------
 * Change Log:
 * yclee 17 Aug 2006: dma_device_reserve() can only be called once
 * ---------------------------------------------------------------------------
 ************************************************************************/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/crypto.h>
#include <asm/scatterlist.h>
#include <asm/byteorder.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#ifdef CONFIG_CRYPTO_DEV_DANUBE_DMA
#include <asm/danube/danube_dma.h>
#endif

#define DEU_MAX_PACKET_SIZE 2048

typedef struct ifx_deu_device {
	struct dma_device_info *dma_device;
	u8 *dst;
	u8 *src;
	int len;
	int dst_count;
	int src_count;
	int recv_count;
	int packet_size;
	int packet_num;
	wait_queue_t wait;
} _ifx_deu_device;

extern _ifx_deu_device ifx_deu[1];

extern int deu_dma_intr_handler (struct dma_device_info *, int);
extern u8 *deu_dma_buffer_alloc (int, int *, void **);
extern int deu_dma_buffer_free (u8 *, void *);
