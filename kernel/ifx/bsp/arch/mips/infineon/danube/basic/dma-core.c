/******************************************************************************
**
** FILE NAME    : dma-core.c
** PROJECT      : Danube
** MODULES      : Central DMA
**
** DATE         : 26 SEP 2005
** AUTHOR       : Wu Qi Ming
** DESCRIPTION  : Central DMA Driver
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
** $Date        $Author         $Comment
** 26 SEP 2005  Wu Qi Ming      Initiate Version
** 25 OCT 2006  Xu Liang        Add GPL header.
** 10 Nov 2006  TC Chen		change the descriptor length
** 20 Dec 2006  TC Chen		Fix cache sync issue.
**  8 Jan 2007  Xu Liang        Declare g_danube_dma_in_process and 
**                              g_danube_dma_int_status as volatile object
**                              and fix problem caused by compiler optimization
*******************************************************************************/

#include <linux/config.h>	/* retrieve the CONFIG_* macros */
#if defined(CONFIG_MODVERSIONS) && !defined(MODVERSIONS)
#define MODVERSIONS
#endif

#if defined(MODVERSIONS) && !defined(__GENKSYMS__)
#include <linux/modversions.h>
#endif

#ifndef EXPORT_SYMTAB
#define EXPORT_SYMTAB		/* need this one 'cause we export symbols */
#endif

#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/proc_fs.h>
#include <linux/stat.h>
#include <linux/mm.h>
#include <linux/tty.h>
#include <linux/selection.h>
#include <linux/kmod.h>
#include <linux/vmalloc.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <linux/errno.h>
#include <asm/io.h>
#include <linux/wrapper.h>
#include <asm-mips/semaphore.h>

#include <asm/danube/danube.h>
#include <asm/danube/irq.h>
#include <asm/danube/danube_dma.h>
#include "dma-core.h"

#define ENABLE_RX_DPLUS_PATH            0

#if defined(CONFIG_DANUBE_ETHERNET_D2) || defined(CONFIG_DANUBE_PPA) || defined(CONFIG_DANUBE_PPA_MODULE)
#undef ENABLE_RX_DPLUS_PATH
#define ENABLE_DANUBE_ETHERNET_D2     1
#else
#define ENABLE_DANUBE_ETHERNET_D2     0
#endif

/*****************definitions for the macros used in dma-core.c***************/

#undef USE_TX_INT

#define IFX_SUCCESS 1
#define IFX_ERROR   0

#define DANUBE_DMA_EMSG(fmt, args...) printk( KERN_ERR  "%s: " fmt,__FUNCTION__, ## args)
#define DANUBE_DMA_WRREG_PROT(chan,addr,value) \
          *DANUBE_DMA_CS=chan;  \
          *addr=value;

#define DANUBE_DMA_RDREG_PROT(chan,addr,value) \
          *DANUBE_DMA_CS=chan;  \
          value=*addr;

#define DANUBE_DMA_ORREG_PROT(chan,addr,value) \
          *DANUBE_DMA_CS=chan;  \
          *addr|=value;

#define DANUBE_DMA_ANDREG_PROT(chan,addr,value) \
          *DANUBE_DMA_CS=chan;  \
          *addr &=value;

/*25 descriptors for each dma channel,4096/8/20=25.xx*/
//HANK2007/7/4 01:59¤U¤È
//#define DANUBE_DMA_DESCRIPTOR_OFFSET 25

#if !defined(ENABLE_RX_DPLUS_PATH) || !ENABLE_RX_DPLUS_PATH
#define MAX_DMA_DEVICE_NUM  6	/*6 max ports connecting to dma */
#else
#define MAX_DMA_DEVICE_NUM  7	/*max ports connecting to dma */
#endif
#define MAX_DMA_CHANNEL_NUM 20	/*20 max dma channels */
#define DMA_INT_BUDGET      100	/*budget for interrupt handling */
#define DMA_POLL_COUNTER    4	/*fix me, set the correct counter value here! */
//#define DANUBE_DMA_CH_INT(n) DANUBE_DMA_CH##n##_INT
/*****************************************************************************/

extern void mask_and_ack_danube_irq (unsigned int irq_nr);
extern void enable_danube_irq (unsigned int irq_nr);
extern void disable_danube_irq (unsigned int irq_nr);
/******************Global variables ******************************************/

struct proc_dir_entry *g_danube_dma_dir;
u64 *g_desc_list;
_dma_device_info dma_devs[MAX_DMA_DEVICE_NUM];
_dma_channel_info dma_chan[MAX_DMA_CHANNEL_NUM];
#if !defined(ENABLE_RX_DPLUS_PATH) || !ENABLE_RX_DPLUS_PATH
char global_device_name[MAX_DMA_DEVICE_NUM][20] =
	{ {"PPE"}, {"DEU"}, {"SPI"}, {"SDIO"}, {"MCTRL0"}, {"MCTRL1"} };
