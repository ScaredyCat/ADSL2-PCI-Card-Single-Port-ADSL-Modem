 /************************************************************************
 *
 * Copyright (c) 2006
 * Infineon Technologies AG
 * St. Martin Strasse 53; 81669 Muenchen; Germany
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 ************************************************************************/

/* Group definitions for Doxygen */
/** \addtogroup API API-Functions */
/** \addtogroup Internal Internally used functions */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/sem.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/delay.h>

#include <asm/danube/ifx_types.h>
#include <asm/danube/danube.h>
#include <asm/danube/mps.h>
#include <asm/danube/danube_gptu.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/delay.h>
#include <asm/danube/irq.h>
#include "mps_device.h"

#ifdef CONFIG_DEBUG_MINI_BOOT
#define CONFIG_DANUBE_USE_IKOS
#endif

extern mps_comm_dev ifx_mps_dev;

#define USE_ENCRYPTED_VOICE_FIRMWARE

/******************************************************************************
 * Function declarations
 ******************************************************************************/
static s32 ifx_mps_bufman_buf_provide (u32 segments, u32 segment_size);
static s32 ifx_mps_bufman_close (void);
static void ifx_mps_mbx_data_upstream (unsigned long dummy);
static void ifx_mps_mbx_cmd_upstream (unsigned long dummy);
static s32 ifx_mps_mbx_write_buffer_prov_message (mem_seg_t * mem_ptr,
						  u8 segments,
						  u32 segment_size);

void *ifx_mps_fastbuf_malloc (size_t size, int priority);
void ifx_mps_fastbuf_free (const void *ptr);
s32 ifx_mps_fastbuf_init (void);
s32 ifx_mps_fastbuf_close (void);

u32 ifx_mps_reset_structures (mps_comm_dev * pDev);

/******************************************************************************
 * Global variables
 ******************************************************************************/

extern u32 danube_cp1_base;

/* global structure that holds VCPU buffer management data */
mps_buf_mng_t mps_buffer = {
	.buf_level = 0,
	.buf_size = MPS_MEM_SEG_DATASIZE,
	.buf_threshold = MPS_BUFFER_THRESHOLD,
	.buf_initial = MPS_BUFFER_INITIAL,
	.buf_state = MPS_BUF_EMPTY,
	/* fast buffer manager */
	.malloc = &ifx_mps_fastbuf_malloc,
	.free = &ifx_mps_fastbuf_free,
	.init = &ifx_mps_fastbuf_init,
	.close = &ifx_mps_fastbuf_close,
};
mps_comm_dev *pMPSDev = &ifx_mps_dev;

int ifx_mps_initialising = 1;

#if CONFIG_DANUBE_MPS_HISTORY > 0
#if CONFIG_DANUBE_MPS_HISTORY > 512
#error "MPS history buffer > 512 words (2kB)"
#endif
#define MPS_HISTORY_BUFFER_SIZE (CONFIG_DANUBE_MPS_HISTORY)
int ifx_mps_history_buffer_freeze = 0;
u32 ifx_mps_history_buffer[MPS_HISTORY_BUFFER_SIZE] = { 0 };
int ifx_mps_history_buffer_words = 0;
#endif /* CONFIG_DANUBE_MPS_HISTORY > 0 */

/******************************************************************************
 * Fast bufferpool
 ******************************************************************************/
#define FASTBUF_USED    0x00000001
#define FASTBUF_BUFS    (MPS_BUFFER_INITIAL * 2)
#define FASTBUF_BUFSIZE MPS_MEM_SEG_DATASIZE

u32 *fastbuf_ptr;
u32 fastbuf_pool[FASTBUF_BUFS] = { 0 };
u32 fastbuf_index = 0;
u32 fastbuf_initialized = 0;

/**
 * Buffer allocate
 * Allocates and returns a buffer from the buffer pool. 
 *
 * \param   size        Size of requested buffer
 * \param   priority    Ignored, always atomic
 * 
 * \return  ptr    Address of the allocated buffer
 * \return  NULL   No buffer available
 * \ingroup Internal
 */
void *
ifx_mps_fastbuf_malloc (size_t size, int priority)
{
	u32 ptr, flags;
	int index = fastbuf_index;

	if (size > FASTBUF_BUFSIZE) {
		printk ("%s() - error, buffer too large\n", __FUNCTION__);
		return NULL;
	}

	save_and_cli (flags);
	do {
		if (index == FASTBUF_BUFS)
			index = 0;
		if ((volatile u32) fastbuf_pool[index] & FASTBUF_USED)
			continue;
		ptr = fastbuf_pool[index];
		(volatile u32) fastbuf_pool[index] |= FASTBUF_USED;
		fastbuf_index = index;
		restore_flags (flags);
		return (void *) ptr;
	}
	while (++index != fastbuf_index);

	restore_flags (flags);
	printk ("%s() - error, buffer pool empty\n", __FUNCTION__);
	return NULL;
}

/**
 * Buffer free
 * Returns a buffer to the buffer pool. 
 *
 * \param   ptr    Address of the allocated buffer
 *
 * \return  none
 * \ingroup Internal
 */
void
ifx_mps_fastbuf_free (const void *ptr)
{
	u32 flags;
	int index = fastbuf_index;

	save_and_cli (flags);
	do {
		if (index < 0)
			index = FASTBUF_BUFS - 1;
		if ((volatile u32) fastbuf_pool[index] ==
		    ((u32) ptr | FASTBUF_USED)) {
			(volatile u32) fastbuf_pool[index] &= ~FASTBUF_USED;
			restore_flags (flags);
			return;
		}
	}
	while (--index != fastbuf_index);

	restore_flags (flags);
	printk ("%s() - error, buffer not inside pool (0x%p)\n", __FUNCTION__,
		ptr);
}

/**
 * Bufferpool init
 * Initializes a buffer pool of size FASTBUF_BUFSIZE * FASTBUF_BUFS and
 * separates it into FASTBUF_BUFS chunks. The 32byte alignment of the chunks
 * is guaranteed by increasing the buffer size accordingly. The pointer to 
 * the pool is stored in fastbuf_ptr, while the pointers to the singles chunks 
 * are maintained in fastbuf_pool.
 * Bit 0 of the address in fastbuf_pool is used as busy indicator.
 *
 * \return -ENOMEM  Memory allocation failed
 * \return  OK      Buffer pool initialized
 * \ingroup Internal
 */
s32
ifx_mps_fastbuf_init (void)
{
	u32 *ptr, i;
	u32 bufsize = (FASTBUF_BUFSIZE + (FASTBUF_BUFSIZE % 32));

	if ((fastbuf_ptr =
	     kmalloc ((FASTBUF_BUFS * bufsize), GFP_KERNEL)) == NULL)
		return -ENOMEM;
	ptr = fastbuf_ptr;

	for (i = 0; i < FASTBUF_BUFS; i++) {
		fastbuf_pool[i] = (u32) ptr;
		ptr = (u32 *) ((u32) ptr + bufsize);
	}
	fastbuf_index = 0;
	fastbuf_initialized = 1;
	return OK;
}

/**
 * Bufferpool free
 * Frees the buffer pool allocated by ifx_mps_fastbuf_init and clears the
 * buffer pool.
 *
 * \return -ENOMEM  Memory allocation failed
 * \return  OK      Buffer pool initialized
 * \ingroup Internal
 */
s32
ifx_mps_fastbuf_close (void)
{
	int i;
	if (fastbuf_initialized) {
		for (i = 0; i < FASTBUF_BUFS; i++)
			fastbuf_pool[i] = 0;
		kfree (fastbuf_ptr);
		fastbuf_initialized = 0;
	}
	return (OK);
}

/******************************************************************************
 * Buffer manager
 ******************************************************************************/

/**
 * Get buffer fill level
 * This function return the current number of buffers provided to CPU1 
 *
 * \return  level    The current number of buffers
 * \return  -1       The buffer state indicates an error
 * \ingroup Internal
 */
static s32
ifx_mps_bufman_get_level (void)
{
	if (mps_buffer.buf_state != MPS_BUF_ERR)
		return mps_buffer.buf_level;
	return -1;
}

/**
 * Update buffer state
 * This function will set the buffer state according to the current buffer level
 * and the previous state.
 *
 * \return  state    The new buffer state
 * \ingroup Internal
 */
static mps_buffer_state_e
ifx_mps_bufman_update_state (void)
{
	if (mps_buffer.buf_state != MPS_BUF_ERR) {
		if (mps_buffer.buf_level == 0)
			mps_buffer.buf_state = MPS_BUF_EMPTY;
		if ((mps_buffer.buf_level > 0)
		    && (mps_buffer.buf_level < mps_buffer.buf_threshold))
			mps_buffer.buf_state = MPS_BUF_LOW;
		if ((mps_buffer.buf_level >= mps_buffer.buf_threshold)
		    && (mps_buffer.buf_level <= MPS_BUFFER_MAX_LEVEL))
			mps_buffer.buf_state = MPS_BUF_OK;
		if (mps_buffer.buf_level > MPS_BUFFER_MAX_LEVEL)
			mps_buffer.buf_state = MPS_BUF_OV;
	}
	return mps_buffer.buf_state;
}

/**
 * Increase buffer level
 * This function increments the buffer level by the passed value.
 *
 * \param   value    Increment value
 * \return  level    The new buffer level
 * \return  -1       Maximum value reached
 * \ingroup Internal
 */
