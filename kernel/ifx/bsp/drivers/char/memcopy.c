/******************************************************************************
**
** FILE NAME    : memcopy.c
** PROJECT      : Danube
** MODULES      : Central DMA / Memory Copy
**
** DATE         : 11 AUG 2005
** AUTHOR       : Wu Qi Ming
** DESCRIPTION  : Memory Copy Service of Central DMA
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
** 11 AUG 2005  Wu Qi Ming      Initiate Version
** 25 OCT 2006  Xu Liang        Add GPL header.
*******************************************************************************/

/* Group definitions for Doxygen */
/** \addtogroup API API-Functions */
/** \addtogroup Internal Internally used functions */

#include <linux/config.h>	/* retrieve the CONFIG_* macros */
#if defined(CONFIG_MODVERSIONS) && !defined(MODVERSIONS)
#   define MODVERSIONS
#endif

#if defined(MODVERSIONS) && !defined(__GENKSYMS__)
#    include <linux/modversions.h>
#endif

#ifndef EXPORT_SYMTAB
#  define EXPORT_SYMTAB		/* need this one 'cause we export symbols */
#endif

//#define CONFIG_DEVFS_FS

#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/proc_fs.h>
#include <linux/stat.h>
#include <asm/uaccess.h>
#include <linux/tty.h>
#include <linux/selection.h>
#include <linux/kmod.h>
#include <linux/vmalloc.h>
#include <linux/fcntl.h>

#include <asm/danube/danube.h>
#include <asm/danube/danube_dma.h>
#include <asm/danube/memcopy.h>

#ifdef CONFIG_DEVFS_FS
#include "linux/devfs_fs_kernel.h"
#endif /* CONFIG_DEVFS_FS */

#define ENABLE_TRACE
#ifdef ENABLE_TRACE
#define TRACE(fmt,args...) printk("%s: " fmt, __FUNCTION__ , ##args)
#else
#define TRACE(fmt,args...)
#endif
#define MAX_BLOCK_SIZE 65535

#ifdef CONFIG_DEVFS_FS
/** handles for Dev FS */
static devfs_handle_t memcopy_devfs_handle[2];
#endif
int memcopy_major = 42;
wait_queue_head_t memcopy_queue;
static u8 *g_src;
static u8 *g_dst;

typedef struct danube_memcopy_device {
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
} _danube_memcopy_device;

_danube_memcopy_device danube_memcopy[2];

u8 *memcopy_dma_buffer_alloc0 (int len, int *byte_offset, void **opt);
int memcopy_dma_buffer_free (u8 * dataptr, void *opt);
int memcopy_dma_intr_handler (struct dma_device_info *dma_dev, int status);

u8 *memcopy_dma_buffer_alloc1 (int len, int *byte_offset, void **opt);

/**
 * @brief buffer allocation function
 *
 * Called by dma driver, allocate buffer for dma to fill in the dma descriptors
 * @param len            data length
 * @param byte_offset    data offset
 * @param opt            optional parameter
 * @return u8*           allocated buffer pointer,NULL if unsuccessful
 * \ingroup Internal
 */
u8 *
memcopy_dma_buffer_alloc0 (int len, int *byte_offset, void **opt)
{
	u8 *addr;
	_danube_memcopy_device *pDev = danube_memcopy;
	if (pDev->len - pDev->dst_count > 0) {
		addr = pDev->dst + pDev->dst_count;
		pDev->dst_count = pDev->dst_count + len;
		*byte_offset = 0;

	}
	else {
		addr = NULL;
	}
	return addr;
}

/**
 * @brief buffer free function
 *
 * Called by dma driver, free the allocated buffer
 * @param dataptr data pointer to be freed
 * @param opt   optional parameter
 * @return int  1 if success, 0 else.
 * \ingroup Internal
 */
int
memcopy_dma_buffer_free (u8 * dataptr, void *opt)
{

	return 1;
}

