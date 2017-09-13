/******************************************************************************
**
** FILE NAME    : danube_mei_bsp.c
** PROJECT      : Danube
** MODULES     	: MEI
**
** DATE         : 1 Jan 2006
** AUTHOR       : TC Chen
** DESCRIPTION  : MEI Driver
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
** $Version $Date      $Author     $Comment
   1.00.01  	       TC Chen	   Fixed cell rate calculation issue
   				   Fixed pvovider_id of adsl mib swapping issue
   1.00.02 	       TC Chen	   Added L3 Low Poewr Mode support.
   1.00.03	       TC Chen     Fixed Clear Eoc transmit issue.
   1.00.04  31/08/2006 TC Chen     Add ADSL Link/Data Led
                                   Add Dual Latency Path
                                   Add AUTOBOOT_ENABLE_SET ioctl for autoboot 
                                       mode enable/disable
                                   Fix fast path cell rate calculation
   2.00.00  02/10/2006 TC Chen     First version for the separation of DSL and 
                                       MEI
   2.00.01  12/10/2006 TC Chen     Fix some compiling warnings
   2.00.02  23/10/2006 TC Chen     Fix /proc/device listing issue
   2.00.03  20/12/2006 TC Chen     Fix /proc/device listing issue

*******************************************************************************/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <asm/semaphore.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <asm/uaccess.h>
#include <asm/danube/danube.h>
#include <asm/danube/irq.h>
#include <asm/danube/port.h>
#include <asm/danube/danube_gpio.h>
#include <asm/danube/danube_led.h>
#include <ifx/dsl/ifx_adsl_linux.h>
#include <asm/danube/danube_mei_bsp.h>

#define DANUBE_MEI_EMSG(fmt, args...) printk( KERN_ERR  "%s: " fmt,__FUNCTION__, ## args)

//TODO
#undef DFE_LOOPBACK		// testing code //undefined by Henry , start to real link test.
		    //165203:henryhsu

#ifdef DFE_LOOPBACK
//#define DFE_MEM_TEST
//#define DFE_PING_TEST
#define DFE_ATM_LOOPBACK
#endif

#ifdef DFE_LOOPBACK
#ifndef UINT32
#define UINT32 unsigned long
#endif
#ifdef DFE_PING_TEST
#include "dsp_xmem_arb_rand_em.h"
#endif
#ifdef DFE_MEM_TEST
#include "aai_mem_test.h"
#endif
#ifdef DFE_ATM_LOOPBACK
#include "aai_lpbk_dyn_rate.h"
#endif
#endif


ifx_adsl_bsp_version_t danube_mei_version = 
    { major:   2,
      minor:   0,
      revision:3
    };
#define DANUBE_MEI_DEVNAME "ifx_mei"
#define DANUBE_MAX_DEVICES 1

MEI_ERROR IFX_ADSL_BSP_FWDownload (struct ifx_adsl_device *dev,const char *buf,
				   unsigned long size, long *loff,
				   long *current_off);
MEI_ERROR IFX_ADSL_BSP_Showtime (struct ifx_adsl_device *dev, u32 rate_fast,
				 u32 rate_intl);
MEI_ERROR IFX_ADSL_BSP_AdslLedInit (struct ifx_adsl_device *dev,
				    ifx_adsl_led_id_t led_number,
				    ifx_adsl_led_type_t type,
				    ifx_adsl_led_handler_t handler);
MEI_ERROR IFX_ADSL_BSP_AdslLedSet (struct ifx_adsl_device *dev,
				   ifx_adsl_led_id_t led_number,
				   ifx_adsl_led_mode_t mode);
MEI_ERROR IFX_ADSL_BSP_MemoryDebugAccess (struct ifx_adsl_device *dev,
					  ifx_adsl_memory_access_type_t type,
					  u32 srcaddr, u32 * databuff,
					  u32 databuffsize);

int IFX_ADSL_BSP_KernelIoctls (ifx_adsl_device_t * pDev, unsigned int command,
			       unsigned long lon);

static MEI_ERROR DANUBE_MEI_RunAdslModem (struct ifx_adsl_device *dev);
MEI_ERROR IFX_ADSL_BSP_SendCMV (struct ifx_adsl_device *dev, u16 * request,
				int reply, u16 * response);
static MEI_ERROR DANUBE_MEI_CpuModeSet (struct ifx_adsl_device *dev,
					ifx_adsl_cpu_mode_t mode);
static MEI_ERROR DANUBE_MEI_DownloadBootCode (struct ifx_adsl_device *dev);
static MEI_ERROR DANUBE_MEI_ArcJtagEnable (struct ifx_adsl_device *dev,
					   int enable);
static MEI_ERROR DANUBE_MEI_AdslMailboxIRQEnable (struct ifx_adsl_device *dev,
						  int enable);

static int DANUBE_MEI_GetPage (ifx_adsl_device_t * pDev, u32 Page, u32 data,
			       u32 MaxSize, u32 * Buffer, u32 * Dest);
static int DANUBE_MEI_BarUpdate (ifx_adsl_device_t * pDev, int nTotalBar);
static int
  IFX_ADSL_BSP_GetEventCB (int (**ifx_adsl_callback)
			     (ifx_adsl_cb_event_t * param));

#if defined(__LINUX__)
static ssize_t DANUBE_MEI_Write (MEI_file_t * filp,const char *buf, size_t size,
				 loff_t * loff);
static int DANUBE_MEI_UserIoctls (MEI_inode_t * ino, MEI_file_t * fil,
				  unsigned int command, unsigned long lon);
static int DANUBE_MEI_Open (MEI_inode_t * ino, MEI_file_t * fil);
static int DANUBE_MEI_Release (MEI_inode_t * ino, MEI_file_t * fil);
#ifdef CONFIG_PROC_FS
static int DANUBE_MEI_ProcRead (struct file *file, char *buf, size_t nbytes,
				loff_t * ppos);
static ssize_t DANUBE_MEI_ProcWrite (struct file *file, const char *buffer,
				     size_t count, loff_t * ppos);
#endif
#endif //__LINUX__

static u32 *mei_arc_swap_buff = NULL;	//  holding swap pages

static int (*IFX_ADSL_EventCB) (ifx_adsl_cb_event_t * param);	// pointer to event callback function

#if defined(__LINUX__)
#define MEI_MASK_AND_ACK_IRQ mask_and_ack_danube_irq

static int major=105;
#ifdef CONFIG_PROC_FS
#define PROC_ITEMS 8
#define MEI_DIRNAME "danube_mei"
#endif
#undef CONFIG_DEVFS_FS
#ifdef CONFIG_DEVFS_FS
static devfs_handle_t danube_mei_devfs_handle[DANUBE_MAX_DEVICES];
#endif
static struct file_operations danube_mei_operations = {
      owner:THIS_MODULE,
      open:DANUBE_MEI_Open,
      release:DANUBE_MEI_Release,
      write:DANUBE_MEI_Write,
      ioctl:DANUBE_MEI_UserIoctls,
};

#ifdef CONFIG_PROC_FS
static struct proc_dir_entry *meidir;
static struct file_operations DANUBE_MEI_ProcOperations = {
      read:DANUBE_MEI_ProcRead,
      write:DANUBE_MEI_ProcWrite,
};
static reg_entry_t regs[DANUBE_MAX_DEVICES][PROC_ITEMS];	//total items to be monitored by /proc/mei
#define NUM_OF_REG_ENTRY	(sizeof(regs[0])/sizeof(reg_entry_t))
#endif //#ifdef CONFIG_PROC_FS
#endif //__LINUX__

static ifx_adsl_device_t sMEI_devices[DANUBE_MAX_DEVICES];

static danube_mei_device_private_t sDanube_Mei_Private[DANUBE_MAX_DEVICES];

/////////////////               mei access Rd/Wr methods       ///////////////
/**
 * Write a value to register
 * This function writes a value to danube register
 *
 * \param  	ul_address	The address to write
 * \param  	ul_data		The value to write
 * \ingroup	Internal
 */
static void
DANUBE_MEI_LongWordWrite (u32 ul_address, u32 ul_data)
{
	DANUBE_WRITE_REGISTER_L (ul_data, ul_address);
	asm ("SYNC");
	return;
}				//    end of "DANUBE_MEI_LongWordWrite(..."

/**
 * Write a value to register
 * This function writes a value to danube register
 *
 * \param 	pDev		the device pointer
 * \param  	ul_address	The address to write
 * \param  	ul_data		The value to write
 * \ingroup	Internal
 */
static void
DANUBE_MEI_LongWordWriteOffset (ifx_adsl_device_t * pDev, u32 ul_address,
				u32 ul_data)
{
	DANUBE_WRITE_REGISTER_L (ul_data, pDev->base_address + ul_address);
	asm ("SYNC");
	return;
}				//    end of "DANUBE_MEI_LongWordWrite(..."

/**
 * Read the danube register
 * This function read the value from danube register
 *
 * \param  	ul_address	The address to write
 * \param  	pul_data	Pointer to the data
 * \ingroup	Internal
 */
static void
DANUBE_MEI_LongWordRead (u32 ul_address, u32 * pul_data)
{
	//*pul_data = *((volatile u32 *)ul_address);
	*pul_data = DANUBE_READ_REGISTER_L (ul_address);
	asm ("SYNC");
	return;
}				//    end of "DANUBE_MEI_LongWordRead(..."

/**
 * Read the danube register
 * This function read the value from danube register
 *
 * \param 	pDev		the device pointer
 * \param  	ul_address	The address to write
 * \param  	pul_data	Pointer to the data
 * \ingroup	Internal
 */
static void
DANUBE_MEI_LongWordReadOffset (ifx_adsl_device_t * pDev, u32 ul_address,
			       u32 * pul_data)
{
	//*pul_data = *((volatile u32 *)ul_address);
	*pul_data = DANUBE_READ_REGISTER_L (pDev->base_address + ul_address);
	asm ("SYNC");
	return;
}				//    end of "DANUBE_MEI_LongWordRead(..."

/**
 * Write several DWORD datas to ARC memory via ARC DMA interface
 * This function writes several DWORD datas to ARC memory via DMA interface.
 *
 * \param 	pDev		the device pointer
 * \param  	destaddr	The address to write
 * \param  	databuff	Pointer to the data buffer
 * \param  	databuffsize	Number of DWORDs to write
 * \return	MEI_SUCCESS or MEI_FAILURE
 * \ingroup	Internal
 */
static MEI_ERROR
DANUBE_MEI_DMAWrite (ifx_adsl_device_t * pDev, u32 destaddr, u32 * databuff,
		     u32 databuffsize)
{
	u32 *p = databuff;
	u32 temp;
#ifdef DANUBE_DMA_DEBUG_MUTEX
	MEI_intstat_t flags;
#endif

	if (destaddr & 3)
		return MEI_FAILURE;

#ifdef DANUBE_DMA_DEBUG_MUTEX
	MEI_LOCKINT (flags);
#endif

	//      Set the write transfer address
	DANUBE_MEI_LongWordWriteOffset (pDev, MEI_XFR_ADDR_OFFSET, destaddr);

	//      Write the data pushed across DMA
	while (databuffsize--) {
		temp = *p;
		if (destaddr == MEI_TO_ARC_MAILBOX)
			MEI_HALF_WORD_SWAP (temp);
		DANUBE_MEI_LongWordWriteOffset (pDev,
						(u32) MEI_DATA_XFR_OFFSET,
						temp);
		p++;
	}			//    end of "while(..."

#ifdef DANUBE_DMA_DEBUG_MUTEX
	MEI_UNLOCKINT (flags);
#endif

	return MEI_SUCCESS;

}				//    end of "DANUBE_MEI_DMAWrite(..."

/**
 * Read several DWORD datas from ARC memory via ARC DMA interface
 * This function reads several DWORD datas from ARC memory via DMA interface.
 *
 * \param 	pDev		the device pointer
 * \param  	srcaddr		The address to read
 * \param  	databuff	Pointer to the data buffer
 * \param  	databuffsize	Number of DWORDs to read
 * \return	MEI_SUCCESS or MEI_FAILURE
 * \ingroup	Internal
 */
static MEI_ERROR
DANUBE_MEI_DMARead (ifx_adsl_device_t * pDev, u32 srcaddr, u32 * databuff,
		    u32 databuffsize)
{
	u32 *p = databuff;
	u32 temp;
#ifdef DANUBE_DMA_DEBUG_MUTEX
	MEI_intstat_t flags;
#endif
	if (srcaddr & 3)
		return MEI_FAILURE;

#ifdef DANUBE_DMA_DEBUG_MUTEX
	MEI_LOCKINT (flags);
#endif

	//      Set the read transfer address
	DANUBE_MEI_LongWordWriteOffset (pDev, (u32) MEI_XFR_ADDR_OFFSET,
					srcaddr);

	//      Read the data popped across DMA
	while (databuffsize--) {
		DANUBE_MEI_LongWordReadOffset (pDev,
					       (u32) MEI_DATA_XFR_OFFSET,
					       &temp);
		if (databuff == (u32 *) ((danube_mei_device_private_t *) pDev->priv)->CMV_RxMsg)	// swap half word
			MEI_HALF_WORD_SWAP (temp);
		*p = temp;
		p++;
	}			//    end of "while(..."

#ifdef DANUBE_DMA_DEBUG_MUTEX
	MEI_UNLOCKINT (flags);
#endif

	return MEI_SUCCESS;

}				//    end of "DANUBE_MEI_DMARead(..."

/**
 * Switch the ARC control mode
 * This function switchs the ARC control mode to JTAG mode or MEI mode
 *
 * \param 	pDev		the device pointer
 * \param  	mode		The mode want to switch: JTAG_MASTER_MODE or MEI_MASTER_MODE.
 * \ingroup	Internal
 */
static void
DANUBE_MEI_ControlModeSet (ifx_adsl_device_t * pDev, int mode)
{
	u32 temp = 0x0;
	DANUBE_MEI_LongWordReadOffset (pDev, (u32) MEI_DBG_MASTER_OFFSET,
				       &temp);
	switch (mode) {
	case JTAG_MASTER_MODE:
		temp &= ~(HOST_MSTR);
		break;
	case MEI_MASTER_MODE:
		temp |= (HOST_MSTR);
		break;
	default:
		DANUBE_MEI_EMSG
			("DANUBE_MEI_ControlModeSet: unkonwn mode [%d]\n",
			 mode);
		return;
	}
	DANUBE_MEI_LongWordWriteOffset (pDev, (u32) MEI_DBG_MASTER_OFFSET,
					temp);
}