static s32
ifx_mps_bufman_inc_level (u32 value)
{
	u32 flags;

	if (mps_buffer.buf_level + value > MPS_BUFFER_MAX_LEVEL) {
		printk ("ifx_mps_bufman_inc_level(): Maximum reached !\n");
		return -1;
	}
	save_and_cli (flags);
	mps_buffer.buf_level += value;
	ifx_mps_bufman_update_state ();
	restore_flags (flags);

	return mps_buffer.buf_level;
}

/**
 * Decrease buffer level
 * This function decrements the buffer level with the passed value. 
 *
 * \param   value    Decrement value
 * \return  level    The new buffer level
 * \return  -1       Minimum value reached
 * \ingroup Internal
 */
static s32
ifx_mps_bufman_dec_level (u32 value)
{
	u32 flags;

	if (mps_buffer.buf_level < value) {
		printk ("ifx_mps_bufman_dec_level(): Minimum reached !\n");
		return -1;
	}
	save_and_cli (flags);
	mps_buffer.buf_level -= value;
	ifx_mps_bufman_update_state ();
	restore_flags (flags);

	return mps_buffer.buf_level;
}

/**
 * Get buffer state
 * This function returns the current buffer state.
 *
 * \return  level    The current buffer level
 * \ingroup Internal
 */
static mps_buffer_state_e
ifx_mps_bufman_get_state (void)
{
	return mps_buffer.buf_state;
}

/**
 * Init buffer management
 * This function initializes the buffer management data structures and
 * provides buffer segments to CPU1.
 *
 * \return  0        OK, initialized and message sent
 * \return  -1       Error during message transmission
 * \ingroup Internal
 */
s32
ifx_mps_bufman_init (void)
{
	int i;
	s32 ret = ERROR;

	mps_buffer.init ();

	for (i = 0; i < mps_buffer.buf_initial;
	     i += MPS_DEFAULT_PROVISION_SEGMENTS_PER_MSG) {
		ret = ifx_mps_bufman_buf_provide
			(MPS_DEFAULT_PROVISION_SEGMENTS_PER_MSG,
			 mps_buffer.buf_size);
	}
	return ret;
}

/**
 * Close buffer management
 * This function is called on termination of voice CPU firmware. The registered
 * close function has to take care of freeing buffers still left in VCPU.
 *
 * \return  0        OK, buffer manage shutdown correctly
 * \return  -1       Error during shutdown
 * \ingroup Internal
 */
s32
ifx_mps_bufman_close (void)
{
	s32 ret = ERROR;

	mps_buffer.close ();

	mps_buffer.buf_level = 0;
	mps_buffer.buf_size = MPS_MEM_SEG_DATASIZE;
	mps_buffer.buf_threshold = MPS_BUFFER_THRESHOLD;
	mps_buffer.buf_initial = MPS_BUFFER_INITIAL;
	mps_buffer.buf_state = MPS_BUF_EMPTY;

	return ret;
}

/**
 * Free buffer
 *
 * \ingroup Internal
 */
void
ifx_mps_bufman_free (const void *ptr)
{
	mps_buffer.free ((void *) KSEG0ADDR (ptr));
}

/**
 * Allocate buffer
 *
 * \ingroup Internal
 */
void *
ifx_mps_bufman_malloc (size_t size, int priority)
{
	void *ptr;
	ptr = mps_buffer.malloc (size, priority);
	return ptr;
}

/**
 * Overwrite buffer management
 * Allows the upper layer to register its own malloc/free functions in order to do
 * its own buffer managment. To unregister driver needs to be re-initialized.
 *
 * \param   malloc      Buffer allocation - arguments and return value as kmalloc   
 * \param   free        Buffer de-allocation - arguments and return value as kmalloc
 * \param   buf_size    Size of buffers provided to voice CPU
 * \param   treshold    Count of buffers provided to voice CPU
 */
void
ifx_mps_bufman_register (void *(*malloc) (size_t size, int priority),
			 void (*free) (const void *ptr), u32 buf_size,
			 u32 treshold)
{
	mps_buffer.buf_size = buf_size;
	mps_buffer.buf_threshold = treshold;
	mps_buffer.buf_initial =
		treshold + MPS_DEFAULT_PROVISION_SEGMENTS_PER_MSG;
	mps_buffer.malloc = malloc;
	mps_buffer.free = free;
}

/**
 * Send buffer provisioning message
 * This function sends a buffer provisioning message to CPU1 using the passed
 * segment parameters.
 *
 * \param   segments     Number of memory segments to be provided to CPU1
 * \param   segment_size Size of each memory segment in bytes
 * \return  0            OK, message sent
 * \return  -1           ERROR, if message could not be sent
 * \ingroup Internal
 */
static s32
ifx_mps_bufman_buf_provide (u32 segments, u32 segment_size)
{
	s32 i, j;
	static mem_seg_t mem_seg_ptr;

	memset (&mem_seg_ptr, 0, sizeof (mem_seg_t));
	for (i = 0; i < segments; i++) {
		mem_seg_ptr[i] =
			(u32 *) PHYSADDR ((u32) mps_buffer.
					  malloc (segment_size, GFP_ATOMIC));
		if (mem_seg_ptr[i] == NULL) {
			printk ("%s(): cannot allocate buffer\n",
				__FUNCTION__);
			goto error;
		}

	}

	if (ifx_mps_mbx_write_buffer_prov_message
	    (&mem_seg_ptr, segments, segment_size) != OK) {
		goto error;
	}
	else {
		return OK;
	}
      error:
	for (j = i - 1; j >= 0; j--)
		mps_buffer.free ((void *) KSEG0ADDR (mem_seg_ptr[j]));
	return -1;
}

/******************************************************************************
 * FIFO Managment 
 ******************************************************************************/

/**
 * Clear FIFO
 * This function clears the FIFO by resetting the pointers. The data itself is 
 * not cleared.
 * 
 * \param   fifo    Pointer to FIFO structure
 * \ingroup Internal
 */
void
ifx_mps_fifo_clear (mps_fifo * fifo)
{
	*fifo->pread_off = fifo->size - 4;
	*fifo->pwrite_off = fifo->size - 4;
	return;
}

/**
 * Check FIFO for being not empty
 * This function checks whether the referenced FIFO contains at least 
 * one unread data byte.
 * 
 * \param   fifo     Pointer to FIFO structure
 * \return  1        TRUE if data to be read is available in FIFO, 
 * \return  0        FALSE if FIFO is empty.
 * \ingroup Internal
 */
bool_t
ifx_mps_fifo_not_empty (mps_fifo * fifo)
{
	if (*fifo->pwrite_off == *fifo->pread_off)
		return FALSE;
	else
		return TRUE;
}

/**
 * Check FIFO for free memory
 * This function returns the amount of free bytes in FIFO.
 *
 * \param   fifo     Pointer to FIFO structure
 * \return  0        The FIFO is full,
 * \return  count    The number of available bytes
 * \ingroup Internal
 */
u32
ifx_mps_fifo_mem_available (mps_fifo * fifo)
{
	u32 retval;

	retval = (fifo->size - 1 -
		  (*fifo->pread_off - *fifo->pwrite_off)) & (fifo->size - 1);
	return (retval);
}

/**
 * Check FIFO for requested amount of memory
 * This function checks whether the requested FIFO is capable to store
 * the requested amount of data bytes.
 * The selected Fifo should be a downstream direction Fifo. 
 *
 * \param   fifo     Pointer to mailbox structure to be checked
 * \param   bytes    Requested data bytes
 * \return  1        TRUE if space is available in FIFO,
 * \return  0        FALSE if not enough space in FIFO.
 * \ingroup Internal
 */
bool_t
ifx_mps_fifo_mem_request (mps_fifo * fifo, u32 bytes)
{
	u32 bytes_avail = ifx_mps_fifo_mem_available (fifo);

	if (bytes_avail > bytes) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}

/**
 * Update FIFO read pointer
 * This function updates the position of the referenced FIFO.In case of 
 * reaching the FIFO's end the pointer is set to the start position.
 *
 * \param   fifo      Pointer to FIFO structure
 * \param   increment Increment for read index
 * \ingroup Internal 
 */
void
ifx_mps_fifo_read_ptr_inc (mps_fifo * fifo, u8 increment)
{
	s32 new_read_index = (s32) (*fifo->pread_off) - (s32) increment;

	if ((u32) increment > fifo->size) {
		printk ("%s(): Invalid offset passed: %d !\n", __FUNCTION__,
			increment);
		return;
	}

	if (new_read_index >= 0) {
		*(fifo->pread_off) = (u32) new_read_index;
	}
	else {
		*(fifo->pread_off) = (u32) (new_read_index + (s32) (fifo->size));	/* overflow */
	}
	return;
}

/**
 * Update FIFO write pointer
 * This function updates the position of the write pointer of the referenced FIFO.
 * In case of reaching the FIFO's end the pointer is set to the start position.
 *
 * \param   fifo      Pointer to FIFO structure 
 * \param   increment Increment of write index
 * \ingroup Internal 
 */
void
ifx_mps_fifo_write_ptr_inc (mps_fifo * fifo, u16 increment)
{
	/* calculate new index ignoring ring buffer overflow */
	s32 new_write_index = (s32) (*fifo->pwrite_off) - (s32) increment;

	if ((u32) increment > fifo->size) {
		printk ("%s(): Invalid offset passed: %d !\n", __FUNCTION__,
			increment);
		return;
	}

	if (new_write_index >= 0) {
		*fifo->pwrite_off = (u32) new_write_index;	/* no overflow */
	}
	else {
		*fifo->pwrite_off =
			(u32) (new_write_index + (s32) (fifo->size));
	}
	return;
}