/**
 * @brief receive packets from dma
 *
 * @param pDev the current memcopy device
 * @return the length of the received packet, 0 if receive unsuccessful
 * \ingroup Internal
 */
int
memcopy_receive (_danube_memcopy_device * pDev)
{
	int len;
	u8 *buf;
	len = dma_device_read (pDev->dma_device, &buf, 0);
	pDev->recv_count += len;
	if (pDev->recv_count == pDev->len) {
		dma_device_unregister (pDev->dma_device);
		dma_device_release (pDev->dma_device);
		pDev->dma_device = NULL;
	}
}

/**
 * @brief dma pseudo interrupt handler
 *
 * Refer to danube dma documentation for detailed description
 * @param dma_dev the dma device linked to the ethernet device
 * @param status  RCV_INT,TX_BUF_FULL_INT or TRANSMIT_CPT_INT
 * @return always return 1
 * \ingroup Internal
 */
int
memcopy_dma_intr_handler (struct dma_device_info *dma_dev, int status)
{
	_danube_memcopy_device *pDev;
	if (dma_dev == danube_memcopy[0].dma_device)
		pDev = danube_memcopy;
	else
		pDev = danube_memcopy + 1;
	switch (status) {
	case RCV_INT:
		//TRACE("switch receive chan=%d\n",dma_dev->current_rx_chan);
		memcopy_receive (pDev);
		break;
	case TX_BUF_FULL_INT:
		TRACE ("tx buffer full\n");
		break;
	case TRANSMIT_CPT_INT:
		TRACE ("tx buffer released\n");
		break;
	}
	return 0;

	return 0;
}

/**
 * @brief buffer allocation function
 *
 * Called by dma driver, allocate buffer for dma to fill in the dma descriptors
 * @param len            data length
 * @param byte_offset    data offset
 * @param opt            optional parameter
 * @return u8*           allocated buffer pointer,NULL if unsuccessful
 * \ingroup Internal
 */
u8 *
memcopy_dma_buffer_alloc1 (int len, int *byte_offset, void **opt)
{
	u8 *addr;
	_danube_memcopy_device *pDev = danube_memcopy + 1;
	if (pDev->len - pDev->dst_count > 0) {
		addr = pDev->dst + pDev->dst_count;
		pDev->dst_count = pDev->dst_count + len;

	}
	else {
		addr = NULL;
	}
	return addr;

}

/**
 * @brief transmit data function
 * @param pDev the current memcopy device
 * \ingroup Internal
 */
int
transmit_data (_danube_memcopy_device * pDev)
{
	int len;
	struct dma_device_info *dma_dev = pDev->dma_device;
	while (pDev->src_count < pDev->len) {
		len = pDev->len > MAX_BLOCK_SIZE ? MAX_BLOCK_SIZE : pDev->len;
		dma_device_write (dma_dev, pDev->src + pDev->src_count, len,
				  NULL);
		pDev->src_count += len;
	}

	return pDev->len;

}

/**
 * @brief memcopy function
 * @param pDev the current memcopy device
 * @param dst  destination address
 * @param src  source address
 * @param len  data length
 * @return 1 if success, 0 if failed
 * \ingroup API
 */
