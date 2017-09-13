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

#ifndef __MPS_DEVICE_H
#define __MPS_DEVICE_H

#define MPS_BASEADDRESS 0xBF107000
#define MPS_RAD0SR      MPS_BASEADDRESS + 0x0004

#define MPS_RAD0SR_DU   (1<<0)
#define MPS_RAD0SR_CU   (1<<1)

#define MBX_BASEADDRESS 0xBF200000
#define VCPU_BASEADDRESS 0xBF208000	/*0xBF108000 */
/*---------------------------------------------------------------------------*/

#define MPS_INTERRUPTS_ENABLE(X)  *((volatile u32*) DANUBE_ICU_IM4_IER) |= X;
#define MPS_INTERRUPTS_DISABLE(X) *((volatile u32*) DANUBE_ICU_IM4_IER) &= ~X;
#define MPS_INTERRUPTS_CLEAR(X)   *((volatile u32*) DANUBE_ICU_IM4_ISR) = X;
#define MPS_INTERRUPTS_SET(X)     *((volatile u32*) DANUBE_ICU_IM4_IRSR) = X;	/*  |= ? */

/*---------------------------------------------------------------------------*/

#define SETBIT(reg, mask)    *reg |= (mask)
#define CLEARBIT(reg, mask)  *reg &= (~mask)

/*---------------------------------------------------------------------------*/
/* Interrupt register values for ICU                                         */
/*---------------------------------------------------------------------------*/
#define DANUBE_MPS_VC0_IR0 (1 << 0)	/* Voice channel 0 */
#define DANUBE_MPS_VC1_IR1 (1 << 1)	/* Voice channel 1 */
#define DANUBE_MPS_VC2_IR2 (1 << 2)	/* Voice channel 2 */
#define DANUBE_MPS_VC3_IR3 (1 << 3)	/* Voice channel 3 */
#define DANUBE_MPS_AD0_IR4 (1 << 4)	/* AFE/DFE Status 0 */
#define DANUBE_MPS_AD1_IR5 (1 << 5)	/* AFE/DFE Status 1 */
#define DANUBE_MPS_VC_AD_ALL_IR6 (1 << 6)	/* ored VC and AD interrupts */
#define DANUBE_MPS_SEM_IR7 (1 << 7)	/* Semaphore Interrupt */
#define DANUBE_MPS_GLB_IR8 (1 << 8)	/* Global Interrupt */

/*---------------------------------------------------------------------------*/
/* Mailbox definitions                                                       */
/*---------------------------------------------------------------------------*/

#define MBX_CMD_FIFO_SIZE  64 /**< Size of command FIFO in bytes */
#define MBX_DATA_FIFO_SIZE 128
#define MBX_DEFINITION_AREA_SIZE 32
#define MBX_RW_POINTER_AREA_SIZE 32

/* base addresses for command and voice mailboxes (upstream and downstream ) */
#define MBX_UPSTRM_CMD_FIFO_BASE   (MBX_BASEADDRESS + MBX_DEFINITION_AREA_SIZE + MBX_RW_POINTER_AREA_SIZE)
#define MBX_DNSTRM_CMD_FIFO_BASE   (MBX_UPSTRM_CMD_FIFO_BASE + MBX_CMD_FIFO_SIZE)
#define MBX_UPSTRM_DATA_FIFO_BASE  (MBX_DNSTRM_CMD_FIFO_BASE + MBX_CMD_FIFO_SIZE)
#define MBX_DNSTRM_DATA_FIFO_BASE  (MBX_UPSTRM_DATA_FIFO_BASE + MBX_DATA_FIFO_SIZE)

#define MBX_DATA_WORDS 96

#define NUM_VOICE_CHANNEL     4	/**< nr of voice channels  */
#define NR_CMD_MAILBOXES 2    /**< nr of command mailboxes  */

#define MAX_UPSTRM_DATAWORDS 30
#define MAX_FIFO_WRITE_RETRIES 80

/*---------------------------------------------------------------------------*/
/* MPS buffer provision management structure definitions                   */
/*---------------------------------------------------------------------------*/

#define MPS_BUFFER_INITIAL                 36
#define MPS_BUFFER_THRESHOLD               24

#define MPS_DEFAULT_PROVISION_SEGMENTS_PER_MSG 12

#define MPS_MAX_PROVISION_SEGMENTS_PER_MSG 60
#define MPS_BUFFER_MAX_LEVEL               64

#define MPS_MEM_SEG_DATASIZE 512
#define MAX_MEM_SEG_DATASIZE 4095

typedef enum {
	MPS_BUF_UNINITIALIZED,
	MPS_BUF_EMPTY,
	MPS_BUF_LOW,
	MPS_BUF_OK,
	MPS_BUF_OV,
	MPS_BUF_ERR
} mps_buffer_state_e;