/**
 * Disable ARC to MEI interrupt
 *
 * \param 	pDev		the device pointer
 * \ingroup	Internal
 */
static void
DANUBE_MEI_IRQDisable (ifx_adsl_device_t * pDev)
{
	DANUBE_MEI_LongWordWriteOffset (pDev,
					(u32) ARC_TO_MEI_INT_MASK_OFFSET,
					0x0);
}				//    end of "DANUBE_MEI_IRQDisable(..."

/**
 * Eable ARC to MEI interrupt
 *
 * \param 	pDev		the device pointer
 * \ingroup	Internal
 */
static void
DANUBE_MEI_IRQEnable (ifx_adsl_device_t * pDev)
{
	DANUBE_MEI_LongWordWriteOffset (pDev,
					(u32) ARC_TO_MEI_INT_MASK_OFFSET,
					MSGAV_EN);
}				//    end of "DANUBE_MEI_IRQEnable(..."

/**
 * Poll for transaction complete signal
 * This function polls and waits for transaction complete signal.
 *
 * \param 	pDev		the device pointer
 * \ingroup	Internal
 */
static void
meiPollForDbgDone (ifx_adsl_device_t * pDev)
{
	u32 query = 0;
	int i = 0;
	while (i < WHILE_DELAY) {
		DANUBE_MEI_LongWordReadOffset (pDev,
					       (u32) ARC_TO_MEI_INT_OFFSET,
					       &query);
		query &= (ARC_TO_MEI_DBG_DONE);
		if (query)
			break;
		i++;
		if (i == WHILE_DELAY) {
			DANUBE_MEI_EMSG ("\n\n PollforDbg fail");
		}
	}
	DANUBE_MEI_LongWordWriteOffset (pDev, (u32) ARC_TO_MEI_INT_OFFSET, ARC_TO_MEI_DBG_DONE);	// to clear this interrupt
}				//    end of "meiPollForDbgDone(..."

/**
 * ARC Debug Memory Access for a single DWORD reading.
 * This function used for direct, address-based access to ARC memory.
 *
 * \param 	pDev		the device pointer
 * \param  	DEC_mode	ARC memory space to used
 * \param  	address	  	Address to read
 * \param  	data	  	Pointer to data
 * \return	MEI_SUCCESS or MEI_FAILURE
 * \ingroup	Internal
 */
static MEI_ERROR
_DANUBE_MEI_DBGLongWordRead (ifx_adsl_device_t * pDev, u32 DEC_mode,
			     u32 address, u32 * data)
{
	DANUBE_MEI_LongWordWriteOffset (pDev, (u32) MEI_DEBUG_DEC_OFFSET,
					DEC_mode);
	DANUBE_MEI_LongWordWriteOffset (pDev, (u32) MEI_DEBUG_RAD_OFFSET,
					address);
	meiPollForDbgDone (pDev);
	DANUBE_MEI_LongWordReadOffset (pDev, (u32) MEI_DEBUG_DATA_OFFSET,
				       data);
	return MEI_SUCCESS;
}

/**
 * ARC Debug Memory Access for a single DWORD writing.
 * This function used for direct, address-based access to ARC memory.
 *
 * \param 	pDev		the device pointer
 * \param  	DEC_mode	ARC memory space to used
 * \param  	address	  	The address to write
 * \param  	data	  	The data to write
 * \return	MEI_SUCCESS or MEI_FAILURE
 * \ingroup	Internal
 */
static MEI_ERROR
_DANUBE_MEI_DBGLongWordWrite (ifx_adsl_device_t * pDev, u32 DEC_mode,
			      u32 address, u32 data)
{
	DANUBE_MEI_LongWordWriteOffset (pDev, (u32) MEI_DEBUG_DEC_OFFSET,
					DEC_mode);
	DANUBE_MEI_LongWordWriteOffset (pDev, (u32) MEI_DEBUG_WAD_OFFSET,
					address);
	DANUBE_MEI_LongWordWriteOffset (pDev, (u32) MEI_DEBUG_DATA_OFFSET,
					data);
	meiPollForDbgDone (pDev);
	return MEI_SUCCESS;
}

/**
 * ARC Debug Memory Access for writing.
 * This function used for direct, address-based access to ARC memory.
 *
 * \param 	pDev		the device pointer
 * \param  	destaddr	The address to read
 * \param  	databuffer  	Pointer to data
 * \param	databuffsize	The number of DWORDs to read
 * \return	MEI_SUCCESS or MEI_FAILURE
 * \ingroup	Internal
 */

static MEI_ERROR
DANUBE_MEI_DebugWrite (ifx_adsl_device_t * pDev, u32 destaddr, u32 * databuff,
		       u32 databuffsize)
{
	u32 i;
	u32 temp = 0x0;
	u32 address = 0x0;
	u32 *buffer = 0x0;
#ifdef DANUBE_DMA_DEBUG_MUTEX
	MEI_intstat_t flags;
#endif

#ifdef DANUBE_DMA_DEBUG_MUTEX
	MEI_LOCKINT (flags);
#endif

	//      Open the debug port before DMP memory write
	DANUBE_MEI_ControlModeSet (pDev, MEI_MASTER_MODE);

	DANUBE_MEI_LongWordWriteOffset (pDev, (u32) MEI_DEBUG_DEC_OFFSET,
					MEI_DEBUG_DEC_DMP1_MASK);

	//      For the requested length, write the address and write the data
	address = destaddr;
	buffer = databuff;
	for (i = 0; i < databuffsize; i++) {
		temp = *buffer;
		_DANUBE_MEI_DBGLongWordWrite (pDev, MEI_DEBUG_DEC_DMP1_MASK,
					      address, temp);
		address += 4;
		buffer++;
	}			//    end of "for(..."

	//      Close the debug port after DMP memory write
	DANUBE_MEI_ControlModeSet (pDev, JTAG_MASTER_MODE);

#ifdef DANUBE_DMA_DEBUG_MUTEX
	MEI_UNLOCKINT (flags);
#endif

	//      Return
	return MEI_SUCCESS;

}				//    end of "DANUBE_MEI_DebugWrite(..."

/**
 * ARC Debug Memory Access for reading.
 * This function used for direct, address-based access to ARC memory.
 *
 * \param 	pDev		the device pointer
 * \param  	srcaddr	  	The address to read
 * \param  	databuffer  	Pointer to data
 * \param	databuffsize	The number of DWORDs to read
 * \return	MEI_SUCCESS or MEI_FAILURE
 * \ingroup	Internal
 */
static MEI_ERROR
DANUBE_MEI_DebugRead (ifx_adsl_device_t * pDev, u32 srcaddr, u32 * databuff,
		      u32 databuffsize)
{
	u32 i;
	u32 temp = 0x0;
	u32 address = 0x0;
	u32 *buffer = 0x0;
#ifdef DANUBE_DMA_DEBUG_MUTEX
	MEI_intstat_t flags;
#endif

#ifdef DANUBE_DMA_DEBUG_MUTEX
	MEI_LOCKINT (flags);
#endif

	//      Open the debug port before DMP memory read
	DANUBE_MEI_ControlModeSet (pDev, MEI_MASTER_MODE);

	DANUBE_MEI_LongWordWriteOffset (pDev, (u32) MEI_DEBUG_DEC_OFFSET,
					MEI_DEBUG_DEC_DMP1_MASK);

	//      For the requested length, write the address and read the data
	address = srcaddr;
	buffer = databuff;
	for (i = 0; i < databuffsize; i++) {
		_DANUBE_MEI_DBGLongWordRead (pDev, MEI_DEBUG_DEC_DMP1_MASK,
					     address, &temp);
		*buffer = temp;
		address += 4;
		buffer++;
	}			//    end of "for(..."

	//      Close the debug port after DMP memory read
	DANUBE_MEI_ControlModeSet (pDev, JTAG_MASTER_MODE);

#ifdef DANUBE_DMA_DEBUG_MUTEX
	MEI_UNLOCKINT (flags);
#endif

	//      Return
	return MEI_SUCCESS;

}				//    end of "DANUBE_MEI_DebugRead(..."

/**
 * Send a message to ARC MailBox.
 * This function sends a message to ARC Mailbox via ARC DMA interface.
 *
 * \param 	pDev		the device pointer
 * \param  	msgsrcbuffer  	Pointer to message.
 * \param	msgsize		The number of words to write.
 * \return	MEI_SUCCESS or MEI_FAILURE
 * \ingroup	Internal
 */
static MEI_ERROR
DANUBE_MEI_MailboxWrite (ifx_adsl_device_t * pDev, u16 * msgsrcbuffer,
			 u16 msgsize)
{
	int i;
	u32 arc_mailbox_status = 0x0;
	u32 temp = 0;
	MEI_ERROR meiMailboxError = MEI_SUCCESS;

	//      Write to mailbox
	meiMailboxError =
		DANUBE_MEI_DMAWrite (pDev, MEI_TO_ARC_MAILBOX,
				     (u32 *) msgsrcbuffer, msgsize / 2);
	meiMailboxError =
		DANUBE_MEI_DMAWrite (pDev, MEI_TO_ARC_MAILBOXR,
				     (u32 *) (&temp), 1);

	//      Notify arc that mailbox write completed
	((danube_mei_device_private_t *) pDev->priv)->cmv_waiting = 1;
	DANUBE_MEI_LongWordWriteOffset (pDev, (u32) MEI_TO_ARC_INT_OFFSET,
					MEI_TO_ARC_MSGAV);

	i = 0;
	while (i < WHILE_DELAY) {	// wait for ARC to clear the bit
		DANUBE_MEI_LongWordReadOffset (pDev,
					       (u32) MEI_TO_ARC_INT_OFFSET,
					       &arc_mailbox_status);
		if ((arc_mailbox_status & MEI_TO_ARC_MSGAV) !=
		    MEI_TO_ARC_MSGAV)
			break;
		i++;
		if (i == WHILE_DELAY) {
			DANUBE_MEI_EMSG
				("\n\n MEI_TO_ARC_MSGAV not cleared by ARC");
			meiMailboxError = MEI_FAILURE;
		}
	}

	//      Return
	return meiMailboxError;

}				//    end of "DANUBE_MEI_MailboxWrite(..."

/**
 * Read a message from ARC MailBox.
 * This function reads a message from ARC Mailbox via ARC DMA interface.
 *
 * \param 	pDev		the device pointer
 * \param  	msgsrcbuffer  	Pointer to message.
 * \param	msgsize		The number of words to read
 * \return	MEI_SUCCESS or MEI_FAILURE
 * \ingroup	Internal
 */
static MEI_ERROR
DANUBE_MEI_MailboxRead (ifx_adsl_device_t * pDev, u16 * msgdestbuffer,
			u16 msgsize)
{
	MEI_ERROR meiMailboxError = MEI_SUCCESS;
	//      Read from mailbox
	meiMailboxError =
		DANUBE_MEI_DMARead (pDev, ARC_TO_MEI_MAILBOX,
				    (u32 *) msgdestbuffer, msgsize / 2);

	//      Notify arc that mailbox read completed
	DANUBE_MEI_LongWordWriteOffset (pDev, (u32) ARC_TO_MEI_INT_OFFSET,
					ARC_TO_MEI_MSGAV);

	//      Return
	return meiMailboxError;

}				//    end of "DANUBE_MEI_MailboxRead(..."

/**
 * Download boot pages to ARC.
 * This function downloads boot pages to ARC.
 *
 * \param 	pDev		the device pointer
 * \return	MEI_SUCCESS or MEI_FAILURE
 * \ingroup	Internal
 */
static MEI_ERROR
DANUBE_MEI_DownloadBootPages (ifx_adsl_device_t * pDev)
{
	int boot_loop;
	int page_size;
	u32 dest_addr;

	/*
	 **     DMA the boot code page(s)
	 */

	for (boot_loop = 1;
	     boot_loop <
	     (((danube_mei_device_private_t *) pDev->priv)->img_hdr->count);
	     boot_loop++) {
		if ((((danube_mei_device_private_t *) pDev->priv)->img_hdr->
		     page[boot_loop].p_size) & BOOT_FLAG) {
			page_size =
				DANUBE_MEI_GetPage (pDev, boot_loop, GET_PROG,
						    MAXSWAPSIZE,
						    mei_arc_swap_buff,
						    &dest_addr);
			if (page_size > 0) {
				DANUBE_MEI_DMAWrite (pDev, dest_addr,
						     mei_arc_swap_buff,
						     page_size);
			}
		}
		if ((((danube_mei_device_private_t *) pDev->priv)->img_hdr->
		     page[boot_loop].d_size) & BOOT_FLAG) {
			page_size =
				DANUBE_MEI_GetPage (pDev, boot_loop, GET_DATA,
						    MAXSWAPSIZE,
						    mei_arc_swap_buff,
						    &dest_addr);
			if (page_size > 0) {
				DANUBE_MEI_DMAWrite (pDev, dest_addr,
						     mei_arc_swap_buff,
						     page_size);
			}
		}
	}
	return MEI_SUCCESS;
}

/**
 * Initial efuse rar.
 **/
static void
DANUBE_MEI_FuseInit (ifx_adsl_device_t * pDev)
{
	u32 data = 0;
	DANUBE_MEI_DMAWrite (pDev, IRAM0_BASE, &data, 1);
	DANUBE_MEI_DMAWrite (pDev, IRAM0_BASE + 4, &data, 1);
	DANUBE_MEI_DMAWrite (pDev, IRAM1_BASE, &data, 1);
	DANUBE_MEI_DMAWrite (pDev, IRAM1_BASE + 4, &data, 1);
	DANUBE_MEI_DMAWrite (pDev, BRAM_BASE, &data, 1);
	DANUBE_MEI_DMAWrite (pDev, BRAM_BASE + 4, &data, 1);
	DANUBE_MEI_DMAWrite (pDev, ADSL_DILV_BASE, &data, 1);
	DANUBE_MEI_DMAWrite (pDev, ADSL_DILV_BASE + 4, &data, 1);
}