int
danube_dma_memcopy (_danube_memcopy_device * pDev, u8 * dst, u8 * src,
		    int len)
{
	int i;
	int ret = 0;
	struct dma_device_info *dma_device = pDev->dma_device;
	pDev->src = src;
	pDev->dst = dst;
	pDev->len = len;
	pDev->src_count = 0;
	pDev->dst_count = 0;
	pDev->recv_count = 0;
	if (len > MAX_BLOCK_SIZE)
		pDev->packet_size = MAX_BLOCK_SIZE;
	else
		pDev->packet_size = len;
	dma_device->num_rx_chan = 1;
	dma_device->num_tx_chan = 1;

	for (i = 0; i < dma_device->num_rx_chan; i++) {
		dma_device->rx_chan[i]->packet_size = pDev->packet_size;
		dma_device->rx_chan[i]->control = DANUBE_DMA_CH_ON;
	}

	for (i = 0; i < dma_device->num_tx_chan; i++) {
		dma_device->tx_chan[i]->control = DANUBE_DMA_CH_ON;
	}

	if (pDev - danube_memcopy == 0) {
		dma_device->buffer_alloc = &memcopy_dma_buffer_alloc0;
	}
	else {
		dma_device->buffer_alloc = &memcopy_dma_buffer_alloc1;
	}
	dma_device->buffer_free = &memcopy_dma_buffer_free;
	dma_device->intr_handler = &memcopy_dma_intr_handler;

	dma_device_register (dma_device);

	for (i = 0; i < dma_device->max_rx_chan_num; i++) {
		if ((dma_device->rx_chan[i])->control == DANUBE_DMA_CH_ON)
			(dma_device->rx_chan[i])->open (dma_device->
							rx_chan[i]);
	}

	transmit_data (pDev);
	i = 0;
	while (1) {
		i++;
		if (pDev->recv_count == pDev->len || i == 100)
			break;
		schedule ();
	}

#if 0
	add_wait_queue (&memcopy_queue, &pDev->wait);
	while (1) {
		set_current_state (TASK_INTERRUPTIBLE);
		if (pDev->recv_count == pDev->len)
			break;
		schedule ();
	}
	set_current_state (TASK_RUNNING);
	remove_wait_queue (&memcopy_queue, &pDev->wait);
#endif
	if (pDev->recv_count == pDev->len)
		ret = 1;
	else {

		ret = 0;
	}

	return ret;
}

/**
 * @brief memcopy open function, called by user mode application
 * @param inode
 * @param file
 * @return 1 if success, 0 if failed
 * \ingroup API
 */
static int
memcopy_open (struct inode *inode, struct file *file)
{
	int result = 0;
	int num;
	struct dma_device_info *dma_device;
	char buf[10] = { 0 };
	int from_kernel = 0;
	file->private_data = NULL;
	if ((int) inode == 0 || (int) inode == 1) {
		num = (int) inode;
		from_kernel = 1;
	}
	else {
		num = MINOR (inode->i_rdev);
		from_kernel = 0;
	}
	sprintf (buf, "MCTRL%d", num);
	dma_device = dma_device_reserve (buf);
	if (!dma_device)
		return -1;
	danube_memcopy[num].dma_device = dma_device;

	if (from_kernel == 0) {
		file->private_data = danube_memcopy + num;
		result = 1;
	}
	else {
		result = (int) (danube_memcopy + num);
	}

	MOD_INC_USE_COUNT;

	return result;
}

/**
 * @brief memcopy release function, called by user mode application
 * @param inode
 * @param file
 * @return 1 if success, 0 if failed
 * \ingroup API
 */

static int
memcopy_release (struct inode *inode, struct file *file)
{
	int num = MINOR (inode->i_rdev);
	struct dma_device_info *dma_device;
	dma_device =
		((_danube_memcopy_device *) file->private_data)->dma_device;
	if (dma_device)
		dma_device_release (dma_device);
	MOD_DEC_USE_COUNT;
	return 0;
}

static struct page *
dma_mmap_nopage (struct vm_area_struct *area,
		 unsigned long address, int write_access)
{
	struct page *pageptr = NULL;

	return pageptr;

}

static struct vm_operations_struct dma_file_mmap = {
      nopage:dma_mmap_nopage,

};

static int
dma_mmap (struct file *file, struct vm_area_struct *vma)
{

	return 0;
}

static ssize_t
memcopy_write (struct file *file, const char *buf, size_t count,
	       loff_t * offset)
{
	return count;
}