typedef struct {
/**< mps buffer monitoring structure */
	s32 buf_level;		    /**< Current bufffer level */
	u32 buf_threshold;	    /**< Minimum buffer count */
	u32 buf_initial;	    /**< Initial buffer count */
	u32 buf_size;		/**< Buffer size for voice cpu */

	mps_buffer_state_e buf_state;
				   /** Buffer alloc function (def. kmalloc) */
	void *(*malloc) (size_t size, int priority);
	void (*free) (const void *ptr);	/**< Buffer free  function (def. kfree) */
	  s32 (*init) (void);		   /** Manager init function */
	  s32 (*close) (void);		   /** Manager shutdown function */
} mps_buf_mng_t;

typedef u32 *mem_seg_t[MPS_MAX_PROVISION_SEGMENTS_PER_MSG];

/*---------------------------------------------------------------------------*/
/* Register structure definitions                   */
/*---------------------------------------------------------------------------*/

typedef enum {
	COMMAND = 1,
	VOICE
} MbxMessageType_e;

typedef enum {
	UPSTREAM,
	DOWNSTREAM
} MbxDirection_e;

typedef struct {
	u32 rw:1;
	u32 res1:2;
	u32 type:5;
	u32 res2:4;
	u32 chan:4;
	u32 res3:1;
	u32 odd:2;
	u32 res4:5;
	u32 plength:8;
} MbxMsgHd_s;

typedef union {
	u32 val;
	MbxMsgHd_s hd;
} MbxMsgHd_u;

typedef struct {
	MbxMsgHd_u header;
	u32 data[MAX_UPSTRM_DATAWORDS];
} MbxMsg_s;

/*---------------------------------------------------------------------------*/
/* FIFO structure                                                            */
/*---------------------------------------------------------------------------*/

typedef struct {
	volatile u32 *pstart; /**< Pointer to FIFO's read/write start address */
	volatile u32 *pend;   /**< Pointer to FIFO's read/write end address */
	volatile u32 *pwrite_off;
			      /**< Pointer to FIFO's write index location */
	volatile u32 *pread_off;
			      /**< Pointer to FIFO's read index location */
	volatile u32 size;    /**< FIFO size */
	volatile u32 min_space;
			      /**< FIFO size */
} mps_fifo;

typedef struct {
	volatile u32 MPS_BOOT_RVEC;    /**< CPU reset vector */
	volatile u32 MPS_BOOT_NVEC;    /**<  */
	volatile u32 MPS_BOOT_EVEC;    /**<  */
	volatile u32 MPS_CP0_STATUS;   /**<  */
	volatile u32 MPS_CP0_EEPC;     /**<  */
	volatile u32 MPS_CP0_EPC;      /**<  */
	volatile u32 MPS_BOOT_SIZE;    /**<  */
	volatile u32 MPS_CFG_STAT;     /**<  */

} mps_boot_cfg_reg;

/*
 * This structure represents the MPS mailbox definition area that is shared
 * by CCPU and VCPU. It comprises the mailboxes' base addresses and sizes in bytes as well as the
 *
 *
 */
typedef struct {
	volatile u32 *MBX_UPSTR_CMD_BASE;  /**< Upstream Command FIFO Base Address */
	volatile u32 MBX_UPSTR_CMD_SIZE;   /**< Upstream Command FIFO size in byte */
	volatile u32 *MBX_DNSTR_CMD_BASE;  /**< Downstream Command FIFO Base Address */
	volatile u32 MBX_DNSTR_CMD_SIZE;   /**< Downstream Command FIFO size in byte */
	volatile u32 *MBX_UPSTR_DATA_BASE; /**< Upstream Data FIFO Base Address */
	volatile u32 MBX_UPSTR_DATA_SIZE;  /**< Upstream Data FIFO size in byte */
	volatile u32 *MBX_DNSTR_DATA_BASE; /**< Downstream Data FIFO Base Address */
	volatile u32 MBX_DNSTR_DATA_SIZE;  /**< Downstream Data FIFO size in byte */
	volatile u32 MBX_UPSTR_CMD_READ;   /**< Upstream Command FIFO Read Index */
	volatile u32 MBX_UPSTR_CMD_WRITE;  /**< Upstream Command FIFO Write Index */
	volatile u32 MBX_DNSTR_CMD_READ;   /**< Downstream Command FIFO Read Index */
	volatile u32 MBX_DNSTR_CMD_WRITE;  /**< Downstream Command FIFO Write Index */
	volatile u32 MBX_UPSTR_DATA_READ;   /**< Upstream Data FIFO Read Index */
	volatile u32 MBX_UPSTR_DATA_WRITE;  /**< Upstream Data FIFO Write Index */
	volatile u32 MBX_DNSTR_DATA_READ;   /**< Downstream Data FIFO Read Index */
	volatile u32 MBX_DNSTR_DATA_WRITE;  /**< Downstream Data FIFO Write Index */
	volatile u32 MBX_DATA[MBX_DATA_WORDS];
	mps_boot_cfg_reg MBX_CPU0_BOOT_CFG; /**< CPU0 Boot Configuration */
	mps_boot_cfg_reg MBX_CPU1_BOOT_CFG; /**< CPU1 Boot Configuration */
} mps_mbx_reg;
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Device connection structure                                               */
/*---------------------------------------------------------------------------*/