_dma_chan_map default_dma_map[MAX_DMA_CHANNEL_NUM] =
	{ {"PPE", DANUBE_DMA_RX, 0, DANUBE_DMA_CH0_INT, 0},
{"PPE", DANUBE_DMA_TX, 0, DANUBE_DMA_CH1_INT, 0},
{"PPE", DANUBE_DMA_RX, 1, DANUBE_DMA_CH2_INT, 1},
{"PPE", DANUBE_DMA_TX, 1, DANUBE_DMA_CH3_INT, 1},
{"PPE", DANUBE_DMA_RX, 2, DANUBE_DMA_CH4_INT, 2},
{"PPE", DANUBE_DMA_TX, 2, DANUBE_DMA_CH5_INT, 2},
{"PPE", DANUBE_DMA_RX, 3, DANUBE_DMA_CH6_INT, 3},
{"PPE", DANUBE_DMA_TX, 3, DANUBE_DMA_CH7_INT, 3},
{"DEU", DANUBE_DMA_RX, 0, DANUBE_DMA_CH8_INT, 0},
{"DEU", DANUBE_DMA_TX, 0, DANUBE_DMA_CH9_INT, 0},
{"DEU", DANUBE_DMA_RX, 1, DANUBE_DMA_CH10_INT, 1},
{"DEU", DANUBE_DMA_TX, 1, DANUBE_DMA_CH11_INT, 1},
{"SPI", DANUBE_DMA_RX, 0, DANUBE_DMA_CH12_INT, 0},
{"SPI", DANUBE_DMA_TX, 0, DANUBE_DMA_CH13_INT, 0},
{"SDIO", DANUBE_DMA_RX, 0, DANUBE_DMA_CH14_INT, 0},
{"SDIO", DANUBE_DMA_TX, 0, DANUBE_DMA_CH15_INT, 0},
{"MCTRL0", DANUBE_DMA_RX, 0, DANUBE_DMA_CH16_INT, 0},
{"MCTRL0", DANUBE_DMA_TX, 0, DANUBE_DMA_CH17_INT, 0},
{"MCTRL1", DANUBE_DMA_RX, 1, DANUBE_DMA_CH18_INT, 1},
{"MCTRL1", DANUBE_DMA_TX, 1, DANUBE_DMA_CH19_INT, 1}
};
#else
char global_device_name[MAX_DMA_DEVICE_NUM][20] =
	{ {"PPE"}, {"ETH2"}, {"DEU"}, {"SPI"}, {"SDIO"}, {"MCTRL0"},
{"MCTRL1"}
};
_dma_chan_map default_dma_map[MAX_DMA_CHANNEL_NUM] =
	{ {"PPE", DANUBE_DMA_RX, 0, DANUBE_DMA_CH0_INT, 0},
{"PPE", DANUBE_DMA_TX, 0, DANUBE_DMA_CH1_INT, 0},
{"PPE", DANUBE_DMA_RX, 1, DANUBE_DMA_CH2_INT, 1},
{"PPE", DANUBE_DMA_TX, 1, DANUBE_DMA_CH3_INT, 1},
{"ETH2", DANUBE_DMA_RX, 0, DANUBE_DMA_CH4_INT, 0},
{"ETH2", DANUBE_DMA_TX, 0, DANUBE_DMA_CH5_INT, 0},
{"ETH2", DANUBE_DMA_RX, 1, DANUBE_DMA_CH6_INT, 1},
{"ETH2", DANUBE_DMA_TX, 1, DANUBE_DMA_CH7_INT, 1},
{"DEU", DANUBE_DMA_RX, 0, DANUBE_DMA_CH8_INT, 0},
{"DEU", DANUBE_DMA_TX, 0, DANUBE_DMA_CH9_INT, 0},
{"DEU", DANUBE_DMA_RX, 1, DANUBE_DMA_CH10_INT, 1},
{"DEU", DANUBE_DMA_TX, 1, DANUBE_DMA_CH11_INT, 1},
{"SPI", DANUBE_DMA_RX, 0, DANUBE_DMA_CH12_INT, 0},
{"SPI", DANUBE_DMA_TX, 0, DANUBE_DMA_CH13_INT, 0},
{"SDIO", DANUBE_DMA_RX, 0, DANUBE_DMA_CH14_INT, 0},
{"SDIO", DANUBE_DMA_TX, 0, DANUBE_DMA_CH15_INT, 0},
{"MCTRL0", DANUBE_DMA_RX, 0, DANUBE_DMA_CH16_INT, 0},
{"MCTRL0", DANUBE_DMA_TX, 0, DANUBE_DMA_CH17_INT, 0},
{"MCTRL1", DANUBE_DMA_RX, 1, DANUBE_DMA_CH18_INT, 1},
{"MCTRL1", DANUBE_DMA_TX, 1, DANUBE_DMA_CH19_INT, 1}
};
#endif
_dma_chan_map *chan_map = default_dma_map;
volatile u32 g_danube_dma_int_status=0;
volatile int g_danube_dma_in_process=0;/*0=not in process,1=in process*/
struct semaphore *danube_dma_sem;
struct semaphore *danube_dma_status_sem;
/******************************************************************************/

/***********function definitions***********************************************/
void do_dma_tasklet (unsigned long);
DECLARE_TASKLET (dma_tasklet, do_dma_tasklet, 0);
void dma_interrupt (int irq, void *dev_id, struct pt_regs *regs);

/******************************************************************************/
int
select_chan (int chan_num)
{
	*DANUBE_DMA_CS = chan_num;

	return IFX_SUCCESS;
}

int
enable_chan (int chan_num)
{
	*DANUBE_DMA_CS = chan_num;
	*DANUBE_DMA_CCTRL |= 1;
	return IFX_SUCCESS;
}

int
disable_chan (int chan_num)
{
	*DANUBE_DMA_CS = chan_num;
	*DANUBE_DMA_CCTRL &= ~1;
	return IFX_SUCCESS;
}

u8 *
common_buffer_alloc (int len, int *byte_offset, void **opt)
{
	u8 *buffer = (u8 *) kmalloc (len * sizeof (u8), GFP_KERNEL);
	*byte_offset = 0;
	return buffer;
}

int
common_buffer_free (u8 * dataptr, void *opt)
{
	if (dataptr)
		kfree (dataptr);
	return 0;
}