/**
 * efuse rar program
 **/
static void
DANUBE_MEI_FuseProg (ifx_adsl_device_t * pDev)
{
	u32 reg_data, fuse_value;
	int i = 0;
	DANUBE_MEI_LongWordRead ((u32) DANUBE_RCU_REQ, &reg_data);
	while ((reg_data & 0x10000000) == 0) {
		DANUBE_MEI_LongWordRead ((u32) DANUBE_RCU_REQ, &reg_data);
		//add a watchdog
		i++;
		/* 0x4000 translate to  about 16 ms@111M, so should be enough */
		if (i == 0x4000)
			return;
	}
	// STEP a: Prepare memory for external accesses
	// Write fuse_en bit24
	DANUBE_MEI_LongWordRead ((u32) DANUBE_RCU_REQ, &reg_data);
	DANUBE_MEI_LongWordWrite ((u32) DANUBE_RCU_REQ, reg_data | (1 << 24));

	DANUBE_MEI_FuseInit (pDev);
	for (i = 0; i < 4; i++) {
		DANUBE_MEI_LongWordRead ((u32) (DANUBE_FUSE_BASE_ADDR) +
					 i * 4, &fuse_value);
		switch (fuse_value & 0xF0000) {
		case 0x80000:
			reg_data =
				((fuse_value & RX_DILV_ADDR_BIT_MASK) |
				 (RX_DILV_ADDR_BIT_MASK + 0x1));
			DANUBE_MEI_DMAWrite (pDev, ADSL_DILV_BASE, &reg_data,
					     1);
			break;
		case 0x90000:
			reg_data =
				((fuse_value & RX_DILV_ADDR_BIT_MASK) |
				 (RX_DILV_ADDR_BIT_MASK + 0x1));
			DANUBE_MEI_DMAWrite (pDev, ADSL_DILV_BASE + 4,
					     &reg_data, 1);
			break;
		case 0xA0000:
			reg_data =
				((fuse_value & IRAM0_ADDR_BIT_MASK) |
				 (IRAM0_ADDR_BIT_MASK + 0x1));
			DANUBE_MEI_DMAWrite (pDev, IRAM0_BASE, &reg_data, 1);
			break;
		case 0xB0000:
			reg_data =
				((fuse_value & IRAM0_ADDR_BIT_MASK) |
				 (IRAM0_ADDR_BIT_MASK + 0x1));
			DANUBE_MEI_DMAWrite (pDev, IRAM0_BASE + 4, &reg_data,
					     1);
			break;
		case 0xC0000:
			reg_data =
				((fuse_value & IRAM1_ADDR_BIT_MASK) |
				 (IRAM1_ADDR_BIT_MASK + 0x1));
			DANUBE_MEI_DMAWrite (pDev, IRAM1_BASE, &reg_data, 1);
			break;
		case 0xD0000:
			reg_data =
				((fuse_value & IRAM1_ADDR_BIT_MASK) |
				 (IRAM1_ADDR_BIT_MASK + 0x1));
			DANUBE_MEI_DMAWrite (pDev, IRAM1_BASE + 4, &reg_data,
					     1);
			break;
		case 0xE0000:
			reg_data =
				((fuse_value & BRAM_ADDR_BIT_MASK) |
				 (BRAM_ADDR_BIT_MASK + 0x1));
			DANUBE_MEI_DMAWrite (pDev, BRAM_BASE, &reg_data, 1);
			break;
		case 0xF0000:
			reg_data =
				((fuse_value & BRAM_ADDR_BIT_MASK) |
				 (BRAM_ADDR_BIT_MASK + 0x1));
			DANUBE_MEI_DMAWrite (pDev, BRAM_BASE + 4, &reg_data,
					     1);
			break;
		default:	// PPE efuse
			break;
		}
	}
	DANUBE_MEI_LongWordRead ((u32) DANUBE_RCU_REQ, &reg_data);
	DANUBE_MEI_LongWordWrite ((u32) DANUBE_RCU_REQ,
				  reg_data & 0xF7FFFFFF);
}

/**
 * Enable DFE Clock
 * This function enables DFE Clock
 *
 * \param 	pDev		the device pointer
 * \return	MEI_SUCCESS or MEI_FAILURE
 * \ingroup	Internal
 */
static MEI_ERROR
DANUBE_MEI_EnableCLK (ifx_adsl_device_t * pDev)
{
	u32 arc_debug_data = 0;
	DANUBE_MEI_ControlModeSet (pDev, MEI_MASTER_MODE);
	//enable ac_clk signal
	_DANUBE_MEI_DBGLongWordRead (pDev, MEI_DEBUG_DEC_DMP1_MASK, CRI_CCR0,
				     &arc_debug_data);
	arc_debug_data |= ACL_CLK_MODE_ENABLE;
	_DANUBE_MEI_DBGLongWordWrite (pDev, MEI_DEBUG_DEC_DMP1_MASK, CRI_CCR0,
				      arc_debug_data);
	DANUBE_MEI_ControlModeSet (pDev, JTAG_MASTER_MODE);
	return MEI_SUCCESS;
}

/**
 * Halt the ARC.
 * This function halts the ARC.
 *
 * \param 	pDev		the device pointer
 * \return	MEI_SUCCESS or MEI_FAILURE
 * \ingroup	Internal
 */
static MEI_ERROR
DANUBE_MEI_HaltArc (ifx_adsl_device_t * pDev)
{
	u32 arc_debug_data = 0x0;

	//      Switch arc control from JTAG mode to MEI mode
	DANUBE_MEI_ControlModeSet (pDev, MEI_MASTER_MODE);
	_DANUBE_MEI_DBGLongWordRead (pDev, MEI_DEBUG_DEC_AUX_MASK, ARC_DEBUG,
				     &arc_debug_data);
	arc_debug_data |= (BIT1);
	_DANUBE_MEI_DBGLongWordWrite (pDev, MEI_DEBUG_DEC_AUX_MASK, ARC_DEBUG,
				      arc_debug_data);
	//      Switch arc control from MEI mode to JTAG mode
	DANUBE_MEI_ControlModeSet (pDev, JTAG_MASTER_MODE);

	MEI_WAIT (10);
	//      Return
	return MEI_SUCCESS;

}				//    end of "meiHalt(..."

/**
 * Run the ARC.
 * This function runs the ARC.
 *
 * \param 	pDev		the device pointer
 * \return	MEI_SUCCESS or MEI_FAILURE
 * \ingroup	Internal
 */
static MEI_ERROR
DANUBE_MEI_RunArc (ifx_adsl_device_t * pDev)
{
	u32 arc_debug_data = 0x0;

	//      Switch arc control from JTAG mode to MEI mode- write '1' to bit0
	DANUBE_MEI_ControlModeSet (pDev, MEI_MASTER_MODE);
	_DANUBE_MEI_DBGLongWordRead (pDev, MEI_DEBUG_DEC_AUX_MASK, AUX_STATUS,
				     &arc_debug_data);

	//      Write debug data reg with content ANDd with 0xFDFFFFFF (halt bit cleared)
	arc_debug_data &= ~(BIT25);
	_DANUBE_MEI_DBGLongWordWrite (pDev, MEI_DEBUG_DEC_AUX_MASK,
				      AUX_STATUS, arc_debug_data);

	//      Switch arc control from MEI mode to JTAG mode- write '0' to bit0
	DANUBE_MEI_ControlModeSet (pDev, JTAG_MASTER_MODE);
	//      Enable mask for arc codeswap interrupts
	DANUBE_MEI_IRQEnable (pDev);

	//      Return
	return MEI_SUCCESS;

}				//    end of "meiActivate(..."

/**
 * Reset the ARC.
 * This function resets the ARC.
 *
 * \param 	pDev		the device pointer
 * \return	MEI_SUCCESS or MEI_FAILURE
 * \ingroup	Internal
 */
static MEI_ERROR
DANUBE_MEI_ResetARC (ifx_adsl_device_t * pDev)
{

	u32 arc_debug_data = 0;

	DANUBE_MEI_HaltArc (pDev);

	DANUBE_MEI_LongWordRead ((u32) DANUBE_RCU_REQ, &arc_debug_data);
	DANUBE_MEI_LongWordWrite ((u32) DANUBE_RCU_REQ,
				  arc_debug_data | DANBUE_RCU_RST_REQ_DFE |
				  DANBUE_RCU_RST_REQ_AFE);
	DANUBE_MEI_LongWordWrite ((u32) DANUBE_RCU_REQ, arc_debug_data);
	// reset ARC
	DANUBE_MEI_LongWordWriteOffset (pDev, (u32) MEI_RST_CONTROL_OFFSET,
					MEI_SOFT_RESET);
	DANUBE_MEI_LongWordWriteOffset (pDev, (u32) MEI_RST_CONTROL_OFFSET,
					0);

	DANUBE_MEI_IRQDisable (pDev);

	DANUBE_MEI_EnableCLK (pDev);

	// reset part of PPE
	*(unsigned long *) (DANUBE_PPE32_SRST) = 0xC30;
	*(unsigned long *) (DANUBE_PPE32_SRST) = 0xFFF;

	((danube_mei_device_private_t *) pDev->priv)->modem_ready = 0;

	return MEI_SUCCESS;
}

MEI_ERROR
IFX_ADSL_BSP_Showtime (struct ifx_adsl_device * dev, u32 rate_fast,
		       u32 rate_intl)
{
#if defined(CONFIG_ATM_DANUBE) && defined (CONFIG_IFX_ADSL)

	DANUBE_MEI_EMSG ("Datarate US intl = %d, fast = %d\n", rate_intl,
			 rate_fast);
	if (rate_intl && rate_fast) {
		ifx_atm_set_cell_rate (0, rate_fast / (53 * 8));
		ifx_atm_set_cell_rate (1, rate_intl / (53 * 8));
	}
	else if (rate_fast) {
		ifx_atm_set_cell_rate (0, rate_fast / (53 * 8));
	}
	else if (rate_intl) {
		ifx_atm_set_cell_rate (0, rate_intl / (53 * 8));
	}
	else {
		DANUBE_MEI_EMSG ("Got rate fail.\n");
	}
#endif
	return MEI_SUCCESS;
};

/**
 * Reset/halt/run the DFE.
 * This function provide operations to reset/halt/run the DFE.
 *
 * \param 	pDev		the device pointer
 * \param	mode		which operation want to do
 * \return	MEI_SUCCESS or MEI_FAILURE
 * \ingroup	Internal
 */
static MEI_ERROR
DANUBE_MEI_CpuModeSet (struct ifx_adsl_device *pDev, ifx_adsl_cpu_mode_t mode)
{
	MEI_ERROR err_ret = MEI_FAILURE;
	switch (mode) {
	case IFX_ADSL_CPU_HALT:
		err_ret = DANUBE_MEI_HaltArc (pDev);
		break;
	case IFX_ADSL_CPU_RUN:
		err_ret = DANUBE_MEI_RunArc (pDev);
		break;
	case IFX_ADSL_CPU_RESET:
		err_ret = DANUBE_MEI_ResetARC (pDev);
		break;
	default:
		break;
	}
	return err_ret;
}

/**
 * Accress DFE memory.
 * This function provide a way to access DFE memory;
 *
 * \param 	pDev		the device pointer
 * \param	type		read or write
 * \param	destaddr	destination address
 * \param	databuff	pointer to hold data
 * \param	databuffsize	size want to read/write
 * \return	MEI_SUCCESS or MEI_FAILURE
 * \ingroup	Internal
 */
MEI_ERROR
IFX_ADSL_BSP_MemoryDebugAccess (struct ifx_adsl_device * pDev,
				ifx_adsl_memory_access_type_t type,
				u32 destaddr, u32 * databuff,
				u32 databuffsize)
{
	MEI_ERROR meierr = MEI_SUCCESS;
	switch (type) {
	case IFX_ADSL_MEMORY_READ:
		meierr = DANUBE_MEI_DebugRead (pDev, destaddr, databuff,
					       databuffsize);
		break;
	case IFX_ADSL_MEMORY_WRITE:
		meierr = DANUBE_MEI_DebugWrite (pDev, destaddr, databuff,
						databuffsize);
		break;
	}
	return MEI_SUCCESS;
};

/**
 * Download boot code to ARC.
 * This function downloads boot code to ARC.
 *
 * \param 	pDev		the device pointer
 * \return	MEI_SUCCESS or MEI_FAILURE
 * \ingroup	Internal
 */
static MEI_ERROR
DANUBE_MEI_DownloadBootCode (struct ifx_adsl_device *pDev)
{
	DANUBE_MEI_IRQDisable (pDev);

	DANUBE_MEI_EnableCLK (pDev);

	DANUBE_MEI_FuseProg (pDev);	//program fuse rar

	DANUBE_MEI_DownloadBootPages (pDev);

	return MEI_SUCCESS;
};

/**
 * Enable Jtag debugger interface
 * This function setups mips gpio to enable jtag debugger
 *
 * \param 	pDev		the device pointer
 * \param 	enable		enable or disable
 * \return	MEI_SUCCESS or MEI_FAILURE
 * \ingroup	Internal
 */