/**
 * Write data word to FIFO
 * This function writes a data word (32bit) to the referenced FIFO. The word is 
 * written to the position defined by the current write pointer index and the 
 * offset being passed.
 *
 * \param   mbx            Pointer to FIFO structure
 * \param   data           Data word to be written
 * \param   offset         Byte offset to be added to write pointer position
 * \return  0              OK, word written
 * \return  -1             Invalid offset.
 * \ingroup Internal 
 */
s32
ifx_mps_fifo_write (mps_fifo * fifo, u32 data, u8 offset)
{
	/* calculate write position */
	s32 new_write_index = (s32) * fifo->pwrite_off - (s32) offset;
	u32 write_address;

	if (offset > fifo->size) {
		printk ("%s(): Invalid offset passed !\n", __FUNCTION__);
		return -1;
	}

	write_address = (u32) fifo->pend + *fifo->pwrite_off - (u32) offset;
	if (new_write_index < 0) {
		write_address += fifo->size;
	}

	*(u32 *) write_address = data;
	return 0;
}

/**
 * Read data word from FIFO
 * This function reads a data word (32bit) from the referenced FIFO. It first
 * calculates and checks the address defined by the FIFO's read index and passed
 * offset. The read pointer is not updated by this function. 
 * It has to be updated after the complete message has been read.
 *
 * \param   fifo          Pointer to FIFO structure
 * \param   offset        Offset to read pointer position to be read from
 * \return  count         Number of data words read.
 * \return  -1            Invalid offset
 * \ingroup Internal 
 */
u32
ifx_mps_fifo_read (mps_fifo * fifo, u8 offset)
{
	u32 read_address;
	s32 new_read_index = ((s32) * fifo->pread_off) - (s32) offset;
	u32 ret;

	if (offset > fifo->size) {
		printk ("%s(): Invalid offset passed: %d !\n", __FUNCTION__,
			offset);
		return -1;
	}

	read_address =
		(u32) fifo->pend + (u32) * fifo->pread_off - (u32) offset;
	if (new_read_index < 0) {
		read_address += fifo->size;
	}
	ret = *(u32 *) read_address;

	return (ret);
}

/******************************************************************************
 * DANUBE Specific Routines
 ******************************************************************************/

/**
 * Initialize Timer
 * This function initializes the general purpose timer of the Danube, which
 * is used by the voice firmware.
 *
 * \ingroup Internal 
 */
void
ifx_mps_init_gpt_danube (void)
{
#if 0
	/* enable supervisor mode */
//    *DANUBE_BIU_FB1_CFG = 0x00000001;

	/* enable power supply for GPTC module */
	*DANUBE_PMU_PWDCR &= (~0x00001000);

	*DANUBE_GPTC_CLC = 0x00010114;
	*DANUBE_GPTC_CON_1B = 0x000005C5;	// 0x000001C4;
	*DANUBE_GPTC_RUN_1B = 0x00000005;
	*DANUBE_GPTC_RELOAD_1B = 0x00000001;

	/* enable GPTC interrupt */
	*DANUBE_GPTC_IRNEN = 0x00000002;

	/* disable supervisor mode -> user mode */
//    *DANUBE_BIU_FB1_CFG = 0x00000000;
#endif

#if 0
	set_timer (TIMER1B, 8000000, 1, TIMER_FLAG_CALLBACK_IN_IRQ, NULL,
		   NULL);
	start_timer (TIMER1B, 0);
	disable_irq (INT_NUM_IM3_IRL23);
#endif

#if 1
	set_counter (TIMER1B, 0, 1, 0, TIMER_FLAG_ANY_EDGE,
		     TIMER_FLAG_CALLBACK_IN_IRQ, NULL, NULL);
	start_timer (TIMER1B, 0);
//      disable_irq(INT_NUM_IM3_IRL23);
#endif

	return;
}

/**
 * Firmware download to Voice CPU
 * This function performs a firmware download to the coprocessor.
 *
 * \param   pMBDev    Pointer to mailbox device structure
 * \param   pFWDwnld  Pointer to firmware structure
 * \return  0         OK, firmware ready
 * \return  -1        ERROR, firmware not downloaded. 
 * \ingroup Internal
 */
s32
ifx_mps_download_firmware (mps_mbx_dev * pMBDev, mps_fw * pFWDwnld)
{
#if 0
	u32 reset_ctrl;
#endif

	u32 *pDwnLd;

	ifx_mps_reset ();

#if ! defined( CONFIG_DANUBE_USE_IKOS )
	memcpy ((char *) danube_cp1_base, (char *) pFWDwnld->data,
		pFWDwnld->length);
	pDwnLd = (u32 *) danube_cp1_base;
#else
#warning Assuming FW to be present in SDRAM!
	pDwnLd = (u32 *) pFWDwnld;

#endif

	/* reconfigure CPU1 boot parameters for DSP restart after FW download
	 *
	 * - enable software boot select
	 * - set boot configuration for SDRAM boot
	 * - write new reset vector (firmware start address) 
	 * - restart program execution 
	 */

	ifx_mps_dev.base_global->MBX_CPU1_BOOT_CFG.MPS_BOOT_SIZE =
		pFWDwnld->length;
	ifx_mps_dev.base_global->MBX_CPU1_BOOT_CFG.MPS_BOOT_RVEC =
		(u32) pDwnLd;

#ifdef USE_ENCRYPTED_VOICE_FIRMWARE
	ifx_mps_dev.base_global->MBX_CPU1_BOOT_CFG.MPS_CFG_STAT |= 0x00020000;
        ifx_mps_release ();
#else
	/* Activate BootROM shortcut */
	ifx_mps_dev.base_global->MBX_CPU1_BOOT_CFG.MPS_CFG_STAT |= 0x00700000;
	ifx_mps_release ();
#endif

//    *DANUBE_RCU_RST_REQ |= DANUBE_RCU_RST_REQ_CPU0_BR;

#if 0				/* FixMe: SWTBOOT only necessary for bootcfg 111 -> blind boot from external FLASH */

	reset_ctrl = *DANUBE_RCU_RST_REQ;
	*DANUBE_RCU_RST_REQ = reset_ctrl | DANUBE_RCU_RST_REQ_CPU0_BR |	/* restart VCPU program execution */
		DANUBE_RCU_RST_REQ_SWTBOOT |	/* enable SW Boot configuration */
		DANUBE_RCU_RST_REQ_CFG_SET (7);	/* set SDRAM as secondary boot source */
#endif

	return (OK);
}

/**
 * Restart CPU1
 * This function restarts CPU1 by accessing the reset request register and 
 * reinitializes the mailbox.
 *
 * \return  0        OK, successful restart 
 * \return  -1       ERROR, if reset failed (currently always OK)
 * \ingroup Internal
 */
s32
ifx_mps_restart (void)
{
	u32 reset_ctrl;

	ifx_mps_reset ();

	/* start VCPU program execution */
	ifx_mps_release ();

//    reset_ctrl = *DANUBE_RCU_RST_REQ;
//    *DANUBE_RCU_RST_REQ = reset_ctrl | DANUBE_RCU_RST_REQ_CPU0_BR;
	printk ("IFX_MPS: Restarting firmware...");
//	return ifx_mps_print_fw_version();
	return (OK);
}

/******************************************************************************
 * Global Routines
 ******************************************************************************/

/**
 * Open MPS device
 * Open routine for the MPS device driver. 
 * 
 * \param   mps_device  MPS communication device structure
 * \param   pMBDev      Pointer to mailbox device structure
 * \param   bcommand    voice/command selector, TRUE -> command, FALSE -> voice
 * \return  0           OK, successfully opened
 * \return  -1          ERROR, Driver already installed
 * \ingroup Internal
 */
s32
ifx_mps_common_open (mps_comm_dev * mps_device, mps_mbx_dev * pMBDev,
		     bool_t bcommand)
{
	MPS_Ad0Reg_u Ad0Reg;

	/* device is already installed */
	if (pMBDev->Installed == TRUE) {
		return (ERROR);
	}

	pMBDev->Installed = TRUE;

	/* enable necessary MPS interrupts */
	Ad0Reg.val = *DANUBE_MPS_AD0ENR;
	if (bcommand == TRUE) {
		/* enable upstream command interrupt */
		Ad0Reg.fld.cu_mbx = 1;
	}
	else {
		/* enable upstream voice interrupt */
		Ad0Reg.fld.du_mbx = 1;
	}
	*DANUBE_MPS_AD0ENR = Ad0Reg.val;

	return (OK);
}

/**
 * Close routine for MPS device driver
 * This function closes the channel assigned to the passed mailbox 
 * device structure.
 *
 * \param   pMBDev   Pointer to mailbox device structure
 * \return  0        OK, will never fail
 * \ingroup Internal
 */