int
enable_ch_irq (_dma_channel_info * pCh)
{
	int chan_no = (int) (pCh - dma_chan);
	int flag;
	local_irq_save (flag);
	*DANUBE_DMA_CS = chan_no;
	*DANUBE_DMA_CIE = 0x4a;
	*DANUBE_DMA_IRNEN |= 1 << chan_no;
	local_irq_restore (flag);
	enable_danube_irq (pCh->irq);
	return IFX_SUCCESS;
}

int
disable_ch_irq (_dma_channel_info * pCh)
{
	int flag;
	int chan_num = (int) (pCh - dma_chan);

	local_irq_save (flag);
	g_danube_dma_int_status &= ~(1 << chan_num);
	*DANUBE_DMA_CS = chan_num;
	*DANUBE_DMA_CIE = 0;
	*DANUBE_DMA_IRNEN &= ~(1 << chan_num);
	local_irq_restore (flag);
	mask_and_ack_danube_irq (pCh->irq);

	return IFX_SUCCESS;
}

int
open_chan (_dma_channel_info * pCh)
{
	int flag;
	int chan_num = (int) (pCh - dma_chan);

	local_irq_save (flag);
	*DANUBE_DMA_CS = chan_num;
	*DANUBE_DMA_CCTRL |= 1;
	if (pCh->dir == DANUBE_DMA_RX)
		enable_ch_irq (pCh);
	local_irq_restore (flag);
	return IFX_SUCCESS;
}

int
close_chan (_dma_channel_info * pCh)
{
	int flag;
	int chan_num = (int) (pCh - dma_chan);
	local_irq_save (flag);
	*DANUBE_DMA_CS = chan_num;
	*DANUBE_DMA_CCTRL &= ~1;
	disable_ch_irq (pCh);
	local_irq_restore (flag);
	return IFX_SUCCESS;
}

int
reset_chan (_dma_channel_info * pCh)
{
	int chan_num = (int) (pCh - dma_chan);
	*DANUBE_DMA_CS = chan_num;
	*DANUBE_DMA_CCTRL |= 2;
	return IFX_SUCCESS;
}

void
rx_chan_intr_handler (int chan_no)
{
	_dma_device_info *pDev =
		(_dma_device_info *) dma_chan[chan_no].dma_dev;
	_dma_channel_info *pCh = &dma_chan[chan_no];
	struct rx_desc *rx_desc_p;
	int tmp;
	int flag;

	/*handle command complete interrupt */
	rx_desc_p = (struct rx_desc *) pCh->desc_base + pCh->curr_desc;
#if !defined(ENABLE_DANUBE_ETHERNET_D2) || !ENABLE_DANUBE_ETHERNET_D2
	if (rx_desc_p->status.field.OWN == CPU_OWN
	    && rx_desc_p->status.field.C
	    && rx_desc_p->status.field.data_length < 1536)
#else
	if (rx_desc_p->status.field.OWN == CPU_OWN
	    && rx_desc_p->status.field.C)
#endif
	{
		/*Every thing is correct, then we inform the upper layer */
		pDev->current_rx_chan = pCh->rel_chan_no;
		if (pDev->intr_handler)
			pDev->intr_handler (pDev, RCV_INT);
		pCh->weight--;
	}
	else {
		local_irq_save (flag);
		tmp = *DANUBE_DMA_CS;
		*DANUBE_DMA_CS = chan_no;
		*DANUBE_DMA_CIS |= 0x7e;
		*DANUBE_DMA_CS = tmp;
		g_danube_dma_int_status &= ~(1 << chan_no);
		local_irq_restore (flag);
		enable_danube_irq (dma_chan[chan_no].irq);
	}
}

inline void
tx_chan_intr_handler (int chan_no)
{
	_dma_device_info *pDev =
		(_dma_device_info *) dma_chan[chan_no].dma_dev;
	_dma_channel_info *pCh = &dma_chan[chan_no];

#if 0
	int tmp;

	tmp = *DANUBE_DMA_CS;
	*DANUBE_DMA_CS = chan_no;
	*DANUBE_DMA_CIS |= 0x7e;
	*DANUBE_DMA_CS = tmp;
#else
    int tmp;
    int flag;

    local_irq_save(flag);
    tmp = *DANUBE_DMA_CS;
    *DANUBE_DMA_CS = chan_no;
    *DANUBE_DMA_CIS |= 0x7e;
    *DANUBE_DMA_CS = tmp;
    g_danube_dma_int_status &= ~(1 << chan_no);
    local_irq_restore(flag);
#endif

	pDev->current_tx_chan = pCh->rel_chan_no;
	if (pDev->intr_handler)
		pDev->intr_handler (pDev, TRANSMIT_CPT_INT);
}

void
do_dma_tasklet (unsigned long unused)
{
	int i;
	int chan_no = 0;
	int budget = DMA_INT_BUDGET;
//	g_danube_dma_in_process = 1;
	int weight = 0;

    int flag;

	while (g_danube_dma_int_status) {
		if (budget-- < 0) {
			tasklet_schedule (&dma_tasklet);
			return;
		}
		chan_no = -1;
		weight = 0;
		/*WFQ algorithm to select the channel */
		for (i = 0; i < MAX_DMA_CHANNEL_NUM; i++) {
			if ( (g_danube_dma_int_status & (1 << i))
			    && dma_chan[i].weight > 0) {
				if (dma_chan[i].weight > weight)
                {
					chan_no = i;
                    weight=dma_chan[chan_no].weight;
                }
			}
		}
		if (chan_no >= 0) {
			if (chan_map[chan_no].dir == DANUBE_DMA_RX)
				rx_chan_intr_handler (chan_no);
			else
				tx_chan_intr_handler (chan_no);
		}
		else {		/*reset all the channels */

			for (i = 0; i < MAX_DMA_CHANNEL_NUM; i++) {
				dma_chan[i].weight =
					dma_chan[i].default_weight;
			}
		}
	}

    local_irq_save(flag);
	g_danube_dma_in_process = 0;
    if ( g_danube_dma_int_status )
    {
        g_danube_dma_in_process = 1;
        tasklet_schedule(&dma_tasklet);
    }
    local_irq_restore(flag);
}

