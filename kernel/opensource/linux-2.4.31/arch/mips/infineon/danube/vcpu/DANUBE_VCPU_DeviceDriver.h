/************************************************************************
 *
 * Copyright (c) 2004
 * Infineon Technologies AG
 * St. Martin Strasse 53; 81669 Muenchen; Germany
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 ************************************************************************/
 
/******************************************************************************
   Module      : DANUBEOAK_DeviceDriver.h
   Date        : $Date: 2005/08/30 10:42:55 $
   Description :
   
   $Log: DANUBE_VCPU_DeviceDriver.h,v $
   Revision 1.1  2005/08/30 10:42:55  pliu
   taken from INCA_IP II
   - change names
   - IM5_11 --> IM4_18

   Revision 1.7  2004/03/22 09:56:25  thomas
   fixed typo

   Revision 1.6  2004/03/04 05:26:15  thomas
   Fixed bit names for MBC_ISR register
   Renamed OAK_DMA to VCPU_GLOBAL_REG
   Added vars for event notification to device struct

   Revision 1.5  2004/02/02 14:08:54  thomas
   Added global flags for event signalling

   Revision 1.22  2003/10/21 14:52:52  aschmidt
   introduced OAK_INTERRUPTS_ENABLE/DISABLE macros to be interrupt safe (Note: to be adapted for Linux!)

   Revision 1.21  2003/07/07 07:21:43  rutkowski
   - Remove unused code.
   - Make the boolean variable to synchronize the interrupt and the task level as 'volatile'.

   Revision 1.20  2003/06/27 12:58:35  rutkowski
   - Add a feature to count the number of different interrupts for debugging. 
     This feature can be enabled by a compiler switch.

   Revision 1.19  2003/06/23 17:05:56  rutkowski
   - First version with the merge of the support for Linux

   Revision 1.18  2003/06/23 15:19:29  rutkowski
   Add the module for the DANUBE VxWorks BSP and change the name of the 
   DANUBE Linux BSP module name to come to a common name convention for 
   both modules
   
******************************************************************************/
#ifndef _DANUBEVCPUDEVICEDRIVER_H
#define _DANUBEVCPUDEVICEDRIVER_H

/*---------------------------------------------------------------------------*/
/* Definition of the Baseaddress of the OAK Device                           */
/*---------------------------------------------------------------------------*/
/* TBD: Adresses still unknown*/

#define MPS_BASEADDRESS 0xBF101400
#define MPS_RAD0SR      MPS_BASEADDRESS + 0x0004

#define MPS_RAD0SR_DU   (1<<0)
#define MPS_RAD0SR_CU   (1<<1)

#define MBX_BASEADDRESS MPS_BASEADDRESS + 0x0100 
#define VCPU_BASEADDRESS 0xBF108000
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Defination of macros for read write of OAK registers                      */
/*---------------------------------------------------------------------------*/
#define OAK_REG_WRITE(Reg_Address,Reg_Value)  (Reg_Address)=(Reg_Value)
#define OAK_REG_READ(Reg_Address)  (Reg_Address)

#define OAK_REG_WRITE_NOP(Reg_Address,Reg_Value)  \
	 asm(".set noreorder"); \
      OAK_REG_WRITE((Reg_Address),(Reg_Value)); \
	 asm("nop;nop;nop;nop;"); \
	 asm(".set reorder"); 


