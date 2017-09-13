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
   File        : $RCSfile: DANUBE_VCPU_DeviceDriver.c,v $
   Revision    : $Revision: 1.1 $
   Date        : $Date: 2005/08/30 10:42:55 $
   Description : Generic driver module for the DANUBE OAK DSP

******************************************************************************/
#include "drv_api.h"
#include "DANUBE_VCPU_DeviceDriver.h"
#include "DANUBE_VCPU_DeviceDriver_Ext.h"
#include <asm/danube/danube.h>

#ifdef VXWORKS
#include <common/src/sys_drv_debug.h>
#endif /* VXWORKS */
   
#ifdef LINUX
/* #include <common/src/sys_debug.h>  */
#include <linux/interrupt.h>
#endif /* LINUX */


/* Declarations for debug interface */
DECLARE_TRACE_GROUP(DANUBE_MPS_DSP_DRV)
DECLARE_LOG_GROUP(DANUBE_MPS_DSP_DRV)

/*----------------------------------------------------------------------------*/
/* Function Declaration which are required by the firmware download feature   */
/*----------------------------------------------------------------------------*/
/*
OAK_INTERN int oak_write_cmd_mailbox(register MPS_MB_DEV_STR *pMBDev, 
   DSP_READWRITE *readWrite);
*/   
   
s32 DANUBE_MPS_DSP_Restart( void );
s32 DANUBE_MPS_DSP_Reset( void );
static void DANUBE_MPS_MbxNotifyUpstrCmdQueue( unsigned long dummy );
static void DANUBE_MPS_MbxNotifyUpstrDataQueue( unsigned long dummy );
 
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* #include "DANUBE_VCPU_Download.h"  */
#include "DANUBE_VCPU_Download.c"
/*

#include "DANUBE_VCPU_Memaccess.c"
#include "DANUBE_VCPU_Primitives.c"
#include "DANUBE_VCPU_Command0.c"
#include "DANUBE_VCPU_Command1.c"
#include "DANUBE_VCPU_Command3.c"
#include "DANUBE_VCPU_Command2.c"
#include "DANUBE_VCPU_Command4.c"

*/
/*----------------------------------------------------------------------------*/
/* Mask bit for the downstream voice package firmware header                  */
/*----------------------------------------------------------------------------*/
#define OAK_VOICE0_AND_MASK   0xFFFEFFFF
#define OAK_VOICE0_OR_MASK    0x80000000
#define OAK_VOICE1_AND_MASK   0xFFFFFFFF
#define OAK_VOICE1_OR_MASK    0x80010000
/*----------------------------------------------------------------------------*/

#define OAK_WRITE_MB_TIMEOUT  10

s32 debug_oak_int=0;

#ifdef VXWORKS
extern MPS_COMM_DEV  *pOakDev;
MPS_COMM_DEV *pMPSDev=pOakDev;
#else
extern MPS_COMM_DEV DANUBE_MPS_dev;
MPS_COMM_DEV *pMPSDev=&DANUBE_MPS_dev;
#endif

/*
 * tasklets for upstream command and data notification 
 */
static void DANUBE_MPS_MbxNotifyUpstrDataQueue( unsigned long dummy );
static void DANUBE_MPS_MbxNotifyUpstrCmdQueue( unsigned long dummy );

DECLARE_TASKLET(DANUBE_MPS_DataUpstrmTasklet, DANUBE_MPS_MbxNotifyUpstrDataQueue, 0 );
DECLARE_TASKLET(DANUBE_MPS_CmdUpstrmTasklet, DANUBE_MPS_MbxNotifyUpstrCmdQueue, 0 );

/**
 * Clear Mailbox Fifos 
 * This function clears the mailbox FIFO assigned by pFifo.
 * 
 * 
 * 
 * \param *pFifo   pointer to mailbox device structure
 * \return Nothing
 */
void DANUBE_MPS_DSP_MbxClearFifo(MailboxParam_s *pFifo)
{
    TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW, ("DANUBE_MPS_DSP_MbxClearFifo()\n"));  

    pFifo->pRead  = pFifo->pStart;
    pFifo->pWrite = pFifo->pStart;
    return;
}


/****************************************************************************
  Common maintenance routines
****************************************************************************/

/**
 * Open routine for the VCPU device driver
 * 
 * 
 * 
 * 
 * \param *pDev   pointer to communication devive structure
 * \param *pMBDev pointer to mailbox structure to be accessed
 * \param  bcommand voice/command selector, TRUE -> command, FALSE -> voice
 * \return OK    - in case of successful opening
 *         ERROR - already installed driver
 */
s32 DANUBE_MPS_DSP_CommonOpen(MPS_COMM_DEV *pDev, 
                               MPS_MB_DEV_STR *pMBDev, 
                               BOOL bcommand)
{
/*   VCPU_GLOBAL_REG *pBaseGlobal;  */
    MPS_Ad0Reg_u Ad0Reg;

    TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW, ("DANUBE_MPS_DSP_CommonOpen()\n"));  

    /* device is already installed */
    if (pMBDev->Installed == TRUE)
    {
#ifdef VXWORKS
        errno = S_iosLib_OAKMB_DEV_INST ;
#endif /* VXWORKS */
        return (ERROR);
    }

    pMBDev->Installed = TRUE;

    /* enable necessary MPS interrupts */   
    Ad0Reg.val = *DANUBE_MPS_AD0ENR;
    if (bcommand == TRUE)
    {
        /* enable upstream command interrupt */
        Ad0Reg.fld.cu_mbx = 1;
    }
    else
    {
        /* enable upstream voice interrupt */
        Ad0Reg.fld.du_mbx = 1;
    }
   
    Ad0Reg.fld.wd_fail   = 1;  /* enable watchdog failure Int. */
    Ad0Reg.fld.mips_ol   = 1;  /* enable MIPS overload Int. */  
    Ad0Reg.fld.data_err  = 1;  /* enable data error Int. */   
    Ad0Reg.fld.pcm_crash = 1;  /* enable PCM highway crash  Int. */
    Ad0Reg.fld.cmd_err   = 1;  /* enable command error Int. */
    *DANUBE_MPS_AD0ENR = Ad0Reg.val;
   
    return (OK);
}



