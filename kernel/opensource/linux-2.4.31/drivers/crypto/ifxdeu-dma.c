/******************************************************************************
**
** FILE NAME    : ifxdeu-dma.c
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
** Nov 2006     TC Chen  improve DMA performance
*******************************************************************************/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <asm/io.h> //dma_cache_inv

#ifdef CONFIG_CRYPTO_DEV_DANUBE_DMA	//deudma
#include <asm/danube/danube.h>
#include <asm/danube/danube_dma.h>
#include "ifxdeu-dma.h"
#include <asm/danube/danube_deu.h>
#include <asm/danube/danube_deu_structs.h>

_ifx_deu_device ifx_deu[1];
static int deu_dma_device_reserved;

int
deu_dma_intr_handler (struct dma_device_info *dma_dev, int status)
{
        volatile struct deu_dma_t *dma = (struct deu_dma_t *) DMA_CON;
	while (dma->controlr.BSY) {
		// this will not take long
	}

	// dummy function for dma driver
//	printk(KERN_INFO "%s %s %d\n",__FILE__, __func__, __LINE__);
	return 0;
}

/* Function to allocate buffers for receive descriptors */
u8 *
deu_dma_buffer_alloc (int len, int *byte_offset, void **opt)
{
	u8 *buffer = NULL;

	buffer = (u8 *) kmalloc (DEU_MAX_PACKET_SIZE, GFP_ATOMIC);
	dma_cache_inv ((unsigned long) buffer,DEU_MAX_PACKET_SIZE);

	*byte_offset = 0;
	return buffer;
}

/* Function to free buffers for transmit descriptors */
int
deu_dma_buffer_free (u8 * dataptr, void *opt)
{
#if 0				//no nesserary to free, free after dma_device_write immediately
	if (dataptr != NULL) {
		kfree (dataptr);
	}
#endif
	return 0;
}

int
deu_dma_reserve (void)
{
	if (deu_dma_device_reserved)
		return -EBUSY;
	deu_dma_device_reserved = 1;
	return 0;
}

int
deu_dma_release (void)
{
	if (!deu_dma_device_reserved)
		return -1;
	deu_dma_device_reserved = 0;
	return 0;
}

int
deu_dma_init (void)
{
	struct dma_device_info *dma_device = NULL;
	int i = 0;
	volatile struct deu_dma_t *dma = (struct deu_dma_t *) DMA_CON;
	struct dma_device_info *deu_dma_device_ptr;

	deu_dma_device_reserved = 0;

	deu_dma_device_ptr = dma_device_reserve ("DEU");
	if (!deu_dma_device_ptr) {
		printk ("DEU: reserve DMA fail!\n");
		return -1;
	}
	ifx_deu[0].dma_device = deu_dma_device_ptr;

	dma_device = deu_dma_device_ptr;

	dma_device->buffer_alloc = &deu_dma_buffer_alloc;
	dma_device->buffer_free = &deu_dma_buffer_free;
	dma_device->intr_handler = &deu_dma_intr_handler;

	dma_device->max_rx_chan_num = 1;
	dma_device->max_tx_chan_num = 1;

	dma_device->tx_burst_len = 4;

	for (i = 0; i < dma_device->max_rx_chan_num; i++) {
		dma_device->rx_chan[i]->packet_size = DEU_MAX_PACKET_SIZE;
		dma_device->rx_chan[i]->desc_len = 1;
		dma_device->rx_chan[i]->control = DANUBE_DMA_CH_ON;
		dma_device->rx_chan[i]->byte_offset = 0;
	}

	for (i = 0; i < dma_device->max_tx_chan_num; i++) {
		dma_device->tx_chan[i]->control = DANUBE_DMA_CH_ON;
		dma_device->tx_chan[i]->desc_len = 1;
	}

	dma_device->current_tx_chan = 0;
        dma_device->current_rx_chan = 0;

	dma_device_register (dma_device);
	for (i = 0; i < dma_device->max_rx_chan_num; i++) {
		(dma_device->rx_chan[i])->open (dma_device->rx_chan[i]);
	}

	dma->controlr.BS = 0;
	dma->controlr.RXCLS = 0;
	dma->controlr.EN = 1;
}

#endif