s32
ifx_mps_common_close (mps_mbx_dev * pMBDev)
{
	MPS_Ad0Reg_u Ad0Reg;

	/* clean data structures */
	pMBDev->Installed = FALSE;

	/* Clear the downstream queues for voice fds only */
	if (pMBDev->devID != command) {
		/* if all voice channel connections are closed disable voice channel interrupt */
		pMPSDev = pMBDev->pVCPU_DEV;
		if ((pMPSDev->voice_mb[0].Installed == FALSE) &&
		    (pMPSDev->voice_mb[1].Installed == FALSE) &&
		    (pMPSDev->voice_mb[2].Installed == FALSE) &&
		    (pMPSDev->voice_mb[3].Installed == FALSE)) {
			/* disable upstream voice interrupt */
			Ad0Reg.val = *DANUBE_MPS_AD0ENR;
			Ad0Reg.fld.du_mbx = 0;
			*DANUBE_MPS_AD0ENR = Ad0Reg.val;
		}
#ifdef CONFIG_PROC_FS
		pMBDev->upstrm_fifo.min_space = MBX_DATA_FIFO_SIZE;
		pMBDev->dwstrm_fifo.min_space = MBX_DATA_FIFO_SIZE;
#endif
	}
	else {
		/* disable upstream command interrupt */
		Ad0Reg.val = *DANUBE_MPS_AD0ENR;
		Ad0Reg.fld.cu_mbx = 0;
		*DANUBE_MPS_AD0ENR = Ad0Reg.val;
#ifdef CONFIG_PROC_FS
		pMBDev->upstrm_fifo.min_space = MBX_CMD_FIFO_SIZE;
		pMBDev->dwstrm_fifo.min_space = MBX_CMD_FIFO_SIZE;
#endif
	}
	return (OK);
}

/******************************************************************************
 * 
 ******************************************************************************/

/**
 * Reset CPU1
 * This function causes a reset of CPU1 by clearing the CPU0 boot ready bit
 * in the reset request register RCU_RST_REQ.
 * It does not change the boot configuration registers for CPU0 or CPU1.
 * 
 * \return  0        OK, cannot fail
 * \ingroup Internal
 */
s32
ifx_mps_reset (void)
{
//    printk("DANUBE_RCU_RST_REQ = 0x%08x\n", *DANUBE_RCU_RST_REQ);
	*DANUBE_RCU_RST_REQ |= (DANUBE_RCU_RST_REQ_CPU1);
	wmb ();
	//*DANUBE_MPS_VCPU_FW_AD = 0x0;
//    printk("%s DANUBE_RCU_RST_REQ(0x%08x) = 0x%08x\n",__FUNCTION__, DANUBE_RCU_RST_REQ, *DANUBE_RCU_RST_REQ);

	/* reset driver */
	ifx_mps_reset_structures (pMPSDev);
	ifx_mps_bufman_close ();

	return (OK);
}

s32
ifx_mps_release (void)
{
	*DANUBE_RCU_RST_REQ |= 0x20000000;
	*DANUBE_RCU_RST_REQ &= (~8);
	wmb ();
//    printk("%s DANUBE_RCU_RST_REQ(0x%08x) = 0x%08x\n",__FUNCTION__, DANUBE_RCU_RST_REQ, *DANUBE_RCU_RST_REQ);

	return (OK);
}

/**
 * MPS Structure Release
 * This function releases the entire MPS data structure used for communication 
 * between the CPUs.
 * 
 * \param   pDev     Poiter to MPS communication structure
 * \ingroup Internal
 */
void
ifx_mps_release_structures (mps_comm_dev * pDev)
{
	s32 count;

	kfree (pDev->command_mb.sem_dev);

	/* Initialize the Message queues for the voice packets */
	for (count = 0; count < NUM_VOICE_CHANNEL; count++) {
		kfree (pDev->voice_mb[count].sem_dev);
		kfree (pDev->voice_mb[count].sem_read_fifo);

#ifdef MPS_FIFO_BLOCKING_WRITE
		kfree (pDev->voice_mb[count].sem_write_fifo);
#endif /* MPS_FIFO_BLOCKING_WRITE */

		kfree (pDev->voice_mb[count].wakeup_pending);
	}

	kfree (pDev->wakeup_pending);
}

/**
 * MPS Structure Initialization
 * This function initializes the data structures of the Multi Processor System 
 * that are necessary for inter processor communication
 *
 * \param   pDev     Pointer to MPS device structure to be initialized
 * \return  0        OK, if initialization was successful 
 * \return  -1       ERROR, allocation or semaphore access problem
 * \ingroup Internal
 */