/**
 * Close routine for the VCPU device driver
 * This function closes the channel assigned to the passed mailbox 
 * device structure.
 * 
 * 
 * 
 *
 * \param *pMBDev pointer to mailbox device structure to be accessed
 *
 * \return OK always
 */
s32 DANUBE_MPS_DSP_CommonClose(MPS_MB_DEV_STR *pMBDev)
{
    MPS_Ad0Reg_u Ad0Reg;
    
    TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW, ("DANUBE_MPS_DSP_CommonClose()\n"));   

    /* clean data structures */
    pMBDev->Installed = FALSE;
       
/*   pBaseGlobal = pMBDev->pBaseGlobal;        */
 

   /* Clear the downstream queues for voice fds only*/
   if(pMBDev->devID != command)
   {
      /* if all voice channel connections are closed disable voice channel interrupt */
      pMPSDev = pMBDev->pVCPU_DEV;
      if( (pMPSDev->VoiceMB[0].Installed == FALSE) &&
          (pMPSDev->VoiceMB[1].Installed == FALSE) &&
          (pMPSDev->VoiceMB[2].Installed == FALSE) &&
          (pMPSDev->VoiceMB[3].Installed == FALSE) )
      {
          /* disable upstream voice interrupt */
          Ad0Reg.val = *DANUBE_MPS_AD0ENR;
          Ad0Reg.fld.du_mbx = 0;
          *DANUBE_MPS_AD0ENR = Ad0Reg.val;          

          /* Clear the upstream downstream queue */
          DANUBE_MPS_DSP_MbxClearFifo(&pMBDev->dwstrm_fifo); 
          DANUBE_MPS_DSP_MbxClearFifo(&pMBDev->upstrm_fifo);
      }
   }
   else
   {
       /* disable upstream command interrupt */
       Ad0Reg.val = *DANUBE_MPS_AD0ENR;
       Ad0Reg.fld.cu_mbx = 0;       
      *DANUBE_MPS_AD0ENR = Ad0Reg.val; 
      /* Clear the upstream queue */
      DANUBE_MPS_DSP_MbxClearFifo(&pMBDev->upstrm_fifo);
      /* Clear the downstream queue */
      DANUBE_MPS_DSP_MbxClearFifo(&pMBDev->dwstrm_fifo);         
   }
   return (OK);
}




/**
 * Restart the VCPU
 * This function restarts the DSP by accessing the reset request register.
 * 
 * 
 * 
 *
 * \param None
 *
 * \return OK    - in case of successful restart 
 *         ERROR - in case of failed VCPU reset (currently always OK)
 */
s32 DANUBE_MPS_DSP_Restart( void )
{
    u32 reset_ctrl;
    s32 i;

    TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW, ("DANUBE_MPS_DSP_Restart()\n"));  

    /* stop VCPU program execution */
    if(DANUBE_MPS_DSP_Reset() == ERROR)
        return(ERROR);

    /* reset all mailbox FIFOs */
    for ( i=0; i<NUM_VOICE_CHANNEL; i++ )
    {    
        DANUBE_MPS_DSP_MbxClearFifo(&(pMPSDev->VoiceMB[i].dwstrm_fifo));
        DANUBE_MPS_DSP_MbxClearFifo(&(pMPSDev->VoiceMB[i].upstrm_fifo));        
    }
    DANUBE_MPS_DSP_MbxClearFifo(&(pMPSDev->CommandMB.dwstrm_fifo));
    DANUBE_MPS_DSP_MbxClearFifo(&(pMPSDev->CommandMB.upstrm_fifo));  
    /* start VCPU program execution */
    reset_ctrl = *DANUBE_RCU_RST_REQ;  
    *DANUBE_RCU_RST_REQ = reset_ctrl | DANUBE_RCU_RST_REQ_CPU0_BR;          
    return (OK);   
    
#if 0    
    u32   j = 0;
   volatile u32 val;
   
   /* reset the DSP again to jump into the code */
   pBaseAddress->MBC_DCTRL = 0;
   pBaseAddress->MBC_DCTRL = OAK_MBC_DCTRL_RES;

   /* wait till the reset is finished */
   do
   {
#ifdef VXWORKS
      if (++j < sysClkRateGet())
#else
      if (++j < 100)
#endif /* VXWORKS */
     {
         Wait(10);
      } else {
         return (ERROR);
      }
      val = pBaseAddress->MBC_DCTRL;
   } while (val & OAK_MBC_DCTRL_RES);

   /* reset the mailboxes of the DSP and enable the swap unit */
   pBaseAddress->MBC_CFG = OAK_MBC_CFG_SWAP2 | OAK_MBC_CFG_RES;
 
   /* wait till the mailbox is reseted */
   j = 0;
   do
   {
#ifdef VXWORKS
      if (++j < sysClkRateGet())
#elif defined(LINUX)
      if (++j < 100)
#else
#error No OS parameter selected
#endif /* VXWORKS */
      {
         Wait(10);
      } else {
         return (ERROR);
      }
      val = pBaseAddress->MBC_CFG;
   } while (val & OAK_MBC_CFG_RES);
   
   return (OK);
#endif

}


/**
 * Resets the DSP at Internal Download Address
 * This function causes a reset of the VCPU by clearing the CPU0 boot ready bit
 * in the reset request register RCU_RR.
 * It does not change the boot configuration registers for CPU0 or CPU1.
 * 
 * 
 * \param   None
 * \return  always OK
 */
s32 DANUBE_MPS_DSP_Reset( void )
{
    u32 reset_ctrl;
    
    TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW, ("DANUBE_MPS_DSP_Reset()\n"));  

    reset_ctrl = *DANUBE_RCU_RST_REQ;  
    *DANUBE_RCU_RST_REQ = reset_ctrl & ~DANUBE_RCU_RST_REQ_CPU0_BR;    
    return (OK);
    
#if 0    
    
    u32   j = 0;
   volatile u32 val;
   
   /* enable the clock for the DSP module */
#ifdef VXWORKS
   sysIncaHWRegSetBit(DANUBE_PMU_PM_GEN, DANUBE_PMU_PM_GEN_EN7);
#else
   /* TODO: Make this atomic... */
   *((volatile u32*)DANUBE_PMU_PM_GEN) |= DANUBE_PMU_PM_GEN_EN7;
