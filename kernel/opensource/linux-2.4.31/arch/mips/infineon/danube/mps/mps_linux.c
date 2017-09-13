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

#ifdef CONFIG_DEBUG_MINI_BOOT
#define IKOS_MINI_BOOT
#endif

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/vmalloc.h>
#include <linux/ioctl.h>
#include <linux/version.h>
#include <linux/delay.h>

#include <asm/irq.h>
#include <asm/danube/irq.h>

#include <asm/danube/ifx_types.h>
#include <asm/danube/danube.h>

#include <asm/danube/mps.h>
#include "mps_device.h"

#ifdef CONFIG_DEVFS_FS
#include <linux/devfs_fs_kernel.h>
#else
typedef void *devfs_handle_t;
#endif

/* external function declaration */
extern void ifx_mps_ad0_irq (int irq, void *pDev, struct pt_regs *regs);
extern void ifx_mps_ad1_irq (int irq, void *pDev, struct pt_regs *regs);
extern void ifx_mps_vc_irq (int irq, void *pDev, struct pt_regs *regs);

/* local function declaration */
s32 ifx_mps_register_data_callback (mps_devices type, u32 dir,
				    void (*callback) (mps_devices type));
s32 ifx_mps_unregister_data_callback (mps_devices type, u32 dir);
s32 ifx_mps_register_event_callback (mps_devices type, MbxEventRegs_s * mask,
				     void (*callback) (MbxEventRegs_s *
						       events));
s32 ifx_mps_unregister_event_callback (mps_devices type);
s32 ifx_mps_read_mailbox (mps_devices type, mps_message * rw);
s32 ifx_mps_write_mailbox (mps_devices type, mps_message * rw);
s32 ifx_mps_open (struct inode *inode, struct file *file_p);
s32 ifx_mps_close (struct inode *inode, struct file *filp);
s32 ifx_mps_ioctl (struct inode *inode, struct file *file_p, u32 nCmd,
		   unsigned long arg);
static unsigned int ifx_mps_poll (struct file *file_p, poll_table * wait);

#ifdef MODULE
MODULE_AUTHOR ("Infineon Technologies AG");
MODULE_DESCRIPTION ("MPS/DSP driver for DANUBE");
MODULE_SUPPORTED_DEVICE ("DANUBE MIPS24KEc");
MODULE_LICENSE ("GPL");
#endif

static u8 ifx_mps_major_id = 0;
MODULE_PARM (ifx_mps_major_id, "b");
MODULE_PARM_DESC (ifx_mps_major_id, "Major ID of device");

char ifx_mps_dev_name[10];
#define IFX_MPS_DEV_NAME       "ifx_mps"
#define IFX_MPS_VER_STR        "1.1.0"
#define IFX_MPS_INFO_STR \
"@(#)DANUBE MIPS24KEc MPS mailbox driver, Version "IFX_MPS_VER_STR
char voice_channel_int_name[4][15];

/* the driver callbacks */
static struct file_operations ifx_mps_fops = {
      owner:
	THIS_MODULE,
      poll:
	ifx_mps_poll,
      ioctl:
	ifx_mps_ioctl,
      open:
	ifx_mps_open,
      release:
	ifx_mps_close
};

/* device structure */
mps_comm_dev ifx_mps_dev = { };

#ifdef CONFIG_PROC_FS
static struct proc_dir_entry *ifx_mps_proc_dir;
static struct proc_dir_entry *ifx_mps_proc_file;

#ifdef CONFIG_DANUBE_MPS_PROC_DEBUG
#define MPS_FIRMWARE_BUFFER_SIZE 512*1024
#define MPS_FW_START_TAG  "IFX-START-FW-NOW"
#define MPS_FW_INIT_TAG   "IFX-INITIALIZE-VCPU-HARDWARE"
#define MPS_FW_BUFFER_TAG "IFX-PROVIDE-BUFFERS"
#define MPS_FW_OPEN_VOICE_TAG "IFX-OPEN-VOICE0-MBX"
#define MPS_FW_REGISTER_CALLBACK_TAG "IFX-REGISTER-CALLBACK-VOICE0"
#define MPS_FW_SEND_MESSAGE_TAG "IFX-SEND-MESSAGE-VOICE0"
#define MPS_FW_RESTART_TAG "IFX-RESTART-VCPU-NOW"
#define MPS_FW_ENABLE_PACKET_LOOP_TAG "IFX-ENABLE-PACKET-LOOP"
#define MPS_FW_DISABLE_PACKET_LOOP_TAG "IFX-DISABLE-PACKET-LOOP"
static char ifx_mps_firmware_buffer[MPS_FIRMWARE_BUFFER_SIZE];
static mps_fw ifx_mps_firmware_struct;
static int ifx_mps_firmware_buffer_pos = 0;
static int ifx_mps_packet_loop = 0;
static u32 ifx_mps_rtp_voice_data_count = 0;

#endif /* CONFIG_DANUBE_MPS_PROC_DEBUG */

#endif /* CONFIG_PROC_FS */

static char ifx_mps_device_version[] = IFX_MPS_VER_STR;

#define DANUBE_MPS_VOICE_STATUS_CLEAR 0xC3FFFFFF

#ifdef CONFIG_DANUBE_MPS_PROC_DEBUG
/**
 * Dummy data callback
 * For test purposes this dummy receive handler function can be used.
 * 
 * \param   type     DSP device entity ( 1 - command, 2 - voice0, 3 - voice1, 
 *                   4 - voice2, 5 - voice3 )
 * \ingroup Internal
 */
void
ifx_mbx_dummy_rcv_data_callback (mps_devices type)
{
	mps_message rw;

	memset (&rw, 0, sizeof (mps_message));
	if (ifx_mps_read_mailbox (type, &rw) != OK) {
		printk ("ifx_mps_read_mailbox failed\n");
		return;
	}

	switch (rw.cmd_type) {
	case CMD_RTP_VOICE_DATA_PACKET:
		ifx_mps_rtp_voice_data_count++;
		if (ifx_mps_rtp_voice_data_count % 10000 == 0) {
			printk ("%s - %d packets received\n", __FUNCTION__,
				ifx_mps_rtp_voice_data_count);
		}
		if (ifx_mps_packet_loop == 1) {
			rw.cmd_type = CMD_RTP_VOICE_DATA_PACKET;
			rw.RTP_PaylOffset = 0x00000000;
			ifx_mps_write_mailbox (2, &rw);
		}
		else
			kfree ((char *) KSEG0ADDR (rw.pData));
		break;
	case CMD_ADDRESS_PACKET:
		kfree ((char *) KSEG0ADDR (rw.pData));
		break;
	case CMD_VOICEREC_STATUS_PACKET:
	case CMD_VOICEREC_DATA_PACKET:
	case CMD_RTP_EVENT_PACKET:
	case CMD_FAX_DATA_PACKET:
	case CMD_FAX_STATUS_PACKET:
	case CMD_P_PHONE_DATA_PACKET:
	case CMD_P_PHONE_STATUS_PACKET:
	case CMD_CID_DATA_PACKET:
		printk ("%s - unexpected packet (%x)\n", __FUNCTION__,
			rw.cmd_type);
		break;
	default:
		printk ("%s - received unknown packet (%x)\n", __FUNCTION__,
			rw.cmd_type);
		break;
	}
	return;
}
#endif /* CONFIG_DANUBE_MPS_PROC_DEBUG */

/**
 * Open MPS device.
 * Open the device from user mode (e.g. application) or kernel mode. An inode 
 * value of 1..5 indicates a kernel mode access. In such a case the inode value
 * is used as minor ID.
 * 
 * \param   inode   Pointer to device inode
 * \param   file_p  Pointer to file descriptor
 * \return  0       OK, device opened
 * \return  EMFILE  Device already open
 * \return  EINVAL  Invalid minor ID
 * \ingroup API
 */