static MEI_ERROR
DANUBE_MEI_ArcJtagEnable (struct ifx_adsl_device *dev, int enable)
{
	int meierr;
	u32 reg_data;

	switch (enable) {
	case 1:
#if defined(__LINUX__)
		//reserve gpio 9, 10, 11, 14, 19 for ARC JTAG
		meierr = danube_port_reserve_pin (0, 9, PORT_MODULE_MEI_JTAG);
		if (meierr < 0) {
			DANUBE_MEI_EMSG ("Reserve GPIO 9 Fail.\n");
			goto jtag_end;
		}
		meierr = danube_port_reserve_pin (0, 10,
						  PORT_MODULE_MEI_JTAG);
		if (meierr < 0) {
			DANUBE_MEI_EMSG ("Reserve GPIO 10 Fail.\n");
			goto jtag_end;
		}
		meierr = danube_port_reserve_pin (0, 11,
						  PORT_MODULE_MEI_JTAG);
		if (meierr < 0) {
			DANUBE_MEI_EMSG ("Reserve GPIO 11 Fail.\n");
			goto jtag_end;
		}
		meierr = danube_port_reserve_pin (0, 14,
						  PORT_MODULE_MEI_JTAG);
		if (meierr < 0) {
			DANUBE_MEI_EMSG ("Reserve GPIO 14 Fail.\n");
			goto jtag_end;
		}
		meierr = danube_port_reserve_pin (1, 3, PORT_MODULE_MEI_JTAG);
		if (meierr < 0) {
			DANUBE_MEI_EMSG ("Reserve GPIO 19 Fail.\n");
			goto jtag_end;
		}
#endif
		*(DANUBE_GPIO_P0_DIR) = (*DANUBE_GPIO_P0_DIR) & (~0x800);	// set gpio11 to input
		*(DANUBE_GPIO_P0_ALTSEL0) =
			((*DANUBE_GPIO_P0_ALTSEL0) & (~0x800));
		*(DANUBE_GPIO_P0_ALTSEL1) =
			((*DANUBE_GPIO_P0_ALTSEL1) & (~0x800));
		*DANUBE_GPIO_P0_OD = (*DANUBE_GPIO_P0_OD) | 0x800;

		//enable ARC JTAG
		DANUBE_MEI_LongWordRead ((u32) DANUBE_RCU_REQ, &reg_data);
		DANUBE_MEI_LongWordWrite ((u32) DANUBE_RCU_REQ,
					  reg_data |
					  DANUBE_RCU_RST_REQ_ARC_JTAG);
		break;
	case 0:
	default:
#if defined(__LINUX__)
		//reserve gpio 9, 10, 11, 14, 19 for ARC JTAG
		meierr = danube_port_free_pin (0, 9, PORT_MODULE_MEI_JTAG);
		if (meierr < 0) {
			DANUBE_MEI_EMSG ("Reserve GPIO 9 Fail.\n");
			goto jtag_end;
		}
		meierr = danube_port_free_pin (0, 10, PORT_MODULE_MEI_JTAG);
		if (meierr < 0) {
			DANUBE_MEI_EMSG ("Reserve GPIO 10 Fail.\n");
			goto jtag_end;
		}
		meierr = danube_port_free_pin (0, 11, PORT_MODULE_MEI_JTAG);
		if (meierr < 0) {
			DANUBE_MEI_EMSG ("Reserve GPIO 11 Fail.\n");
			goto jtag_end;
		}
		meierr = danube_port_free_pin (0, 14, PORT_MODULE_MEI_JTAG);
		if (meierr < 0) {
			DANUBE_MEI_EMSG ("Reserve GPIO 14 Fail.\n");
			goto jtag_end;
		}
		meierr = danube_port_free_pin (1, 3, PORT_MODULE_MEI_JTAG);
		if (meierr < 0) {
			DANUBE_MEI_EMSG ("Reserve GPIO 19 Fail.\n");
			goto jtag_end;
		}
#endif
		break;
	}

      jtag_end:
	if (meierr)
		return MEI_FAILURE;
	return MEI_SUCCESS;
};

/**
 * Enable DFE to MIPS interrupt
 * This function enable DFE to MIPS interrupt
 *
 * \param 	pDev		the device pointer
 * \param 	enable		enable or disable
 * \return	MEI_SUCCESS or MEI_FAILURE
 * \ingroup	Internal
 */
static MEI_ERROR
DANUBE_MEI_AdslMailboxIRQEnable (struct ifx_adsl_device *pDev, int enable)
{
	MEI_ERROR meierr;
	switch (enable) {
	case 0:
		meierr = MEI_SUCCESS;
		DANUBE_MEI_IRQDisable (pDev);
		break;
	case 1:
		DANUBE_MEI_IRQEnable (pDev);
		meierr = MEI_SUCCESS;
		break;
	default:
		meierr = MEI_FAILURE;
		break;

	}
	return meierr;
}

/**
 * Get the modem status
 * This function return the modem status
 *
 * \param 	pDev		the device pointer
 * \return	1: modem ready 0: not ready
 * \ingroup	Internal
 */
static int
DANUBE_MEI_IsModemReady (ifx_adsl_device_t * pDev)
{
	return ((danube_mei_device_private_t *) pDev->priv)->modem_ready;
}

MEI_ERROR
IFX_ADSL_BSP_AdslLedInit (struct ifx_adsl_device * dev,
			  ifx_adsl_led_id_t led_number,
			  ifx_adsl_led_type_t type,
			  ifx_adsl_led_handler_t handler)
{
	if (led_number == IFX_ADSL_LED_DATA_ID
	    && type == IFX_ADSL_LED_DATA_TYPE
	    && handler == IFX_ADSL_LED_HD_CPU) {
		struct led_config_param param;
		param.operation_mask = CONFIG_OPERATION_UPDATE_SOURCE;
		param.led = 0x01;
		param.source = 0x0;
		danube_led_config (&param);
	}
	return MEI_SUCCESS;
};

MEI_ERROR
IFX_ADSL_BSP_AdslLedSet (struct ifx_adsl_device * dev,
			 ifx_adsl_led_id_t led_number,
			 ifx_adsl_led_mode_t mode)
{
	switch (mode) {
	case IFX_ADSL_LED_OFF:
		switch (led_number) {
		case IFX_ADSL_LED_LINK_ID:
			danube_led_set_blink (1, 0);
			danube_led_set_data (1, 0);
			break;
		case IFX_ADSL_LED_DATA_ID:
			danube_led_set_blink (0, 0);
			danube_led_set_data (0, 0);
			break;
		}
		break;
	case IFX_ADSL_LED_FLASH:
		switch (led_number) {
		case IFX_ADSL_LED_LINK_ID:
			danube_led_set_blink (1, 1);	// data
			break;
		case IFX_ADSL_LED_DATA_ID:
			danube_led_set_blink (0, 1);	// data 
			break;
		}
		break;
	case IFX_ADSL_LED_ON:
		switch (led_number) {
		case IFX_ADSL_LED_LINK_ID:
			danube_led_set_blink (1, 0);
			danube_led_set_data (1, 1);
			break;
		case IFX_ADSL_LED_DATA_ID:
			danube_led_set_blink (0, 0);
			danube_led_set_data (0, 1);
			break;
		}
		break;
	}
	return MEI_SUCCESS;
};

/**
 * Send a message to ARC and read the response
 * This function sends a message to arc, waits the response, and reads the responses.
 *
 * \param 	pDev		the device pointer
 * \param	request		Pointer to the request
 * \param	reply		Wait reply or not.
 * \param	response	Pointer to the response
 * \return	MEI_SUCCESS or MEI_FAILURE
 * \ingroup	Internal
 */
MEI_ERROR
IFX_ADSL_BSP_SendCMV (struct ifx_adsl_device * pDev, u16 * request, int reply, u16 * response)	// write cmv to arc, if reply needed, wait for reply
{
	MEI_ERROR meierror;
#if defined(DANUBE_PORT_RTEMS)
	int delay_counter = 0;
#endif

	if (MEI_MUTEX_LOCK
	    (((danube_mei_device_private_t *) pDev->priv)->mei_cmv_sema))
		return -ERESTARTSYS;

	((danube_mei_device_private_t *) pDev->priv)->cmv_reply = reply;
	memset (((danube_mei_device_private_t *) pDev->priv)->CMV_RxMsg, 0,
		sizeof (((danube_mei_device_private_t *) pDev->priv)->
			CMV_RxMsg));
	((danube_mei_device_private_t *) pDev->priv)->arcmsgav = 0;

	meierror = DANUBE_MEI_MailboxWrite (pDev, request, MSG_LENGTH);

	if (meierror != MEI_SUCCESS) {
		((danube_mei_device_private_t *) pDev->priv)->cmv_waiting = 0;
		((danube_mei_device_private_t *) pDev->priv)->arcmsgav = 0;
		DANUBE_MEI_EMSG ("\n\n MailboxWrite Fail.");
		MEI_MUTEX_UNLOCK (((danube_mei_device_private_t *) pDev->
				   priv)->mei_cmv_sema);
		return meierror;
	}
	else {
		((danube_mei_device_private_t *) pDev->priv)->cmv_count++;
	}

	if (((danube_mei_device_private_t *) pDev->priv)->cmv_reply ==
	    NO_REPLY) {
		MEI_MUTEX_UNLOCK (((danube_mei_device_private_t *) pDev->
				   priv)->mei_cmv_sema);
		return MEI_SUCCESS;
	}

#if !defined(DANUBE_PORT_RTEMS)
	if (((danube_mei_device_private_t *) pDev->priv)->arcmsgav == 0)
		MEI_WAIT_EVENT_TIMEOUT (((danube_mei_device_private_t *)
					 pDev->priv)->wait_queue_arcmsgav,
					CMV_TIMEOUT);
#else
	while (((danube_mei_device_private_t *) pDev->priv)->arcmsgav == 0
	       && delay_counter < CMV_TIMEOUT / 5) {
		MEI_WAIT (5);
		delay_counter++;
	}
#endif

	((danube_mei_device_private_t *) pDev->priv)->cmv_waiting = 0;
	if (((danube_mei_device_private_t *) pDev->priv)->arcmsgav == 0) {	//CMV_timeout
		((danube_mei_device_private_t *) pDev->priv)->arcmsgav = 0;
		DANUBE_MEI_EMSG ("\%s: MEI_MAILBOX_TIMEOUT\n", __FUNCTION__);
		MEI_MUTEX_UNLOCK (((danube_mei_device_private_t *) pDev->
				   priv)->mei_cmv_sema);
		return MEI_MAILBOX_TIMEOUT;
	}
	else {
		((danube_mei_device_private_t *) pDev->priv)->arcmsgav = 0;
		((danube_mei_device_private_t *) pDev->priv)->reply_count++;
		memcpy (response,
			((danube_mei_device_private_t *) pDev->priv)->
			CMV_RxMsg, MSG_LENGTH * 2);
		MEI_MUTEX_UNLOCK (((danube_mei_device_private_t *) pDev->
				   priv)->mei_cmv_sema);
		return MEI_SUCCESS;
	}
	MEI_MUTEX_UNLOCK (((danube_mei_device_private_t *) pDev->priv)->
			  mei_cmv_sema);
	return MEI_SUCCESS;
}

/**
 * Reset the ARC, download boot codes, and run the ARC.
 * This function resets the ARC, downloads boot codes to ARC, and runs the ARC.
 *
 * \param 	pDev		the device pointer
 * \return	MEI_SUCCESS or MEI_FAILURE
 * \ingroup	Internal
 */
static MEI_ERROR
DANUBE_MEI_RunAdslModem (struct ifx_adsl_device *pDev)
{
	int nSize = 0, idx = 0;
	MEI_ERROR meierr = MEI_FAILURE;
	uint32_t irq_register = 0; //2.00.03  20/12/2006 TC Chen

	if (mei_arc_swap_buff == NULL) {
		mei_arc_swap_buff =
			(u32 *) kmalloc (MAXSWAPSIZE * 4, GFP_KERNEL);
		if (mei_arc_swap_buff == NULL) {
			DANUBE_MEI_EMSG
				("\n\n malloc fail for codeswap buff");
			meierr = MEI_FAILURE;
		}
	}

	((danube_mei_device_private_t *) pDev->priv)->img_hdr =
		(ARC_IMG_HDR *) ((danube_mei_device_private_t *) pDev->priv)->
		adsl_mem_info[0].address;
	if ((((danube_mei_device_private_t *) pDev->priv)->img_hdr->count) *
	    sizeof (ARC_SWP_PAGE_HDR) > SDRAM_SEGMENT_SIZE) {
		DANUBE_MEI_EMSG
			("Firmware Header size is bigger than Segment size!\n");
		return MEI_FAILURE;
	}
	// check image size
	for (idx = 0; idx < MAX_BAR_REGISTERS; idx++) {
		nSize += ((danube_mei_device_private_t *) pDev->priv)->
			adsl_mem_info[idx].nCopy;
	}
	if (nSize != ((danube_mei_device_private_t *) pDev->priv)->image_size) {
		DANUBE_MEI_EMSG
			("Firmware download is not completed. \nPlease download firmware again!\n");
		return MEI_FAILURE;
	}
	// TODO: check crc
	///

	DANUBE_MEI_ResetARC (pDev);
	DANUBE_MEI_HaltArc (pDev);
	DANUBE_MEI_BarUpdate (pDev,
			      ((danube_mei_device_private_t *) pDev->priv)->
			      nBar);

	//DANUBE_MEI_DMSG("Starting to meiDownloadBootCode\n");

	DANUBE_MEI_DownloadBootCode (pDev);

	// 2.00.03  20/12/2006 TC Chen
	// disable USB OC interrupt, reset DSL chip will triger OC interrupt
	irq_register = (*DANUBE_ICU_IM4_IER) & (1<<23);
	MEI_DISABLE_IRQ(DANUBE_USB_OC_INT);

	DANUBE_MEI_RunArc (pDev);

	MEI_WAIT_EVENT_TIMEOUT (((danube_mei_device_private_t *) pDev->priv)->
				wait_queue_modemready, 100);

	//2.00.03  20/12/2006 TC Chen
	// restore USB OC interrupt
	MEI_MASK_AND_ACK_IRQ(DANUBE_USB_OC_INT);
	*DANUBE_ICU_IM4_IER |= irq_register;

	if (((danube_mei_device_private_t *) pDev->priv)->modem_ready != 1)
		meierr = MEI_FAILURE;
	else
		meierr = MEI_SUCCESS;
	return meierr;
}

/**
 * Get the page's data pointer
 * This function caculats the data address from the firmware header.
 *
 * \param 	pDev		the device pointer
 * \param	Page		The page number.
 * \param	data		Data page or program page.
 * \param	MaxSize		The maximum size to read.
 * \param	Buffer		Pointer to data.
 * \param	Dest		Pointer to the destination address.
 * \return	The number of bytes to read.
 * \ingroup	Internal
 */