#endif

   /* soft reset the DSP to bring up in download mode */
   pBaseAddress->MBC_DCTRL = OAK_MBC_DCTRL_BA | OAK_MBC_DCTRL_BMOD;
   pBaseAddress->MBC_DCTRL = OAK_MBC_DCTRL_BA | OAK_MBC_DCTRL_BMOD |
      OAK_MBC_DCTRL_RES;

   /* wait till the reset is finished */
   do
   {
#ifdef VXWORKS
      if (++j < sysClkRateGet())
#else
      if (++j < 100)
#endif /* VXWORKS */
     {
         Wait(10);
      } else {
         return (ERROR);
      }
      val = pBaseAddress->MBC_DCTRL;
   } while (val & OAK_MBC_DCTRL_RES);

   /* reset the mailboxes of the DSP and enable the swap unit */
   pBaseAddress->MBC_CFG = OAK_MBC_CFG_SWAP2 | OAK_MBC_CFG_RES;

   /* wait till the mailbox is reseted */
   j = 0;
   do
   {
#ifdef VXWORKS
      if (++j < sysClkRateGet())
#elif LINUX
      if (++j < 100)
#else
#error No OS parameter selected
#endif /* VXWORKS */
      {
         Wait(10);
      } else {
         return (ERROR);
      }
      val = pBaseAddress->MBC_CFG;
   } while (val & OAK_MBC_CFG_RES);
   
   return (OK);
#endif

}

/****************************************************************************
Description :
   Initialize and reboot the DSP  
Arguments   :
Return      :
Remarks     :
****************************************************************************/
void OAK_Drv(void)
{
   /* disable the interrupt of the ICU */
   /* disable IM1_IER register for bits IRL20 to IRL26 */
   MPS_INTERRUPTS_DISABLE(0x07F00000);
}

#ifdef LINUX

/**
 * 
 * 
 * 
 * 
 * \param 
 * \return
 */
 
 
 
/**
 * MPS Structure Release
 * This function releases the entire MPS data structure used for communication 
 * between the CPUs of DANUBE.
 * 
 * 
 * \param  *pdev poiter to MPS communication structure
 * \return Nothing
 */
void DANUBE_MPS_DSP_ReleaseStructures(MPS_COMM_DEV *pDev)
{
   s32      count;

   TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW, ("DANUBE_MPS_DSP_ReleaseStructures()\n"));  
   /* Initialize the Message queues for the voice packets */
   for( count=0 ; count < NUM_VOICE_CHANNEL ; count++)
   {
      Free_Mem(pDev->VoiceMB[count].upstrm_fifo.pStart);
      Free_Mem(pDev->VoiceMB[count].dwstrm_fifo.pStart);

      Sem_Free(pDev->VoiceMB[count].sem_dev);
      Sem_Free(pDev->VoiceMB[count].sem_read_fifo);

#ifdef OAK_FIFO_BLOCKING_WRITE
      Sem_Free(pDev->VoiceMB[count].sem_write_fifo);
#endif /* OAK_FIFO_BLOCKING_WRITE */
  }

   Free_Mem(pDev->CommandMB.upstrm_fifo.pStart);
   Free_Mem(pDev->CommandMB.dwstrm_fifo.pStart);      

   Sem_Free(pDev->CommandMB.sem_dev);
   Sem_Free(pDev->CommandMB.sem_read_fifo);
}
#endif /* LINUX */


/**
 * MPS Structure Initialization
 * This function initializes the data structures of the Multi Processor System 
 * that are necessary for the 2 processor communication
 *
 * \param *pDev  Pointer to MPS device structure to be initialized
 * \return Returns OK if initialization was successful 
 * or ERROR in case of allocation or semaphore access problems
 */