u32
ifx_mps_init_structures (mps_comm_dev * pDev)
{
	mps_mbx_reg *MBX_Memory;
	s32 i;

	/* Initialize MPS main structure */
	memset ((void *) pDev, 0, sizeof (mps_comm_dev));

	pDev->base_global = (mps_mbx_reg *) DANUBE_MPS_SRAM;
	pDev->flags = 0x00000000;
	MBX_Memory = pDev->base_global;

	/*
	 * Initialize common mailbox definition area which is used by both CPUs
	 * for MBX communication. These are: mailbox base address, mailbox size,
	 * mailbox read index and mailbox write index. for command and voice mailbox,
	 * upstream and downstream direction.
	 */

	memset ((void *) MBX_Memory,	/* avoid to overwrite CPU boot registers */
		0, sizeof (mps_mbx_reg) - 2 * sizeof (mps_boot_cfg_reg));
	MBX_Memory->MBX_UPSTR_CMD_BASE =
		(u32 *) PHYSADDR ((u32) MBX_UPSTRM_CMD_FIFO_BASE);
	MBX_Memory->MBX_UPSTR_CMD_SIZE = MBX_CMD_FIFO_SIZE;
	MBX_Memory->MBX_DNSTR_CMD_BASE =
		(u32 *) PHYSADDR ((u32) MBX_DNSTRM_CMD_FIFO_BASE);
	MBX_Memory->MBX_DNSTR_CMD_SIZE = MBX_CMD_FIFO_SIZE;
	MBX_Memory->MBX_UPSTR_DATA_BASE =
		(u32 *) PHYSADDR ((u32) MBX_UPSTRM_DATA_FIFO_BASE);
	MBX_Memory->MBX_UPSTR_DATA_SIZE = MBX_DATA_FIFO_SIZE;
	MBX_Memory->MBX_DNSTR_DATA_BASE =
		(u32 *) PHYSADDR ((u32) MBX_DNSTRM_DATA_FIFO_BASE);
	MBX_Memory->MBX_DNSTR_DATA_SIZE = MBX_DATA_FIFO_SIZE;
	/* set read and write pointers below to the FIFO's uppermost address */
	MBX_Memory->MBX_UPSTR_CMD_READ = (MBX_Memory->MBX_UPSTR_CMD_SIZE - 4);
	MBX_Memory->MBX_UPSTR_CMD_WRITE = (MBX_Memory->MBX_UPSTR_CMD_READ);
	MBX_Memory->MBX_DNSTR_CMD_READ = (MBX_Memory->MBX_DNSTR_CMD_SIZE - 4);
	MBX_Memory->MBX_DNSTR_CMD_WRITE = MBX_Memory->MBX_DNSTR_CMD_READ;
	MBX_Memory->MBX_UPSTR_DATA_READ =
		(MBX_Memory->MBX_UPSTR_DATA_SIZE - 4);
	MBX_Memory->MBX_UPSTR_DATA_WRITE = MBX_Memory->MBX_UPSTR_DATA_READ;
	MBX_Memory->MBX_DNSTR_DATA_READ =
		(MBX_Memory->MBX_DNSTR_DATA_SIZE - 4);
	MBX_Memory->MBX_DNSTR_DATA_WRITE = MBX_Memory->MBX_DNSTR_DATA_READ;

	/*
	 * Configure command mailbox sub structure pointers
	 * to global mailbox register addresses
	 */
	/*
	 * set command mailbox sub structure pointers
	 * to global mailbox register addresses
	 */
	pDev->command_mb.upstrm_fifo.pstart = (u32 *)
		KSEG1ADDR ((MBX_Memory->MBX_UPSTR_CMD_BASE +
			    ((MBX_Memory->MBX_UPSTR_CMD_SIZE - 4) >> 2)));
	pDev->command_mb.upstrm_fifo.pend =
		(u32 *) KSEG1ADDR (MBX_Memory->MBX_UPSTR_CMD_BASE);
	pDev->command_mb.upstrm_fifo.pwrite_off =
		(u32 *) & (MBX_Memory->MBX_UPSTR_CMD_WRITE);
	pDev->command_mb.upstrm_fifo.pread_off =
		(u32 *) & (MBX_Memory->MBX_UPSTR_CMD_READ);
	pDev->command_mb.upstrm_fifo.size = MBX_Memory->MBX_UPSTR_CMD_SIZE;
#ifdef CONFIG_PROC_FS
	pDev->command_mb.upstrm_fifo.min_space =
		MBX_Memory->MBX_UPSTR_CMD_SIZE;
#endif

	pDev->command_mb.dwstrm_fifo.pstart = (u32 *)
		KSEG1ADDR ((MBX_Memory->MBX_DNSTR_CMD_BASE +
			    ((MBX_Memory->MBX_DNSTR_CMD_SIZE - 4) >> 2)));
	pDev->command_mb.dwstrm_fifo.pend =
		(u32 *) KSEG1ADDR (MBX_Memory->MBX_DNSTR_CMD_BASE);
	pDev->command_mb.dwstrm_fifo.pwrite_off =
		(u32 *) & (MBX_Memory->MBX_DNSTR_CMD_WRITE);
	pDev->command_mb.dwstrm_fifo.pread_off =
		(u32 *) & (MBX_Memory->MBX_DNSTR_CMD_READ);
	pDev->command_mb.dwstrm_fifo.size = MBX_Memory->MBX_DNSTR_CMD_SIZE;
#ifdef CONFIG_PROC_FS
	pDev->command_mb.dwstrm_fifo.min_space =
		MBX_Memory->MBX_DNSTR_CMD_SIZE;
#endif

	pDev->command_mb.pVCPU_DEV = pDev;	/* global pointer reference */
	pDev->command_mb.Installed = FALSE;	/* current installation status */

	memset (&pDev->event, 0, sizeof (MbxEventRegs_s));

	/* initialize the semaphores for multitasking access */
	pDev->command_mb.sem_dev =
		kmalloc (sizeof (struct semaphore), GFP_KERNEL);
	sema_init (pDev->command_mb.sem_dev, 1);

	/* select mechanism implemented for each queue */
	init_waitqueue_head (&pDev->command_mb.mps_wakeuplist);

	/*
	 * configure voice channel communication structure fields
	 * that are common to all voice channels
	 */
	for (i = 0; i < NUM_VOICE_CHANNEL; i++) {
		/*
		 * Since all voice channels use the same physical mailbox memory,
		 * set the upstream mailbox pointers of all voice channels
		 * to the upstream data mailbox area and set the downstream mailbox pointers of all
		 * voice channels to the downstream data mailbox area.
		 */
		/* voice upstream data mailbox area */
		pDev->voice_mb[i].upstrm_fifo.pstart =
			(u32 *) KSEG1ADDR (MBX_Memory->MBX_UPSTR_DATA_BASE +
					   ((MBX_Memory->MBX_UPSTR_DATA_SIZE -
					     4) >> 2));
		pDev->voice_mb[i].upstrm_fifo.pend =
			(u32 *) KSEG1ADDR (MBX_Memory->MBX_UPSTR_DATA_BASE);
		pDev->voice_mb[i].upstrm_fifo.pwrite_off =
			(u32 *) & (MBX_Memory->MBX_UPSTR_DATA_WRITE);
		pDev->voice_mb[i].upstrm_fifo.pread_off =
			(u32 *) & (MBX_Memory->MBX_UPSTR_DATA_READ);
		pDev->voice_mb[i].upstrm_fifo.size =
			MBX_Memory->MBX_UPSTR_DATA_SIZE;
#ifdef CONFIG_PROC_FS
		pDev->voice_mb[i].upstrm_fifo.min_space =
			MBX_Memory->MBX_UPSTR_DATA_SIZE;
#endif

		/* voice downstream data mailbox area */
		pDev->voice_mb[i].dwstrm_fifo.pstart =
			(u32 *) KSEG1ADDR (MBX_Memory->MBX_DNSTR_DATA_BASE +
					   ((MBX_Memory->MBX_DNSTR_DATA_SIZE -
					     4) >> 2));
		pDev->voice_mb[i].dwstrm_fifo.pend =
			(u32 *) KSEG1ADDR (MBX_Memory->MBX_DNSTR_DATA_BASE);
		pDev->voice_mb[i].dwstrm_fifo.pwrite_off =
			(u32 *) & (MBX_Memory->MBX_DNSTR_DATA_WRITE);
		pDev->voice_mb[i].dwstrm_fifo.pread_off =
			(u32 *) & (MBX_Memory->MBX_DNSTR_DATA_READ);
		pDev->voice_mb[i].dwstrm_fifo.size =
			MBX_Memory->MBX_DNSTR_DATA_SIZE;
#ifdef CONFIG_PROC_FS
		pDev->voice_mb[i].dwstrm_fifo.min_space =
			MBX_Memory->MBX_UPSTR_DATA_SIZE;
#endif

		pDev->voice_mb[i].Installed = FALSE;	/* current mbx installation status */
		pDev->voice_mb[i].base_global =
			(mps_mbx_reg *) VCPU_BASEADDRESS;
		pDev->voice_mb[i].pVCPU_DEV = pDev;	/* global pointer reference */
		pDev->voice_mb[i].down_callback = NULL;	/* callback functions for */
		pDev->voice_mb[i].up_callback = NULL;	/* down- and upstream dir. */

		/* initialize the semaphores for multitasking access */
		pDev->voice_mb[i].sem_dev =
			kmalloc (sizeof (struct semaphore), GFP_KERNEL);
		sema_init (pDev->voice_mb[i].sem_dev, 1);

		/* initialize the semaphores to read from the fifo */
		pDev->voice_mb[i].sem_read_fifo =
			kmalloc (sizeof (struct semaphore), GFP_KERNEL);
		sema_init (pDev->voice_mb[i].sem_read_fifo, 0);

#ifdef MPS_FIFO_BLOCKING_WRITE
		pDev->voice_mb[i].sem_write_fifo =
			kmalloc (sizeof (struct semaphore), GFP_KERNEL);
		sema_init (pDev->voice_mb[i].sem_write_fifo, 0);
		pDev->voice_mb[i].bBlockWriteMB = TRUE;
#endif /* MPS_FIFO_BLOCKING_WRITE */

		if (pDev->voice_mb[i].sem_dev == NULL)
			return (ERROR);

		/* select mechanism implemented for each queue */
		init_waitqueue_head (&pDev->voice_mb[i].mps_wakeuplist);

		memset (&pDev->voice_mb[i].event_mask, 0,
			sizeof (MbxEventRegs_s));

		pDev->voice_mb[i].wakeup_pending =
			kmalloc (sizeof (struct semaphore), GFP_KERNEL);
		sema_init (pDev->voice_mb[i].wakeup_pending, 1);
	}

	/* set channel identifiers */
	pDev->command_mb.devID = command;
	pDev->voice_mb[0].devID = voice0;
	pDev->voice_mb[1].devID = voice1;
	pDev->voice_mb[2].devID = voice2;
	pDev->voice_mb[3].devID = voice3;

	pDev->wakeup_pending =
		kmalloc (sizeof (struct semaphore), GFP_KERNEL);
	sema_init (pDev->wakeup_pending, 1);
	pDev->provide_buffer =
		kmalloc (sizeof (struct semaphore), GFP_KERNEL);
	sema_init (pDev->provide_buffer, 0);

	return 0;
}

/**
 * MPS Structure Reset
 * This function resets the global structures into inital state
 *
 * \param   pDev     Pointer to MPS device structure
 * \return  0        OK, if initialization was successful 
 * \return  -1       ERROR, allocation or semaphore access problem
 * \ingroup Internal
 */
u32
ifx_mps_reset_structures (mps_comm_dev * pDev)
{
	int i;

	for (i = 0; i < NUM_VOICE_CHANNEL; i++) {
		ifx_mps_fifo_clear (&(pDev->voice_mb[i].dwstrm_fifo));
		ifx_mps_fifo_clear (&(pDev->voice_mb[i].upstrm_fifo));
		if (sem_getcount (pDev->voice_mb[i].wakeup_pending) == 0)
			up (pDev->voice_mb[i].wakeup_pending);
#ifdef CONFIG_PROC_FS
		pDev->voice_mb[i].TxnumIRQs = 0;
		pDev->voice_mb[i].RxnumIRQs = 0;
		pDev->voice_mb[i].TxnumBytes = 0;
		pDev->voice_mb[i].RxnumBytes = 0;
		pDev->voice_mb[i].upstrm_fifo.min_space =
			pDev->voice_mb[i].upstrm_fifo.size;
		pDev->voice_mb[i].dwstrm_fifo.min_space =
			pDev->voice_mb[i].dwstrm_fifo.size;
#endif
	}
	ifx_mps_fifo_clear (&(pDev->command_mb.dwstrm_fifo));
	ifx_mps_fifo_clear (&(pDev->command_mb.upstrm_fifo));
#ifdef CONFIG_PROC_FS
	pDev->command_mb.TxnumIRQs = 0;
	pDev->command_mb.RxnumIRQs = 0;
	pDev->command_mb.TxnumBytes = 0;
	pDev->command_mb.RxnumBytes = 0;
	pDev->command_mb.upstrm_fifo.min_space =
		pDev->command_mb.upstrm_fifo.size;
	pDev->command_mb.dwstrm_fifo.min_space =
		pDev->command_mb.dwstrm_fifo.size;
#endif

#if CONFIG_DANUBE_MPS_HISTORY > 0
	ifx_mps_history_buffer_freeze = 0;
	ifx_mps_history_buffer_words = 0;
#endif

	if (sem_getcount (pDev->wakeup_pending) == 0)
		up (pDev->wakeup_pending);
	down_trylock (pDev->provide_buffer);

	return OK;
}

/******************************************************************************
 * Mailbox Managment 
 ******************************************************************************/

/**
 * Gets channel ID field from message header
 * This function reads the data word at the read pointer position 
 * of the mailbox FIFO pointed to by mbx and extracts the channel ID field 
 * from the data word read.
 *
 * \param   mbx      Pointer to mailbox structure to be accessed
 * \return  ID       Voice channel identifier.
 * \ingroup Internal 
 */
mps_devices
ifx_mps_mbx_get_message_channel (mps_fifo * mbx)
{
	MbxMsgHd_u msg_hd;
	mps_devices retval = unknown;

	msg_hd.val = ifx_mps_fifo_read (mbx, 0);
	switch (msg_hd.hd.chan) {
	case 0:
		retval = voice0;
		break;
	case 1:
		retval = voice1;
		break;
	case 2:
		retval = voice2;
		break;
	case 3:
		retval = voice3;
		break;
	default:
		retval = unknown;
		printk ("%s(): unknown channel ID\n", __FUNCTION__);
		break;
	}

	return retval;
}