s32
ifx_mps_open (struct inode * inode, struct file * file_p)
{
	mps_comm_dev *pDev = &ifx_mps_dev;
	mps_mbx_dev *pMBDev;
	bool_t bcommand = FALSE;
	int from_kernel = 0;
	mps_devices num;

	/* Check whether called from user or kernel mode */
	if ((inode == (struct inode *) 1) || (inode == (struct inode *) 2)
	    || (inode == (struct inode *) 3) || (inode == (struct inode *) 4)
	    || (inode == (struct inode *) 5)) {
		from_kernel = 1;
		num = (int) inode;
	}
	else {
		num = (mps_devices) MINOR (inode->i_rdev);	/* the real device */
	}

	/* check the device number */
	switch (num) {
	case command:
		pMBDev = &(pDev->command_mb);
		bcommand = TRUE;
		break;
	case voice0:
		pMBDev = &(pDev->voice_mb[0]);
		break;
	case voice1:
		pMBDev = &pDev->voice_mb[1];
		break;
	case voice2:
		pMBDev = &pDev->voice_mb[2];
		break;
	case voice3:
		pMBDev = &pDev->voice_mb[3];
		break;
	default:
		printk ("IFX_MPS ERROR: max. device number exceed!\n");
		return -EINVAL;
	}

	if ((OK) == ifx_mps_common_open (pDev, pMBDev, bcommand)) {
		if (!from_kernel) {
			/* installation was successfull */
			/* and use filp->private_data to point to the device data */
			file_p->private_data = pMBDev;
#ifdef MODULE
			/* increment module use counter */
			MOD_INC_USE_COUNT;
#endif

		}
		return 0;
	}
	else {
		/* installation failed */
		printk ("IFX_MPS ERROR: Device is already open!\n");
		return -EMFILE;
	}

}

/**
 * Close MPS device.
 * Close the device from user mode (e.g. application) or kernel mode. An inode 
 * value of 1..5 indicates a kernel mode access. In such a case the inode value
 * is used as minor ID.
 * 
 * \param   inode   Pointer to device inode
 * \param   filp    Pointer to file descriptor
 * \return  0       OK, device closed
 * \return  ENODEV  Device invalid
 * \return  EINVAL  Invalid minor ID
 * \ingroup API
 */
s32
ifx_mps_close (struct inode * inode, struct file * filp)
{
	mps_mbx_dev *pMBDev;
	int from_kernel = 0;

	/* Check whether called from user or kernel mode */
	if ((inode == (struct inode *) 1) || (inode == (struct inode *) 2) ||
	    (inode == (struct inode *) 3) || (inode == (struct inode *) 4) ||
	    (inode == (struct inode *) 5)) {
		from_kernel = 1;
		switch ((int) inode) {
		case command:
			pMBDev = &ifx_mps_dev.command_mb;
			break;
		case voice0:
			pMBDev = &ifx_mps_dev.voice_mb[0];
			break;
		case voice1:
			pMBDev = &ifx_mps_dev.voice_mb[1];
			break;
		case voice2:
			pMBDev = &ifx_mps_dev.voice_mb[2];
			break;
		case voice3:
			pMBDev = &ifx_mps_dev.voice_mb[3];
			break;
		default:
			return (-EINVAL);
		}
	}
	else {
		pMBDev = filp->private_data;
	}

	if (NULL != pMBDev) {
		/* device is still available */
		ifx_mps_common_close (pMBDev);

#ifdef MODULE
		if (!from_kernel) {
			/* increment module use counter */
			MOD_DEC_USE_COUNT;
		}
#endif
		return 0;
	}
	else {
		/* something went totally wrong */
		printk ("IFX_MPS ERROR: pMBDev pointer is NULL!\n");
		return -ENODEV;
	}
}

/**
 * Poll handler.
 * The select function of the driver. A user space program may sleep until
 * the driver wakes it up.
 *
 * \param   file_p  File structure of device
 * \param   wait    Internal table of poll wait queues
 * \return  mask    If new data is available the POLLPRI bit is set, 
 *                  triggering an exception indication. If the device pointer
 *                  is null POLLERR is set.
 * \ingroup API  
 */
static u32
ifx_mps_poll (struct file *file_p, poll_table * wait)
{
	mps_mbx_dev *pMBDev = file_p->private_data;
	unsigned int mask;

	/* add to poll queue */
	poll_wait (file_p, &(pMBDev->mps_wakeuplist), wait);

	mask = 0;

	/* upstream queue */
	if (*pMBDev->upstrm_fifo.pwrite_off != *pMBDev->upstrm_fifo.pread_off) {
		if ((pMBDev->devID == command)
		    || (sem_getcount (pMBDev->wakeup_pending) == 0)) {
			/* queue is not empty */
			mask = POLLIN | POLLRDNORM;
		}
	}

	/* downstream queue */
	if (ifx_mps_fifo_mem_available (&pMBDev->dwstrm_fifo) != 0) {
		/* queue is not full */
		mask |= POLLOUT | POLLWRNORM;
	}
	if ((ifx_mps_dev.event.MPS_Ad0Reg.val & pMBDev->event_mask.MPS_Ad0Reg.
	     val)
	    || (ifx_mps_dev.event.MPS_Ad1Reg.val & pMBDev->event_mask.
		MPS_Ad1Reg.val)
	    || (ifx_mps_dev.event.MPS_VCStatReg[0].val & pMBDev->event_mask.
		MPS_VCStatReg[0].val)
	    || (ifx_mps_dev.event.MPS_VCStatReg[1].val & pMBDev->event_mask.
		MPS_VCStatReg[1].val)
	    || (ifx_mps_dev.event.MPS_VCStatReg[2].val & pMBDev->event_mask.
		MPS_VCStatReg[2].val)
	    || (ifx_mps_dev.event.MPS_VCStatReg[3].val & pMBDev->event_mask.
		MPS_VCStatReg[3].val)) {
		mask |= POLLPRI;
	}
	return mask;
}

/**
 * MPS IOCTL handler.
 * An inode value of 1..5 indicates a kernel mode access. In such a case the 
 * inode value is used as minor ID.
 * The following IOCTLs are supported for the MPS device.
 * - #FIO_MPS_EVENT_REG
 * - #FIO_MPS_EVENT_UNREG
 * - #FIO_MPS_MB_READ
 * - #FIO_MPS_MB_WRITE
 * - #FIO_MPS_DOWNLOAD
 * - #FIO_MPS_GETVERSION
 * - #FIO_MPS_MB_RST_QUEUE
 * - #FIO_MPS_RESET
 * - #FIO_MPS_RESTART
 * - #FIO_MPS_GET_STATUS
 *
 * If MPS_FIFO_BLOCKING_WRITE is defined the following commands are also 
 * available.
 * - #FIO_MPS_TXFIFO_SET
 * - #FIO_MPS_TXFIFO_GET
 *
 * \param   inode        Inode of device 
 * \param   file_p       File structure of device
 * \param   nCmd         IOCTL command 
 * \param   arg          Argument for some IOCTL commands
 * \return  0            Setting the LED bits was successfull
 * \return  -EINVAL      Invalid minor ID
 * \return  -ENOIOCTLCMD Invalid command
 * \ingroup API  
 */