static ssize_t
memcopy_read (struct file *file, char *buf, size_t count, loff_t * offset)
{
	return 0;
}

/**
 * \brief memcopy ioctl functions
 * \param inode standard inode, defined by the system
 * \param file  standard file, defined by the system
 * \param cmd can only be MEMCOPY
 * \param arg should be a memcopy_req pointer
 * \ingroup API
 */
static int
memcopy_ioctl (struct inode *inode, struct file *file,
	       unsigned int cmd, unsigned long arg)
{

	int result = 0;
	_danube_memcopy_device *pDev;
	pDev = (_danube_memcopy_device *) file->private_data;
	struct memcopy_req *req =
		(struct memcopy_req *) kmalloc (sizeof (struct memcopy_req),
						GFP_KERNEL);
	switch (cmd) {
	case MEMCOPY:
		memset ((u8 *) g_src, 0, 4096);
		memset ((u8 *) g_dst, 0, 4096);
		copy_from_user (req, (struct memcopy_req *) arg,
				sizeof (struct memcopy_req));
		copy_from_user (g_src, req->src, req->len);
		result = danube_dma_memcopy (pDev, g_dst, g_src, req->len);
		copy_to_user (req->dst, g_dst, req->len);
		if (pDev->dma_device) {
			dma_device_unregister (pDev->dma_device);
		}
		//free_page((KSEG1ADDR)src);
		//free_page((KSEG1ADDR)dst);
		//result=danube_dma_memcopy(pDev,req->dst,req->src,req->len);
		break;

	}
	kfree (req);
	return result;
}

static struct file_operations memcopy_fops = {
      owner:THIS_MODULE,
      open:memcopy_open,
      release:memcopy_release,
      read:memcopy_read,
      write:memcopy_write,
      ioctl:memcopy_ioctl,
      mmap:dma_mmap,
};

/**
 * @brief memcopy module init function
 *
 * module init function
 * @param void
 * @return
 * \ingroup Internal
 */

int __init
memcopy_init (void)
{
	int result, i;
	char buf[10];
	TRACE ("memcopy init\n");

#ifdef CONFIG_DEVFS_FS
	memset (&(memcopy_devfs_handle), 0x00, sizeof (memcopy_devfs_handle));
	for (i = 0; i < 2; i++) {
		sprintf (buf, "memcopy%d", i);
		if ((memcopy_devfs_handle[i] = devfs_register (NULL,
							       buf,
							       DEVFS_FL_DEFAULT,
							       memcopy_major,
							       i,
							       S_IFCHR |
							       S_IRUGO |
							       S_IWUGO,
							       &memcopy_fops,
							       (void *) 0)) ==
		    NULL) {
			TRACE ("add device error!\n");
			result = -ENODEV;
		}
	}
#else
	result = register_chrdev (memcopy_major, "memcopy", &memcopy_fops);
	if (result < 0) {
		TRACE (KERN_WARNING "memcopy: can't get major %d\n",
		       memcopy_major);
		return result;
	}
#endif
	g_src = (u8 *) KSEG1ADDR (__get_free_page (GFP_DMA));
	g_dst = (u8 *) KSEG1ADDR (__get_free_page (GFP_DMA));
	//init_waitqueue_entry(&(danube_memcopy[0].wait),current);

	return 0;
}

/**
 * @brief memcopy module exit function
 *
 * module exit function
 * @param
 * @return
 * \ingroup Internal
 */
void
memcopy_cleanup (void)
{
	int i;
	TRACE ("memcopy cleanup\n");
#ifdef CONFIG_DEVFS_FS
	for (i = 0; i < 2; i++)
		devfs_unregister (memcopy_devfs_handle[i]);
#else
	unregister_chrdev (memcopy_major, "memcopy");
#endif
	free_page (g_src);
	free_page (g_dst);

}

module_init (memcopy_init);
module_exit (memcopy_cleanup);

MODULE_LICENSE ("GPL");