static int
DANUBE_MEI_GetPage (ifx_adsl_device_t * pDev, u32 Page, u32 data, u32 MaxSize,
		    u32 * Buffer, u32 * Dest)
{
	u32 size;
	u32 i;
	u32 *p;
	u32 idx, offset, nBar = 0;

	if (Page >
	    ((danube_mei_device_private_t *) pDev->priv)->img_hdr->count)
		return -2;
	/*
	 **     Get program or data size, depending on "data" flag
	 */
	size = (data ==
		GET_DATA) ? (((danube_mei_device_private_t *) pDev->priv)->
			     img_hdr->page[Page].
			     d_size) : (((danube_mei_device_private_t *)
					 pDev->priv)->img_hdr->page[Page].
					p_size);
	size &= BOOT_FLAG_MASK;	//      Clear boot bit!
	if (size > MaxSize)
		return -1;

	if (size == 0)
		return 0;
	/*
	 **     Get program or data offset, depending on "data" flag
	 */
	i = data ? (((danube_mei_device_private_t *) pDev->priv)->img_hdr->
		    page[Page].
		    d_offset) : (((danube_mei_device_private_t *) pDev->
				  priv)->img_hdr->page[Page].p_offset);

	/*
	 **     Copy data/program to buffer
	 */

	idx = i / SDRAM_SEGMENT_SIZE;
	offset = i % SDRAM_SEGMENT_SIZE;
	p = (u32 *) ((u8 *) ((danube_mei_device_private_t *) pDev->priv)->
		     adsl_mem_info[idx].address + offset);

	for (i = 0; i < size; i++) {
		if (offset + i * 4 - (nBar * SDRAM_SEGMENT_SIZE) >=
		    SDRAM_SEGMENT_SIZE) {
			idx++;
			nBar++;
			p = (u32 *) ((u8 *)
				     KSEG1ADDR ((u32)
						((danube_mei_device_private_t
						  *) pDev->priv)->
						adsl_mem_info[idx].address));
		}
		Buffer[i] = *p++;
	}

	/*
	 **     Pass back data/program destination address
	 */
	*Dest = data ? (((danube_mei_device_private_t *) pDev->priv)->
			img_hdr->page[Page].
			d_dest) : (((danube_mei_device_private_t *) pDev->
				    priv)->img_hdr->page[Page].p_dest);

	return size;
}

/**
 * Free the memory for ARC firmware
 *
 * \param 	pDev		the device pointer
 * \param	type	Free all memory or free the unused memory after showtime
 * \ingroup	Internal
 */
static int
DANUBE_MEI_DFEMemoryFree (ifx_adsl_device_t * pDev, int type)
{
	int idx = 0;
	for (idx = 0; idx < MAX_BAR_REGISTERS; idx++) {
		//DANUBE_MEI_DMSG("meminfo[%d].type=%d,size=%ld,addr=%X\n",idx,adsl_mem_info[idx].type,adsl_mem_info[idx].size,(int)adsl_mem_info[idx].address);
		if (type == FREE_ALL
		    || ((danube_mei_device_private_t *) pDev->priv)->
		    adsl_mem_info[idx].type == type) {
			if (((danube_mei_device_private_t *) pDev->priv)->
			    adsl_mem_info[idx].size > 0) {
				kfree (((danube_mei_device_private_t *) pDev->
					priv)->adsl_mem_info[idx].
				       org_address);
				((danube_mei_device_private_t *) pDev->priv)->
					adsl_mem_info[idx].address = 0;
				((danube_mei_device_private_t *) pDev->priv)->
					adsl_mem_info[idx].size = 0;
				((danube_mei_device_private_t *) pDev->priv)->
					adsl_mem_info[idx].type = 0;
				((danube_mei_device_private_t *) pDev->priv)->
					adsl_mem_info[idx].nCopy = 0;
			}
		}
	}
	return 0;
}

static int
DANUBE_MEI_DFEMemoryAlloc (ifx_adsl_device_t * pDev, unsigned long size)
{
	unsigned long mem_ptr;
	char *org_mem_ptr = NULL;
	int idx = 0;
	long total_size = 0;
	long img_size = size;
	int err = 0;
	smmu_mem_info_t *adsl_mem_info =
		((danube_mei_device_private_t *) pDev->priv)->adsl_mem_info;
	int allocate_size = SDRAM_SEGMENT_SIZE;

	// Alloc Swap Pages
	while (img_size > 0 && idx < MAX_BAR_REGISTERS) {

		if (img_size < SDRAM_SEGMENT_SIZE) {
			allocate_size = img_size;
			if (allocate_size < 1024)
				allocate_size = 1024;
		}

		// skip bar15 for XDATA usage.
#ifndef DFE_LOOPBACK
		if (idx == XDATA_REGISTER)
			idx++;
#endif
		if (idx == MAX_BAR_REGISTERS - 1) {
			//allocate 1MB memory for bar16
#if !defined(__LINUX__)
			org_mem_ptr = kmalloc (img_size + 1024, GFP_ATOMIC);
#else
			org_mem_ptr = kmalloc (img_size, GFP_ATOMIC);
#endif
			mem_ptr =
				(unsigned long) (org_mem_ptr +
						 1023) & 0xFFFFFC00;

			adsl_mem_info[idx].size = img_size;
		}
		else {
#if !defined(__LINUX__)
			org_mem_ptr =
				kmalloc (allocate_size + 1024, GFP_ATOMIC);
#else
			org_mem_ptr = kmalloc (allocate_size, GFP_ATOMIC);
#endif
			mem_ptr =
				(unsigned long) (org_mem_ptr +
						 1023) & 0xFFFFFC00;
			adsl_mem_info[idx].size = allocate_size;
		}
		if (org_mem_ptr == NULL) {
			DANUBE_MEI_EMSG ("kmalloc memory fail!\n");
			err = -ENOMEM;
			goto allocate_error;
		}
		adsl_mem_info[idx].address = (char *) mem_ptr;
		adsl_mem_info[idx].org_address = org_mem_ptr;

		img_size -= allocate_size;
		total_size += allocate_size;
		//printk("alloc memory idx=%d,img_size=%ld,addr=%X\n",idx,img_size,(int)adsl_mem_info[idx].address);
		idx++;
	}
	if (img_size > 0) {
		DANUBE_MEI_EMSG ("Image size is too large!\n");
		err = -EFBIG;
		goto allocate_error;
	}
	err = idx;
	return err;

      allocate_error:
	DANUBE_MEI_DFEMemoryFree (pDev, FREE_ALL);
	return err;
}

/**
 * Program the BAR registers
 *
 * \param 	pDev		the device pointer
 * \param	nTotalBar	The number of bar to program.
 * \ingroup	Internal
 */
static int
DANUBE_MEI_BarUpdate (ifx_adsl_device_t * pDev, int nTotalBar)
{
	int idx = 0;

	for (idx = 0; idx < nTotalBar; idx++) {
		//skip XDATA register
		if (idx == XDATA_REGISTER)
			idx++;
		DANUBE_MEI_LongWordWriteOffset (pDev,
						(u32) MEI_XMEM_BAR_BASE_OFFSET
						+ idx * 4, (((uint32_t)
							     ((danube_mei_device_private_t *) pDev->priv)->adsl_mem_info[idx].address) & 0x0FFFFFFF));
		//DANUBE_MEI_DMSG("BAR%d=%08X, addr=%08X\n",idx,(((uint32_t)adsl_mem_info[idx].address)&0x0FFFFFFF),(((uint32_t)adsl_mem_info[idx].address)));
	}
	for (idx = nTotalBar; idx < MAX_BAR_REGISTERS; idx++) {
		if (idx == XDATA_REGISTER)
			idx++;
		DANUBE_MEI_LongWordWriteOffset (pDev,
						(u32) MEI_XMEM_BAR_BASE_OFFSET
						+ idx * 4, (((uint32_t)
							     ((danube_mei_device_private_t *) pDev->priv)->adsl_mem_info[nTotalBar - 1].address) & 0x0FFFFFFF));
	}

	DANUBE_MEI_LongWordWriteOffset (pDev,
					(u32) MEI_XMEM_BAR_BASE_OFFSET +
					XDATA_REGISTER * 4, (((uint32_t)
							      ((danube_mei_device_private_t *)
							       pDev->priv)->
							      adsl_mem_info
							      [XDATA_REGISTER].
							      address) &
							     0x0FFFFFFF));
	// update MEI_XDATA_BASE_SH
	DANUBE_MEI_LongWordWriteOffset (pDev, (u32) MEI_XDATA_BASE_SH_OFFSET,
					((unsigned
					  long) ((danube_mei_device_private_t
						  *) pDev->priv)->
					 adsl_mem_info[XDATA_REGISTER].
					 address) & 0x0FFFFFFF);
	return MEI_SUCCESS;
}

MEI_ERROR
IFX_ADSL_BSP_FWDownload (struct ifx_adsl_device * pDev,const char *buf,
			 unsigned long size, long *loff, long *current_offset)
{
	ARC_IMG_HDR img_hdr_tmp;

	size_t nRead = 0, nCopy = 0;
	char *mem_ptr;
	ssize_t retval = -ENOMEM;
	int idx = 0;

	if (*loff == 0) {
		if (size < sizeof (img_hdr_tmp)) {
			DANUBE_MEI_EMSG ("Firmware size is too small!\n");
			return retval;
		}
		copy_from_user ((char *) &img_hdr_tmp, buf,
				sizeof (img_hdr_tmp));
		((danube_mei_device_private_t *) pDev->priv)->image_size = le32_to_cpu (img_hdr_tmp.size) + 8;	// header of image_size and crc are not included.
		if (((danube_mei_device_private_t *) pDev->priv)->image_size >
		    1024 * 1024) {
			DANUBE_MEI_EMSG ("Firmware size is too large!\n");
			return retval;
		}
		// check if arc is halt
		DANUBE_MEI_ResetARC (pDev);
		DANUBE_MEI_HaltArc (pDev);

		DANUBE_MEI_DFEMemoryFree (pDev, FREE_ALL);	//free all

		retval = DANUBE_MEI_DFEMemoryAlloc (pDev,
						    ((danube_mei_device_private_t *) pDev->priv)->image_size);
		if (retval < 0) {
			DANUBE_MEI_EMSG ("Error: No memory space left.\n");
			goto error;
		}

		for (idx = 0; idx < retval; idx++) {
			//skip XDATA register
			if (idx == XDATA_REGISTER)
				idx++;
			if (idx * SDRAM_SEGMENT_SIZE <
			    le32_to_cpu (img_hdr_tmp.page[0].p_offset)) {
				((danube_mei_device_private_t *) pDev->priv)->
					adsl_mem_info[idx].type = FREE_RELOAD;
			}
			else {
				((danube_mei_device_private_t *) pDev->priv)->
					adsl_mem_info[idx].type =
					FREE_SHOWTIME;
			}

		}
		((danube_mei_device_private_t *) pDev->priv)->nBar = retval;

		((danube_mei_device_private_t *) pDev->priv)->img_hdr =
			(ARC_IMG_HDR *) ((danube_mei_device_private_t *)
					 pDev->priv)->adsl_mem_info[0].
			address;

#if !defined(__LINUX__)
		((danube_mei_device_private_t *) pDev->priv)->
			adsl_mem_info[XDATA_REGISTER].org_address =
			kmalloc (SDRAM_SEGMENT_SIZE + 1023, GFP_ATOMIC);
#else
		((danube_mei_device_private_t *) pDev->priv)->
			adsl_mem_info[XDATA_REGISTER].org_address =
			kmalloc (SDRAM_SEGMENT_SIZE, GFP_ATOMIC);
#endif
		((danube_mei_device_private_t *) pDev->priv)->
			adsl_mem_info[XDATA_REGISTER].address =
			(char
			 *) ((unsigned
			      long) (((danube_mei_device_private_t *) pDev->
				      priv)->adsl_mem_info[XDATA_REGISTER].
				     org_address + 1023) & 0xFFFFFC00);
		((danube_mei_device_private_t *) pDev->priv)->
			adsl_mem_info[XDATA_REGISTER].size =
			SDRAM_SEGMENT_SIZE;
		if (((danube_mei_device_private_t *) pDev->priv)->
		    adsl_mem_info[XDATA_REGISTER].address == NULL) {
			DANUBE_MEI_EMSG ("kmalloc memory fail!\n");
			retval = -ENOMEM;
			goto error;
		}
		((danube_mei_device_private_t *) pDev->priv)->
			adsl_mem_info[XDATA_REGISTER].type = FREE_RELOAD;
		DANUBE_MEI_BarUpdate (pDev,
				      ((danube_mei_device_private_t *) pDev->
				       priv)->nBar);

	}
	else if (((danube_mei_device_private_t *) pDev->priv)->image_size ==
		 0) {
		DANUBE_MEI_EMSG ("Error: Firmware size=0! \n");
		goto error;
	}
	else {
	}

	nRead = 0;
	while (nRead < size) {
		long offset = ((long) (*loff) + nRead) % SDRAM_SEGMENT_SIZE;
		idx = (((long) (*loff)) + nRead) / SDRAM_SEGMENT_SIZE;
		mem_ptr = (char *)
			KSEG1ADDR ((unsigned
				    long) (((danube_mei_device_private_t *)
					    pDev->priv)->adsl_mem_info[idx].
					   address) + offset);
		if ((size - nRead + offset) > SDRAM_SEGMENT_SIZE)
			nCopy = SDRAM_SEGMENT_SIZE - offset;
		else
			nCopy = size - nRead;
		copy_from_user (mem_ptr, buf + nRead, nCopy);
		for (offset = 0; offset < (nCopy / 4); offset++) {
			((unsigned long *) mem_ptr)[offset] =
				le32_to_cpu (((unsigned long *)
					      mem_ptr)[offset]);
		}
		nRead += nCopy;
		((danube_mei_device_private_t *) pDev->priv)->
			adsl_mem_info[idx].nCopy += nCopy;
	}

	*loff += size;
	*current_offset = size;
	return MEI_SUCCESS;
      error:
	DANUBE_MEI_DFEMemoryFree (pDev, FREE_ALL);

	return MEI_FAILURE;
}

/**
 * MEI interrupt handler
 *
 * \param int1
 * \param void0
 * \param regs	Pointer to the structure of danube mips registers
 * \ingroup	Internal
 */