s32
ifx_mps_ioctl (struct inode * inode, struct file * file_p,
	       u32 nCmd, unsigned long arg)
{
	int retvalue = -EINVAL;
	mps_message rw_struct;
	mps_mbx_dev *pMBDev;
	int from_kernel = 0;

	if ((inode == (struct inode *) 1) || (inode == (struct inode *) 2) ||
	    (inode == (struct inode *) 3) || (inode == (struct inode *) 4) ||
	    (inode == (struct inode *) 5)) {
		from_kernel = 1;
		switch ((int) inode) {
		case command:
			pMBDev = &ifx_mps_dev.command_mb;
			break;
		case voice0:
			pMBDev = &ifx_mps_dev.voice_mb[0];
			break;
		case voice1:
			pMBDev = &ifx_mps_dev.voice_mb[1];
			break;
		case voice2:
			pMBDev = &ifx_mps_dev.voice_mb[2];
			break;
		case voice3:
			pMBDev = &ifx_mps_dev.voice_mb[3];
			break;
		default:
			return (-EINVAL);
		}
	}
	else {
		pMBDev = file_p->private_data;
	}

	switch (nCmd) {
	case FIO_MPS_EVENT_REG:
		{
			MbxEventRegs_s events;
			copy_from_user ((char *) &events, (char *) arg,
					sizeof (MbxEventRegs_s));
			retvalue =
				ifx_mps_register_event_callback (pMBDev->
								 devID,
								 &events,
								 NULL);
			if (retvalue == OK) {
				retvalue =
					ifx_mps_event_activation (pMBDev->
								  devID,
								  &events);
			}
			break;
		}
	case FIO_MPS_EVENT_UNREG:
		{
			MbxEventRegs_s events;
			events.MPS_Ad0Reg.val = 0;
			events.MPS_Ad1Reg.val = 0;
			events.MPS_VCStatReg[0].val = 0;
			events.MPS_VCStatReg[1].val = 0;
			events.MPS_VCStatReg[2].val = 0;
			events.MPS_VCStatReg[3].val = 0;
			ifx_mps_event_activation (pMBDev->devID, &events);
			retvalue =
				ifx_mps_unregister_event_callback (pMBDev->
								   devID);
			break;
		}
	case FIO_MPS_MB_READ:
		/* Read the data from mailbox stored in local FIFO */
		printk (KERN_DEBUG "IFX_MPS: ioctl MB read\n");

		if (from_kernel) {
			retvalue =
				ifx_mps_mbx_read (pMBDev, (mps_message *) arg,
						  0);
		}
		else {
			u32 *pUserBuf;
			/* Initialize destination and copy mps_message from usermode */
			memset (&rw_struct, 0, sizeof (mps_message));
			copy_from_user ((char *) &rw_struct, (char *) arg,
					sizeof (mps_message));

			pUserBuf = (u32 *) rw_struct.pData;	/* Remember usermode buffer */

			/* read data from from upstream mailbox FIFO */
			retvalue = ifx_mps_mbx_read (pMBDev, &rw_struct, 0);
			if (retvalue)
				return -ENOMSG;

			/* Copy data to usermode buffer... */
			copy_to_user ((char *) pUserBuf,
				      (char *) rw_struct.pData,
				      rw_struct.nDataBytes);
			ifx_mps_bufman_free (rw_struct.pData);

			/* ... and finally restore the buffer pointer and copy mps_message back! */
			rw_struct.pData = (u8 *) pUserBuf;
			copy_to_user ((char *) arg, (char *) &rw_struct,
				      sizeof (mps_message));
		}
		break;

	case FIO_MPS_MB_WRITE:
		/* Write data to send to the mailbox into the local FIFO */
		printk (KERN_DEBUG "IFX_MPS: ioctl MB write\n");

		if (from_kernel) {
			if (pMBDev->devID == command) {
				return (ifx_mps_mbx_write_cmd
					(pMBDev, (mps_message *) arg));
			}
			else {
				return (ifx_mps_mbx_write_data
					(pMBDev, (mps_message *) arg));
			}
		}
		else {
			u32 *pUserBuf;
			copy_from_user ((char *) &rw_struct, (char *) arg,
					sizeof (mps_message));

			/* Remember usermode buffer */
			pUserBuf = (u32 *) rw_struct.pData;
			/* Allocate kernelmode buffer for writing data */
			rw_struct.pData =
				ifx_mps_bufman_malloc (rw_struct.nDataBytes,
						       GFP_KERNEL);
			if (rw_struct.pData == NULL) {
				return (-ENOMEM);
			}

			/* copy data to kernelmode buffer and write to mailbox FIFO */
			copy_from_user ((char *) rw_struct.pData,
					(char *) pUserBuf,
					rw_struct.nDataBytes);

			if (pMBDev->devID == command) {
				retvalue =
					ifx_mps_mbx_write_cmd (pMBDev,
							       &rw_struct);
				ifx_mps_bufman_free (rw_struct.pData);
			}
			else {
				retvalue =
					ifx_mps_mbx_write_data (pMBDev,
								&rw_struct);
			}
			/* ... and finally restore the buffer pointer and copy mps_message back! */
			rw_struct.pData = (u8 *) pUserBuf;
			copy_to_user ((char *) arg, (char *) &rw_struct,
				      sizeof (mps_message));
		}
		break;
	case FIO_MPS_DOWNLOAD:
		/* Download firmware file */
		ifx_mps_init_gpt_danube ();
		if (pMBDev->devID == command) {
			if (from_kernel) {
				//printk("IFX_MPS: Download firmware (size %d bytes)... ", ((mps_fw*)arg)->length);

				retvalue =
					ifx_mps_download_firmware (pMBDev,
								   (mps_fw *)
								   arg);
			}
			else {
				u32 *pUserBuffer;
				mps_fw dwnld_struct;

				copy_from_user ((char *) &dwnld_struct,
						(char *) arg,
						sizeof (mps_fw));
				//printk("IFX_MPS: Download firmware (size %d bytes)... ", dwnld_struct.length);
				pUserBuffer = dwnld_struct.data;
				dwnld_struct.data =
					vmalloc (dwnld_struct.length);
				retvalue =
					copy_from_user ((char *) dwnld_struct.
							data,
							(char *) pUserBuffer,
							dwnld_struct.length);
				retvalue =
					ifx_mps_download_firmware (pMBDev,
								   &dwnld_struct);
				vfree (dwnld_struct.data);
			}
			if (retvalue != 0) {
				printk (" error (%i)!\n", retvalue);
			}
			else {
				retvalue = ifx_mps_print_fw_version ();
				ifx_mps_bufman_init ();
			}
		}
		else {
			retvalue = -EINVAL;
		}
		break;

	case FIO_MPS_GETVERSION:
		if (from_kernel) {
			memcpy ((char *) arg, (char *) ifx_mps_device_version,
				strlen (ifx_mps_device_version));
		}
		else {
			copy_to_user ((char *) arg,
				      (char *) ifx_mps_device_version,
				      strlen (ifx_mps_device_version));
		}
		retvalue = OK;
		break;

	case FIO_MPS_RESET:
		/* Reset of the DSP */
		retvalue = ifx_mps_reset ();
		break;

	case FIO_MPS_RESTART:
		/* Restart of the DSP */
		retvalue = ifx_mps_restart ();
		if (retvalue == 0)
			ifx_mps_bufman_init ();
		break;

#ifdef MPS_FIFO_BLOCKING_WRITE
	case FIO_MPS_TXFIFO_SET:
		/* Set the mailbox TX FIFO blocking mode */

		if (pMBDev->devID == command) {
			retvalue = -EINVAL;	/* not supported for this command MB */
		}
		else {
			if (arg > 0) {
				pMBDev->bBlockWriteMB = TRUE;
			}
			else {
				pMBDev->bBlockWriteMB = FALSE;
				Sem_Unlock (pMBDev->sem_write_fifo);
			}
			retvalue = OK;
		}
		break;

	case FIO_MPS_TXFIFO_GET:
		/* Get the mailbox TX FIFO to blocking */

		if (pMBDev->devID == command) {
			retvalue = -EINVAL;
		}
		else {
			if (!from_kernel) {
				copy_to_user ((char *) arg,
					      (char *) &pMBDev->bBlockWriteMB,
					      sizeof (bool_t));
			}
			retvalue = OK;
		}
		break;
#endif /* MPS_FIFO_BLOCKING_WRITE */

	case FIO_MPS_GET_STATUS:
		{
			u32 flags;

			save_and_cli (flags);
			/* get the status of the channel */
			if (!from_kernel)
				copy_to_user ((char *) arg,
					      (char *) &(ifx_mps_dev.event),
					      sizeof (MbxEventRegs_s));
			if (pMBDev->devID == command) {
				ifx_mps_dev.event.MPS_Ad0Reg.val = 0;
				ifx_mps_dev.event.MPS_Ad1Reg.val = 0;
			}
			else {
				switch (pMBDev->devID) {
				case voice0:
					ifx_mps_dev.event.MPS_VCStatReg[0].
						val = 0;
					break;
				case voice1:
					ifx_mps_dev.event.MPS_VCStatReg[1].
						val = 0;
					break;
				case voice2:
					ifx_mps_dev.event.MPS_VCStatReg[2].
						val = 0;
					break;
				case voice3:
					ifx_mps_dev.event.MPS_VCStatReg[3].
						val = 0;
					break;
				}
			}
			restore_flags (flags);
			retvalue = OK;
			break;
		}
	default:
		printk ("IFX_MPS_Ioctl: Invalid IOCTL handle %d passed.\n",
			nCmd);
		retvalue = -ENOIOCTLCMD;
		break;
	}
	return retvalue;
}