u32 DANUBE_MPS_InitStructures(MPS_COMM_DEV *pDev)
{
    s32      count;
    u32   *ui32ptr;
    MBX_GLOBAL_REG *MBX_Memory; 
    s32 i;

    TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW, ("DANUBE_MPS_InitStructures()\n"));  
    /* Initialize MPS_MB_DEV_STR stucture */
    memset((void*)pDev, 0, sizeof(MPS_COMM_DEV));

    pDev->pBaseGlobal = (MBX_GLOBAL_REG*) MBX_BASEADDRESS;
    pDev->flags       = 0x00000000;   
    MBX_Memory = pDev->pBaseGlobal;    

    /* Initialize common mailbox registers that are used by both CPUs 
     * for MBX communication 
     */
    MBX_Memory->MBX_UPSTR_CMD_BASE   = MBX_UPSTRM_CMD_FIFO_BASE;  
    MBX_Memory->MBX_UPSTR_CMD_SIZE   = MBX_CMD_FIFO_SIZE;  
    MBX_Memory->MBX_DNSTR_CMD_BASE   = MBX_DNSTRM_CMD_FIFO_BASE;  
    MBX_Memory->MBX_DNSTR_CMD_SIZE   = MBX_CMD_FIFO_SIZE;  
    MBX_Memory->MBX_UPSTR_DATA_BASE  = MBX_UPSTRM_DATA_FIFO_BASE; 
    MBX_Memory->MBX_UPSTR_DATA_SIZE  = MBX_DATA_FIFO_SIZE; 
    MBX_Memory->MBX_DNSTR_DATA_BASE  = MBX_DNSTRM_DATA_FIFO_BASE; 
    MBX_Memory->MBX_DNSTR_DATA_SIZE  = MBX_DATA_FIFO_SIZE; 
    /* set read and write pointers to FIFO's uppermost addresses */
    MBX_Memory->MBX_UPSTR_CMD_READ   = MBX_UPSTRM_CMD_FIFO_BASE + 
                                      (MBX_Memory->MBX_UPSTR_CMD_SIZE/4) - 1;  
    MBX_Memory->MBX_UPSTR_CMD_WRITE  = MBX_Memory->MBX_UPSTR_CMD_READ; 
    MBX_Memory->MBX_DNSTR_CMD_READ   = MBX_Memory->MBX_DNSTR_CMD_BASE + 
                                      (MBX_Memory->MBX_DNSTR_CMD_SIZE/4) - 1;  
    MBX_Memory->MBX_DNSTR_CMD_WRITE  = MBX_Memory->MBX_DNSTR_CMD_READ; 
    MBX_Memory->MBX_UPSTR_DATA_READ  = MBX_Memory->MBX_UPSTR_DATA_BASE +
                                      (MBX_Memory->MBX_UPSTR_DATA_SIZE/4) - 1; 
    MBX_Memory->MBX_UPSTR_DATA_WRITE = MBX_Memory->MBX_UPSTR_DATA_READ;
    MBX_Memory->MBX_DNSTR_DATA_READ  = MBX_Memory->MBX_DNSTR_DATA_BASE +
                                      (MBX_Memory->MBX_DNSTR_DATA_SIZE/4) - 1; ; 
    MBX_Memory->MBX_DNSTR_DATA_WRITE = MBX_Memory->MBX_DNSTR_DATA_READ;

    /* Initialize data exchange memory area with zeros*/
    memset((void*)MBX_Memory->MBX_DATA, 0, MBX_DATA_WORDS*sizeof(u32));
  
    /* set command mailbox sub structure pointers 
       to global mailbox register addresses */
    pDev->CommandMB.upstrm_fifo.pStart   =
        (u32*)(MBX_Memory->MBX_UPSTR_CMD_BASE + (MBX_Memory->MBX_UPSTR_CMD_SIZE/4) - 1);  
    pDev->CommandMB.upstrm_fifo.pEnd     = (u32*)MBX_Memory->MBX_UPSTR_CMD_BASE;
    pDev->CommandMB.upstrm_fifo.pWrite   = (u32*)MBX_Memory->MBX_UPSTR_CMD_WRITE;
    pDev->CommandMB.upstrm_fifo.pRead    = (u32*)MBX_Memory->MBX_UPSTR_CMD_READ;
    pDev->CommandMB.upstrm_fifo.FifoSize = MBX_Memory->MBX_UPSTR_CMD_SIZE;  
  
    pDev->CommandMB.dwstrm_fifo.pStart   = 
        (u32*) (MBX_DNSTRM_CMD_FIFO_BASE + (MBX_Memory->MBX_DNSTR_CMD_SIZE/4) - 1);  
    pDev->CommandMB.dwstrm_fifo.pEnd     = (u32*)MBX_Memory->MBX_DNSTR_CMD_BASE;
    pDev->CommandMB.dwstrm_fifo.pWrite   = (u32*)MBX_Memory->MBX_DNSTR_CMD_WRITE;
    pDev->CommandMB.dwstrm_fifo.pRead    = (u32*)MBX_Memory->MBX_DNSTR_CMD_READ;
    pDev->CommandMB.dwstrm_fifo.FifoSize = MBX_Memory->MBX_DNSTR_CMD_SIZE;    
 
    
    pDev->CommandMB.pVCPU_DEV = pDev;    /* global pointer reference */
    pDev->CommandMB.Installed = FALSE ;  /* current installation status */

   
    /* initialize voice channel communication structure fields 
     * that are common to all voice channels  */
    for ( i=0; i<NUM_VOICE_CHANNEL; i++ )
    {
        /* 
         * Set the upstream mailbox pointers of all voice channels 
         * to the upstream data mailbox area. 
         * All voice channels use the same mailbox memory. 
         */  
        /* voice upstream data mailbox area */ 
        pDev->VoiceMB[i].upstrm_fifo.pStart      = 
            (u32*)(MBX_Memory->MBX_UPSTR_DATA_BASE + 
                  (MBX_Memory->MBX_UPSTR_DATA_SIZE/4) - 1);  
        pDev->VoiceMB[i].upstrm_fifo.pEnd        = (u32*)MBX_Memory->MBX_UPSTR_DATA_BASE;
        pDev->VoiceMB[i].upstrm_fifo.pWrite      = (u32*)MBX_Memory->MBX_UPSTR_DATA_WRITE;
        pDev->VoiceMB[i].upstrm_fifo.pRead       = (u32*)MBX_Memory->MBX_UPSTR_DATA_READ;
        pDev->VoiceMB[i].upstrm_fifo.FifoSize    = MBX_Memory->MBX_UPSTR_DATA_SIZE;  
        /* voice downstream data mailbox area */    
        pDev->VoiceMB[i].dwstrm_fifo.pStart      = 
            (u32*) (MBX_DNSTRM_DATA_FIFO_BASE + (MBX_Memory->MBX_DNSTR_DATA_SIZE/4) - 1);  
        pDev->VoiceMB[i].dwstrm_fifo.pEnd        = (u32*)MBX_Memory->MBX_DNSTR_DATA_BASE;
        pDev->VoiceMB[i].dwstrm_fifo.pWrite      = (u32*)MBX_Memory->MBX_DNSTR_DATA_WRITE;
        pDev->VoiceMB[i].dwstrm_fifo.pRead       = (u32*)MBX_Memory->MBX_DNSTR_DATA_READ;
        pDev->VoiceMB[i].dwstrm_fifo.FifoSize    = MBX_Memory->MBX_DNSTR_DATA_SIZE;    

        pDev->VoiceMB[i].Installed = FALSE ;   /* current mbx installation status */     
        pDev->VoiceMB[i].pBaseGlobal = (MBX_GLOBAL_REG*) VCPU_BASEADDRESS;
        pDev->VoiceMB[i].pVCPU_DEV = pDev; /* global pointer reference */
#if 0
        /* initialize devices with control addresses of the voice mailbox */
        pDev->VoiceMB[i].pBaseWriteMB =
         (OAK_RW_MB*) &(pDev->VoiceMB[i].pBaseGlobal)->Reserved[0];

        pDev->VoiceMB[i].pBaseReadMB =
            (OAK_RW_MB*) &(pDev->VoiceMB[i].pBaseGlobal)->Reserved[2];
#endif
        pDev->VoiceMB[i].down_callback = NULL; /* callback functions for */
        pDev->VoiceMB[i].up_callback = NULL;   /* down- and upstream dir. */

        /* initialize the semaphores for multitasking access */
        Sem_BinaryInit(pDev->VoiceMB[i].sem_dev);

        /* initialize the semaphores to read from the fifo */
        Sem_BinaryInit(pDev->VoiceMB[i].sem_read_fifo);
        Sem_Lock(pDev->VoiceMB[i].sem_read_fifo);

#ifdef OAK_FIFO_BLOCKING_WRITE
        Sem_BinaryInit(pDev->VoiceMB[i].sem_write_fifo);
        Sem_Lock(pDev->VoiceMB[i].sem_write_fifo);
        pDev->VoiceMB[i].bBlockWriteMB = TRUE;        
#endif /* OAK_FIFO_BLOCKING_WRITE */
    
        if(pDev->VoiceMB[i].sem_dev == NULL)
            return (ERROR);

        /* select mechanism implemented for each queue */
        Init_WakeupQueue(pDev->VoiceMB[i].oak_wklist);        
    }

    /* set channel identifiers */
    pDev->CommandMB.devID  = command;    
    pDev->VoiceMB[0].devID = voice0;
    pDev->VoiceMB[1].devID = voice1;
    pDev->VoiceMB[2].devID = voice2;
    pDev->VoiceMB[3].devID = voice3;    