#ifdef VXWORKS
#define OAK_INTERRUPTS_ENABLE(X) 	sysIncaHWRegSetBit(DANUBE_ICU_IM1_IER, X);   
#define OAK_INTERRUPTS_DISABLE(X) 	sysIncaHWRegResetBit(DANUBE_ICU_IM1_IER, X);
#define OAK_INTERRUPTS_CLEAR(X) 	sysIncaHWRegWrite(DANUBE_ICU_IM1_ISR, X);   
#define OAK_INTERRUPTS_SET(X) 		sysIncaHWRegWrite(DANUBE_ICU_IM1_IRSR, X); /* ? */  
#else /* LINUX */
#define MPS_INTERRUPTS_ENABLE(X)  *((volatile u32*) DANUBE_ICU_IM4_IER) |= X;
#define MPS_INTERRUPTS_DISABLE(X) *((volatile u32*) DANUBE_ICU_IM4_IER) &= ~X;
#define MPS_INTERRUPTS_CLEAR(X)   *((volatile u32*) DANUBE_ICU_IM4_ISR) = X;
#define MPS_INTERRUPTS_SET(X)     *((volatile u32*) DANUBE_ICU_IM4_IRSR) = X;  /*  |= ? */

#endif

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Interrupt register values for ICU                                         */
/*---------------------------------------------------------------------------*/
#define DANUBE_MPS_VC0_IR0 (1 << 0)   /* Voice channel 0 */
#define DANUBE_MPS_VC1_IR1 (1 << 1)   /* Voice channel 1 */
#define DANUBE_MPS_VC2_IR2 (1 << 2)   /* Voice channel 2 */
#define DANUBE_MPS_VC3_IR3 (1 << 3)   /* Voice channel 3 */
#define DANUBE_MPS_AD0_IR4 (1 << 4)   /* AFE/DFE Status 0 */
#define DANUBE_MPS_AD1_IR5 (1 << 5)   /* AFE/DFE Status 1 */
#define DANUBE_MPS_VC_AD_ALL_IR6 (1 << 6)  /* ored VC and AD interrupts */
#define DANUBE_MPS_SEM_IR7 (1 << 7)   /* Semaphore Interrupt */
#define DANUBE_MPS_GLB_IR8 (1 << 8)   /* Global Interrupt */



#if 0
#define  EXC_DANUBE_DSP_B3DAIR  (1 << 20)  /* IM1_IRL20  */
#define  EXC_DANUBE_DSP_B2DAIR  (1 << 21)  /* IM1_IRL21  */
#define  EXC_DANUBE_DSP_B1EIR   (1 << 22)  /* IM1_IRL22  */
#define  EXC_DANUBE_DSP_B0EIR   (1 << 23)  /* IM1_IRL23  */
#define  EXC_DANUBE_DSP_WDTIR   (1 << 24)  /* IM1_IRL24  */
#define  EXC_DANUBE_DSP_SCIR0   (1 << 25)  /* IM1_IRL25  */
#define  EXC_DANUBE_DSP_SCIR8   (1 << 26)  /* IM1_IRL26  */
#endif
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Interrupt register values for MBC_ISR                                     */
/*---------------------------------------------------------------------------*/
#define OAK_MBC_ISR_B3DA      (1 << 31)
#define OAK_MBC_ISR_B2DA      (1 << 30)
#define OAK_MBC_ISR_B1E       (1 << 29)
#define OAK_MBC_ISR_B0E       (1 << 28)
#define OAK_MBC_ISR_WDT       (1 << 27)
#define OAK_MBC_ISR_SP_TRA    (1 << 26)
#define OAK_MBC_ISR_SP_IDLE   (1 << 25)
#define OAK_MBC_ISR_SP_DBL    (1 << 24)
#define OAK_MBC_ISR_CMDCERR   (1 << 23)
#define OAK_MBC_ISR_MIPS_OL   (1 << 22)
#define OAK_MBC_ISR_AFE_OF    (1 << 21)
#define OAK_MBC_ISR_DWL_END   (1 << 20)
#define OAK_MBC_ISR_CIDTR_ACT (1 << 18)
#define OAK_MBC_ISR_CIDTR_REQ (1 << 17)
#define OAK_MBC_ISR_CIDTR_BUF (1 << 16)
#define OAK_MBC_ISR_TG_ACT    (1 << 14)
#define OAK_MBC_ISR_DTMFR_DTV (1 << 13)
#define OAK_MBC_ISR_DTMFR_DTP (1 << 12)
#define OAK_MBC_ISR_DTMFR_DTC (0xF << 8)
#define OAK_MBC_ISR_ENC1_OF   (1 << 7)
#define OAK_MBC_ISR_ENC2_OF   (1 << 6)
#define OAK_MBC_ISR_DEC1_ERR  (1 << 5)
#define OAK_MBC_ISR_SR1_ERR   (1 << 5)
#define OAK_MBC_ISR_DEC2_ERR  (1 << 4)
#define OAK_MBC_ISR_SR2_ERR   (1 << 4)
#define OAK_MBC_ISR_DEC1_REQ  (1 << 3)
#define OAK_MBC_ISR_SR1_REQ   (1 << 3)
#define OAK_MBC_ISR_DS1_UF    (1 << 2)
#define OAK_MBC_ISR_SR1_IVW   (1 << 2)
#define OAK_MBC_ISR_DEC2_REQ  (1 << 1)
#define OAK_MBC_ISR_SR2_REQ   (1 << 1)
#define OAK_MBC_ISR_DS2_UF    (1 << 0)
#define OAK_MBC_ISR_SR2_IVW   (1 << 0)
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Register bits definition of the Control Register                          */
/*---------------------------------------------------------------------------*/
#define OAK_MBC_DCTRL_BA      (1 << 0)
#define OAK_MBC_DCTRL_BMOD    (1 << 1)
#define OAK_MBC_DCTRL_RES     (1 << 15)
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Register bits definition of the Configuration Register                    */
/*---------------------------------------------------------------------------*/
#define OAK_MBC_CFG_SWAP2     (2 << 6)
#define OAK_MBC_CFG_RES       (1 << 5)
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Bit definition of the driver-internal status flags                        */
/*---------------------------------------------------------------------------*/
#define OAK_STATUS_EVENT             (1 << 0)
#define OAK_STATUS_                  (1 << 1)
/*---------------------------------------------------------------------------*/