/**
 * Register data callback.
 * Allows the upper layer to register a callback function either for
 * downstream (tranmsit mailbox space available) or for upstream (read data 
 * available)
 * 
 * \param   type     DSP device entity ( 1 - command, 2 - voice0, 3 - voice1, 
 *                   4 - voice2, 5 - voice3 )
 * \param   dir      Direction (1 - upstream, 2 - downstream)
 * \param   callback Callback function to register
 * \return  0        OK, callback registered successfully
 * \return  ENXIO    Wrong DSP device entity (only 1-5 supported)
 * \return  EBUSY    Callback already registered
 * \return  EINVAL   Callback parameter null
 * \ingroup API  
 */
s32
ifx_mps_register_data_callback (mps_devices type, u32 dir,
				void (*callback) (mps_devices type))
{
	mps_mbx_dev *pMBDev;

	if (callback == NULL) {
		return (-EINVAL);
	}

	/* Get corresponding mailbox device structure */
	switch (type) {
	case command:
		pMBDev = &ifx_mps_dev.command_mb;
		break;
	case voice0:
		pMBDev = &ifx_mps_dev.voice_mb[0];
		break;
	case voice1:
		pMBDev = &ifx_mps_dev.voice_mb[1];
		break;
	case voice2:
		pMBDev = &ifx_mps_dev.voice_mb[2];
		break;
	case voice3:
		pMBDev = &ifx_mps_dev.voice_mb[3];
		break;
	default:
		return (-ENXIO);
	}

	/* Enter the desired callback function */
	switch (dir) {
	case 1:		/* register upstream callback function */
		if (pMBDev->up_callback != NULL) {
			return (-EBUSY);
		}
		else {
			pMBDev->up_callback = callback;
		}
		break;
	case 2:		/* register downstream callback function */
		if (pMBDev->down_callback != NULL) {
			return (-EBUSY);
		}
		else {
			pMBDev->down_callback = callback;
		}
		break;
	default:
		break;
	}
	return (OK);
}

/**
 * Unregister data callback.
 * Allows the upper layer to unregister the callback function previously
 * registered.
 * 
 * \param   type   DSP device entity ( 1 - command, 2 - voice0, 3 - voice1,
 *                 4 - voice2, 5 - voice3 )
 * \param   dir    Direction (1 - upstream, 2 - downstream)
 * \return  0      OK, callback registered successfully
 * \return  ENXIO  Wrong DSP device entity (only 1-5 supported)
 * \return  EINVAL Nothing to unregister
 * \return  EINVAL Callback value null
 * \ingroup API  
 */
s32
ifx_mps_unregister_data_callback (mps_devices type, u32 dir)
{
	mps_mbx_dev *pMBDev;
	/* Get corresponding mailbox device structure */
	switch (type) {
	case command:
		pMBDev = &ifx_mps_dev.command_mb;
		break;
	case voice0:
		pMBDev = &ifx_mps_dev.voice_mb[0];
		break;
	case voice1:
		pMBDev = &ifx_mps_dev.voice_mb[1];
		break;
	case voice2:
		pMBDev = &ifx_mps_dev.voice_mb[2];
		break;
	case voice3:
		pMBDev = &ifx_mps_dev.voice_mb[3];
		break;
	default:
		return (-ENXIO);
	}
	/* Delete the desired callback function */
	switch (dir) {
	case 1:
		if (pMBDev->up_callback == NULL) {
			return (-EINVAL);
		}
		else {
			pMBDev->up_callback = NULL;
		}
		break;
	case 2:
		if (pMBDev->down_callback == NULL) {
			return (-EINVAL);
		}
		else {
			pMBDev->down_callback = NULL;
		}
		break;
	default:
		printk ("DANUBE_MPS_DSP_UnregisterDataCallback: Invalid Direction %d\n", dir);
		return (-ENXIO);
	}
	return (OK);
}

/**
 * Register event callback.
 * Allows the upper layer to register a callback function either for events
 * specified by the mask parameter. 
 * 
 * \param   type     DSP device entity ( 1 - command, 2 - voice0, 3 - voice1, 
 *                   4 - voice2, 5 - voice3 )
 * \param   mask     Mask according to MBC_ISR content
 * \param   callback Callback function to register
 * \return  0        OK, callback registered successfully
 * \return  ENXIO    Wrong DSP device entity (only 1-5 supported)
 * \return  EBUSY    Callback already registered
 * \ingroup API  
 */
s32
ifx_mps_register_event_callback (mps_devices type, MbxEventRegs_s * mask,
				 void (*callback) (MbxEventRegs_s * events))
{
	mps_mbx_dev *pMBDev;

	/* Get corresponding mailbox device structure */
	switch (type) {
	case command:
		pMBDev = &ifx_mps_dev.command_mb;
		break;
	case voice0:
		pMBDev = &ifx_mps_dev.voice_mb[0];
		break;
	case voice1:
		pMBDev = &ifx_mps_dev.voice_mb[1];
		break;
	case voice2:
		pMBDev = &ifx_mps_dev.voice_mb[2];
		break;
	case voice3:
		pMBDev = &ifx_mps_dev.voice_mb[3];
		break;
	default:
		return (-ENXIO);
	}

	/* Enter the desired callback function */
	if (pMBDev->event_callback != NULL) {
		return (-EBUSY);
	}
	else {
		memcpy ((char *) &pMBDev->event_mask, (char *) mask,
			sizeof (MbxEventRegs_s));
		pMBDev->event_callback = callback;
	}
	return (OK);
}

/**
 * Unregister event callback.
 * Allows the upper layer to unregister the callback function previously
 * registered.
 * 
 * \param   type   DSP device entity ( 1 - command, 2 - voice0, 3 - voice1,
 *                 4 - voice2, 5 - voice3 )
 * \return  0      OK, callback registered successfully
 * \return  ENXIO  Wrong DSP device entity (only 1-5 supported)
 * \ingroup API  
 */
s32
ifx_mps_unregister_event_callback (mps_devices type)
{
	mps_mbx_dev *pMBDev;
	/* Get corresponding mailbox device structure */
	switch (type) {
	case command:
		pMBDev = &ifx_mps_dev.command_mb;
		break;
	case voice0:
		pMBDev = &ifx_mps_dev.voice_mb[0];
		break;
	case voice1:
		pMBDev = &ifx_mps_dev.voice_mb[1];
		break;
	case voice2:
		pMBDev = &ifx_mps_dev.voice_mb[2];
		break;
	case voice3:
		pMBDev = &ifx_mps_dev.voice_mb[3];
		break;
	default:
		return (-ENXIO);
	}
	/* Delete the desired callback function */
	memset ((char *) &pMBDev->event_mask, 0, sizeof (MbxEventRegs_s));
	pMBDev->event_callback = NULL;
	return (OK);
}

/**
 * Change event interrupt activation.
 * Allows the upper layer enable or disable interrupt generation of event previously
 * registered. Note that 
 * 
 * \param   type   DSP device entity ( 1 - command, 2 - voice0, 3 - voice1,
 *                 4 - voice2, 5 - voice3 )
 * \param   act    Register values according to MbxEvent_Regs, whereas bit=1 means
 *                 active, bit=0 means inactive
 * \return  0      OK, interrupt masked changed accordingly
 * \return  ENXIO  Wrong DSP device entity (only 1-5 supported)
 * \return  EINVAL Callback value null
 * \ingroup API  
 */