#if 0      

   /* setup the interrupt mask for the device */
   pDev->VoiceMB[0].MBC_Mask   = OAK_MBC_ISR_B2DA;
   pDev->VoiceMB[0].MBC_Mask01 = OAK_MBC_ISR_DEC1_REQ;
   pDev->VoiceMB[0].MBC_Mask10 = OAK_MBC_ISR_DEC1_REQ;
   pDev->VoiceMB[1].MBC_Mask   = OAK_MBC_ISR_B2DA;
   pDev->VoiceMB[1].MBC_Mask01 = OAK_MBC_ISR_DEC2_REQ;
   pDev->VoiceMB[1].MBC_Mask10 = OAK_MBC_ISR_DEC2_REQ;
   pDev->CommandMB.MBC_Mask    = OAK_MBC_ISR_B3DA;

#endif   

   return 0;   
}



/**
 * Check Fifo for being not empty
 * This function checks whether the Fifo assigned by mbx contains at least 
 * one unread data word 
 * 
 *
 * \param 
 * \param *mbx pointer to mailbox structure to be checked
 *
 * \return Returns TRUE if data to be read is available in FIFO, 
 * returns FALSE if FIFO is empty.
 */
BOOL DANUBE_MPS_MbxFifoNotEmpty( MailboxParam_s *mbx )
{
    TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW, ("DANUBE_MPS_MbxFifoNotEmpty()\n"));  

    if(mbx->pWrite == mbx->pRead)
        return FALSE ;
    else 
        return TRUE;
}



/**
 * Check Fifo for free memory
 * This function returns the amount of free memory of the assigned mbx-FIFO
 * 
 *
 * \param *mbx pointer to mailbox structure to be checked
 *
 * \return Returns 0 if the FIFO is full, 
 * returns the amount of available 32 bit words (1 ... FIFO_MAX_SIZE) 
 * in case of available memory.
 */
s8 DANUBE_MPS_MbxFifoMemAvailable( MailboxParam_s *mbx )
{
    TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW, ("DANUBE_MPS_MbxFifoMemAvailable()\n"));  
    TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW, ("Free Mbx Memory: %d words\n", mbx->FifoSize - (((u32)(mbx->pRead - mbx->pWrite)) & ((u32)(mbx->FifoSize - 1)))));
    return(mbx->FifoSize - (((u32)(mbx->pRead - mbx->pWrite)) & ((u32)(mbx->FifoSize - 1))));
}



/**
 * Check Fifo for requested amount of memory
 * This function checks whether the requested mbx-FIFO is capable to store
 * the requested amount of data words.
 * The selected Fifo should be a downstream direction Fifo. 
 * 
 *
 * \param *mbx    pointer to mailbox structure to be checked
 * \param req_mem requested data words 
 *
 * \return Returns TRUE if data to be read is available in FIFO, 
 * returns FALSE if FIFO is empty.
 */
BOOL DANUBE_MPS_MbxFifoMemReq( MailboxParam_s *mbx, u32 req_mem )
{
    TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW, ("DANUBE_MPS_MbxFifoMemReq(req: %d words)\n",req_mem));  

    if( ((u32)DANUBE_MPS_MbxFifoMemAvailable( mbx )) >= req_mem )
        return TRUE;
    else
        return FALSE ;
}



/**
 * Gets channel ID field from message header
 * This function reads the data word at the read pointer position 
 * of the mailbox FIFO pointed to by mbx and extracts the channel ID field 
 * from the data word read.  
 *
 * \param *mbx pointer to mailbox structure to be accessed
 *
 * \return voice channel identifier. 
 */
DEVTYPE DANUBE_MPS_MbxGetMsgChannel( MailboxParam_s *mbx )
{
    u8 channel;
    MbxMsgHd_u  msg_hd;
    DEVTYPE retval=unknown;

    TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW, ("DANUBE_MPS_MbxGetMsgChannel()\n"));      
    msg_hd.val = *(mbx->pRead);
    switch(msg_hd.hd.chan)
    {
        case 0:
            retval = voice0; 
            TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW, 
                  ("DANUBE_MPS_MbxGetMsgChannel(): voice0 channel\n"));      
            break;
        case 1:
            retval = voice1;
            TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW, 
                  ("DANUBE_MPS_MbxGetMsgChannel(): voice1 channel\n"));                  
            break;
        case 2:
            retval = voice2;
            TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW, 
                  ("DANUBE_MPS_MbxGetMsgChannel(): voice2 channel\n"));                  
            break;            
        case 3:
            TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW, 
              ("DANUBE_MPS_MbxGetMsgChannel(): voice3 channel\n"));      
            retval = voice3;
            break; 
        default:
            retval = unknown;
            TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_HIGH, 
                ("DANUBE_MPS_MbxGetMsgChannel(): unknown channel ID\n"));                  
            break;
    }

    return retval;
}



/**
 * Return length of message located at read pointer position in bytes 
 * This function reads the plength field of the message header (length in 
 * 16 bit halfwords) adds the header length and returns the complete length in bytes. 
 * 
 *
 * \param *mbx pointer to mailbox structure to be accessed
 *
 * \return length of message in bytes. 
 */
s32 DANUBE_MPS_MbxGetMsgLengthBytes( MailboxParam_s *mbx )
{
    MbxMsgHd_u  msg_hd;
    s32 retval=0;

    TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW, ("DANUBE_MPS_MbxGetMsgLengthBytes()\n"));   
    msg_hd.val = *(mbx->pRead);
    /* return payload + header length in bytes */
    return( (((s32) msg_hd.hd.plength + 2)<<1) );
}