void
dma_interrupt (int irq, void *dev_id, struct pt_regs *regs)
{
	_dma_channel_info *pCh;
	int chan_num = 0;
	int tmp;

	pCh = (_dma_channel_info *) dev_id;
	chan_num = (int) (pCh - dma_chan);
	if (chan_num < 0 || chan_num > 19) {
		printk ("dma_interrupt irq=%d chan_num=%d\n", irq, chan_num);
	}
	//local_irq_save(flag);
	tmp = *DANUBE_DMA_IRNEN;
	*DANUBE_DMA_IRNEN = 0;
	g_danube_dma_int_status |= 1 << chan_num;
	*DANUBE_DMA_IRNEN = tmp;
	//local_irq_restore(flag);
	mask_and_ack_danube_irq (irq);

	// if(!g_danube_dma_in_process)/*if not in process, then invoke the tasklet*/
    if ( !g_danube_dma_in_process )
    {
        g_danube_dma_in_process = 1;
        tasklet_schedule(&dma_tasklet);
    }
}

_dma_device_info *
dma_device_reserve (char *dev_name)
{
	int i;
	for (i = 0; i < MAX_DMA_DEVICE_NUM; i++) {	/*may put some hash function here in the future:) */
		if (strcmp (dev_name, dma_devs[i].device_name) == 0) {
			if (dma_devs[i].reserved)
				return NULL;
			dma_devs[i].reserved = 1;
			break;
		}
	}
	return &dma_devs[i];
}

int
dma_device_release (_dma_device_info * dev)
{
	dev->reserved = 0;

	return IFX_SUCCESS;
}

int
dma_device_register (_dma_device_info * dev)
{
	int result = IFX_SUCCESS;
	int i, j;
	int chan_no = 0;
	u8 *buffer;
	int byte_offset;
	int flag;
	_dma_device_info *pDev;
	_dma_channel_info *pCh;
	struct rx_desc *rx_desc_p;
	struct tx_desc *tx_desc_p;
#if 0
	if (strcmp (dev->device_name, "MCTRL0") == 0 || strcmp (dev->device_name, "MCTRL1") == 0) {	/*select the port */
		*DANUBE_DMA_PS = 4;
		/*set port parameters */
		*DANUBE_DMA_PCTRL |= 1 << 16;	/*flush memcopy */
	}
#endif
	for (i = 0; i < dev->max_tx_chan_num; i++) {
		pCh = dev->tx_chan[i];
		if (pCh->control == DANUBE_DMA_CH_ON) {
			chan_no = (int) (pCh - dma_chan);
			for (j = 0; j < pCh->desc_len; j++) {
				tx_desc_p =
					(struct tx_desc *) pCh->desc_base + j;
				memset (tx_desc_p, 0,
					sizeof (struct tx_desc));
			}
			local_irq_save (flag);
			*DANUBE_DMA_CS = chan_no;
#if defined(ENABLE_DANUBE_ETHERNET_D2) && ENABLE_DANUBE_ETHERNET_D2
			/*check if the descriptor base is changed */
			if (*DANUBE_DMA_CDBA !=
			    (u32) CPHYSADDR (pCh->desc_base))
				*DANUBE_DMA_CDBA =
					(u32) CPHYSADDR (pCh->desc_base);
#endif
			/*check if the descriptor length is changed */
			if (*DANUBE_DMA_CDLEN != pCh->desc_len)
				*DANUBE_DMA_CDLEN = pCh->desc_len;

			*DANUBE_DMA_CCTRL &= ~1;
			*DANUBE_DMA_CCTRL |= 2;
			while (*DANUBE_DMA_CCTRL & 2) {
			};
			//disable_danube_irq(pCh->irq);
			//*DANUBE_DMA_CIE=0x0a;
			*DANUBE_DMA_IRNEN |= 1 << chan_no;
			*DANUBE_DMA_CCTRL = 0x30100;	/*reset and enable channel,enable channel later */
			local_irq_restore (flag);
		}
	}

	for (i = 0; i < dev->max_rx_chan_num; i++) {
		pCh = dev->rx_chan[i];
		if (pCh->control == DANUBE_DMA_CH_ON) {
			chan_no = (int) (pCh - dma_chan);

			for (j = 0; j < pCh->desc_len; j++) {
				rx_desc_p =
					(struct rx_desc *) pCh->desc_base + j;
				pDev = (_dma_device_info *) (pCh->dma_dev);
				buffer = pDev->buffer_alloc (pCh->packet_size,
							     &byte_offset,
							     (void *) &(pCh->
									opt
									[j]));
				if (!buffer)
					break;
#ifndef CONFIG_MIPS_UNCACHED
    				/* tc.chen: invalidate cache    */
    				dma_cache_inv ((unsigned long) buffer,
                  			pCh->packet_size);
#endif

				rx_desc_p->Data_Pointer =
					(u32) CPHYSADDR ((u32) buffer);
				rx_desc_p->status.word = 0;
				rx_desc_p->status.field.byte_offset =
					byte_offset;
				rx_desc_p->status.field.OWN = DMA_OWN;
				rx_desc_p->status.field.data_length =
					pCh->packet_size;
			}

			local_irq_save (flag);
			*DANUBE_DMA_CS = chan_no;
#if defined(ENABLE_DANUBE_ETHERNET_D2) && ENABLE_DANUBE_ETHERNET_D2
			/*check if the descriptor base is changed */
			if (*DANUBE_DMA_CDBA !=
			    (u32) CPHYSADDR (pCh->desc_base))
				*DANUBE_DMA_CDBA =
					(u32) CPHYSADDR (pCh->desc_base);
#endif
			/*check if the descriptor length is changed */
			if (*DANUBE_DMA_CDLEN != pCh->desc_len)
				*DANUBE_DMA_CDLEN = pCh->desc_len;
			*DANUBE_DMA_CCTRL &= ~1;
			*DANUBE_DMA_CCTRL |= 2;
			while (*DANUBE_DMA_CCTRL & 2) {
			};
			*DANUBE_DMA_CIE = 0x0A;	/*fix me, should enable all the interrupts here? */
			*DANUBE_DMA_IRNEN |= 1 << chan_no;
			*DANUBE_DMA_CCTRL = 0x30000;
			local_irq_restore (flag);
			enable_danube_irq (dma_chan[chan_no].irq);
		}
	}
	return result;
}