s32
ifx_mps_event_activation (mps_devices type, MbxEventRegs_s * act)
{
	mps_mbx_dev *pMBDev;
	MPS_Ad0Reg_u Ad0Reg;
	MPS_Ad1Reg_u Ad1Reg;
	MPS_VCStatReg_u VCStatReg;
	int i;

	/* Get corresponding mailbox device structure */
	switch (type) {
	case command:
		pMBDev = &ifx_mps_dev.command_mb;
		break;
	case voice0:
		pMBDev = &ifx_mps_dev.voice_mb[0];
		break;
	case voice1:
		pMBDev = &ifx_mps_dev.voice_mb[1];
		break;
	case voice2:
		pMBDev = &ifx_mps_dev.voice_mb[2];
		break;
	case voice3:
		pMBDev = &ifx_mps_dev.voice_mb[3];
		break;
	default:
		return (-ENXIO);
	}

	/* Enable necessary MPS interrupts */
	Ad0Reg.val = *DANUBE_MPS_AD0ENR;
	Ad0Reg.val = (Ad0Reg.val & ~pMBDev->event_mask.MPS_Ad0Reg.val)
		| (act->MPS_Ad0Reg.val & pMBDev->event_mask.MPS_Ad0Reg.val);
	*DANUBE_MPS_AD0ENR = Ad0Reg.val;
	Ad1Reg.val = *DANUBE_MPS_AD1ENR;
	Ad1Reg.val = (Ad1Reg.val & ~pMBDev->event_mask.MPS_Ad1Reg.val)
		| (act->MPS_Ad1Reg.val & pMBDev->event_mask.MPS_Ad1Reg.val);
	*DANUBE_MPS_AD1ENR = Ad1Reg.val;

	for (i = 0; i < 4; i++) {
		VCStatReg.val = DANUBE_MPS_VC0ENR[i];
		VCStatReg.val =
			(VCStatReg.val & ~pMBDev->event_mask.MPS_VCStatReg[i].
			 val)
			| (act->MPS_VCStatReg[i].val & pMBDev->event_mask.
			   MPS_VCStatReg[i].val);
		DANUBE_MPS_VC0ENR[i] = VCStatReg.val;
	}

	return (OK);

}

/**
 * Read from mailbox upstream FIFO.
 * This function reads from the mailbox upstream FIFO selected by type.
 * 
 * \param   type  DSP device entity ( 1 - command, 2 - voice0, 3 - voice1,
 *                4 - voice2, 5 - voice3 )
 * \param   rw    Pointer to message structure for received data 
 * \return  0     OK, successful read operation
 * \return  ENXIO Wrong DSP device entity (only 1-5 supported)
 * \return  -1    ERROR, in case of read error.
 * \ingroup API  
 */
s32
ifx_mps_read_mailbox (mps_devices type, mps_message * rw)
{
	s32 ret;

	switch (type) {
	case command:
		ret = ifx_mps_mbx_read (&ifx_mps_dev.command_mb, rw, 0);
		break;
	case voice0:
		ret = ifx_mps_mbx_read (&ifx_mps_dev.voice_mb[0], rw, 0);
		break;
	case voice1:
		ret = ifx_mps_mbx_read (&ifx_mps_dev.voice_mb[1], rw, 0);
		break;
	case voice2:
		ret = ifx_mps_mbx_read (&ifx_mps_dev.voice_mb[2], rw, 0);
		break;
	case voice3:
		ret = ifx_mps_mbx_read (&ifx_mps_dev.voice_mb[3], rw, 0);
		break;
	default:
		ret = -ENXIO;
	}
	return (ret);
}

/**
 * Write to downstream mailbox buffer.
 * This function writes data to either the command or to the voice FIFO 
 * 
 * \param   type  DSP device entity ( 1 - command, 2 - voice0, 3 - voice1,
 *                4 - voice2, 5 - voice3 )
 * \param   rw    Pointer to message structure
 * \return  0     OK, successful read operation
 * \return  ENXIO Wrong DSP device entity (only 1-5 supported)
 * \return  -1    ERROR, in case of read error.
 * \ingroup API  
 */
s32
ifx_mps_write_mailbox (mps_devices type, mps_message * rw)
{
	int ret;

	switch (type) {
	case command:
		ret = ifx_mps_mbx_write_cmd (&ifx_mps_dev.command_mb, rw);
		break;
	case voice0:
		ret = ifx_mps_mbx_write_data (&ifx_mps_dev.voice_mb[0], rw);
		break;
	case voice1:
		ret = ifx_mps_mbx_write_data (&ifx_mps_dev.voice_mb[1], rw);
		break;
	case voice2:
		ret = ifx_mps_mbx_write_data (&ifx_mps_dev.voice_mb[2], rw);
		break;
	case voice3:
		ret = ifx_mps_mbx_write_data (&ifx_mps_dev.voice_mb[3], rw);
		break;
	default:
		ret = -ENXIO;
	}
	return (ret);
}

#if CONFIG_PROC_FS

/**
 * Create MPS version proc file output.
 * This function creates the output for the MPS version proc file
 *
 * \param   buf      Buffer to write the string to
 * \return  len      Lenght of data in buffer 
 * \ingroup Internal  
 */
static int
ifx_mps_get_version_proc (char *buf)
{
	int len;

	len = sprintf (buf, "%s\n", &IFX_MPS_INFO_STR[4]);

	len += sprintf (buf + len, "Compiled on %s, %s for Linux kernel %s\n",
			__DATE__, __TIME__, UTS_RELEASE);

#ifdef CONFIG_DANUBE_MPS_PROC_DEBUG
	len += sprintf (buf + len, "Supported debug tags:\n");
	len += sprintf (buf + len, "%s = Initialize hardware for voice CPU\n",
			MPS_FW_INIT_TAG);
	len += sprintf (buf + len, "%s = Start firmware\n", MPS_FW_START_TAG);
	len += sprintf (buf + len, "%s = Send buffer provisioning message\n",
			MPS_FW_BUFFER_TAG);
	len += sprintf (buf + len, "%s = Opening voice0 mailbox\n",
			MPS_FW_OPEN_VOICE_TAG);
	len += sprintf (buf + len, "%s = Register voice0 data callback\n",
			MPS_FW_REGISTER_CALLBACK_TAG);
	len += sprintf (buf + len, "%s = Send message to voice0 mailbox\n",
			MPS_FW_SEND_MESSAGE_TAG);
	len += sprintf (buf + len, "%s = Restart voice CPU\n",
			MPS_FW_RESTART_TAG);
	len += sprintf (buf + len, "%s = Packet loop enable\n",
			MPS_FW_ENABLE_PACKET_LOOP_TAG);
	len += sprintf (buf + len, "%s = Packet loop disable\n",
			MPS_FW_DISABLE_PACKET_LOOP_TAG);

	len += sprintf (buf + len, "%d Packets received\n",
			ifx_mps_rtp_voice_data_count);

#endif /* CONFIG_DANUBE_MPS_PROC_DEBUG */

	return len;
}

/**
 * Create MPS status proc file output.
 * This function creates the output for the MPS status proc file
 *
 * \param   buf      Buffer to write the string to
 * \return  len      Lenght of data in buffer 
 * \ingroup Internal  
 */