/**
 * Get message length
 * This function returns the length in bytes of the message located at read pointer 
 * position. It reads the plength field of the message header (length in 16 bit 
 * halfwords) adds the header length and returns the complete length in bytes. 
 *
 * \param   mbx      Pointer to mailbox structure to be accessed
 * \return  length   Length of message in bytes.
 * \ingroup Internal 
 */
s32
ifx_mps_mbx_get_message_length (mps_fifo * mbx)
{
	MbxMsgHd_u msg_hd;

	msg_hd.val = ifx_mps_fifo_read (mbx, 0);
	/* return payload + header length in bytes */
	return ((((s32) msg_hd.hd.plength + 2) << 1));
}

/**
 * Read message from upstream data mailbox
 * This function reads a complete data message from the upstream data mailbox.
 * It reads the header checks how many payload words are included in the message
 * and reads the payload afterwards. The mailbox's read pointer is updated afterwards
 * by the amount of words read.
 *
 * \param   fifo        Pointer to mailbox structure to be read from
 * \param   msg         Pointer to message structure read from buffer
 * \param   bytes       Pointer to number of bytes included in read message
 * \return  0           OK, successful read operation,
 * \return  -1          Invalid length field read.
 * \ingroup Internal 
 */
s32
ifx_mps_mbx_read_message (mps_fifo * fifo, MbxMsg_s * msg, u32 * bytes)
{
	s32 i;
	u32 flags;

	memset ((void *) msg, 0, sizeof (MbxMsg_s));

	save_and_cli (flags);

	/* read message header from buffer */
	msg->header.val = ifx_mps_fifo_read (fifo, 0);
	if ((msg->header.hd.plength % 4) != 0) {	/* check payload length */
		printk ("%s(): Odd payload length %d\n", __FUNCTION__,
			msg->header.hd.plength);
		return -1;
	}

	for (i = 0; i < msg->header.hd.plength; i += 4) {	/* read message payload */
		msg->data[i / 4] = ifx_mps_fifo_read (fifo, (u8) (i + 4));
	}
	*bytes = msg->header.hd.plength + 4;
	ifx_mps_fifo_read_ptr_inc (fifo, (msg->header.hd.plength + 4));

	restore_flags (flags);

	return 0;
}

/**
 * Read message from FIFO
 * This function reads a message from the upstream data mailbox and passes it 
 * to the calling function. A call to the notify_upstream function will trigger
 * another wakeup in case there is already more data available.
 *
 * \param   pMBDev   Pointer to mailbox device structure
 * \param   pPkg     Pointer to data transfer structure (output parameter)
 * \param   timeout  Currently unused
 * \return  0        OK, successful read operation,
 * \return  -1       ERROR, in case of read error.
 * \ingroup Internal
 */
s32
ifx_mps_mbx_read (mps_mbx_dev * pMBDev, mps_message * pPkg, s32 timeout)
{
	static MbxMsg_s msg;
	u32 bytes = 0;
	mps_fifo *fifo;
	int retval = ERROR;

	fifo = &pMBDev->upstrm_fifo;
	memset (&msg, 0, sizeof (msg));	/* initialize msg pointer */

	msg.header.val = ifx_mps_fifo_read (&pMBDev->upstrm_fifo, 0);
	if (msg.header.hd.type == CMD_ADDRESS_PACKET) {
		/* Nothing available for this channel... */
		return ERROR;
	}

	/* read message from mailbox */
	if (ifx_mps_mbx_read_message (fifo, &msg, &bytes) == 0) {
		switch (pMBDev->devID) {
		case command:
			/*
			 * command messages are completely passed to the caller.
			 * The mps_message structure comprises a pointer to the
			 * message start and the message size in bytes 
			 */
			pPkg->pData = mps_buffer.malloc (bytes, GFP_ATOMIC);
			memcpy ((u8 *) pPkg->pData, (u8 *) & msg, bytes);
			pPkg->cmd_type = msg.header.hd.type;
			pPkg->nDataBytes = bytes;
			pPkg->RTP_PaylOffset = 0;
			retval = OK;

#ifdef CONFIG_PROC_FS
			pMBDev->RxnumBytes += bytes;
#endif

			/* do another wakeup in case there is more data available... */
			ifx_mps_mbx_cmd_upstream (0);
			break;

		case voice0:
		case voice1:
		case voice2:
		case voice3:
			/*
			 * data messages are passed as mps_message pointer that comprises
			 * a pointer to the payload start address and the payload size in
			 * bytes.
			 * The message header is removed and the payload pointer, payload size,
			 * payload type and and RTP payload offset are passed to CPU0.
			 */
			pPkg->cmd_type = msg.header.hd.type;
			pPkg->pData = (u32 *) KSEG0ADDR ((u8 *) msg.data[0]);	/* get payload pointer */
			pPkg->nDataBytes = msg.data[1];	/* get payload size */

#ifndef CONFIG_MIPS_UNCACHED
			dma_cache_inv (KSEG0ADDR (pPkg->pData),
				       pPkg->nDataBytes);
#endif
			/* set RTP payload offset for RTP messages
			   to be clarified how this should look like exactly 
			 */
			pPkg->RTP_PaylOffset = 0;
			retval = OK;

#ifdef CONFIG_PROC_FS
			pMBDev->RxnumBytes += bytes;
#endif

			if (down_trylock (pMPSDev->provide_buffer) == 0) {
				if (ifx_mps_bufman_buf_provide
				    (MPS_DEFAULT_PROVISION_SEGMENTS_PER_MSG,
				     mps_buffer.buf_size) != OK) {
					printk ("%s(): Warning - provide buffer failed...\n", __FUNCTION__);
					up (pMPSDev->provide_buffer);
				}
			}

			if (sem_getcount (pMBDev->wakeup_pending) == 0) {
				up (pMBDev->wakeup_pending);
				up (pMPSDev->wakeup_pending);
			}

			/* Don't do this when using callback, since user might call read from
			   callback, causing recursion */
			if (pMBDev->up_callback == NULL) {
				/* do another wakeup in case there is more data available... */
				ifx_mps_mbx_data_upstream (0);
			}

			break;

		default:
			break;
		}
	}
	return retval;
}

/**
 * Build 32 bit word starting at byte_ptr.
 * This function builds a 32 bit word out of 4 consecutive bytes
 * starting at byte_ptr position.
 *
 * \param   byte_ptr  Pointer to first byte (most significat 8 bits) of word calculation
 * \return  value     Returns value of word starting at byte_ptr position
 * \ingroup Internal
 */
u32
ifx_mps_mbx_build_word (u8 * byte_ptr)
{
	u32 result = 0x00000000;
	s32 i;

	for (i = 0; i < 4; i++) {
		result += *(byte_ptr + i) << ((3 - i) * 8);
	}
	return (result);
}

/**
 * Write to Downstream Mailbox of MPS.
 * This function writes messages into the downstream mailbox to be read
 * by CPU1
 *
 * \param   pMBDev    Pointer to mailbox device structure
 * \param   msg_ptr   Pointer to message
 * \param   msg_words Number of 16bit words in message
 * \return  0         Returns OK in case of successful write operation
 * \return  -1        ERROR, in case of access fails with FIFO overflow
 * \ingroup Internal
 */
s32
ifx_mps_mbx_write_message (mps_mbx_dev * pMBDev, u8 * msg_ptr, u32 msg_bytes)
{
	mps_fifo *mbx;
	u32 i;
	u32 flags;
	int retval = -EAGAIN;
	s32 retries = 0;
	u32 word = 0;
	bool_t word_aligned = TRUE;

	save_and_cli (flags);

	mbx = &(pMBDev->dwstrm_fifo);	/* set pointer to downstream command mailbox
					   FIFO structure */

	if ((u32) msg_ptr & 0x00000003) {
		word_aligned = FALSE;
		printk ("%s(): Passed message not word aligned !!!\n",
			__FUNCTION__);
	}

	/*
	 * request for downstream mailbox buffer memory,  make MAX_FIFO_WRITE_RETRIES
	 * retries in case of memory is not available
	 */
	while (++retries < MAX_FIFO_WRITE_RETRIES) {
		if (ifx_mps_fifo_mem_request (mbx, msg_bytes) == TRUE)
			break;
		if (in_interrupt ()) {
			retries = MAX_FIFO_WRITE_RETRIES;
			break;
		}
		else {
			restore_flags (flags);
			udelay (125);
			save_and_cli (flags);
		}
	}

	if (retries < MAX_FIFO_WRITE_RETRIES) {
		/*
		 * write message words to mailbox buffer starting at write pointer position and
		 * update the write pointer index by the amount of written data afterwards
		 */
		for (i = 0; i < msg_bytes; i += 4) {
			if (word_aligned)
				ifx_mps_fifo_write (mbx,
						    *(u32 *) (msg_ptr + i),
						    i);
			else {
				word = ifx_mps_mbx_build_word (msg_ptr + i);
				ifx_mps_fifo_write (mbx, word, i);
			}
		}
		ifx_mps_fifo_write_ptr_inc (mbx, msg_bytes);
		retval = OK;
#ifdef CONFIG_PROC_FS
		/* update mailbox statistics */
		pMBDev->TxnumBytes += msg_bytes;
		if (mbx->min_space < ifx_mps_fifo_mem_available (mbx))
			mbx->min_space = ifx_mps_fifo_mem_available (mbx);
#endif /* CONFIG_PROC_FS */
	}
	else {
		/*
		 * insufficient mailbox buffer memory
		 * -> return error, or can we do anything else ?
		 */
		printk ("%s(): write message timeout\n", __FUNCTION__);
	}
	restore_flags (flags);
	return retval;
}