#if defined(DANUBE_PORT_RTEMS)
static void
DANUBE_MEI_IrqHandle (unsigned long int1)
#else
static void
DANUBE_MEI_IrqHandle (int int1, void *void0, struct pt_regs *regs)
#endif				//defined(DANUBE_PORT_RTEMS)
{
	u32 scratch;
	ifx_adsl_device_t *pDev = (ifx_adsl_device_t *) void0;
#if defined(DFE_LOOPBACK) && defined(DFE_PING_TEST)
	dfe_loopback_irq_handler ();
	return;
#endif //DFE_LOOPBACK

	if (pDev == NULL) {
		printk ("Error: Got Interrupt but pDev is NULL!!!!\n");
	}

	DANUBE_MEI_DebugRead (pDev, ARC_MEI_MAILBOXR, &scratch, 1);
	if (scratch & OMB_CODESWAP_MESSAGE_MSG_TYPE_MASK) {
		DANUBE_MEI_EMSG
			("\n\n Receive Code Swap Request interrupt!!!");
		return;
	}
	else if (scratch & OMB_CLEAREOC_INTERRUPT_CODE)	// clear eoc message interrupt
	{
		ifx_adsl_cb_event_t adsl_event;
		ifx_adsl_cbparam_t event_param;
		int (*ifx_adsl_callback) (ifx_adsl_cb_event_t * param);

		DANUBE_MEI_LongWordWriteOffset (pDev,
						(u32) ARC_TO_MEI_INT_OFFSET,
						ARC_TO_MEI_MSGAV);
		adsl_event.ID = IFX_ADSL_BSP_EVENT_CEOC_IRQ;
		adsl_event.pDev = pDev;
		event_param.irq_message = scratch;
		adsl_event.param = &event_param;

		if (IFX_ADSL_BSP_GetEventCB (&ifx_adsl_callback) == 0) {
			ifx_adsl_callback (&adsl_event);
		}

	}
	else {			// normal message
		DANUBE_MEI_MailboxRead (pDev, ((danube_mei_device_private_t *)
					       pDev->priv)->CMV_RxMsg,
					MSG_LENGTH);
		if (((danube_mei_device_private_t *) pDev->priv)->
		    cmv_waiting == 1) {
			((danube_mei_device_private_t *) pDev->priv)->
				arcmsgav = 1;
			((danube_mei_device_private_t *) pDev->priv)->
				cmv_waiting = 0;
#if !defined(DANUBE_PORT_RTEMS)
			MEI_WAKEUP_EVENT (((danube_mei_device_private_t *)
					   pDev->priv)->wait_queue_arcmsgav);
#endif
		}
		else {
			((danube_mei_device_private_t *) pDev->priv)->
				indicator_count++;
			memcpy ((char *) ((danube_mei_device_private_t *)
					  pDev->priv)->Recent_indicator,
				(char *) ((danube_mei_device_private_t *)
					  pDev->priv)->CMV_RxMsg,
				MSG_LENGTH * 2);
			if (((((danube_mei_device_private_t *) pDev->priv)->CMV_RxMsg[0] & 0xff0) >> 4) == D2H_AUTONOMOUS_MODEM_READY_MSG)	// arc ready
			{	//check ARC ready message

				DANUBE_MEI_EMSG ("Got MODEM_READY_MSG\n");
				((danube_mei_device_private_t *) pDev->priv)->
					modem_ready = 1;
				MEI_WAKEUP_EVENT (((danube_mei_device_private_t *) pDev->priv)->wait_queue_modemready);
			}
		}
	}

	MEI_MASK_AND_ACK_IRQ (pDev->nIrq);
	return;
}

int
IFX_ADSL_BSP_ATMLedCBRegister (int (*ifx_adsl_ledcallback) (void))
{
	return IFX_ATM_LED_Callback_Register (ifx_adsl_ledcallback);
}

int
IFX_ADSL_BSP_ATMLedCBUnregister (int (*ifx_adsl_ledcallback) (void))
{
	return IFX_ATM_LED_Callback_Unregister (ifx_adsl_ledcallback);
}

int
IFX_ADSL_BSP_EventCBRegister (int (*ifx_adsl_callback)
			        (ifx_adsl_cb_event_t * param))
{
	int error = 0;
	if (IFX_ADSL_EventCB == NULL) {
		IFX_ADSL_EventCB = ifx_adsl_callback;
	}
	else {
		error = -EIO;
	}
	return error;
}

int
IFX_ADSL_BSP_EventCBUnregister (int (*ifx_adsl_callback)
				  (ifx_adsl_cb_event_t * param))
{
	int error = 0;
	if (IFX_ADSL_EventCB == ifx_adsl_callback) {
		IFX_ADSL_EventCB = NULL;
	}
	else {
		error = -EIO;
	}
	return error;
}

static int
IFX_ADSL_BSP_GetEventCB (int (**ifx_adsl_callback)
			   (ifx_adsl_cb_event_t * param))
{
	*ifx_adsl_callback = IFX_ADSL_EventCB;
	return 0;
}

//TODO, for loopback test
#ifdef DFE_LOOPBACK
#define mte_reg_base	(0x4800*4+0x20000)

/* Iridia Registers Address Constants */
#define MTE_Reg(r)    	(int)(mte_reg_base + (r*4))

#define IT_AMODE       	MTE_Reg(0x0004)

#define OMBOX_BASE 	0xDF80
#define OMBOX1 	(OMBOX_BASE+0x4)
#define IMBOX_BASE 	0xDFC0

#define TIMER_DELAY   	(1024)
#define BC0_BYTES     	(32)
#define BC1_BYTES     	(30)
#define NUM_MB        	(12)
#define TIMEOUT_VALUE 	2000

static void
BFMWait (u32 cycle)
{
	u32 i;
	for (i = 0; i < cycle; i++);
}

static void
WriteRegLong (u32 addr, u32 data)
{
	//*((volatile u32 *)(addr)) =  data;
	DANUBE_WRITE_REGISTER_L (data, addr);
}

static u32
ReadRegLong (u32 addr)
{
	// u32  rd_val;
	//rd_val = *((volatile u32 *)(addr));
	// return rd_val;
	return DANUBE_READ_REGISTER_L (addr);
}

/* This routine writes the mailbox with the data in an input array */
static void
WriteMbox (u32 * mboxarray, u32 size)
{
	DANUBE_MEI_DebugWrite (&sMEI_devices[0], IMBOX_BASE, mboxarray, size);
	printk ("write to %X\n", IMBOX_BASE);
	DANUBE_MEI_LongWordWriteOffset (&sMEI_devices[0],
					(u32) MEI_TO_ARC_INT_OFFSET,
					MEI_TO_ARC_MSGAV);
}

/* This routine reads the output mailbox and places the results into an array */
static void
ReadMbox (u32 * mboxarray, u32 size)
{
	DANUBE_MEI_DebugRead (&sMEI_devices[0], OMBOX_BASE, mboxarray, size);
	printk ("read from %X\n", OMBOX_BASE);
}

static void
MEIWriteARCValue (u32 address, u32 value)
{
	u32 i, check = 0;

	/* Write address register */
	DANUBE_WRITE_REGISTER_L (address, MEI_DEBUG_WAD);

	/* Write data register */
	DANUBE_WRITE_REGISTER_L (value, MEI_DEBUG_DATA);

	/* wait until complete - timeout at 40 */
	for (i = 0; i < 40; i++) {
		check = DANUBE_READ_REGISTER_L (ARC_TO_MEI_INT);

		if ((check & ARC_TO_MEI_DBG_DONE))
			break;
	}
	/* clear the flag */
	DANUBE_WRITE_REGISTER_L (ARC_TO_MEI_DBG_DONE, ARC_TO_MEI_INT);
}

void
arc_code_page_download (uint32_t arc_code_length, uint32_t * start_address)
{
	int count;
	DANUBE_MEI_DMSG ("try to download pages,size=%d\n", arc_code_length);
	DANUBE_MEI_ControlModeSet (&sMEI_devices[0], MEI_MASTER_MODE);
	DANUBE_MEI_HaltArc (&sMEI_devices[0]);
	DANUBE_MEI_LongWordWriteOffset (&sMEI_devices[0],
					(u32) MEI_XFR_ADDR_OFFSET, 0);
	for (count = 0; count < arc_code_length; count++) {
		DANUBE_MEI_LongWordWriteOffset (&sMEI_devices[0],
						(u32) MEI_DATA_XFR_OFFSET,
						*(start_address + count));
	}
	DANUBE_MEI_ControlModeSet (&sMEI_devices[0], JTAG_MASTER_MODE);
}
static int
load_jump_table (unsigned long addr)
{
	int i;
	uint32_t addr_le, addr_be;
	uint32_t jump_table[32];
	for (i = 0; i < 16; i++) {
		addr_le = i * 8 + addr;
		addr_be = ((addr_le >> 16) & 0xffff);
		addr_be |= ((addr_le & 0xffff) << 16);
		jump_table[i * 2 + 0] = 0x0f802020;
		jump_table[i * 2 + 1] = addr_be;
		//printk("jt %X %08X %08X\n",i,jump_table[i*2+0],jump_table[i*2+1]);
	}
	arc_code_page_download (32, &jump_table[0]);
	return 0;
}

void
dfe_loopback_irq_handler (void)
{
	uint32_t rd_mbox[10];

	memset (&rd_mbox[0], 0, 10 * 4);
	ReadMbox (&rd_mbox[0], 6);
	if (rd_mbox[0] == 0x0) {
		DANUBE_MEI_DMSG ("Get ARC_ACK\n");
		got_int = 1;
	}
	else if (rd_mbox[0] == 0x5) {
		DANUBE_MEI_DMSG ("Get ARC_BUSY\n");
		got_int = 2;
	}
	else if (rd_mbox[0] == 0x3) {
		DANUBE_MEI_DMSG ("Get ARC_EDONE\n");
		if (rd_mbox[1] == 0x0) {
			got_int = 3;
			DANUBE_MEI_DMSG ("Get E_MEMTEST\n");
			if (rd_mbox[2] != 0x1) {
				got_int = 4;
				DANUBE_MEI_DMSG ("Get Result %X\n",
						 rd_mbox[2]);
			}
		}
	}
	DANUBE_MEI_LongWordWriteOffset (&sMEI_devices[0],
					(u32) ARC_TO_MEI_INT_OFFSET,
					ARC_TO_MEI_DBG_DONE);
	MEI_MASK_AND_ACK_IRQ (DANUBE_MEI_INT);
	MEI_DISABLE_IRQ (DANUBE_MEI_INT);
	//got_int = 1;
	return;
}

static void
wait_mem_test_result (void)
{
	uint32_t mbox[5];
	mbox[0] = 0;
	DANUBE_MEI_DMSG ("Waiting Starting\n");
	while (mbox[0] == 0) {
		ReadMbox (&mbox[0], 5);
	}
	DANUBE_MEI_DMSG ("Try to get mem test result.\n");
	ReadMbox (&mbox[0], 5);
	if (mbox[0] == 0xA) {
		DANUBE_MEI_DMSG ("Success.\n");
	}
	else if (mbox[0] == 0xA) {
		DANUBE_MEI_DMSG
			("Fail,address %X,except data %X,receive data %X\n",
			 mbox[1], mbox[2], mbox[3]);
	}
	else {
		DANUBE_MEI_DMSG ("Fail\n");
	}
}

static int
arc_ping_testing (void)
{
#define MEI_PING 0x00000001
	uint32_t wr_mbox[10], rd_mbox[10];
	int i;
	for (i = 0; i < 10; i++) {
		wr_mbox[i] = 0;
		rd_mbox[i] = 0;
	}

	DANUBE_MEI_DMSG ("send ping msg\n");
	wr_mbox[0] = MEI_PING;
	WriteMbox (&wr_mbox[0], 10);

	while (got_int == 0) {
		MEI_WAIT (100);
	}

	DANUBE_MEI_DMSG ("send start event\n");
	got_int = 0;

	wr_mbox[0] = 0x4;
	wr_mbox[1] = 0;
	wr_mbox[2] = 0;
	wr_mbox[3] = (uint32_t) 0xf5acc307e;
	wr_mbox[4] = 5;
	wr_mbox[5] = 2;
	wr_mbox[6] = 0x1c000;
	wr_mbox[7] = 64;
	wr_mbox[8] = 0;
	wr_mbox[9] = 0;
	WriteMbox (&wr_mbox[0], 10);
	MEI_ENABLE_IRQ (DANUBE_MEI_INT);
	//printk("DANUBE_MEI_MailboxWrite ret=%d\n",i);
	DANUBE_MEI_LongWordWriteOffset (pDev, (u32) MEI_TO_ARC_INT_OFFSET,
					MEI_TO_ARC_MSGAV);
	printk ("sleeping\n");
	while (1) {
		if (got_int > 0) {

			if (got_int > 3)
				printk ("got_int >>>> 3\n");
			else
				printk ("got int = %d\n", got_int);
			got_int = 0;
			//schedule();
			MEI_ENABLE_IRQ (DANUBE_MEI_INT);
		}
		//mbox_read(&rd_mbox[0],6);
		MEI_WAIT (100);
	}
}