static int
ifx_mps_get_status_proc (char *buf)
{
	int len, i;

	len = sprintf (buf, "Open files: %d\n", MOD_IN_USE);

	/* Print internals of the command mailbox fifo */
	len += sprintf (buf + len, "\n * CMD *\t\tUP\t\tDO\t%s\n",
			(ifx_mps_dev.command_mb.Installed ==
			 TRUE) ? "(active)" : "(idle)");
	len += sprintf (buf + len, "   Size: \t  %8d\t  %8d\n",
			ifx_mps_dev.command_mb.upstrm_fifo.size,
			ifx_mps_dev.command_mb.dwstrm_fifo.size);

	len += sprintf (buf + len,
			"   Fill: \t  %8d\t  %8d\n",
			ifx_mps_dev.command_mb.upstrm_fifo.size - 1 -
			ifx_mps_fifo_mem_available (&ifx_mps_dev.command_mb.
						    upstrm_fifo),
			ifx_mps_dev.command_mb.dwstrm_fifo.size - 1 -
			ifx_mps_fifo_mem_available (&ifx_mps_dev.command_mb.
						    dwstrm_fifo));

	len += sprintf (buf + len,
			"   Free: \t  %8d\t  %8d\n",
			ifx_mps_fifo_mem_available (&ifx_mps_dev.command_mb.
						    upstrm_fifo),
			ifx_mps_fifo_mem_available (&ifx_mps_dev.command_mb.
						    dwstrm_fifo));

	len += sprintf (buf + len,
			"   Start:\t0x%08X\t0x%08X"
			"\n   Write:\t0x%08X\t0x%08X"
			"\n   Read: \t0x%08X\t0x%08X"
			"\n   End:  \t0x%08X\t0x%08X\n",
			(u32) ifx_mps_dev.command_mb.upstrm_fifo.pstart,
			(u32) ifx_mps_dev.command_mb.dwstrm_fifo.pstart,
			(u32) ifx_mps_dev.command_mb.upstrm_fifo.pend +
			(u32) * ifx_mps_dev.command_mb.upstrm_fifo.pwrite_off,
			(u32) ifx_mps_dev.command_mb.dwstrm_fifo.pend +
			(u32) * ifx_mps_dev.command_mb.dwstrm_fifo.pwrite_off,
			(u32) ifx_mps_dev.command_mb.upstrm_fifo.pend +
			(u32) * ifx_mps_dev.command_mb.upstrm_fifo.pread_off,
			(u32) ifx_mps_dev.command_mb.dwstrm_fifo.pend +
			(u32) * ifx_mps_dev.command_mb.dwstrm_fifo.pread_off,
			(u32) ifx_mps_dev.command_mb.upstrm_fifo.pend,
			(u32) ifx_mps_dev.command_mb.dwstrm_fifo.pend);

	/* Printout the number of interrupts and fifo misses */
	len += sprintf (buf + len,
			"   Ints: \t  %8d\t  %8d\n",
			ifx_mps_dev.command_mb.RxnumIRQs,
			ifx_mps_dev.command_mb.TxnumIRQs);

	len += sprintf (buf + len,
			"   Bytes:\t  %8d\t  %8d\n",
			ifx_mps_dev.command_mb.RxnumBytes,
			ifx_mps_dev.command_mb.TxnumBytes);

	len += sprintf (buf + len, "\n * VOICE *\t\tUP\t\tDO\n");
	len += sprintf (buf + len,
			"   Size: \t  %8d\t  %8d\n",
			ifx_mps_dev.voice_mb[0].upstrm_fifo.size,
			ifx_mps_dev.voice_mb[0].dwstrm_fifo.size);

	len += sprintf (buf + len,
			"   Fill: \t  %8d\t  %8d\n",
			ifx_mps_dev.voice_mb[0].upstrm_fifo.size - 1 -
			ifx_mps_fifo_mem_available (&ifx_mps_dev.voice_mb[0].
						    upstrm_fifo),
			ifx_mps_dev.voice_mb[0].dwstrm_fifo.size - 1 -
			ifx_mps_fifo_mem_available (&ifx_mps_dev.voice_mb[0].
						    dwstrm_fifo));

	len += sprintf (buf + len,
			"   Free: \t  %8d\t  %8d\n",
			ifx_mps_fifo_mem_available (&ifx_mps_dev.voice_mb[0].
						    upstrm_fifo),
			ifx_mps_fifo_mem_available (&ifx_mps_dev.voice_mb[0].
						    dwstrm_fifo));

	len += sprintf (buf + len,
			"   Start:\t0x%08X\t0x%08X"
			"\n   Write:\t0x%08X\t0x%08X"
			"\n   Read: \t0x%08X\t0x%08X"
			"\n   End:  \t0x%08X\t0x%08X\n",
			(u32) ifx_mps_dev.voice_mb[0].upstrm_fifo.pstart,
			(u32) ifx_mps_dev.voice_mb[0].dwstrm_fifo.pstart,
			(u32) ifx_mps_dev.voice_mb[0].upstrm_fifo.pend +
			(u32) *
			ifx_mps_dev.voice_mb[0].upstrm_fifo.pwrite_off,
			(u32) ifx_mps_dev.voice_mb[0].dwstrm_fifo.pend +
			(u32) *
			ifx_mps_dev.voice_mb[0].dwstrm_fifo.pwrite_off,
			(u32) ifx_mps_dev.voice_mb[0].upstrm_fifo.pend +
			(u32) * ifx_mps_dev.voice_mb[0].upstrm_fifo.pread_off,
			(u32) ifx_mps_dev.voice_mb[0].dwstrm_fifo.pend +
			(u32) * ifx_mps_dev.voice_mb[0].dwstrm_fifo.pread_off,
			(u32) ifx_mps_dev.voice_mb[0].upstrm_fifo.pend,
			(u32) ifx_mps_dev.voice_mb[0].dwstrm_fifo.pend);

	for (i = 0; i < 4; i++) {
		len += sprintf (buf + len, "\n * CH%i *\t\tUP\t\tDO\t%s\n", i,
				(ifx_mps_dev.voice_mb[i].Installed ==
				 TRUE) ? "(active)" : "(idle)");
		/* Printout the number of interrupts and fifo misses */
		len += sprintf (buf + len,
				"   Ints: \t  %8d\t  %8d\n",
				ifx_mps_dev.voice_mb[i].RxnumIRQs,
				ifx_mps_dev.voice_mb[i].TxnumIRQs);

		len += sprintf (buf + len,
				"   Bytes: \t  %8d\t  %8d\n",
				ifx_mps_dev.voice_mb[i].RxnumBytes,
				ifx_mps_dev.voice_mb[i].TxnumBytes);
#if 1
		len += sprintf (buf + len,
				"   minLv: \t  %8d\t  %8d\n",
				ifx_mps_dev.voice_mb[i].upstrm_fifo.min_space,
				ifx_mps_dev.voice_mb[i].dwstrm_fifo.
				min_space);
#endif
	}
	return len;
}

/**
 * Create MPS proc file output.
 * This function creates the output for the MPS proc file according to the 
 * function specified in the data parameter, which is setup during registration.
 *
 * \param   buf      Buffer to write the string to
 * \param   start    not used (Linux internal)
 * \param   offset   not used (Linux internal)
 * \param   count    not used (Linux internal)
 * \param   eof      Set to 1 when all data is stored in buffer
 * \param   data     not used (Linux internal)
 * \return  len      Lenght of data in buffer 
 * \ingroup Internal  
 */
static int
ifx_mps_read_proc (char *page, char **start, off_t off,
		   int count, int *eof, void *data)
{
	int len;

	int (*fn) (char *buf);

	if (data != NULL) {
		fn = data;
		len = fn (page);
	}
	else
		return 0;

	if (len <= off + count)
		*eof = 1;
	*start = page + off;
	len -= off;
	if (len > count)
		len = count;
	if (len < 0)
		len = 0;
	return len;
}

#ifdef CONFIG_DANUBE_MPS_PROC_DEBUG
/**
 * Process MPS proc file input.
 * This function evaluates the input of the MPS formware proc file and downloads
 * the given firmware into the voice CPU.
 *
 * \param   file    file structure for proc file
 * \param   buffer  buffer holding the data
 * \param   count   number of characters in buffer
 * \param   data    unused
 * \return  count   Number of processed characters 
 * \ingroup Internal  
 */