/**
 * Mailbox Device Structure.
 * This Structure holds top level parameters of the mailboxes used to allow 
 * the communication between the control CPU and the Voice CPU
 */
typedef struct {
	/* void pointer to the base device driver structure */
	void *pVCPU_DEV;

	/* Mutex semaphore to access the device */
	struct semaphore *sem_dev;

	/* Mutex semaphore to access the device */
	struct semaphore *sem_read_fifo;

	/* Wakeuplist for the select mechanism */
	wait_queue_head_t mps_wakeuplist;

	/* Base Address of the Global Register Access */
	mps_mbx_reg *base_global;

	mps_fifo upstrm_fifo; /**< Data exchange FIFO for write (downstream) */
	mps_fifo dwstrm_fifo; /**< Data exchange FIFO for read (upstream) */

#ifdef MPS_FIFO_BLOCKING_WRITE
	struct semaphore *sem_write_fifo;
	volatile bool_t full_write_fifo;
	/* variable if the driver should block on write to the transmit FIFO */
	bool_t bBlockWriteMB;
#endif				/* MPS_FIFO_BLOCKING_WRITE */

#ifdef CONFIG_PROC_FS
	/* interrupt counter */
	volatile u32 RxnumIRQs;
	volatile u32 TxnumIRQs;
	volatile u32 RxnumMiss;
	volatile u32 TxnumMiss;
	volatile u32 RxnumBytes;
	volatile u32 TxnumBytes;
#endif				/* CONFIG_PROC_FS */

	mps_devices devID;   /**< Device ID  1->command 
                                             2->voice chan 0
                                             3->voice chan 1 
                                             4->voice chan 2 
                                             5->voice chan 3 */

	bool_t Installed;

	void (*down_callback) (mps_devices type);
	void (*up_callback) (mps_devices type);
	MbxEventRegs_s event_mask;
	void (*event_callback) (MbxEventRegs_s * events);

	struct semaphore *wakeup_pending;
} mps_mbx_dev;
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Device structure                                                          */
/*---------------------------------------------------------------------------*/

/**
 * Mailbox Device Structure.
 * This Structure represents the communication device that provides the resources 
 * for the communication between CPU0 and CPU1
 */
typedef struct {
	mps_mbx_reg *base_global;/**< global register pointer for the ISR */
	u32 flags;		 /**< Pointer to private date of the specific handler */

	mps_mbx_dev voice_mb[NUM_VOICE_CHANNEL];  /**< Data upstream and downstream mailboxes */
	mps_mbx_dev command_mb;			 /**< Command upstream and downstream mailbox */

	MbxEventRegs_s event;
			    /**< global structure holding the interrupt status */

	struct semaphore *wakeup_pending;
	struct semaphore *provide_buffer;

} mps_comm_dev;

/*---------------------------------------------------------------------------*/

s32 ifx_mps_common_open (mps_comm_dev * pDev, mps_mbx_dev * pMBDev,
			 bool_t bcommand);
s32 ifx_mps_common_close (mps_mbx_dev * pMBDev);
s32 ifx_mps_mbx_read (mps_mbx_dev * pMBDev, mps_message * pPkg, s32 timeout);
s32 ifx_mps_mbx_write_cmd (mps_mbx_dev * pMBDev, mps_message * readWrite);
s32 ifx_mps_mbx_write_data (mps_mbx_dev * pMBDev, mps_message * readWrite);
u32 ifx_mps_init_structures (mps_comm_dev * pDev);
void ifx_mps_release_structures (mps_comm_dev * pDev);
s32 ifx_mps_restart (void);
s32 ifx_mps_reset (void);
s32 ifx_mps_release (void);
s32 ifx_mps_download_firmware (mps_mbx_dev * pDev, mps_fw * pFWDwnld);
u32 ifx_mps_fifo_mem_available (mps_fifo * mbx);

void ifx_mps_init_gpt_danube (void);
s32 ifx_mps_bufman_init (void);

int ifx_mps_print_fw_version (void);
void ifx_mps_bufman_free (const void *ptr);
void *ifx_mps_bufman_malloc (size_t size, int priority);
#endif /* __MPS_DEVICE_H */