int
dma_device_unregister (_dma_device_info * dev)
{
	int result = IFX_SUCCESS;
	int i, j;
	int chan_num;
	_dma_channel_info *pCh;
	struct rx_desc *rx_desc_p;
	struct tx_desc *tx_desc_p;
	int flag;

#if 0
	if (strcmp (dev->device_name, "MCTRL0") == 0 || strcmp (dev->device_name, "MCTRL1") == 0) {	/*select the port */
		*DANUBE_DMA_PS = 4;
		/*set port parameters */
		*dma_deDANUBE_DMA_PCTRL |= 1 << 16;	/*flush memcopy */
	}
#endif
	for (i = 0; i < dev->max_tx_chan_num; i++) {
		pCh = dev->tx_chan[i];
		if (pCh->control == DANUBE_DMA_CH_ON) {
			chan_num = (int) (dev->tx_chan[i] - dma_chan);
			//down(danube_dma_sem);
			local_irq_save (flag);
			*DANUBE_DMA_CS = chan_num;
			pCh->curr_desc = 0;
			pCh->prev_desc = 0;
			pCh->control = DANUBE_DMA_CH_OFF;
			*DANUBE_DMA_CIE = 0;	/*fix me, should disable all the interrupts here? */
			*DANUBE_DMA_IRNEN &= ~(1 << chan_num);	/*disable interrupts */
			*DANUBE_DMA_CCTRL &= ~1;
			while (*DANUBE_DMA_CCTRL & 1) {
			};
			local_irq_restore (flag);

			//up(danube_dma_sem);
			for (j = 0; j < pCh->desc_len; j++) {
				tx_desc_p =
					(struct tx_desc *) pCh->desc_base + j;
				if ((tx_desc_p->status.field.OWN == CPU_OWN
				     && tx_desc_p->status.field.C)
				    || (tx_desc_p->status.field.OWN == DMA_OWN
					&& tx_desc_p->status.field.
					data_length > 0)) {
					dev->buffer_free ((u8 *)
							  __va (tx_desc_p->
								Data_Pointer),
							  (void *) pCh->
							  opt[j]);
				}
				tx_desc_p->status.field.OWN = CPU_OWN;
				memset (tx_desc_p, 0,
					sizeof (struct tx_desc));
			}
			/*fix me: should free buffer that is not transferred by dma */
		}
	}

	for (i = 0; i < dev->max_rx_chan_num; i++) {
		pCh = dev->rx_chan[i];
		chan_num = (int) (dev->rx_chan[i] - dma_chan);
		//down(danube_dma_sem);
		disable_danube_irq (pCh->irq);

		local_irq_save (flag);
		if (chan_num == 6)
			printk ("62");
		g_danube_dma_int_status &= ~(1 << chan_num);
		pCh->curr_desc = 0;
		pCh->prev_desc = 0;
		pCh->control = DANUBE_DMA_CH_OFF;

		*DANUBE_DMA_CS = chan_num;
		if (*DANUBE_DMA_CS != chan_num)
			printk ("e");
		*DANUBE_DMA_CIE = 0;	/*fix me, should disable all the interrupts here? */
		*DANUBE_DMA_IRNEN &= ~(1 << chan_num);	/*disable interrupts */
		*DANUBE_DMA_CCTRL &= ~1;
		while (*DANUBE_DMA_CCTRL & 1) {
		};

		local_irq_restore (flag);
		for (j = 0; j < pCh->desc_len; j++) {
			rx_desc_p = (struct rx_desc *) pCh->desc_base + j;
			if ((rx_desc_p->status.field.OWN == CPU_OWN
			     && rx_desc_p->status.field.C)
			    || (rx_desc_p->status.field.OWN == DMA_OWN
				&& rx_desc_p->status.field.data_length > 0)) {
				dev->buffer_free ((u8 *)
						  __va (rx_desc_p->
							Data_Pointer),
						  (void *) pCh->opt[j]);
			}
		}
	}

	return result;
}

