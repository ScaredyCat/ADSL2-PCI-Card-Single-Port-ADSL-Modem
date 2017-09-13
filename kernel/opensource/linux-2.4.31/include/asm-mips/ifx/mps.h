/************************************************************************
 *
 * Copyright (c) 2005
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
/** \addtogroup IOCTL Defined IOCTLs */

#ifndef __MPS_H
#define __MPS_H

/**
 * MPS Mailbox Message.
 */
typedef struct {
	u8 *pData;	/**< Pointer to data location in SDRAM to be passed to other CPU */
	u32 nDataBytes;	/**< Amount of valid data bytes in SDRAM starting at pData */
	u32 RTP_PaylOffset;
			/**< Byte offset to reserve space for RTP header */
	u32 cmd_type;	/**< Type of command */
} mps_message;

typedef enum {
	unknown,
	command,
	voice0,
	voice1,
	voice2,
	voice3
} mps_devices;

/**
 * Mailbox history structure.
 * This structure contains the history of messages sent to the mailbox.
 */
typedef struct {
	u32 *buf;     /**< History buffer */
	int len;      /**< Length of history buffer in words */
	int total_words;
		      /**< Overall number of words sent to mailbox */
	int freeze;   /**< Indication whether logging was stopped */
} mps_history;

/**
 * Firmware structure.
 * This structure contains a pointer to the firmware and its length.
 */
typedef struct {
	u32 *data;
		 /**< Pointer to firmware image */
	u32 length;
		 /**< Length of firmware in bytes */
} mps_fw;

#define CMD_VOICEREC_STATUS_PACKET  0x0
#define CMD_VOICEREC_DATA_PACKET    0x1
#define CMD_RTP_VOICE_DATA_PACKET   0x4
#define CMD_RTP_EVENT_PACKET        0x5
#define CMD_ADDRESS_PACKET          0x8
#define CMD_FAX_DATA_PACKET         0x10
#define CMD_FAX_STATUS_PACKET       0x11
#define CMD_P_PHONE_DATA_PACKET     0x12
#define CMD_P_PHONE_STATUS_PACKET   0x13
#define CMD_CID_DATA_PACKET         0x14

#define CMD_ALI_PACKET              0x1
#define CMD_COP_PACKET              0x2
#define CMD_EOP_PACKET              0x6

/******************************************************************************
 * Exported IOCTLs
 ******************************************************************************/
/** magic number */
#define IFX_MPS_MAGIC 'O'

/**
 * Set event notification mask.
 * \param   arg Event mask
 * \ingroup IOCTL
 */
#define FIO_MPS_EVENT_REG _IOW(IFX_MPS_MAGIC, 1, unsigned int)
/**
 * Mask Event Notification.
 * \ingroup IOCTL
 */
#define FIO_MPS_EVENT_UNREG _IO(IFX_MPS_MAGIC, 2)
/**
 * Read Message from Mailbox.
 * \param   arg Pointer to structure #mps_message
 * \ingroup IOCTL
 */
#define FIO_MPS_MB_READ _IOR(IFX_MPS_MAGIC, 3, mps_message)
/**
 * Write Message to Mailbox.
 * \param   arg Pointer to structure #mps_message
 * \ingroup IOCTL
 */
#define FIO_MPS_MB_WRITE _IOW(IFX_MPS_MAGIC, 4, mps_message)
/**
 * Reset Voice CPU.
 * \ingroup IOCTL
 */
#define FIO_MPS_RESET _IO(IFX_MPS_MAGIC, 6)
/**
 * Restart Voice CPU.
 * \ingroup IOCTL
 */
#define FIO_MPS_RESTART _IO(IFX_MPS_MAGIC, 7)
/**
 * Read Version String.
 * \param   arg Pointer to version string.
 * \ingroup IOCTL
 */
#define FIO_MPS_GETVERSION      _IOR(IFX_MPS_MAGIC, 8, char*)
/**
 * Reset Mailbox Queue.
 * \ingroup IOCTL
 */
#define FIO_MPS_MB_RST_QUEUE _IO(IFX_MPS_MAGIC, 9)
/**
 * Download Firmware
 * \param   arg Pointer to structure #mps_fw
 * \ingroup IOCTL
 */
#define  FIO_MPS_DOWNLOAD _IO(IFX_MPS_MAGIC, 17)
/**
 * Set FIFO Blocking State.
 * \param   arg Boolean value [0=off,1=on]
 * \ingroup IOCTL
 */
#define  FIO_MPS_TXFIFO_SET _IOW(IFX_MPS_MAGIC, 18, bool_t)
/**
 * Read FIFO Blocking State.
 * \param   arg Boolean value [0=off,1=on]
 * \ingroup IOCTL
 */