/*---------------------------------------------------------------------------*/
/* Mailbox definitions                                                       */
/*---------------------------------------------------------------------------*/

#define MBX_UPSTRM_CMD_FIFO_BASE   MBX_BASEADDRESS
#define MBX_DNSTRM_CMD_FIFO_BASE   MBX_UPSTRM_CMD_FIFO_BASE + 8
#define MBX_UPSTRM_DATA_FIFO_BASE  MBX_DNSTRM_CMD_FIFO_BASE + 8
#define MBX_DNSTRM_DATA_FIFO_BASE  MBX_UPSTRM_DATA_FIFO_BASE + 8

#define MBX_CMD_FIFO_SIZE  64 /**< Size of command FIFO in bytes */
#define MBX_DATA_FIFO_SIZE 128

#define MBX_DATA_WORDS 96


/*#define NR_DATA_MAILBOXES 4 */    
#define NUM_VOICE_CHANNEL     4 /**< nr of voice channels  */  
#define NR_CMD_MAILBOXES 2    /**< nr of command mailboxes  */

#define MAX_UPSTRM_DATAWORDS 30
#define MAX_FIFO_WRITE_RETRIES 10



/*---------------------------------------------------------------------------*/
/* Register structure definitions                   */
/*---------------------------------------------------------------------------*/

typedef struct  /**< Register structure for Common status registers MPS_RAD0SR, MPS_SAD0SR, 
                     MPS_CAD0SR and MPS_AD0ENR  */ 
{
    u32 res1      : 17;
    u32 wd_fail   : 1;
    u32 res2      : 2;
    u32 mips_ol   : 1;
    u32 data_err  : 1;
    u32 pcm_crash : 1;
    u32 cmd_err   : 1;
    u32 res3      : 6;
    u32 du_mbx    : 1;
    u32 cu_mbx    : 1;    
} MPS_Ad0Reg_s;


typedef union
{
    u32          val;
    MPS_Ad0Reg_s fld;
} MPS_Ad0Reg_u;