/**
 * Write to Downstream Data Mailbox of MPS.
 * This function writes the passed message into the downstream data mailbox.
 *
 * \param   pMBDev     Pointer to mailbox device structure
 * \param   readWrite  Pointer to message structure
 * \return  0          OK in case of successful write operation
 * \return  -1         ERROR in case of access fails with FIFO overflow
 * \ingroup Internal 
 */
s32
ifx_mps_mbx_write_data (mps_mbx_dev * pMBDev, mps_message * readWrite)
{
	int retval = ERROR;
	static MbxMsg_s msg;

	if ((pMBDev->devID == voice0) || (pMBDev->devID == voice1) ||
	    (pMBDev->devID == voice2) || (pMBDev->devID == voice3)) {
		if (down_trylock (pMPSDev->provide_buffer) == 0) {
			if (ifx_mps_bufman_buf_provide
			    (MPS_DEFAULT_PROVISION_SEGMENTS_PER_MSG,
			     mps_buffer.buf_size) != OK) {
				printk ("%s(): Warning - provide buffer failed...\n", __FUNCTION__);
				up (pMPSDev->provide_buffer);
			}
		}

		memset (&msg, 0, sizeof (msg));	/* initialize msg structure */
		/* build data message from passed payload data structure */
		msg.header.hd.plength = 0x8;

		switch (pMBDev->devID) {
		case voice0:
			msg.header.hd.chan = 0;
			break;
		case voice1:
			msg.header.hd.chan = 1;
			break;
		case voice2:
			msg.header.hd.chan = 2;
			break;
		case voice3:
			msg.header.hd.chan = 3;
			break;
		default:
			return retval;
		}

		msg.header.hd.type = readWrite->cmd_type;

		msg.data[0] = PHYSADDR ((u32) readWrite->pData);
		msg.data[1] = readWrite->nDataBytes;

#ifndef CONFIG_MIPS_UNCACHED
		dma_cache_wback (readWrite->pData, readWrite->nDataBytes);
#endif
		if ((retval =
		     ifx_mps_mbx_write_message (pMBDev, (u8 *) & msg,
						12)) != OK) {
			printk ("%s(): Writing data failed ! *\n",
				__FUNCTION__);
		}
	}
	else {
		printk ("%s(): Invalid device ID %d !\n", __FUNCTION__,
			pMBDev->devID);
	}

	return retval;
}

/**
 * Write buffer provisioning message to mailbox.
 * This function writes a buffer provisioning message to the downstream data 
 * mailbox that provides the specified amount of memory segments .
 *
 * \param   mem_ptr      Pointer to segment pointer array
 * \param   segments     Number of valid segment pointers in array
 * \param   segment_size Size of segements in array
 * \return  0            OK in case of successful write operation
 * \return  -1           ERROR in case of access fails with FIFO overflow
 * \ingroup Internal
 */
s32
ifx_mps_mbx_write_buffer_prov_message (mem_seg_t * mem_ptr, u8 segments,
				       u32 segment_size)
{
	int retval = ERROR;
	int i;
	static MbxMsg_s msg;

	memset (&msg, 0, sizeof (msg));	/* initialize msg structure */
	/* build data message from passed payload data structure */
	msg.header.hd.plength = (segments * 4) + 4;
	msg.header.hd.type = CMD_ADDRESS_PACKET;

	for (i = 0; i < segments; i++) {
		msg.data[i] = *((u32 *) mem_ptr + i);
	}
	msg.data[segments] = segment_size;

	/*
	 * send buffer provision message and update buffer management
	 */
	retval = ifx_mps_mbx_write_message ((&pMPSDev->voice_mb[0]),
					    (u8 *) & msg,
					    (u32) (segments + 2) * 4);
	if (retval == OK) {
		ifx_mps_bufman_inc_level (segments);
	}
	return retval;
}

/**
 * Write to downstream command mailbox.
 * This is the function to write commands into the downstream command mailbox
 * to be read by CPU1 
 *
 * \param   pMBDev     Pointer to mailbox device structure
 * \param   readWrite  Pointer to transmission data container
 * \return  0          OK in case of successful write operation
 * \return  -1         ERROR in case of access fails with FIFO overflow
 * \ingroup Internal
 */
s32
ifx_mps_mbx_write_cmd (mps_mbx_dev * pMBDev, mps_message * readWrite)
{
	int retval = ERROR;

	if (pMBDev->devID == command) {
		if ((readWrite->nDataBytes) % 4) {
			printk ("%s(): invalid number of bytes %d\n",
				__FUNCTION__, readWrite->nDataBytes);
		}
		if ((u32) (readWrite->pData) & 0x00000003)
			printk ("%s(): non word aligned data passed to mailbox\n", __FUNCTION__);
		if (readWrite->nDataBytes > (MBX_CMD_FIFO_SIZE - 4))
			printk ("%s(): command size too large!\n",
				__FUNCTION__);

#if CONFIG_DANUBE_MPS_HISTORY > 0
		if (!ifx_mps_history_buffer_freeze) {
			int i, pos;
			for (i = 0; i < (readWrite->nDataBytes / 4); i++) {
				pos = ifx_mps_history_buffer_words %
					MPS_HISTORY_BUFFER_SIZE;
				ifx_mps_history_buffer[pos] =
					((u32 *) readWrite->pData)[i];
				ifx_mps_history_buffer_words++;
			}
		}
#endif
		retval = ifx_mps_mbx_write_message (pMBDev,
						    (u8 *) readWrite->pData,
						    readWrite->nDataBytes);
	}
	else {
		/* invalid device id read from mailbox FIFO structure */
		printk ("%s(): Invalid device ID %d !\n", __FUNCTION__,
			pMBDev->devID);
	}
	return retval;
}

/**
 * Notify queue about upstream data reception 
 * This function checks the channel identifier included in the header 
 * of the message currently pointed to by the upstream data mailbox's 
 * read pointer. It wakes up the related queue to read the received data message 
 * out of the mailbox for further processing. The process is repeated 
 * as long as upstream messages are avaiilable in the mailbox. 
 * The function is attached to the driver's poll/select functionality.  
 *
 * \param   dummy    Tasklet parameter, not used.
 * \ingroup Internal
 */
static void
ifx_mps_mbx_data_upstream (unsigned long dummy)
{
	mps_devices channel;
	mps_fifo *mbx;
	mps_mbx_dev *mbx_dev;
	MbxMsg_s msg;
	u32 bytes_read = 0;
	u32 flags;

	/* set pointer to data upstream mailbox, no matter if 0,1,2 or 3
	 * because they point to the same shared  mailbox memory */
	mbx = (mps_fifo *) & (pMPSDev->voice_mb[0].upstrm_fifo);

	while (ifx_mps_fifo_not_empty (mbx)) {
		save_and_cli (flags);
		channel = ifx_mps_mbx_get_message_channel (mbx);
		/* select mailbox device structure acc. to channel ID read from current msg */
		switch (channel) {
		case voice0:
			mbx_dev = (mps_mbx_dev *) & (pMPSDev->voice_mb[0]);
			break;

		case voice1:
			mbx_dev = (mps_mbx_dev *) & (pMPSDev->voice_mb[1]);
			break;

		case voice2:
			mbx_dev = (mps_mbx_dev *) & (pMPSDev->voice_mb[2]);
			break;

		case voice3:
			mbx_dev = (mps_mbx_dev *) & (pMPSDev->voice_mb[3]);
			break;

		default:
			printk ("%s(): Invalid channel ID %d read from mailbox\n", __FUNCTION__, channel);
			restore_flags (flags);
			return;
		}

#ifdef CONFIG_PROC_FS
		if (mbx->min_space < ifx_mps_fifo_mem_available (mbx))
			mbx->min_space = ifx_mps_fifo_mem_available (mbx);
#endif

		/* read message header from buffer */
		msg.header.val =
			ifx_mps_fifo_read (&pMPSDev->voice_mb[0].upstrm_fifo,
					   0);
		if (msg.header.hd.type == CMD_ADDRESS_PACKET) {
			int i;

			ifx_mps_mbx_read_message (&pMPSDev->voice_mb[0].
						  upstrm_fifo, &msg,
						  &bytes_read);
			for (i = 0; i < (msg.header.hd.plength / 4 - 1); i++) {
				mps_buffer.free ((void *)
						 KSEG0ADDR (msg.data[i]));
			}
			restore_flags (flags);
			continue;
		}
		else {
#ifdef CONFIG_PROC_FS
			/* update mailbox statistics */
			mbx_dev->RxnumIRQs++;	/* increase the counter of rx interrupts */

#endif /* CONFIG_PROC_FS */
			if (mbx_dev->up_callback != NULL) {
				ifx_mps_bufman_dec_level (1);

				if ((ifx_mps_bufman_get_level () <=
				     mps_buffer.buf_threshold)
				    && (sem_getcount (pMPSDev->provide_buffer)
					== 0)) {
					up (pMPSDev->provide_buffer);
				}

				/* use callback function to notify about data reception */
				mbx_dev->up_callback (channel);
				restore_flags (flags);
				continue;
			}
			else {
				/* Don't do a wakeup if a notification is waiting on read... */
				if (down_trylock (pMPSDev->wakeup_pending) ==
				    0) {
					ifx_mps_bufman_dec_level (1);

					if ((ifx_mps_bufman_get_level () <=
					     mps_buffer.buf_threshold)
					    &&
					    (sem_getcount
					     (pMPSDev->provide_buffer) ==
					     0)) {
						up (pMPSDev->provide_buffer);
					}
					/* use queue wake up to notify about data reception */
					down (mbx_dev->wakeup_pending);
					wake_up_interruptible (&
							       (mbx_dev->
								mps_wakeuplist));
				}
				restore_flags (flags);
				break;
			}
		}
	}
	return;
}