static int
ifx_mps_write_fw_procmem (struct file *file, const char *buffer,
			  unsigned long count, void *data)
{
	int t = 0;

	if (count == (sizeof (MPS_FW_INIT_TAG))) {
		if (!strncmp
		    (buffer, MPS_FW_INIT_TAG, sizeof (MPS_FW_INIT_TAG) - 1)) {
			printk ("Initializing Voice CPU...\n");
			ifx_mps_init_gpt_danube ();
			return count;
		}
	}
	if (count == (sizeof (MPS_FW_START_TAG))) {
		if (!strncmp
		    (buffer, MPS_FW_START_TAG,
		     sizeof (MPS_FW_START_TAG) - 1)) {
			printk ("Starting FW (size=%d)...\n",
				ifx_mps_firmware_buffer_pos);
			ifx_mps_firmware_struct.data =
				(u32 *) ifx_mps_firmware_buffer;
			ifx_mps_firmware_struct.length =
				ifx_mps_firmware_buffer_pos;
			ifx_mps_download_firmware (NULL,
						   &ifx_mps_firmware_struct);
			return count;
		}
	}
	if (count == (sizeof (MPS_FW_BUFFER_TAG))) {
		if (!strncmp
		    (buffer, MPS_FW_BUFFER_TAG,
		     sizeof (MPS_FW_BUFFER_TAG) - 1)) {
			printk ("Providing Buffers...\n");
			ifx_mps_bufman_init ();
			return count;
		}
	}
	if (count == (sizeof (MPS_FW_OPEN_VOICE_TAG))) {
		if (!strncmp
		    (buffer, MPS_FW_OPEN_VOICE_TAG,
		     sizeof (MPS_FW_OPEN_VOICE_TAG) - 1)) {
			printk ("Opening voice0 mailbox...\n");
			ifx_mps_open ((struct inode *) 2, NULL);
			return count;
		}
	}
	if (count == (sizeof (MPS_FW_REGISTER_CALLBACK_TAG))) {
		if (!strncmp
		    (buffer, MPS_FW_REGISTER_CALLBACK_TAG,
		     sizeof (MPS_FW_REGISTER_CALLBACK_TAG) - 1)) {
			printk ("Opening voice0 mailbox...\n");
			ifx_mps_register_data_callback (2, 1,
							ifx_mbx_dummy_rcv_data_callback);
			return count;
		}
	}
	if (count == (sizeof (MPS_FW_SEND_MESSAGE_TAG))) {
		if (!strncmp
		    (buffer, MPS_FW_SEND_MESSAGE_TAG,
		     sizeof (MPS_FW_SEND_MESSAGE_TAG) - 1)) {
			mps_message wrt_data;
			int i;

			printk ("Sending message to voice0 mailbox...\n");

			wrt_data.pData = (u8 *) kmalloc (100, GFP_KERNEL);
			wrt_data.nDataBytes = 100;
			wrt_data.cmd_type = CMD_RTP_VOICE_DATA_PACKET;
			wrt_data.RTP_PaylOffset = 0x00000000;
			for (i = 0; i < (100 / 4); i++)
				*((u32 *)
				  KSEG1ADDR ((wrt_data.pData + (i << 2)))) =
					0xdeadbeef;
			ifx_mps_write_mailbox (2, &wrt_data);

			return count;
		}
	}
	if (count == (sizeof (MPS_FW_RESTART_TAG))) {
		if (!strncmp
		    (buffer, MPS_FW_RESTART_TAG,
		     sizeof (MPS_FW_RESTART_TAG) - 1)) {
			printk ("Restarting voice CPU...\n");
			ifx_mps_restart ();
			return count;
		}
	}
	if (count == (sizeof (MPS_FW_ENABLE_PACKET_LOOP_TAG))) {
		if (!strncmp
		    (buffer, MPS_FW_ENABLE_PACKET_LOOP_TAG,
		     sizeof (MPS_FW_ENABLE_PACKET_LOOP_TAG) - 1)) {
			printk ("Enabling packet loop...\n");
			ifx_mps_packet_loop = 1;
			return count;
		}
	}
	if (count == (sizeof (MPS_FW_DISABLE_PACKET_LOOP_TAG))) {
		if (!strncmp
		    (buffer, MPS_FW_DISABLE_PACKET_LOOP_TAG,
		     sizeof (MPS_FW_DISABLE_PACKET_LOOP_TAG) - 1)) {
			printk ("Disabling packet loop...\n");
			ifx_mps_packet_loop = 0;
			return count;
		}
	}

	if ((count + ifx_mps_firmware_buffer_pos) < MPS_FIRMWARE_BUFFER_SIZE) {
		for (t = 0; t < count; t++)
			ifx_mps_firmware_buffer[ifx_mps_firmware_buffer_pos +
						t] = buffer[t];
		ifx_mps_firmware_buffer_pos += count;
		return count;
	}

	return 0;
}
#endif /* CONFIG_DANUBE_MPS_PROC_DEBUG */

#endif /* CONFIG_PROC_FS */

/**
 * Initialize the module.
 * This is the initialization function of the MPS module.
 *          
 * \return  0        OK, module initialized
 * \return  EPERM    Reset of CPU1 failed
 * \return  ENOMEM   No memory left for structures
 * \ingroup Internal  
 */