#define  FIO_MPS_TXFIFO_GET _IOR(IFX_MPS_MAGIC, 19, bool_t)
/**
 * Read channel Status Register.
 * \param   arg Content of status register
 * \ingroup IOCTL
 */
#define  FIO_MPS_GET_STATUS _IOR(IFX_MPS_MAGIC, 20, u32)
/**
 * Read command history buffer.
 * \param   arg Pointer to structure #mps_history
 * \ingroup IOCTL
 */
#define  FIO_MPS_GET_CMD_HISTORY _IOR(IFX_MPS_MAGIC, 21, u32)

/******************************************************************************
 * Register structure definitions
 ******************************************************************************/
typedef struct {
/**< Register structure for Common status registers MPS_RAD0SR, MPS_SAD0SR, 
                     MPS_CAD0SR and MPS_AD0ENR  */
	u32 res1:17;
	u32 wd_fail:1;
	u32 res2:2;
	u32 mips_ol:1;
	u32 data_err:1;
	u32 pcm_crash:1;
	u32 cmd_err:1;
	u32 res3:6;
	u32 du_mbx:1;
	u32 cu_mbx:1;
} MPS_Ad0Reg_s;

typedef union {
	u32 val;
	MPS_Ad0Reg_s fld;
} MPS_Ad0Reg_u;

typedef struct {
/**< Register structure for Common status registers MPS_RAD1SR, MPS_SAD1SR, 
                     MPS_CAD1SR and MPS_AD1ENR  */
	u32 res1:18;
	u32 onhook1:1;
	u32 offhook1:1;
	u32 otemp1:1;
	u32 ltest1:1;
	u32 res2:4;
	u32 onhook0:1;
	u32 offhook0:1;
	u32 otemp0:1;
	u32 ltest0:1;
	u32 hs_con:1;
	u32 hs_dis:1;
} MPS_Ad1Reg_s;

typedef union {
	u32 val;
	MPS_Ad1Reg_s fld;
} MPS_Ad1Reg_u;

typedef struct {
/**< Register structure for Voice Channel Status registers 
                      MPS_RVCxSR, MPS_SVCxSR, MPS_CVCxSR and MPS_VCxENR   */
	u32 dtmfr_dt:1;
	u32 dtmfr_pdt:1;
	u32 dtmfr_dtc:4;
	u32 res1:1;
	u32 utgus:1;
	u32 cptd:1;
	u32 rcv_ov:1;
	u32 dtmfg_buf:1;
	u32 dtmfg_req:1;
	u32 tg_inact:1;
	u32 cis_buf:1;
	u32 cis_req:1;
	u32 cis_inact:1;
	u32 mftd2:5;
	u32 mftd1:5;
	u32 lin_req:1;
	u32 epq_st:1;
	u32 vpou_st:1;
	u32 vpou_jbl:1;
	u32 vpou_jbh:1;
	u32 dec_chg:1;
} MPS_VCStatReg_s;

typedef union {
	u32 val;
	MPS_VCStatReg_s fld;
} MPS_VCStatReg_u;

typedef struct {
	MPS_Ad0Reg_u MPS_Ad0Reg;
	MPS_Ad1Reg_u MPS_Ad1Reg;
	MPS_VCStatReg_u MPS_VCStatReg[4];
} MbxEventRegs_s;

/******************************************************************************
 * Exported functions
 ******************************************************************************/
#ifdef __KERNEL__
s32 ifx_mps_open (struct inode *inode, struct file *file_p);
s32 ifx_mps_close (struct inode *inode, struct file *filp);
s32 ifx_mps_ioctl (struct inode *inode, struct file *file_p, u32 nCmd,
		   unsigned long arg);
s32 ifx_mps_register_data_callback (mps_devices type, u32 dir,
				    void (*callback) (mps_devices type));
s32 ifx_mps_unregister_data_callback (mps_devices type, u32 dir);
s32 ifx_mps_register_event_callback (mps_devices type, MbxEventRegs_s * mask,
				     void (*callback) (MbxEventRegs_s *
						       events));
s32 ifx_mps_unregister_event_callback (mps_devices type);
s32 ifx_mps_event_activation (mps_devices type, MbxEventRegs_s * act);
s32 ifx_mps_read_mailbox (mps_devices type, mps_message * rw);
s32 ifx_mps_write_mailbox (mps_devices type, mps_message * rw);
void ifx_mps_bufman_register (void *(*malloc) (size_t size, int priority),
			      void (*free) (const void *ptr), u32 buf_size,
			      u32 treshold);
#endif

#endif /* #ifndef __MPS_H */