/**
 * Update mailbox's read pointer position
 * This function updates the position of the read pointer assigned to the mailbox
 * pointed to by mbx. In case of reaching the FIFO's end the pointer is set 
 * to the start position.
 * 
 *
 * \param *mbx pointer to mailbox structure to be accessed 
 *
 * \return Nothing. 
 */
void DANUBE_MPS_MbxUpdReadPtr( MailboxParam_s *mbx )
{
    TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW, ("DANUBE_MPS_MbxUpdReadPtr()\n"));   
    if(mbx->pRead > mbx->pEnd)
        mbx->pRead--;
    else
        mbx->pRead = mbx->pStart; /* set pointer to start of segment */
    return;
}


/**
 * Update mailbox's write pointer position
 * This function updates the position of the write pointer assigned to the mailbox
 * pointed to by mbx. In case of reaching the FIFO's end the pointer is set 
 * to the start position.
 *
 * \param *mbx pointer to mailbox structure to be accessed 
 *
 * \return Nothing.
 */
void DANUBE_MPS_MbxUpdWritePtr( MailboxParam_s *mbx )
{
    TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW, ("DANUBE_MPS_MbxUpdWritePtr()\n"));   
    if(mbx->pWrite > mbx->pEnd)
        mbx->pWrite--;
    else
        mbx->pWrite = mbx->pStart; /* set pointer to start of segment */
    return;
}


/**
 * Write data word to downstream mailbox
 * This function writes a data word to the mailbox assigned by mbx.
 * After writing the word the function updates the write pointer position.
 *
 * \param *mbx pointer to mailbox structure to be accessed
 * \param data data to be written to write pointer address
 *
 * \return Nothing.
 */
void DANUBE_MPS_MbxWriteWord( MailboxParam_s *mbx, u32 data )
{
    TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW, ("DANUBE_MPS_MbxWriteWord()\n"));   
    *(mbx->pWrite) = data;
    DANUBE_MPS_MbxUpdWritePtr( mbx );    
    return; 
}

/**
 * Read mailbox content from read pointer address
 * This function reads a data word from the mailbox assigned by mbx.
 * It first reads the data word and increments the read pointer afterwards
 *
 * \param *mbx pointer to mailbox structure to be accessed
 * \param ptr_upd selects if the read pointer is updated after read or not 
 *
 * \return Returns the data word read. 
 */
u32 DANUBE_MPS_MbxReadWord( MailboxParam_s *mbx, BOOL ptr_upd )
{
    u32 retval;

    TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW, ("DANUBE_MPS_MbxReadWord()\n"));   
    retval = *(mbx->pRead);  /* read data at read pointer position */
    if( ptr_upd == TRUE )
        DANUBE_MPS_MbxUpdReadPtr( mbx );    
    return( retval ); 
}



/**
 * Read message from upstream data mailbox
 * This function reads a complete data message from the upstream data mailbox.
 * It reads the header checks how many payload words are included in the message 
 * and reads the payload afterwards.
 *
 * \param *msg pointer to message structure read from buffer
 *
 * \return Returns 0 in case of successful read operation, 
 *         returns -1 in case of invalid length field read.
 */
s32 DANUBE_MPS_MbxReadUpstrmMsg( MailboxParam_s  *fifo, 
                                  MbxMsg_s *msg,
                                  u8 *words_read)
{
    u8 data_words;
    s32 i;

    TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW, ("DANUBE_MPS_MbxReadUpstrmMsg()\n"));   
    memset((void*)msg, 0, sizeof(MbxMsg_s));
    *words_read = 0;
    
    /* read message header from buffer */
    msg->header.val = DANUBE_MPS_MbxReadWord( fifo, TRUE );
    if((msg->header.hd.plength % 2) != 0)  /* check payload length */
    {
        TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_HIGH, 
            ("DANUBE_MPS_MbxReadUpstrmMsg(): Odd payload length %d\n", msg->header.hd.plength));         
        return -1;
    }

    data_words = msg->header.hd.plength>>1;  /* plength counts 16 bit half words */
    for(i=0; i<data_words; i++)  /* read message payload */
        msg->data[i] = DANUBE_MPS_MbxReadWord( fifo, TRUE );
    *words_read = data_words+1;
    return 0;
}



/**
 * Read message from upstream data mailbox and pass it to calling function
 * This function reads a data message from the upstream data mailbox and 
 * passes the message to the calling function. Then a notification is sent 
 * to DANUBE_MPS_MbxNotifyUpstrDataQueue() which is waiting for read completion.
 *
 * \param *pMBDev - pointer to mailbox device structure
 * \param *pPkg   - pointer to data transfer structure (output parameter) 
 * \param *timeout currently unused 
 *
 * \return Returns OK in case of successful read operation, 
 *         returns ERROR in case of read error.
 */
s32  DANUBE_MPS_MbxReadUpstrmFifo( MPS_MB_DEV_STR  *pMBDev, 
                                    DSP_READWRITE   *pPkg, 
                                    s32             timeout)
{
    MbxMsg_s msg;
    u8 data_words = 0;
    MailboxParam_s  *fifo;
    int retval = ERROR;
    s32 index;
    
    TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW, ("%s()\n",__FUNCTION__));   

    fifo = &pMBDev->upstrm_fifo; 
    memset(&msg, 0, sizeof(msg));  /* initialize msg pointer */
    
    /* read message from mailbox */
    if(DANUBE_MPS_MbxReadUpstrmMsg( fifo, &msg, &data_words ) == 0)
    {
         TRACE( DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW, 
                  ("msg.header: 0x%08X\n", msg.header.val));  
         for( index=0; index<data_words; index++ );
            TRACE( DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW, 
                  ("msg.data[%d]: 0x%08X\n", index, msg.data[index]));   
         
         switch(pMBDev->devID)
         {
             case command:
                /*
                 * command messages are completely passedto the caller.
                 * The DSP_READWRITE structure comprises a pointer to the 
                  * message start and the message size in bytes 
                 */
                memcpy((void*)pPkg->pData, (void*) &msg, ((u32) data_words)<<2);
                pPkg->nDataBytes = ((u32) data_words)<<2;
                pPkg->RTP_PaylOffset = 0;
                retval = OK;
                break;
                
             case voice0:
             case voice1:
             case voice2:
             case voice3:
                /*
                 * data messages are passed as DSP_READWRITE pointer that comprises 
                 * a pointer to the payload start address and the payload size in
                 * bytes. 
                 * The message header is removed and the payload pointer, payload size and
                 * and RTP payload offset are passed to CPU0.
                 */
                pPkg->pData      =     msg.data[0]; /* get payload pointer */
                pPkg->nDataBytes =     msg.data[1]; /* get payload size */
                pPkg->RTP_PaylOffset = msg.data[2]  /* get RTP payload offset */
                                       & 0x0000FFFF;
                retval = OK;
                break; 
             
             default:
                break;
         }

    }
    /* wake up the notification process to proceed checking message headers */
    wake_up_interruptible(&(pMBDev->oak_wklist));
    return retval;
}