typedef struct  /**< Register structure for Common status registers MPS_RAD1SR, MPS_SAD1SR, 
                     MPS_CAD1SR and MPS_AD1ENR  */ 
{
    u32 res1      : 18;
    u32 onhook1   : 1;
    u32 offhook1  : 1;
    u32 otemp1    : 1;
    u32 ltest1    : 1;
    u32 res2      : 4;
    u32 onhook0   : 1;
    u32 offhook0  : 1;
    u32 otemp0    : 1;
    u32 ltest0    : 1;    
    u32 hs_con    : 1;
    u32 hs_dis    : 1;    
} MPS_Ad1Reg_s;


typedef union
{
    u32          val;
    MPS_Ad1Reg_s fld;
} MPS_Ad1Reg_u;



typedef struct   /**< Register structure for Voice Channel Status registers 
                      MPS_RVCxSR, MPS_SVCxSR, MPS_CVCxSR and MPS_VCxENR   */ 
{
    u32 dtmfr_dt   : 1;
    u32 dtmfr_pdt  : 1;  
    u32 dtmfr_dtc  : 4;  
    u32 res1       : 2;
    u32 rcv_ov     : 1;
    u32 dtmfg_buf  : 1;
    u32 dtmfg_req  : 1;
    u32 tg_inact   : 1;
    u32 cis_buf    : 1;
    u32 cis_req    : 1;
    u32 cis_inact  : 1;
    u32 res2       : 11;
    u32 epq_st     : 1;    
    u32 vpou_st    : 1;   
    u32 vpou_jbl   : 1;   
    u32 vpou_jbh   : 1;    
    u32 dec_chg    : 1;        
} MPS_VCStatReg_s;

typedef union
{
    u32          val;
    MPS_Ad0Reg_s fld;
} MMPS_VCStatReg_u;

/*---------------------------------------------------------------------------*/
/* Enum for type of the connection for the device driver                     */
/*---------------------------------------------------------------------------*/
typedef enum
{
   unknown,
   command,
   voice0,
   voice1,
   voice2,
   voice3
}DEVTYPE;


typedef enum
{
   COMMAND=1,
   VOICE
}MbxMessageType_e;



typedef enum
{
   UPSTREAM,
   DOWNSTREAM
}MbxDirection_e;


typedef struct
{
    u32 rw      : 1;
    u32 res1    : 2;
    u32 type    : 5;
    u32 res2    : 4;
    u32 chan    : 4;
    u32 res3    : 1;
    u32 odd     : 2;
    u32 res4    : 5;
    u32 plength : 8;
} MbxMsgHd_s;

typedef union
{
    u32        val;
    MbxMsgHd_s hd;
} MbxMsgHd_u;


typedef struct
{
   MbxMsgHd_u header;
   u32        data[MAX_UPSTRM_DATAWORDS];
}MbxMsg_s;


/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* structure which describes the read- or write- mailbox of the OAK          */
/*---------------------------------------------------------------------------*/
typedef struct
{
   /* Mailbox */
   volatile u32   MBC_IOD;
   volatile u32   MBC_CR;
   volatile u32   MBC_FS;
   volatile u32   MBC_DA;
   volatile u32   MBC_IABS;
   volatile u32   MBC_ITMP;
   volatile u32   MBC_OABS;
   volatile u32   MBC_OTMP;
}OAK_RW_MB;
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* FIFO structure                                                            */
/*---------------------------------------------------------------------------*/
typedef struct
{
   u32            *pStart;   /**< FIFO start address */
   u32            *pEnd;     /**< FIFO end address */
   u32            *pWrite;   /**< Pointer to current FIFO write position */
   u32            *pRead;    /**< Pointer to current FIFO read position */
   u32            FifoSize;  /**< Size of FIFO in words */
}OAK_FIFO_MB;



typedef struct
{
   u32            *pStart;   
   u32            *pEnd;     
   u32            *pWrite;   
   u32            *pRead;    
   u32            FifoSize;  
}MailboxParam_s;