int
dma_device_read (struct dma_device_info *dma_dev, u8 ** dataptr, void **opt)
{
	u8 *buf;
	int len;
	int byte_offset = 0;
	void *p = NULL;

	_dma_channel_info *pCh = dma_dev->rx_chan[dma_dev->current_rx_chan];

	struct rx_desc *rx_desc_p;

	/*get the rx data first */
	rx_desc_p = (struct rx_desc *) pCh->desc_base + pCh->curr_desc;
	if (!
	    (rx_desc_p->status.field.OWN == CPU_OWN
	     && rx_desc_p->status.field.C)) {
		return 0;
	}
	buf = (u8 *) __va (rx_desc_p->Data_Pointer);
	*(u32 *) dataptr = (u32) buf;
	len = rx_desc_p->status.field.data_length;
#ifndef CONFIG_MIPS_UNCACHED
	/* 20/12/2006 tc.chen , move to buffer_alloc function
	dma_cache_inv ((unsigned long) buf, len);
	*/
#endif //CONFIG_MIPS_UNCACHED
	if (opt) {
		*(int *) opt = (int) pCh->opt[pCh->curr_desc];
	}

	/*replace with a new allocated buffer */
	buf = dma_dev->buffer_alloc (pCh->packet_size, &byte_offset, &p);
	if (buf) {
#ifndef CONFIG_MIPS_UNCACHED
    		/* tc.chen: invalidate cache    */
    		dma_cache_inv ((unsigned long) buf,
               			pCh->packet_size);
#endif

		pCh->opt[pCh->curr_desc] = p;

		wmb ();
		rx_desc_p->Data_Pointer = (u32) CPHYSADDR ((u32) buf);
#if 0
		wmb ();
		if ((rx_desc_p->Data_Pointer & 0x1F) == 0)
			printk ("wild dma: rx_desc_p = %08X, rx_desc_p->Data_Pointer = %08X, buf = %08X\n", (u32) rx_desc_p, (u32) rx_desc_p->Data_Pointer, (u32) CPHYSADDR ((u32) buf));
#endif
		rx_desc_p->status.word = (DMA_OWN << 31)
			| ((byte_offset) << 23)
			| pCh->packet_size;

		wmb ();
	}
	else {
		*(u32 *) dataptr = 0;
		if (opt)
			*(int *) opt = 0;
		len = 0;
	}

	/*increase the curr_desc pointer */
	pCh->curr_desc++;
	if (pCh->curr_desc == pCh->desc_len)
		pCh->curr_desc = 0;
	/*return the length of the received packet */
	return len;
}

int
dma_device_write (struct dma_device_info *dma_dev, u8 * dataptr, int len,
		  void *opt)
{
	int flag;
	u32 tmp, byte_offset;
	_dma_channel_info *pCh;
	int chan_no;
	struct tx_desc *tx_desc_p;
	local_irq_save (flag);

	pCh = dma_dev->tx_chan[dma_dev->current_tx_chan];
	chan_no = (int) (pCh - (_dma_channel_info *) dma_chan);

	tx_desc_p = (struct tx_desc *) pCh->desc_base + pCh->prev_desc;
	while (tx_desc_p->status.field.OWN == CPU_OWN
	       && tx_desc_p->status.field.C) {
		dma_dev->buffer_free ((u8 *) __va (tx_desc_p->Data_Pointer),
				      pCh->opt[pCh->prev_desc]);
		memset (tx_desc_p, 0, sizeof (struct tx_desc));
		pCh->prev_desc = (pCh->prev_desc + 1) % (pCh->desc_len);
		tx_desc_p =
			(struct tx_desc *) pCh->desc_base + pCh->prev_desc;
	}
	tx_desc_p = (struct tx_desc *) pCh->desc_base + pCh->curr_desc;
	/*Check whether this descriptor is available */
	if (tx_desc_p->status.field.OWN == DMA_OWN
	    || tx_desc_p->status.field.C) {
		/*if not , the tell the upper layer device */
		dma_dev->intr_handler (dma_dev, TX_BUF_FULL_INT);
                local_irq_restore(flag);
		//printk (KERN_INFO "%s %d: failed to write!\n", __func__,__LINE__);
		return 0;
	}
	pCh->opt[pCh->curr_desc] = opt;
	/*byte offset----to adjust the starting address of the data buffer, should be multiple of the burst length. */
	byte_offset =
		((u32) CPHYSADDR ((u32) dataptr)) % ((dma_dev->tx_burst_len) *
						     4);
#ifndef CONFIG_MIPS_UNCACHED
	dma_cache_wback ((unsigned long) dataptr, len);
	wmb ();
#endif //CONFIG_MIPS_UNCACHED

	tx_desc_p->Data_Pointer =
		(u32) CPHYSADDR ((u32) dataptr) - byte_offset;
	wmb ();
	tx_desc_p->status.word = (DMA_OWN << 31)
		| DMA_DESC_SOP_SET | DMA_DESC_EOP_SET | ((byte_offset) << 23)
		| len;
	wmb ();

	pCh->curr_desc++;
	if (pCh->curr_desc == pCh->desc_len)
		pCh->curr_desc = 0;

	/*Check whether this descriptor is available */
	tx_desc_p = (struct tx_desc *) pCh->desc_base + pCh->curr_desc;
	if (tx_desc_p->status.field.OWN == DMA_OWN) {
		/*if not , the tell the upper layer device */
		dma_dev->intr_handler (dma_dev, TX_BUF_FULL_INT);
	}
	DANUBE_DMA_RDREG_PROT (chan_no, DANUBE_DMA_CCTRL, tmp);
	if (!(tmp & 1))
		pCh->open (pCh);

	local_irq_restore (flag);
	return len;
}

int
desc_list_proc_read (char *buf, char **start, off_t offset,
		     int count, int *eof, void *data)
{
	int i, j;
	int len = 0;
	len += sprintf (buf + len, "descriptor list:\n");
	for (i = 0; i < MAX_DMA_CHANNEL_NUM; i++) {
		sprintf (buf + len, "channel %d\n", i);
		for (j = 0; j < dma_chan[i].desc_len; j++) {
			sprintf (buf + len, "%08x\n%08x\n\n",
				 (u32) ((u64) dma_chan[i].desc_base + j),
				 (u32) dma_chan[i].desc_base + j * 2 + 1);
		}
		sprintf (buf + len, "\n\n\n");
	}
	return len;
}

int
channel_weight_proc_read (char *buf, char **start, off_t offset,
			  int count, int *eof, void *data)
{
	int i = 0;
	int len = 0;

	for (i = 0; i < MAX_DMA_CHANNEL_NUM; i++) {
		len += sprintf (buf + len, "channel %d  %08x\n", i,
				dma_chan[i].weight);
	}
	return len;
}