/**
 * Write to Downstream Command Mailbox of MPS.
 * This is the function to write commands into the downstream command mailbox
 * to be read by the VCPU
 *
 * \param 
 * \param 
 *
 * \return Returns OK in case of successful write operation or ERROR in case
 * of access failures like FIFO overflow
 */
s32 DANUBE_MPS_MbxWriteDwnstr( register MPS_MB_DEV_STR *pMBDev, DSP_READWRITE *readWrite )
{
    MailboxParam_s *mbx; 
    u32            i, req_32;
    u8             req_16;
    int            retval=-EAGAIN;
    s32            retries=0;

    TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW, ("DANUBE_MPS_MbxWriteDwnstr()\n"));       
    Sem_Lock(pMBDev->sem_dev);
    mbx = &(pMBDev->dwstrm_fifo);/* set pointer to downstream command mailbox 
                                    FIFO structure */
    
    /* Calculate the required data words from header field plength which 
       counts 16 bit halfwords */
    req_16 = (u8) *(readWrite->pData); 
    req_32 = (req_16>>1);
    if ( req_16 % 2 )
        req_32+=2;  /* add header length and add. word for not word aligned data */
    else
        req_32 +=1; /* add header length */
        
    /* request for downstream mailbox buffer memory,  make MAX_FIFO_WRITE_RETRIES 
     * retries in case of memory is not available */
    while( !DANUBE_MPS_MbxFifoMemReq( mbx, req_32 ) && 
           (++retries < MAX_FIFO_WRITE_RETRIES) )  
        Wait(1);
    
    if( retries < MAX_FIFO_WRITE_RETRIES )
    {
        /* write message words to mailbox buffer */
        for( i=0; i<req_32; i++)
        {
            DANUBE_MPS_MbxWriteWord( mbx, *((readWrite->pData)+i) );
        }
        retval = OK;
    }
    else
    {
        /* 
         * insufficient mailbox buffer memory 
         * return error or can we do anything else ?
         */
        TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_HIGH, 
              ("DANUBE_MPS_MbxWriteDwnstr(): no write buffer available\n"));
    }
        
    Sem_Unlock(pMBDev->sem_dev);    
    return retval;
}    



/**
 * Write to Downstream Data Mailbox of MPS.
 * This is the function to write a message into the downstream data mailbox.
 *
 * \param 
 * \param flag Flag for priority indication
 *
 * \return Returns OK in case of successful write operation or ERROR in case
 * of access failures like FIFO overflow
 */
s32 DANUBE_MPS_MbxWriteDwnstrData( register MPS_MB_DEV_STR *pMBDev, DSP_READWRITE *readWrite  )
{
    int retval=ERROR;
    MbxMsg_s msg;

    TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW, ("DANUBE_MPS_MbxWriteDwnstrData()\n"));   

    if( (pMBDev->devID == voice0) ||
        (pMBDev->devID == voice1) ||
        (pMBDev->devID == voice2) ||
        (pMBDev->devID == voice3) )
    {
        memset(&msg, 0, sizeof(msg));  /* initialize msg structure */        
        /* build data message from passed payload data structure */
        msg.header.hd.plength = ((readWrite->nDataBytes) > 1) + 2;
        msg.header.hd.chan = pMBDev->devID;
/* any more header fields to set ???
         rw      
        type    
        odd     
        plength
*/        

        msg.data[0] = readWrite->pData;    
        msg.data[1] = readWrite->nDataBytes;
        readWrite->pData = (u32*)&msg; /* set payload pointer to start of message */ 

        retval=DANUBE_MPS_MbxWriteDwnstr( pMBDev, readWrite );                                          
    }
    else
    {
        /* invalid device id read from mailbox FIFO structure */
        TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_HIGH,
            ("DANUBE_MPS_MbxWriteDwnstrData(): Invalid device ID %d !\n", pMBDev->devID));          
    }    
    return retval;
}

/**
 * Write to Downstream Command Mailbox of MPS.
 * This is the function to write commands into the downstream command mailbox
 * to be read by the VCPU
 *
 * \param *pMBDev    pointer to mailbox device structure
 * \param *readWrite pointer to transmission data container
 *
 * \return Returns OK in case of successful write operation or ERROR in case
 * of access failures like FIFO overflow
 */
s32 DANUBE_MPS_MbxWriteDwnstrCmd( register MPS_MB_DEV_STR *pMBDev, DSP_READWRITE *readWrite )
{
    int retval=ERROR;
    
    TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW, ("DANUBE_MPS_MbxWriteDwnstrCmd()\n"));     
    if( pMBDev->devID == command )
    {
        retval=DANUBE_MPS_MbxWriteDwnstr( pMBDev, readWrite );                                          
    }
    else
    {
        /* invalid device id read from mailbox FIFO structure */
        TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_HIGH,
            ("DANUBE_MPS_MbxWriteDwnstrCmd(): Invalid device ID %d !\n", pMBDev->devID));            
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
 *
 * \param None.
 *
 * \return Nothing.
 */