typedef struct
{
   OAK_RW_MB            Reserved[4];
   volatile u32      MBC_CFG;       /* Offset 0x80 */
   volatile u32      MBC_ISR;       /* Offset 0x84 */
   volatile u32      MBC_MSK;       /* Offset 0x88 */
   volatile u32      MBC_MSK01;     /* Offset 0x8C */
   volatile u32      MBC_MSK10;     /* Offset 0x90 */
   volatile u32      MBC_CMD;       /* Offset 0x94 */
   volatile u32      MBC_ACK;       /* Offset 0x98 */
   volatile u32      Reserved2;     /* Offset 0x9C */
   volatile u32      MBC_DCTRL;     /* Offset 0xA0 */
   volatile u32      MBC_DSTA;      /* Offset 0xA4 */
   volatile u32      MBC_DTST1;     /* Offset 0xA8 */
}VCPU_GLOBAL_REG;

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* structure which describes global register of the OAK                      */
/*---------------------------------------------------------------------------*/

typedef struct
{
    volatile u32      MPS_BOOT_RVEC;
    volatile u32      MPS_BOOT_NVEC;
    volatile u32      MPS_BOOT_EVEC;
    volatile u32      MPS_CP0_STATUS;
    volatile u32      MPS_CP0_EEPC;
    volatile u32      MPS_CP0_EPC;    
    volatile u32      MPS_BOOT_SIZE;
    volatile u32      MPS_CFG_STAT;    

} CPU_BOOT_CFG_REG;

typedef struct
{
   volatile u32      MBX_UPSTR_CMD_BASE;  /**< Upstream Command FIFO Base Address */
   volatile u32      MBX_UPSTR_CMD_SIZE;  /**< Upstream Command FIFO size in byte */
   volatile u32      MBX_DNSTR_CMD_BASE;  /**< Downstream Command FIFO Base Address */
   volatile u32      MBX_DNSTR_CMD_SIZE;  /**< Downstream Command FIFO size in byte */
   volatile u32      MBX_UPSTR_DATA_BASE; /**< Upstream Data FIFO Base Address */
   volatile u32      MBX_UPSTR_DATA_SIZE; /**< Upstream Data FIFO size in byte */
   volatile u32      MBX_DNSTR_DATA_BASE; /**< Downstream Data FIFO Base Address */
   volatile u32      MBX_DNSTR_DATA_SIZE; /**< Downstream Data FIFO size in byte */
   volatile u32      MBX_UPSTR_CMD_READ;  /**< Upstream Command FIFO Read Index */
   volatile u32      MBX_UPSTR_CMD_WRITE; /**< Upstream Command FIFO Write Index */
   volatile u32      MBX_DNSTR_CMD_READ;  /**< Downstream Command FIFO Read Index */
   volatile u32      MBX_DNSTR_CMD_WRITE; /**< Downstream Command FIFO Write Index */   
   volatile u32      MBX_UPSTR_DATA_READ;  /**< Upstream Data FIFO Read Index */
   volatile u32      MBX_UPSTR_DATA_WRITE; /**< Upstream Data FIFO Write Index */
   volatile u32      MBX_DNSTR_DATA_READ;  /**< Downstream Data FIFO Read Index */
   volatile u32      MBX_DNSTR_DATA_WRITE; /**< Downstream Data FIFO Write Index */   
   volatile u32      MBX_DATA[MBX_DATA_WORDS];
   CPU_BOOT_CFG_REG  MBX_CPU0_BOOT_CFG;    /**< CPU0 Boot Configuration */   
   CPU_BOOT_CFG_REG  MBX_CPU1_BOOT_CFG;    /**< CPU1 Boot Configuration */  
}MBX_GLOBAL_REG;
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Device connection structure                                               */
/*---------------------------------------------------------------------------*/


/**
 * Mailbox Device Structure.
 * This Structure holds top level parameters of the mailboxes used in DANUBE
 * to allow the communication between the control CPU and the Voice CPU
 */ 