static MEI_ERROR
DFE_Loopback_Test (void)
{
	int i = 0;
	u32 arc_debug_data = 0, temp;
	ifx_adsl_device_t *pDev = &sMEI_devices[0];

	DANUBE_MEI_ResetARC (pDev);
	// start the clock
	arc_debug_data = ACL_CLK_MODE_ENABLE;
	DANUBE_MEI_DebugWrite (pDev, CRI_CCR0, &arc_debug_data, 1);

#if defined( DFE_PING_TEST )|| defined( DFE_ATM_LOOPBACK)
	// WriteARCreg(AUX_XMEM_LTEST,0);
	DANUBE_MEI_ControlModeSet (pDev, MEI_MASTER_MODE);
#define AUX_XMEM_LTEST 0x128
	_DANUBE_MEI_DBGLongWordWrite (pDev, MEI_DEBUG_DEC_AUX_MASK,
				      AUX_XMEM_LTEST, 0);
	DANUBE_MEI_ControlModeSet (pDev, JTAG_MASTER_MODE);

	// WriteARCreg(AUX_XDMA_GAP,0);
	DANUBE_MEI_ControlModeSet (pDev, MEI_MASTER_MODE);
#define AUX_XDMA_GAP 0x114
	_DANUBE_MEI_DBGLongWordWrite (pDev, MEI_DEBUG_DEC_AUX_MASK,
				      AUX_XDMA_GAP, 0);
	DANUBE_MEI_ControlModeSet (pDev, JTAG_MASTER_MODE);

	DANUBE_MEI_ControlModeSet (pDev, MEI_MASTER_MODE);
	temp = 0;
	_DANUBE_MEI_DBGLongWordWrite (pDev, MEI_DEBUG_DEC_AUX_MASK,
				      (u32) MEI_XDATA_BASE_SH, temp);
	DANUBE_MEI_ControlModeSet (pDev, JTAG_MASTER_MODE);

	i = DANUBE_MEI_DFEMemoryAlloc (pDev, SDRAM_SEGMENT_SIZE * 16);
	if (i >= 0) {
		int idx;

		for (idx = 0; idx < i; idx++) {
			((danube_mei_device_private_t *) pDev->priv)->
				adsl_mem_info[idx].type = FREE_RELOAD;
			DANUBE_WRITE_REGISTER_L ((((uint32_t)
						   ((danube_mei_device_private_t *) pDev->priv)->adsl_mem_info[idx].address) & 0x0fffffff), MEI_XMEM_BAR_BASE + idx * 4);
			DANUBE_MEI_DMSG ("bar%d(%X)=%X\n", idx,
					 MEI_XMEM_BAR_BASE + idx * 4,
					 (((uint32_t)
					   ((danube_mei_device_private_t *)
					    pDev->priv)->adsl_mem_info[idx].
					   address) & 0x0fffffff));
			memset ((u8 *) ((danube_mei_device_private_t *) pDev->
					priv)->adsl_mem_info[idx].address, 0,
				SDRAM_SEGMENT_SIZE);
		}

		DANUBE_MEI_LongWordWriteOffset (pDev, (u32)
						MEI_XDATA_BASE_SH_OFFSET,
						((unsigned
						  long) ((danube_mei_device_private_t *) pDev->priv)->adsl_mem_info[XDATA_REGISTER].address) & 0x0FFFFFFF);

	}
	else {
		DANUBE_MEI_EMSG ("cannot load image: no memory\n\n");
		return MEI_FAILURE;
	}
	//WriteARCreg(AUX_IC_CTRL,2);
	DANUBE_MEI_ControlModeSet (pDev, MEI_MASTER_MODE);
#define AUX_IC_CTRL 0x11
	_DANUBE_MEI_DBGLongWordWrite (pDev, MEI_DEBUG_DEC_AUX_MASK,
				      AUX_IC_CTRL, 2);
	DANUBE_MEI_ControlModeSet (pDev, JTAG_MASTER_MODE);

	DANUBE_MEI_HaltArc (&sMEI_devices[0]);

#ifdef DFE_PING_TEST

	DANUBE_MEI_DMSG ("ping test image size=%d\n", sizeof (code_array));
	memcpy ((u8 *) (((danube_mei_device_private_t *) pDev->priv)->
			adsl_mem_info[0].address + 0x1004), &code_array[0],
		sizeof (code_array));
	load_jump_table (0x80000 + 0x1004);

#endif //DFE_PING_TEST

	DANUBE_MEI_DMSG ("ARC ping test code download complete\n");
#endif //defined( DFE_PING_TEST )|| defined( DFE_ATM_LOOPBACK)
#ifdef DFE_MEM_TEST
	DANUBE_MEI_LongWordWriteOffset (&sMEI_devices[0],
					(u32) ARC_TO_MEI_INT_MASK_OFFSET,
					MSGAV_EN);

	arc_code_page_download (1537, &mem_test_code_array[0]);
	DANUBE_MEI_DMSG ("ARC mem test code download complete\n");
#endif //DFE_MEM_TEST
#ifdef DFE_ATM_LOOPBACK
	arc_debug_data = 0xf;
	arc_code_page_download (1077, &code_array[0]);
	// Start Iridia IT_AMODE (in dmp access) why is it required?
	DANUBE_MEI_DebugWrite (&sMEI_devices[0], 0x32010, &arc_debug_data, 1);
#endif //DFE_ATM_LOOPBACK
	DANUBE_MEI_IRQEnable (pDev);
	Danube_MEI_RunArc (&sMEI_devices[0]);

#ifdef DFE_PING_TEST
	arc_ping_testing ();
#endif //DFE_PING_TEST
#ifdef DFE_MEM_TEST
	wait_mem_test_result ();
#endif //DFE_MEM_TEST

	DANUBE_MEI_DFEMemoryFree (pDev, FREE_ALL);
	return MEI_SUCCESS;
}

#endif //DFE_LOOPBACK
//end of TODO, for loopback test

static int
DANUBE_MEI_InitDevNode (int num)
{
#ifdef CONFIG_DEVFS_FS
	char buf[36];
	sprintf (buf, "%s%d", DANUBE_MEI_DEVNAME, num);
	if ((danube_mei_devfs_handle[num] =
	     devfs_register (NULL, buf, DEVFS_FL_DEFAULT, major, 0,
			     S_IFCHR | S_IRUGO | S_IWUGO,
			     &danube_mei_operations, (void *) 0)) == NULL) {
		DANUBE_MEI_EMSG ("Register mei devfs error.\n");
		return -ENODEV;
	}

#else
	if (num == 0) {
		if ((major =
		     register_chrdev (major, DANUBE_MEI_DEVNAME,
				      &danube_mei_operations)) < 0) {
			DANUBE_MEI_EMSG
				("\n\n unable to register %d for danube_mei!!!",
				 major);
			return -ENODEV;
		}
	}
#endif
	return 0;
}

static int
DANUBE_MEI_CleanUpDevNode (int num)
{
#ifdef CONFIG_DEVFS_FS
	devfs_unregister (danubemei_devfs_handle[num]);
#else
	if (num == 0)
		unregister_chrdev (major, "danube_mei");
#endif
        return 0;
}

static int
DANUBE_MEI_InitDevice (int num)
{
	ifx_adsl_device_t *pDev;
	pDev = &sMEI_devices[num];
	if (pDev == NULL)
		return -ENOMEM;
	pDev->priv = &sDanube_Mei_Private[num];
	memset (pDev->priv, 0, sizeof (danube_mei_device_private_t));

	memset (&((danube_mei_device_private_t *) pDev->priv)->
		adsl_mem_info[0], 0,
		sizeof (smmu_mem_info_t) * MAX_BAR_REGISTERS);

	if (num == 0) {
		u32 pmu_reg = 0;

		pDev->nIrq = DANUBE_MEI_INT;
		pDev->base_address = MEI_SPACE_ACCESS;

		// power up mei
		pmu_reg = DANUBE_READ_REGISTER_L (DANUBE_PMU_PWDCR);
		pmu_reg &= 0xffff7dbe;
		DANUBE_WRITE_REGISTER_L (pmu_reg, DANUBE_PMU_PWDCR);
	}
	pDev->nInUse = 0;
	((danube_mei_device_private_t *) pDev->priv)->modem_ready = 0;
	((danube_mei_device_private_t *) pDev->priv)->arcmsgav = 0;

	MEI_INIT_WAKELIST ("arcq", ((danube_mei_device_private_t *) pDev->priv)->wait_queue_arcmsgav);	// for ARCMSGAV
	MEI_INIT_WAKELIST ("arcr", ((danube_mei_device_private_t *) pDev->priv)->wait_queue_modemready);	// for arc modem ready

	MEI_MUTEX_INIT (((danube_mei_device_private_t *) pDev->priv)->mei_cmv_sema, 1);	// semaphore initialization, mutex

	MEI_DISABLE_IRQ (pDev->nIrq);
	if (REQUEST_IRQ_HANDLER
	    (pDev->nIrq, DANUBE_MEI_IrqHandle, 0, "danube_mei_arcmsgav",
	     pDev) != 0) {
		DANUBE_MEI_EMSG
			("\n\n unable to register irq(%d) for danube_mei!!!",
			 pDev->nIrq);
		return -1;
	}

	return 0;
}

static int
DANUBE_MEI_ExitDevice (int num)
{
	ifx_adsl_device_t *pDev;
	pDev = &sMEI_devices[num];

	if (pDev == NULL)
		return -EIO;

	MEI_DISABLE_IRQ (pDev->nIrq);
	FREE_IRQ_HANDLER (pDev->nIrq, pDev);

	return 0;
}

static ifx_adsl_device_t *
DANUBE_BSP_HandleGet (int maj, int num)
{
	if (num > DANUBE_MAX_DEVICES)
		return NULL;
	return &sMEI_devices[num];
}

ifx_adsl_device_t *
IFX_ADSL_BSP_DriverHandleGet (int maj, int num)
{
	ifx_adsl_device_t *pDev;
	if (num > DANUBE_MAX_DEVICES)
		return NULL;

	pDev = &sMEI_devices[num];
	pDev->nInUse++;
	MOD_INC_USE_COUNT;
	return pDev;
}

int
IFX_ADSL_BSP_DriverHandleDelete (ifx_adsl_device_t * nHandle)
{
	ifx_adsl_device_t *pDev = (ifx_adsl_device_t *) nHandle;
	if (pDev->nInUse) {
		pDev->nInUse--;
		MOD_DEC_USE_COUNT;
	}
	return 0;
}
static int
DANUBE_MEI_Open (MEI_inode_t * ino, MEI_file_t * fil)
{
	int maj = MAJOR (ino->i_rdev);
	int num = MINOR (ino->i_rdev);
	ifx_adsl_device_t *pDev = NULL;
	if ( (pDev = IFX_ADSL_BSP_DriverHandleGet(maj,num))==NULL)
        {
		printk("%s %d:open fail!\n",__FUNCTION__,__LINE__);
		return -EIO;
        }
	fil->private_data = pDev;
	return 0;
}

static int
DANUBE_MEI_Release (MEI_inode_t * ino, MEI_file_t * fil)
{
	//int maj = MAJOR(ino->i_rdev);
	int num = MINOR (ino->i_rdev);
	ifx_adsl_device_t *pDev;

	pDev = &sMEI_devices[num];
	if (pDev == NULL)
		return -EIO;
	IFX_ADSL_BSP_DriverHandleDelete (pDev);
	return 0;
}

/**
 * Callback function for linux userspace program writing
 */
static ssize_t
DANUBE_MEI_Write (MEI_file_t * filp,const char *buf, size_t size, loff_t * loff)
{
	MEI_ERROR mei_error = MEI_FAILURE;
	long offset = 0;
	ifx_adsl_device_t *pDev = (ifx_adsl_device_t *)filp->private_data;
		
	if (pDev == NULL)
		return -EIO;

	mei_error =
		IFX_ADSL_BSP_FWDownload (pDev, buf, size, (long *) loff,
					 &offset);
	if (mei_error == MEI_FAILURE)
		return -EIO;
	return (ssize_t) offset;
}

/**
 * Callback function for linux userspace program ioctling
 */
static int
DANUBE_MEI_IoctlCopyFrom (int from_kernel, char *dest, char *from, int size)
{
	int ret = 0;

	if (!from_kernel)
		ret = copy_from_user ((char *) dest, (char *) from, size);
	else
		ret = memcpy ((char *) dest, (char *) from, size);
	return ret;
}

static int
DANUBE_MEI_IoctlCopyTo (int from_kernel, char *dest, char *from, int size)
{
	int ret = 0;

	if (!from_kernel)
		ret = copy_to_user ((char *) dest, (char *) from, size);
	else
		ret = memcpy ((char *) dest, (char *) from, size);
	return ret;
}