/**
 * Notify queue about upstream command reception 
 * This function checks the channel identifier included in the header 
 * of the message currently pointed to by the upstream command mailbox's
 * read pointer. It wakes up the related queue to read the received command
 * message out of the mailbox for further processing. The process is repeated 
 * as long as upstream messages are avaiilable in the mailbox. 
 * The function is attached to the driver's poll/select functionality.  
 *
 * \param   dummy    Tasklet parameter, not used.
 * \ingroup Internal
 */
static void
ifx_mps_mbx_cmd_upstream (unsigned long dummy)
{
	mps_fifo *mbx;

	/* set pointer to upstream command mailbox */
	mbx = (mps_fifo *) & (pMPSDev->command_mb.upstrm_fifo);
	if (ifx_mps_fifo_not_empty (mbx)) {
#ifdef CONFIG_PROC_FS
		if (mbx->min_space < ifx_mps_fifo_mem_available (mbx))
			mbx->min_space = ifx_mps_fifo_mem_available (mbx);
#endif
		if (pMPSDev->command_mb.up_callback != NULL) {
			pMPSDev->command_mb.up_callback (command);
		}
		else {
			/* wake up sleeping process for further processing of received command */
			wake_up_interruptible (&
					       (pMPSDev->command_mb.
						mps_wakeuplist));
		}
	}
	return;
}

/******************************************************************************
 * Interrupt service routines
 ******************************************************************************/

/**
 * Upstream data interrupt handler
 * This function is called on occurence of an data upstream interrupt.
 * Depending on the occured interrupt either the command or data upstream
 * message processing is started via tasklet
 *
 * \param   irq      Interrupt number
 * \param   pDev     Pointer to MPS communication device structure
 * \param   regs     Pointer to system registers
 * \ingroup Internal
 */
void
ifx_mps_ad0_irq (int irq, mps_comm_dev * pDev, struct pt_regs *regs)
{
	MPS_Ad0Reg_u MPS_Ad0StatusReg;
	mps_mbx_dev *mbx_dev = (mps_mbx_dev *) & (pMPSDev->command_mb);;

	MPS_Ad0StatusReg.val = *DANUBE_MPS_RAD0SR;	/* read interrupt status */
	*DANUBE_MPS_CAD0SR = MPS_Ad0StatusReg.val;	/* and acknowledge */
	mask_and_ack_danube_irq (irq);

	if (ifx_mps_initialising == 1)
		return;

	if (MPS_Ad0StatusReg.fld.du_mbx) {
		ifx_mps_mbx_data_upstream (0);
	}
	if (MPS_Ad0StatusReg.fld.cu_mbx) {
#ifdef CONFIG_PROC_FS
		/* increase counter of read messages and bytes */
		pMPSDev->command_mb.RxnumIRQs++;
#endif /* CONFIG_PROC_FS */

		ifx_mps_mbx_cmd_upstream (0);
	}
#if CONFIG_DANUBE_MPS_HISTORY > 0
	if (MPS_Ad0StatusReg.fld.cmd_err) {
		ifx_mps_history_buffer_freeze = 1;
		printk ("MPS cmd_err interrupt!\n");
	}
#endif
	pMPSDev->event.MPS_Ad0Reg.val =
		MPS_Ad0StatusReg.val & mbx_dev->event_mask.MPS_Ad0Reg.val;
	if (MPS_Ad0StatusReg.val) {
		/* use callback function or queue wake up to notify about data reception */
		if (mbx_dev->event_callback != NULL) {
			mbx_dev->event_callback (&pMPSDev->event);
			pMPSDev->event.MPS_Ad0Reg.val = 0;
		}
		else {
			wake_up_interruptible (&(mbx_dev->mps_wakeuplist));
		}
	}
	return;
}

/**
 * Upstream data interrupt handler
 * This function is called on occurence of an data upstream interrupt.
 * Depending on the occured interrupt either the command or data upstream
 * message processing is started via tasklet
 *
 * \param   irq      Interrupt number
 * \param   pDev     Pointer to MPS communication device structure
 * \param   regs     Pointer to system registers
 * \ingroup Internal
 */
void
ifx_mps_ad1_irq (int irq, mps_comm_dev * pDev, struct pt_regs *regs)
{
	MPS_Ad1Reg_u MPS_Ad1StatusReg;
	mps_mbx_dev *mbx_dev = (mps_mbx_dev *) & (pMPSDev->command_mb);;

	MPS_Ad1StatusReg.val = *DANUBE_MPS_RAD1SR;	/* read interrupt status */
	*DANUBE_MPS_CAD1SR = MPS_Ad1StatusReg.val;	/* and acknowledge */
	mask_and_ack_danube_irq (irq);

	pMPSDev->event.MPS_Ad1Reg.val =
		MPS_Ad1StatusReg.val & mbx_dev->event_mask.MPS_Ad1Reg.val;
	if (MPS_Ad1StatusReg.val) {
		/* use callback function or queue wake up to notify about data reception */
		if (mbx_dev->event_callback != NULL) {
			mbx_dev->event_callback (&pMPSDev->event);
			pMPSDev->event.MPS_Ad1Reg.val = 0;
		}
		else {
			wake_up_interruptible (&(mbx_dev->mps_wakeuplist));
		}
	}
	return;
}

/**
 * Voice channel status interrupt handler
 * This function is called on occurence of an status interrupt.
 *
 * \param   irq      Interrupt number
 * \param   pDev     Pointer to MPS communication device structure
 * \param   regs     Pointer to system registers
 * \ingroup Internal
 */
void
ifx_mps_vc_irq (int irq, mps_comm_dev * pDev, struct pt_regs *regs)
{
	u32 chan = irq - /*INT_NUM_IM5_IRL7 */ INT_NUM_IM4_IRL14;
	u32 VCnSR = DANUBE_MPS_RVC0SR[chan];
	mps_mbx_dev *mbx_dev = (mps_mbx_dev *) & (pMPSDev->voice_mb[chan]);
	MPS_VCStatReg_u MPS_VCStatusReg;

	MPS_VCStatusReg.val = VCnSR;

	/* acknowledge interrupt */
	DANUBE_MPS_CVC0SR[chan] = MPS_VCStatusReg.val;
	mask_and_ack_danube_irq (irq);

	pMPSDev->event.MPS_VCStatReg[chan].val =
		MPS_VCStatusReg.val & mbx_dev->event_mask.MPS_VCStatReg[chan].
		val;
	if (MPS_VCStatusReg.val) {
		/* use callback function or queue wake up to notify about data reception */
		if (mbx_dev->event_callback != NULL) {
			mbx_dev->event_callback (&pMPSDev->event);
			pMPSDev->event.MPS_VCStatReg[chan].val = 0;

		}
		else {
			wake_up_interruptible (&(mbx_dev->mps_wakeuplist));
		}
	}
	return;
}

/**
 * Print firmware version.
 * This function queries the current firmware version and prints it.
 *
 * \return  0        OK
 * \return  -EFAULT  Error while fetching version
 * \ingroup Internal
 */
int
ifx_mps_print_fw_version (void)
{
	MbxMsg_s msg;
	MbxMsg_s msg2;
	u32 *ptmp;
	mps_fifo *fifo;
	int timeout = 300;	/* 3s timeout */
	int retval;
	u32 bytes_read = 0;

	fifo = &(ifx_mps_dev.command_mb.upstrm_fifo);
	/* build message */
	ptmp = (u32 *) & msg;
	ptmp[0] = 0x8600e604;
	ptmp[1] = 0x00000000;

	ifx_mps_initialising = 1;

	/* send message */
	retval = ifx_mps_mbx_write_message (&(ifx_mps_dev.command_mb),
					    (u8 *) & msg, 4);

	while (!ifx_mps_fifo_not_empty (fifo) && timeout > 0) {
		timeout--;
		/* Sleep for 10ms */
		current->state = TASK_INTERRUPTIBLE;
		schedule_timeout (1);
	}
	if (timeout == 0) {
		printk ("timeout\n");
		return -EFAULT;
	}

	retval = ifx_mps_mbx_read_message (fifo, &msg2, &bytes_read);
	if ((retval != 0) || (bytes_read != 8)) {
		printk ("error\n");
		return -EFAULT;
	}

	ptmp = (u32 *) & msg2;
	//printk("ok!\nVersion %d.%d.%d up and running...\n",(ptmp[1]>>24)&0xff,(ptmp[1]>>16)&0xff,(ptmp[1]>>12)&0xf);

	ifx_mps_initialising = 0;
	return 0;
}