typedef struct
{
   /* void pointer to the base device driver structure */
   void              *pVCPU_DEV;

   /* Mutex semaphore to access the device */
   SEM_ID            sem_dev;

   /* Mutex semaphore to access the device */
   SEM_ID            sem_read_fifo;
   
   /* Wakeuplist for the select mechanism */
   SYS_POLL          oak_wklist;

   /* Base Address of the Mailbox Access */
   OAK_RW_MB         *pBaseReadMB;
   OAK_RW_MB         *pBaseWriteMB;
      
   /* Base Address of the Global Register Access */
   MBX_GLOBAL_REG    *pBaseGlobal;
      
   /* missing space in fifo */
   /* used if the interrupt is disabled and waiting for this amount of  */
   /* free bytes in the fifo to get enabled again                       */
   u32            irqMissingBytes;
   u32            VoiceChannelStream;
   
   /* Mask Bits for the IRQ register of the mailbox */
   u32            MBC_Mask;
   u32            MBC_Mask01;
   u32            MBC_Mask10;
   
   MailboxParam_s    upstrm_fifo;  /**< Data exchange FIFO for write (downstream) */ 
   MailboxParam_s    dwstrm_fifo;  /**< Data exchange FIFO for read (upstream) */
   
#ifdef OAK_FIFO_BLOCKING_WRITE
   SEM_ID           sem_write_fifo;
#endif /* OAK_FIFO_BLOCKING_WRITE */

   u32            errnum;
   
#ifdef CONFIG_PROC_FS
   /* interrupt counter */
   volatile u32   RxnumIRQs;
   volatile u32   TxnumIRQs;

   volatile u32   RxnumMiss;
   volatile u32   TxnumMiss;
  
   volatile u32   RxnumBytes;
   volatile u32   TxnumBytes;
#endif /* CONFIG_PROC_FS */

   DEVTYPE           devID;  /**< Device ID  1->command 
                                             2->voice chan 0
                                             3->voice chan 1 
                                             4->voice chan 2 
                                             5->voice chan 3 */
   
   u32            HeaderORMask;
   u32            HeaderANDMask;

#ifdef OAK_FIFO_BLOCKING_WRITE
   volatile BOOL     full_write_fifo;   
#endif /* OAK_FIFO_BLOCKING_WRITE */

   /* installed or not? */
   BOOL              Installed;
   
   /* missing voice package for downstream */
   BOOL              VoiceMissingPackage;
   
#ifdef OAK_FIFO_BLOCKING_WRITE
   /* variable if the driver should block on write to the transmit FIFO */
   BOOL              bBlockWriteMB;
#endif /* OAK_FIFO_BLOCKING_WRITE */
   void              (*down_callback)(DEVTYPE type);
   void              (*up_callback)(DEVTYPE type);
   UINT              event;
   UINT              event_mask;
   void              (*event_callback)(UINT mask);
   
} MPS_MB_DEV_STR;
/*---------------------------------------------------------------------------*/




/*---------------------------------------------------------------------------*/
/* Device structure                                                          */
/*---------------------------------------------------------------------------*/

/**
 * Mailbox Device Structure.
 * This Structure represents the communication device that provides the resources 
 * for the communication between CPU0 and CPU1
 */ 
typedef struct
{
#ifdef VXWORKS
   /* VxWorks dependent structure */
   DEV_HDR           oak_dev_hdr; /**< Pointer to private date of the specific handler */
#endif /* #ifdef VXWORKS */

   MBX_GLOBAL_REG    *pBaseGlobal; /**< global register pointer for the ISR */
   u32	         flags;        /**< Pointer to private date of the specific handler */
   

   MPS_MB_DEV_STR    VoiceMB[NUM_VOICE_CHANNEL];   /**< Data upstream and downstream mailboxes */
   MPS_MB_DEV_STR    CommandMB;                   /**< Command upstream and downstream mailbox */
      
}MPS_COMM_DEV;

/*---------------------------------------------------------------------------*/
#endif /* _DANUBEVCPUDEVICEDRIVER_H */