int
dma_register_proc_read (char *buf, char **start, off_t offset,
			int count, int *eof, void *data)
{
	/*fix me, do I need to implement this? */
	int len = 0;;
	len += sprintf (buf + len, "register of DMA:\n");
	return len;
}

static int
dma_open (struct inode *inode, struct file *file)
{
	MOD_INC_USE_COUNT;
	return 0;
}

static int
dma_release (struct inode *inode, struct file *file)
{
	/*release the resources */
	MOD_DEC_USE_COUNT;
	return 0;
}

static int
dma_ioctl (struct inode *inode, struct file *file, unsigned int cmd,
	   unsigned long arg)
{
	int result = 0;
	/*TODO: add some user controled functions here */
	return result;
}

static struct file_operations dma_fops = {
      owner:THIS_MODULE,
      open:dma_open,
      release:dma_release,
      ioctl:dma_ioctl,
};

int
map_dma_chan (_dma_chan_map * map)
{
	int i, j;
	int result;
	for (i = 0; i < MAX_DMA_DEVICE_NUM; i++) {
		strcpy (dma_devs[i].device_name, global_device_name[i]);
	}
	for (i = 0; i < MAX_DMA_CHANNEL_NUM; i++) {
		dma_chan[i].irq = map[i].irq;
#if defined(ENABLE_DANUBE_ETHERNET_D2) && ENABLE_DANUBE_ETHERNET_D2
		if (map[i].irq != DANUBE_DMA_CH2_INT
		    && map[i].irq != DANUBE_DMA_CH4_INT) {
			result = request_irq (dma_chan[i].irq, dma_interrupt,
					      SA_INTERRUPT, "dma-core",
					      (void *) &dma_chan[i]);
			if (result) {
				DANUBE_DMA_EMSG
					("error, cannot get dma_irq!\n");
				free_irq (dma_chan[i].irq,
					  (void *) &dma_interrupt);
				return -EFAULT;
			}
		}
#else
		result = request_irq (dma_chan[i].irq, dma_interrupt,
				      SA_INTERRUPT, "dma-core",
				      (void *) &dma_chan[i]);
		if (result) {
			DANUBE_DMA_EMSG ("error, cannot get dma_irq!\n");
			free_irq (dma_chan[i].irq, (void *) &dma_interrupt);
			return -EFAULT;
		}
#endif
	}

	for (i = 0; i < MAX_DMA_DEVICE_NUM; i++) {
		dma_devs[i].num_tx_chan = 0;	/*set default tx channel number to be one */
		dma_devs[i].num_rx_chan = 0;	/*set default rx channel number to be one */
		dma_devs[i].max_rx_chan_num = 0;
		dma_devs[i].max_tx_chan_num = 0;
		dma_devs[i].buffer_alloc = &common_buffer_alloc;
		dma_devs[i].buffer_free = &common_buffer_free;
		dma_devs[i].intr_handler = NULL;
		dma_devs[i].tx_burst_len = 4; 
		dma_devs[i].rx_burst_len = 4; 
		if (i == 0) {
			*DANUBE_DMA_PS = 0;
			*DANUBE_DMA_PCTRL |= (0xf << 8) | (1 << 6);	/*enable dma drop */
		}

		if (i == 1) {
			*DANUBE_DMA_PS = 1;
			*DANUBE_DMA_PCTRL = 0x14;	/*deu port setting */
		}

		for (j = 0; j < MAX_DMA_CHANNEL_NUM; j++) {
			dma_chan[j].byte_offset = 0;
			dma_chan[j].open = &open_chan;
			dma_chan[j].close = &close_chan;
			dma_chan[j].reset = &reset_chan;
			dma_chan[j].enable_irq = &enable_ch_irq;
			dma_chan[j].disable_irq = &disable_ch_irq;
			dma_chan[j].rel_chan_no = map[j].rel_chan_no;
			dma_chan[j].control = DANUBE_DMA_CH_OFF;
			dma_chan[j].default_weight =
				DANUBE_DMA_CH_DEFAULT_WEIGHT;
			dma_chan[j].weight = dma_chan[j].default_weight;
			dma_chan[j].curr_desc = 0;
			dma_chan[j].prev_desc = 0;
		}

		for (j = 0; j < MAX_DMA_CHANNEL_NUM; j++) {
			if (strcmp (dma_devs[i].device_name, map[j].dev_name)
			    == 0) {
				if (map[j].dir == DANUBE_DMA_RX) {
					dma_chan[j].dir = DANUBE_DMA_RX;
					dma_devs[i].max_rx_chan_num++;
					dma_devs[i].rx_chan[dma_devs[i].
							    max_rx_chan_num -
							    1] = &dma_chan[j];
					dma_devs[i].rx_chan[dma_devs[i].
							    max_rx_chan_num -
							    1]->pri =
						map[j].pri;
					dma_chan[j].dma_dev =
						(void *) &dma_devs[i];

					/*have to program the class value into the register later, fix me */
				}
				else if (map[j].dir == DANUBE_DMA_TX) {	/*TX direction */
					dma_chan[j].dir = DANUBE_DMA_TX;
					dma_devs[i].max_tx_chan_num++;
					dma_devs[i].tx_chan[dma_devs[i].
							    max_tx_chan_num -
							    1] = &dma_chan[j];
					dma_devs[i].tx_chan[dma_devs[i].
							    max_tx_chan_num -
							    1]->pri =
						map[j].pri;
					dma_chan[j].dma_dev =
						(void *) &dma_devs[i];
				}
				else
					printk ("WRONG MAP!\n");
			}
		}
	}

	return IFX_SUCCESS;
}