#if !defined( IKOS_MINI_BOOT )
static int __init
ifx_mps_init_module (void)
#else
int __init
ifx_mps_init_module (void)
#endif
{
	int result;
	devfs_handle_t mps_dir;
	int i;

	printk (KERN_INFO "%s\n", &IFX_MPS_INFO_STR[4]);
	printk (KERN_INFO "(c) Copyright 2006, Infineon Technologies AG\n");

#ifndef CONFIG_DEVFS_FS
	sprintf (ifx_mps_dev_name, IFX_MPS_DEV_NAME);
	result = register_chrdev (ifx_mps_major_id, ifx_mps_dev_name,
				  &ifx_mps_fops);

	if (result < 0) {
		printk ("IFX_MPS: can't get major %d\n", ifx_mps_major_id);
		return result;
	}

	/* dynamic major                       */
	if (ifx_mps_major_id == 0)
		ifx_mps_major_id = result;
#else
	sprintf (ifx_mps_dev_name, IFX_MPS_DEV_NAME);
	result = devfs_register_chrdev (ifx_mps_major_id, ifx_mps_dev_name,
					&ifx_mps_fops);

	if (result < 0) {
		printk ("IFX_MPS: can't get major %d\n", ifx_mps_major_id);
		return result;
	}

	/* dynamic major                       */
	if (ifx_mps_major_id == 0)
		ifx_mps_major_id = result;

	mps_dir = devfs_mk_dir (NULL, ifx_mps_dev_name, NULL);
	if (!mps_dir)
		return -EBUSY;	/* problem */

	devfs_register (mps_dir, "cmd", DEVFS_FL_NONE, ifx_mps_major_id, 1,
			S_IFCHR | S_IRUGO | S_IWUGO, &ifx_mps_fops, NULL);
	devfs_register (mps_dir, "voice0", DEVFS_FL_NONE, ifx_mps_major_id, 2,
			S_IFCHR | S_IRUGO | S_IWUGO, &ifx_mps_fops, NULL);
	devfs_register (mps_dir, "voice1", DEVFS_FL_NONE, ifx_mps_major_id, 3,
			S_IFCHR | S_IRUGO | S_IWUGO, &ifx_mps_fops, NULL);
	devfs_register (mps_dir, "voice2", DEVFS_FL_NONE, ifx_mps_major_id, 4,
			S_IFCHR | S_IRUGO | S_IWUGO, &ifx_mps_fops, NULL);
	devfs_register (mps_dir, "voice3", DEVFS_FL_NONE, ifx_mps_major_id, 5,
			S_IFCHR | S_IRUGO | S_IWUGO, &ifx_mps_fops, NULL);

#endif /* !CONFIG_DEVFS_FS */

	/* init the device driver structure */
	if (0 != ifx_mps_init_structures (&ifx_mps_dev))
		return -ENOMEM;

	/* reset the device before initializing the device driver */
	if ((ERROR) == ifx_mps_reset ()) {
		printk ("IFX_MPS: Hardware reset failed!\n");
		return -EPERM;
	}

	result = request_irq ( /*INT_NUM_IM5_IRL11 */ INT_NUM_IM4_IRL18,
			      ifx_mps_ad0_irq,
			      SA_INTERRUPT, "mps_mbx ad0", &ifx_mps_dev);
	if (result)
		return result;

	result = request_irq ( /*INT_NUM_IM5_IRL12 */ INT_NUM_IM4_IRL19,
			      ifx_mps_ad1_irq,
			      SA_INTERRUPT, "mps_mbx ad1", &ifx_mps_dev);
	if (result)
		return result;

	/* register status interrupts for voice channels */
	for (i = 0; i < 4; ++i) {
		sprintf (&voice_channel_int_name[i][0], "mps_mbx vc%d", i);
		result = request_irq ( /*INT_NUM_IM5_IRL7 */ INT_NUM_IM4_IRL14
				      + i, ifx_mps_vc_irq,
				      SA_INTERRUPT,
				      &voice_channel_int_name[i][0],
				      &ifx_mps_dev);
		if (result)
			return result;
	}

	/* Enable all MPS Interrupts at ICU0     22|21|20||19|18|17|16||15|14|..|            */
	MPS_INTERRUPTS_ENABLE (0x007FC000);

#if CONFIG_PROC_FS
	/* install the proc entry */
	printk (KERN_INFO "IFX_MPS: using proc fs\n");

	ifx_mps_proc_dir = proc_mkdir ("driver/" IFX_MPS_DEV_NAME, NULL);
	if (ifx_mps_proc_dir != NULL) {
#ifdef CONFIG_DANUBE_MPS_PROC_DEBUG
		ifx_mps_proc_dir->owner = THIS_MODULE;
		ifx_mps_proc_file =
			create_proc_entry ("firmware", 0, ifx_mps_proc_dir);
		ifx_mps_proc_file->write_proc = ifx_mps_write_fw_procmem;
#endif /* CONFIG_DANUBE_MPS_PROC_DEBUG */

		create_proc_read_entry ("version", S_IFREG | S_IRUGO,
					ifx_mps_proc_dir, ifx_mps_read_proc,
					(void *) ifx_mps_get_version_proc);
		create_proc_read_entry ("status", S_IFREG | S_IRUGO,
					ifx_mps_proc_dir, ifx_mps_read_proc,
					(void *) ifx_mps_get_status_proc);
	}
	else {
		printk ("IFX_MPS: cannot create proc entry\n");
	}
#endif

#if 1				/* PCM GPIO pin init */
	CLEARBIT (DANUBE_GPIO_P1_ALTSEL0, 0x0100);
	SETBIT (DANUBE_GPIO_P1_ALTSEL1, 0x0100);
	SETBIT (DANUBE_GPIO_P1_DIR, 0x0100);	//tsc

	SETBIT (DANUBE_GPIO_P1_ALTSEL0, 0x0200);
	CLEARBIT (DANUBE_GPIO_P1_ALTSEL1, 0x0200);
	SETBIT (DANUBE_GPIO_P1_DIR, 0x0200);	//do
	SETBIT (DANUBE_GPIO_P1_OD, 0x0200);

	CLEARBIT (DANUBE_GPIO_P1_ALTSEL0, 0x0400);
	SETBIT (DANUBE_GPIO_P1_ALTSEL1, 0x0400);
	CLEARBIT (DANUBE_GPIO_P1_DIR, 0x0400);	//di

	SETBIT (DANUBE_GPIO_P1_ALTSEL0, 0x0800);
	CLEARBIT (DANUBE_GPIO_P1_ALTSEL1, 0x0800);
	SETBIT (DANUBE_GPIO_P1_DIR, 0x0800);	//dcl used as output
	SETBIT (DANUBE_GPIO_P1_OD, 0x0800);

	SETBIT (DANUBE_GPIO_P0_ALTSEL0, 0x01);
	SETBIT (DANUBE_GPIO_P0_ALTSEL1, 0x01);
	SETBIT (DANUBE_GPIO_P0_DIR, 0x01);	//fsc used as output
	SETBIT (DANUBE_GPIO_P0_OD, 0x01);
#endif

#if 1				/* enable TD and Vodec on PMU */

	CLEARBIT (DANUBE_PMU_PWDCR, 0x00000008);
	CLEARBIT (DANUBE_PMU_PWDCR, 0x02000000);
#endif

#if 1				/* PCM setting */

	(*(volatile u32 *) (0xBF103030)) = 0x04000000;	/* CGU_PCMCR */

//        (*(volatile u32*)(0xBF10001C)) = 0x00;          /* PCM_TEST */
//        (*(volatile u32*)(0xBF100020)) = 0x00;          /* PCM_MODE */
//        (*(volatile u32*)(0xBF100028)) = 0x01f;         /* PCM_CFG1 */
//        (*(volatile u32*)(0xBF100018)) = 0x0000ffff;    /* PCM_EN */
//        (*(volatile u32*)(0xBF100014)) = 0x0000C101;    /* PCM_CFG */

//        (*(volatile u32*)(0xBF103008)) = 0x1B1E0C99;  /* CGU_PLL1_CFG */
#endif

#ifdef IKOS_MINI_BOOT
	printk ("Enabling GPTC...\n");
	ifx_mps_init_gpt_danube ();

	printk ("Downloading Firmware...\n");
	ifx_mps_download_firmware (NULL, (mps_fw *) 0xa0a00000);
	udelay (500);
	printk ("Providing Buffers...\n");
	ifx_mps_bufman_init ();
#endif

	return 0;
}

/**
 * Cleanup MPS module.
 * Clean up the module for unloading.
 *
 * \ingroup Internal  
 */
static void __exit
ifx_mps_cleanup_module (void)
{
	/* disable Interrupts at ICU0 */
	MPS_INTERRUPTS_DISABLE (DANUBE_MPS_AD0_IR4);	/* Disable DFE/AFE 0 Interrupts */
	/* disable all MPS interrupts */
	*DANUBE_MPS_SAD0SR = 0x00000000;

	unregister_chrdev (ifx_mps_major_id, IFX_MPS_DEV_NAME);

	/* release the memory usage of the device driver structure */
	ifx_mps_release_structures (&ifx_mps_dev);

	/* release all interrupts at the system */
	free_irq ( /*INT_NUM_IM5_IRL11 */ INT_NUM_IM4_IRL18, &ifx_mps_dev);

#if CONFIG_PROC_FS

#ifdef CONFIG_DANUBE_MPS_PROC_DEBUG
	remove_proc_entry ("firmware", ifx_mps_proc_dir);
#endif /* CONFIG_DANUBE_MPS_PROC_DEBUG */
	remove_proc_entry ("version", ifx_mps_proc_dir);
	remove_proc_entry ("status", ifx_mps_proc_dir);
	remove_proc_entry ("driver/" IFX_MPS_DEV_NAME, NULL);
#endif

	printk (KERN_INFO "IFX_MPS: cleanup done\n");
}

module_init (ifx_mps_init_module);
module_exit (ifx_mps_cleanup_module);

#if !defined( IKOS_MINI_BOOT )
#ifndef DEBUG
EXPORT_SYMBOL (ifx_mps_write_mailbox);
EXPORT_SYMBOL (ifx_mps_register_data_callback);
EXPORT_SYMBOL (ifx_mps_unregister_data_callback);
EXPORT_SYMBOL (ifx_mps_register_event_callback);
EXPORT_SYMBOL (ifx_mps_unregister_event_callback);
EXPORT_SYMBOL (ifx_mps_event_activation);
EXPORT_SYMBOL (ifx_mps_read_mailbox);
EXPORT_SYMBOL (ifx_mps_bufman_register);

EXPORT_SYMBOL (ifx_mps_ioctl);
EXPORT_SYMBOL (ifx_mps_open);
EXPORT_SYMBOL (ifx_mps_close);
#endif
#endif