static int
DANUBE_MEI_Ioctls (ifx_adsl_device_t * pDev, int from_kernel,
		   unsigned int command, unsigned long lon)
{
	int meierr = MEI_SUCCESS;

	u16 RxMessage[MSG_LENGTH] __attribute__ ((aligned (4)));
	u16 TxMessage[MSG_LENGTH] __attribute__ ((aligned (4)));
	meidebug debugrdwr;
	int i = 0;
	u32 base_address = MEI_SPACE_ACCESS;
	meireg regrdwr;

	switch (command) {

	case IFX_ADSL_BSP_IOC_CMV_WINHOST:
		DANUBE_MEI_IoctlCopyFrom (from_kernel, (char *) TxMessage,
					  (char *) lon, MSG_LENGTH * 2);

		if ((meierr =
		     IFX_ADSL_BSP_SendCMV (pDev, TxMessage, YES_REPLY,
					   RxMessage)) != MEI_SUCCESS) {
			DANUBE_MEI_EMSG
				("\n\nWINHOST CMV fail :TxMessage:%X %X %X %X, RxMessage:%X %X %X %X %X\n",
				 TxMessage[0], TxMessage[1], TxMessage[2],
				 TxMessage[3], RxMessage[0], RxMessage[1],
				 RxMessage[2], RxMessage[3], RxMessage[4]);
			meierr = MEI_FAILURE;
		}
		else {
			DANUBE_MEI_IoctlCopyTo (from_kernel, (char *) lon,
						(char *) RxMessage,
						MSG_LENGTH * 2);
		}
		break;

	case IFX_ADSL_BSP_IOC_CMV_READ:
		DANUBE_MEI_IoctlCopyFrom (from_kernel, (char *) (&regrdwr),
					  (char *) lon, sizeof (meireg));

		DANUBE_MEI_LongWordRead ((u32) regrdwr.iAddress,
					 (u32 *) & (regrdwr.iData));

		DANUBE_MEI_IoctlCopyTo (from_kernel, (char *) lon,
					(char *) (&regrdwr), sizeof (meireg));

		break;

	case IFX_ADSL_BSP_IOC_CMV_WRITE:
		DANUBE_MEI_IoctlCopyFrom (from_kernel, (char *) (&regrdwr),
					  (char *) lon, sizeof (meireg));

		DANUBE_MEI_LongWordWrite ((u32) regrdwr.iAddress,
					  regrdwr.iData);
		break;

	case IFX_ADSL_IOC_GET_BASE_ADDRESS:
		DANUBE_MEI_IoctlCopyTo (from_kernel, (char *) lon,
					(char *) (&base_address),
					sizeof (base_address));
		break;

	case IFX_ADSL_BSP_IOC_IS_MODEM_READY:
		i = DANUBE_MEI_IsModemReady (pDev);
		DANUBE_MEI_IoctlCopyTo (from_kernel, (char *) lon,
					(char *) (&i), sizeof (int));
		meierr = MEI_SUCCESS;
		break;
	case IFX_ADSL_BSP_IOC_RESET:
	case IFX_ADSL_BSP_IOC_REBOOT:
		meierr = DANUBE_MEI_CpuModeSet (pDev, IFX_ADSL_CPU_RESET);
		meierr = DANUBE_MEI_CpuModeSet (pDev, IFX_ADSL_CPU_HALT);
		break;

	case IFX_ADSL_BSP_IOC_HALT:
		meierr = DANUBE_MEI_CpuModeSet (pDev, IFX_ADSL_CPU_HALT);
		break;

	case IFX_ADSL_BSP_IOC_RUN:
		meierr = DANUBE_MEI_CpuModeSet (pDev, IFX_ADSL_CPU_RUN);
		break;
	case IFX_ADSL_BSP_IOC_BOOTDOWNLOAD:
		meierr = DANUBE_MEI_DownloadBootCode (pDev);
		break;
	case IFX_ADSL_BSP_IOC_JTAG_ENABLE:
		meierr = DANUBE_MEI_ArcJtagEnable (pDev, 1);
		break;

	case IFX_ADSL_BSP_IOC_REMOTE:
		DANUBE_MEI_IoctlCopyFrom (from_kernel, (char *) (&i),
					  (char *) lon, sizeof (int));

		meierr = DANUBE_MEI_AdslMailboxIRQEnable (pDev, i);
		break;

	case IFX_ADSL_BSP_IOC_DSLSTART:
		if ((meierr = DANUBE_MEI_RunAdslModem (pDev)) != MEI_SUCCESS) {
			DANUBE_MEI_EMSG
				("DANUBE_MEI_RunAdslModem() error...");
			meierr = MEI_FAILURE;
		}
		break;

	case IFX_ADSL_BSP_IOC_DEBUG_READ:
	case IFX_ADSL_BSP_IOC_DEBUG_WRITE:
		DANUBE_MEI_IoctlCopyFrom (from_kernel, (char *) (&debugrdwr),
					  (char *) lon, sizeof (debugrdwr));

		if (command == IFX_ADSL_BSP_IOC_DEBUG_READ)
			meierr = IFX_ADSL_BSP_MemoryDebugAccess (pDev,
								 IFX_ADSL_MEMORY_READ,
								 debugrdwr.
								 iAddress,
								 (u32 *)
								 debugrdwr.
								 buffer,
								 debugrdwr.
								 iCount);
		else
			meierr = IFX_ADSL_BSP_MemoryDebugAccess (pDev,
								 IFX_ADSL_MEMORY_WRITE,
								 debugrdwr.
								 iAddress,
								 (u32 *)
								 debugrdwr.
								 buffer,
								 debugrdwr.
								 iCount);

		DANUBE_MEI_IoctlCopyTo (from_kernel, (char *) lon, (char *) (&debugrdwr), sizeof (debugrdwr));	//dying gasp
		break;
	case IFX_ADSL_BSP_IOC_GET_BSP_VERSION:
		DANUBE_MEI_IoctlCopyTo( from_kernel, (char *)lon, (char*)(&danube_mei_version), sizeof(ifx_adsl_bsp_version_t));//dying gasp
		break;

	}
	return meierr;
}

int
IFX_ADSL_BSP_KernelIoctls (ifx_adsl_device_t * pDev, unsigned int command,
			   unsigned long lon)
{
	int error = 0;
	error = DANUBE_MEI_Ioctls (pDev, 1, command, lon);
	return error;
}

static int
DANUBE_MEI_UserIoctls (MEI_inode_t * ino, MEI_file_t * fil,
		       unsigned int command, unsigned long lon)
{
	int error = 0;
	int maj = MAJOR (ino->i_rdev);
	int num = MINOR (ino->i_rdev);
	ifx_adsl_device_t *pDev;

	pDev = DANUBE_BSP_HandleGet (maj, num);
	if (pDev == NULL)
		return -EIO;

	error = DANUBE_MEI_Ioctls (pDev, 0, command, lon);
	return error;
}

////////////////////     procfs debug    ///////////////////////////

#ifdef CONFIG_PROC_FS
/*
 * Register a callback function for linux proc filesystem
 */
static int
DANUBE_MEI_InitProcFS (int num)
{
	struct proc_dir_entry *entry;
	int i = 0;

	ifx_adsl_device_t *pDev;
	pDev = &sMEI_devices[num];
	if (pDev == NULL)
		return -ENOMEM;
	reg_entry_t regs_temp[PROC_ITEMS] =	// Items being debugged
	{
		/*  {   flag,                   name,              description } */
		{NULL, "arcmsgav", "arc to mei message ", 0}
		,
		{NULL, "cmv_reply", "cmv needs reply", 0}
		,
		{NULL, "cmv_waiting", "waiting for cmv reply from arc", 0}
		,
		{NULL, "indicator_count", "ARC to MEI indicator count", 0}
		,
		{NULL, "cmv_count", "MEI to ARC CMVs", 0}
		,
		{NULL, "reply_count", "ARC to MEI Reply", 0}
		,
		{NULL, "Recent_indicator", "most recent indicator", 0}
		,
		{(int *) 7, "meminfo", "Memory Allocation Information", 0},
	};

	regs_temp[0].flag =
		&(((danube_mei_device_private_t *) pDev->priv)->arcmsgav);
	regs_temp[1].flag =
		&(((danube_mei_device_private_t *) pDev->priv)->cmv_reply);
	regs_temp[2].flag =
		&(((danube_mei_device_private_t *) pDev->priv)->cmv_waiting);
	regs_temp[3].flag =
		&(((danube_mei_device_private_t *) pDev->priv)->
		  indicator_count);
	regs_temp[4].flag =
		&(((danube_mei_device_private_t *) pDev->priv)->cmv_count);
	regs_temp[5].flag =
		&(((danube_mei_device_private_t *) pDev->priv)->reply_count);
	regs_temp[6].flag =
		(int *) &(((danube_mei_device_private_t *) pDev->priv)->
			  Recent_indicator);

	memcpy ((char *) regs[num], (char *) regs_temp, sizeof (regs_temp));
	// procfs
	meidir = proc_mkdir (MEI_DIRNAME, &proc_root);
	if (meidir == NULL) {
		DANUBE_MEI_EMSG (": can't create /proc/" MEI_DIRNAME "\n\n");
		return (-ENOMEM);
	}

	for (i = 0; i < NUM_OF_REG_ENTRY; i++) {
		entry = create_proc_entry (regs[num][i].name,
					   S_IWUSR | S_IRUSR | S_IRGRP |
					   S_IROTH, meidir);
		if (entry) {
			regs[num][i].low_ino = entry->low_ino;
			entry->proc_fops = &DANUBE_MEI_ProcOperations;
		}
		else {
			DANUBE_MEI_EMSG (": can't create /proc/" MEI_DIRNAME
					 "/%s\n\n", regs[num][i].name);
			return (-ENOMEM);
		}
	}
	return 0;
}

/*
 * Reading function for linux proc filesystem
 */
static int
DANUBE_MEI_ProcRead (struct file *file, char *buf, size_t nbytes,
		     loff_t * ppos)
{
	int i_ino = (file->f_dentry->d_inode)->i_ino;
	char outputbuf[1024];
	int count = 0;
	int i;
	int num;
	reg_entry_t *current_reg = NULL;
	ifx_adsl_device_t *pDev = NULL;

	for (num = 0; num < DANUBE_MAX_DEVICES; num++) {
		for (i = 0; i < NUM_OF_REG_ENTRY; i++) {
			if (regs[num][i].low_ino == i_ino) {
				current_reg = &regs[num][i];
				pDev = &sMEI_devices[num];
				break;
			}
		}
	}
	if (current_reg == NULL) {
		return -EINVAL;
	}
	else if (current_reg->flag == (int *) 7) {
		if (*ppos > 0)	/* Assume reading completed in previous read */
			return 0;
		count = sprintf (outputbuf,
				 "No           Address     Size\n");
		for (i = 0; i < MAX_BAR_REGISTERS; i++) {
			count += sprintf (outputbuf + count,
					  "BAR[%02d] Addr:0x%08X Size:%lu\n",
					  i,
					  (u32) ((danube_mei_device_private_t
						  *) pDev->priv)->
					  adsl_mem_info[i].address,
					  ((danube_mei_device_private_t *)
					   pDev->priv)->adsl_mem_info[i].
					  size);
			//printk( "BAR[%02d] Addr:0x%08X Size:%d\n",i,adsl_mem_info[i].address,adsl_mem_info[i].size);
		}
		*ppos += count;
	}
	else if (current_reg->flag !=
		 (int *) ((danube_mei_device_private_t *) pDev->priv)->
		 Recent_indicator) {
		if (*ppos > 0)	/* Assume reading completed in previous read */
			return 0;	// indicates end of file
		count = sprintf (outputbuf, "0x%08X\n\n",
				 *(current_reg->flag));
		*ppos += count;
		if (count > nbytes)	/* Assume output can be read at one time */
			return -EINVAL;
	}
	else {
		if ((int) (*ppos) / ((int) 7) == 16)
			return 0;	// indicate end of the message
		count = sprintf (outputbuf, "0x%04X\n\n",
				 *(((u16 *) (current_reg->flag)) +
				   (int) (*ppos) / ((int) 7)));
		*ppos += count;
	}
	if (copy_to_user (buf, outputbuf, count))
		return -EFAULT;
	return count;
}

/*
 * Writing function for linux proc filesystem
 */
static ssize_t
DANUBE_MEI_ProcWrite (struct file *file, const char *buffer, size_t count,
		      loff_t * ppos)
{
	int i_ino = (file->f_dentry->d_inode)->i_ino;
	reg_entry_t *current_reg = NULL;
	int i = 0;
	int num = 0;
	unsigned long newRegValue = 0;
	char *endp = NULL;
	ifx_adsl_device_t *pDev = NULL;

	for (num = 0; num < DANUBE_MAX_DEVICES; num++) {
		for (i = 0; i < NUM_OF_REG_ENTRY; i++) {
			if (regs[num][i].low_ino == i_ino) {
				current_reg = &regs[num][i];
				pDev = &sMEI_devices[num];
				break;
			}
		}
	}
	if ((current_reg == NULL)
	    || (current_reg->flag ==
		(int *) ((danube_mei_device_private_t *) pDev->priv)->
		Recent_indicator))
		return -EINVAL;

	newRegValue = simple_strtoul (buffer, &endp, 0);
	*(current_reg->flag) = (int) newRegValue;
	return (count + endp - buffer);
}
#endif //CONFIG_PROC_FS

////////////////////////////////////////////////////////////////////////////
/*
 * Writing function for linux proc filesystem
 */
int __init
DANUBE_MEI_ModuleInit (void)
{
	int i = 0;

	printk("Danube MEI version:%d.%02d.%02d\n", danube_mei_version.major,danube_mei_version.minor,danube_mei_version.revision);


	for (i = 0; i < DANUBE_MAX_DEVICES; i++) {
		if (DANUBE_MEI_InitDevice (i) != 0) {
			printk ("%s: Init device fail!\n", __FUNCTION__);
			return -EIO;
		}
#if defined(__LINUX__)
		DANUBE_MEI_InitDevNode (i);
#ifdef CONFIG_PROC_FS
		DANUBE_MEI_InitProcFS (i);
#endif //CONFIG_PROC_FS
#endif
	}

#ifdef DFE_LOOPBACK
	DFE_Loopback_Test ();
#endif //DFE_LOOPBACK

	return 0;
}

void __exit
DANUBE_MEI_ModuleExit (void)
{
	int i = 0;
	int num;

#if defined(__LINUX__)
	for (num = 0; num < DANUBE_MAX_DEVICES; num++) {
		DANUBE_MEI_CleanUpDevNode (num);
#ifdef CONFIG_PROC_FS
		for (i = 0; i < NUM_OF_REG_ENTRY; i++) {
			remove_proc_entry (regs[num][i].name, meidir);
		}
#endif
	}

	remove_proc_entry (MEI_DIRNAME, &proc_root);
#endif //CONFIG_PROC_FS
	for (i = 0; i < DANUBE_MAX_DEVICES; i++) {
		for (i = 0; i < DANUBE_MAX_DEVICES; i++) {
			DANUBE_MEI_ExitDevice (i);
		}
	}
}

/* export function for DSL Driver */
// provide a register/unregister function for DSL driver to register a event callback function
EXPORT_SYMBOL (IFX_ADSL_BSP_EventCBRegister);
EXPORT_SYMBOL (IFX_ADSL_BSP_EventCBUnregister);

/* The functions of MEI_DriverHandleGet and MEI_DriverHandleDelete are 
something like open/close in kernel space , where the open could be used 
to register a callback for autonomous messages and returns a mei driver context pointer (comparable to the file descriptor in user space) 
EXPORT_SYMBOL (IFX_ADSL_BSP_DriverHandleGet);
EXPORT_SYMBOL (IFX_ADSL_BSP_DriverHandleDelete);
EXPORT_SYMBOL (IFX_ADSL_BSP_ATMLedCBRegister);
EXPORT_SYMBOL (IFX_ADSL_BSP_ATMLedCBUnregister);
EXPORT_SYMBOL (IFX_ADSL_BSP_KernelIoctls);
EXPORT_SYMBOL (IFX_ADSL_BSP_AdslLedInit);
EXPORT_SYMBOL (IFX_ADSL_BSP_AdslLedSet);
EXPORT_SYMBOL (IFX_ADSL_BSP_FWDownload);
EXPORT_SYMBOL (IFX_ADSL_BSP_Showtime);

EXPORT_SYMBOL (IFX_ADSL_BSP_MemoryDebugAccess);
EXPORT_SYMBOL (IFX_ADSL_BSP_SendCMV);

module_init (DANUBE_MEI_ModuleInit);
module_exit (DANUBE_MEI_ModuleExit);