int
dma_chip_init (void)
{
	int i;
	*DANUBE_PMU_PWDCR &= ~(1 << DANUBE_PMU_DMA_SHIFT);	/*enable DMA from PMU */
	/*reset DMA, necessary? */
	*DANUBE_DMA_CTRL |= 1;
	*DANUBE_DMA_IRNEN = 0;	/*disable all the interrupts first */

	for (i = 0; i < MAX_DMA_CHANNEL_NUM; i++) {
		*DANUBE_DMA_CS = i;
		*DANUBE_DMA_CCTRL = 0x2;	/*fix me, need to reset this channel first? */
		*DANUBE_DMA_CPOLL = 0x80000040;
		/*set memcopy channel class */
		if (i == 8 || i == 9)
			*DANUBE_DMA_CCTRL = (*DANUBE_DMA_CCTRL & (~(3 << 9)));
		if (i == 10 || i == 11)
			*DANUBE_DMA_CCTRL =
				(*DANUBE_DMA_CCTRL & (~(3 << 9))) | (1 << 9);

		/*set memcopy channel class */
		if (i == 16 || i == 17)
			*DANUBE_DMA_CCTRL = (*DANUBE_DMA_CCTRL | (3 << 9));
		if (i == 18 || i == 19)
			*DANUBE_DMA_CCTRL =
				(*DANUBE_DMA_CCTRL & (~(3 << 9))) | (1 << 9);
	}
#if 0
	/*TODO: add the port settings, ENDIAN conversion here */
	for (i = 0; i < MAX_DMA_DEVICE_NUM; i++) {
		/*select the port */
		*DANUBE_DMA_PS = i;
		/*set port parameters */
		*DANUBE_DMA_PCTRL = 0x1028;	/*tx and rx burst length both set to be 4 */
	}
#endif
    /****************************************************/
	for (i = 0; i < MAX_DMA_CHANNEL_NUM; i++) {
		disable_chan (i);	/*disable all the dma channel first */
	}
	return IFX_SUCCESS;
}

int
danube_dma_init (void)
{
	int result = 0;
	int i;
	result = register_chrdev (DMA_MAJOR, "dma-core", &dma_fops);
	if (result) {
		DANUBE_DMA_EMSG ("cannot register device dma-core!\n");
		return result;
	}
	danube_dma_sem =
		(struct semaphore *) kmalloc (sizeof (struct semaphore),
					      GFP_KERNEL);
	init_MUTEX (danube_dma_sem);
	dma_chip_init ();
	map_dma_chan (default_dma_map);
	//g_desc_list = (u64 *) KSEG1ADDR (__get_free_page (GFP_DMA));
	g_desc_list=(u64*)kmalloc(DANUBE_DMA_DESCRIPTOR_OFFSET * MAX_DMA_CHANNEL_NUM * sizeof(u64), GFP_DMA);//joelin
	if (g_desc_list == NULL) {
		DANUBE_DMA_EMSG ("no memory for desriptor\n");
		return -ENOMEM;
	}
	dma_cache_inv(g_desc_list, DANUBE_DMA_DESCRIPTOR_OFFSET * MAX_DMA_CHANNEL_NUM * sizeof(u64));
	g_desc_list = (u64*)((u32)g_desc_list | 0xA0000000);
//	g_desc_list = KSEG1ADDR(g_desc_list);
    memset(g_desc_list, 0, DANUBE_DMA_DESCRIPTOR_OFFSET * MAX_DMA_CHANNEL_NUM * sizeof(u64));	
	//memset (g_desc_list, 0, PAGE_SIZE);
	for (i = 0; i < MAX_DMA_CHANNEL_NUM; i++) {
		dma_chan[i].desc_base =
			(u32) g_desc_list +
			i * DANUBE_DMA_DESCRIPTOR_OFFSET * 8;
		dma_chan[i].curr_desc = 0;
		dma_chan[i].desc_len = DANUBE_DMA_DESCRIPTOR_OFFSET;
		select_chan (i);
		*DANUBE_DMA_CDBA = (u32) CPHYSADDR (dma_chan[i].desc_base);
		*DANUBE_DMA_CDLEN = dma_chan[i].desc_len;
	}

	g_danube_dma_dir = proc_mkdir ("danube_dma", NULL);

	create_proc_read_entry ("dma_register",
				0,
				g_danube_dma_dir,
				dma_register_proc_read, NULL);

	create_proc_read_entry ("g_desc_list",
				0,
				g_danube_dma_dir, desc_list_proc_read, NULL);

	create_proc_read_entry ("channel_weight",
				0,
				g_danube_dma_dir,
				channel_weight_proc_read, NULL);
	return 0;
}

void
dma_cleanup (void)
{
	int i;
	unregister_chrdev (DMA_MAJOR, "dma-core");
	free_page (KSEG0ADDR ((unsigned long) g_desc_list));
	remove_proc_entry ("channel_weight", g_danube_dma_dir);
	remove_proc_entry ("dma_list", g_danube_dma_dir);
	remove_proc_entry ("dma_register", g_danube_dma_dir);
	remove_proc_entry ("danube_dma", NULL);
	/*release the resources */
	for (i = 0; i < MAX_DMA_CHANNEL_NUM; i++)
		free_irq (dma_chan[i].irq, (void *) &dma_interrupt);
}

EXPORT_SYMBOL (dma_device_reserve);
EXPORT_SYMBOL (dma_device_release);
EXPORT_SYMBOL (dma_device_register);
EXPORT_SYMBOL (dma_device_unregister);
EXPORT_SYMBOL (dma_device_read);
EXPORT_SYMBOL (dma_device_write);

MODULE_LICENSE ("GPL");