static void DANUBE_MPS_MbxNotifyUpstrDataQueue( unsigned long dummy )
{
    DEVTYPE channel;
    MailboxParam_s *mbx;
    MPS_MB_DEV_STR *mbx_dev;

    TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW, ("DANUBE_MPS_MbxNotifyUpstrDataQueue()\n"));      
    /* set pointer to data upstream mailbox, no matter if 0,1,2 or 3 
     * because they point to the same shared  mailbox memory */
    mbx = (MailboxParam_s*) &(pMPSDev->VoiceMB[0].upstrm_fifo);
          
    while( DANUBE_MPS_MbxFifoNotEmpty( mbx ) )
    {
        channel = DANUBE_MPS_MbxGetMsgChannel( mbx );
        /* select mailbox device structure acc. to channel ID read from current msg */
        switch(channel)
        {
            case voice0:
                mbx_dev = (MPS_MB_DEV_STR*) &(pMPSDev->VoiceMB[0]);
                break;

            case voice1:
                mbx_dev = (MPS_MB_DEV_STR*) &(pMPSDev->VoiceMB[1]);
                break;
                
            case voice2:
                mbx_dev = (MPS_MB_DEV_STR*) &(pMPSDev->VoiceMB[2]);           
                break;   
                
            case voice3:
                mbx_dev = (MPS_MB_DEV_STR*) &(pMPSDev->VoiceMB[3]);            
                break;                
                
            default:
                TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_HIGH, 
                    ("Invalid channel ID %d read from mailbox\n", channel));
                return;
        }

#ifdef CONFIG_PROC_FS
        /* update mailbox statistics */
        mbx_dev->RxnumIRQs++;  /* increase the counter of rx interrupts */
        mbx_dev->RxnumBytes += /* increase the counter of rx bytes */
            DANUBE_MPS_MbxGetMsgLengthBytes(&(mbx_dev->upstrm_fifo));
#endif /* CONFIG_PROC_FS */             
        /* use callback function or queue wake up to notify about data reception */
        if(mbx_dev->up_callback != NULL) 
        {
            mbx_dev->up_callback( channel );
        } 
        else 
        {
            wake_up_interruptible(&(mbx_dev->oak_wklist));
            /* now we go to sleep until wake up from process recently started is received */
            interruptible_sleep_on(&(mbx_dev->oak_wklist));
        }                       
    }
    return;
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
 *
 * \param None.
 *
 * \return Nothing.
 */

static void DANUBE_MPS_MbxNotifyUpstrCmdQueue( unsigned long dummy )
{
    MailboxParam_s *mbx;

    TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW, ("DANUBE_MPS_MbxNotifyUpstrCmdQueue()\n"));  
    /* set pointer to upstream command mailbox*/
    mbx = (MailboxParam_s*) &(pMPSDev->CommandMB.upstrm_fifo);
    /* Start cmd message processing as long as cmd messages are available 
     * in the upstream command queue */      
    while( DANUBE_MPS_MbxFifoNotEmpty( mbx ) )
    {
#ifdef CONFIG_PROC_FS
        /* increase counter of read messages and bytes */
        pMPSDev->CommandMB.RxnumIRQs++;
        pMPSDev->CommandMB.RxnumBytes += 
        DANUBE_MPS_MbxGetMsgLengthBytes( mbx );
#endif /* CONFIG_PROC_FS */     
        if(pMPSDev->CommandMB.up_callback != NULL)
        {
            pMPSDev->CommandMB.up_callback( command );
        } 
        else 
        {
            /* wake up sleeping process for further processing of received command */
            wake_up_interruptible(&(pMPSDev->CommandMB.oak_wklist));
            /* go to sleep until woken up from process recently started */
            interruptible_sleep_on(&(pMPSDev->CommandMB.oak_wklist));
        }       
    }
    return;
}



/****************************************************************************
  MPS/VCPU interrupt service routines
****************************************************************************/

/**
 * Upstream data interrupt handler
 * This function is called on occurence of an data upstream interrupt.
 * Depending on the occured interrupt either the command or data upstream 
 * message processing is started via tasklet
 * 
 *
 * \param *pDev - pointer to MPS communication device structure
 * \param *regs - pointer to system registers
 *
 * \return Nothing
 */
void DANUBE_MbxUpstrmISR(int irq, MPS_COMM_DEV *pDev, struct pt_regs *regs)
{
    MPS_Ad0Reg_u MPS_AdoStatusReg, MPS_AdoClearReg;

    TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW, ("DANUBE_MbxUpstrmDataISR: Voice ISR\n"));

    MPS_AdoStatusReg.val = *DANUBE_MPS_RAD0SR;  /* read interrupt status */

    if( MPS_AdoStatusReg.fld.du_mbx )
    {
        /* start channel ID check and queue notification via tasklet */
        tasklet_schedule( &DANUBE_MPS_DataUpstrmTasklet );
    }

    if( MPS_AdoStatusReg.fld.cu_mbx )
    {
        /* start the queue notification via tasklet */
        tasklet_schedule( &DANUBE_MPS_CmdUpstrmTasklet );
    }

    if( MPS_AdoStatusReg.fld.wd_fail )
    {
        TRACE( DANUBE_MPS_DSP_DRV, 
               DBG_LEVEL_LOW, 
               ("Voice CPU Watchdog Failure Interrupt !!!\n"));        
    }
    if( MPS_AdoStatusReg.fld.mips_ol )
    {
        TRACE( DANUBE_MPS_DSP_DRV, 
               DBG_LEVEL_LOW, 
               ("Voice CPU Overload Interrupt !!!\n"));            
    }
    if( MPS_AdoStatusReg.fld.data_err )
    {
        TRACE( DANUBE_MPS_DSP_DRV, 
               DBG_LEVEL_LOW, 
               ("Downstream Voice Message Pointer Error Interrupt !!!\n"));         
    }
    if( MPS_AdoStatusReg.fld.pcm_crash )
    {
        TRACE( DANUBE_MPS_DSP_DRV, 
               DBG_LEVEL_LOW, 
               ("Voice CPU PCM Timeslot Access Error Interrupt !!!\n"));         
    }    
    if( MPS_AdoStatusReg.fld.cmd_err )
    {
        TRACE( DANUBE_MPS_DSP_DRV, 
               DBG_LEVEL_LOW, 
               ("Downstream Command Message Pointer Error Interrupt !!!\n"));     
    }
#ifdef CCPU_CLEAR_INTERRUPTS
    /* acc. to FW spec. it is not necessary to clear DU and CU interrupt bits 
       because they are set and cleared by the VCPU */ 
    /* acknowledge the upstream interrupt(s) */
    *DANUBE_MPS_RAD0SR = MPS_AdoClearReg.val; 
#endif         
    return;    
}


