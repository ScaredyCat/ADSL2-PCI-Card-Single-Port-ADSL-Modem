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
   File        : $RCSfile: DANUBE_VCPU_Download.c,v $
   Revision    : $Revision: 1.1 $
   Date        : $Date: 2005/08/30 10:42:55 $
   Description : 

******************************************************************************/



extern MPS_COMM_DEV DANUBE_MPS_dev;

/****************************************************************************
Description :
     This will handle the complete binary download to the OAK
Arguments   :
Return      :
Remarks     :
****************************************************************************/

/**
 * Firmware download to Voice CPU
 * This function performs a firmware download to the VCPU. The firmware image's 
 * location and size is used for
 * 
 * 
 * 
 *
 * \param *pDev pointer to mailbox device structure (comprises global pointer)
 *
 * \return voice channel identifier. 
 */
s32 DANUBE_MPS_VCPU_DownloadFirmware(MPS_MB_DEV_STR *pDev, VCPU_FWDWNLD *pFWDwnld)
{
    u32 reset_ctrl;
    s32 retvalue = ERROR;  
    u32 *pDwnLd;

    TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW, ("DANUBE_MPS_DSP_Restart()\n"));  
    
    /* reset the DSP to bring up in download mode */
    if ((ERROR) == DANUBE_MPS_DSP_Reset())
        return (ERROR);
    
    /* 
     * Allocate and initialize memory to hold firmware binary and 
     * copy firmware binary to allocated memory area 
     */
    pDwnLd = (u32*) Alloc_Mem((pFWDwnld->Length)<<2);
    if(pDwnLd==NULL)
    {
        TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_HIGH, ("Not enough memory available for FW download !\n"));  
        return(ERROR);
    }
    memset(pDwnLd, 0, (pFWDwnld->Length)<<2); 
    memcpy((void*)pDwnLd, (void*)pFWDwnld->pData, (pFWDwnld->Length)<<2);
   
    /* reconfigure CPU1 boot parameters for DSP restart after FW download 
     *
     * - enable software boot select
     * - set boot configuration for SDRAM boot
     * - write new reset vector (firmware start address) 
     * - restart program execution 
     */
    DANUBE_MPS_dev.pBaseGlobal->MBX_CPU1_BOOT_CFG.MPS_BOOT_RVEC = (u32)pDwnLd;    
    reset_ctrl = *DANUBE_RCU_RST_REQ;  
    *DANUBE_RCU_RST_REQ = 
        reset_ctrl | 
        DANUBE_RCU_RST_REQ_CPU0_BR |   /* restart VCPU program execution */
        DANUBE_RCU_RST_REQ_SWTBOOT |   /* enable SW Boot configuration */
        DANUBE_RCU_RST_REQ_CFG_SET(7); /* set SDRAM as secondary boot source */

    return(OK);
    
    
    
#if 0    
    UINT8          run_flag = TRUE;
   void           *p, *pend, *pbase;
   s32          res = 0;
   s32          retvalue = (OK);
   DWNLD          *pDwnLd;
   DSP_READWRITE  rw;
   static u32     ui_workaround2[] = {0x006000102, 0x003BF0080};

    TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW, ("DANUBE_MPS_VCPU_DownloadFirmware()\n"));   

   rw.pWrite = ui_workaround2;
   rw.nWrite = sizeof(ui_workaround2);

   /* reset the DSP to bring up in download mode */
   if ((ERROR) == DANUBE_MPS_DSP_Reset(pMBDev->pBaseGlobal))
      return (ERROR);
   
   /* initialize the local download structure */
   pDwnLd = (DWNLD*)Alloc_Mem(sizeof(DWNLD));
   
   if (!pDwnLd)
      return (ERROR);
   
   memset(pDwnLd, 0, sizeof(DWNLD));
   pDwnLd->status.status = EXIT_FALSE;
   pDwnLd->glob_var.timeout = 0x400;

   p = pbase = pFWDwnld->pData;
   pend = pbase + pFWDwnld->Length;

   /*validate data in memory by reading file header*/
   if (Check4FileID(pDwnLd, &p))
   {
      retvalue = (ERROR);
      run_flag = FALSE;
   }

   /*start with binary download*/
   p = pbase + pDwnLd->file_hdr.block_ptr;
   while(run_flag)
   {
      pDwnLd->block_hdr.block_type    = Read_uI32(&p);
      pDwnLd->block_hdr.unknown_ptr   = Read_uI32(&p);

      switch(pDwnLd->block_hdr.block_type)
      {
         case FLAG_SS:
            DEBUGMSG("ss");
            pDwnLd->status.next_ptr   = Read_uI32(&p);
            pDwnLd->status.status     = Read_uI32(&p);
            p = pbase + pDwnLd->status.next_ptr;
            break;

         case FLAG_SC:
            DEBUGMSG("sc");
            command_sc(pDwnLd, &p);
            p = pbase + pDwnLd->glob_var.next_ptr;
            break;

         case FLAG_GO:
            DEBUGMSG("go");
            p = pbase + Read_uI32(&p);
            break;

         case FLAG_M1:
            DEBUGMSG("M1");
            MessageAckL1(pDwnLd, &p);
            p = pbase + pDwnLd->next_ptr;
            break;

         case FLAG_M2:
            DEBUGMSG("M2");
            MessageAckL2(pDwnLd, &p);
            p = pbase + pDwnLd->next_ptr;
            break;

         case FLAG_I1:
            DEBUGMSG("I1");
            MessageL1(pDwnLd, &p);
            p = pbase + pDwnLd->next_ptr;
            break;

         case FLAG_I2:
            DEBUGMSG("I2");
            MessageL2(pDwnLd, &p);
            p = pbase + pDwnLd->next_ptr;
            break;

         case FLAG_SB:
            DEBUGMSG("sb");
            if(SendMailBox(pDwnLd, pMBDev, &p)) 
               p = pbase + pDwnLd->glob_var.timeout_ptr;
            else 
               p = pbase + pDwnLd->next_ptr;
            break;

         case FLAG_RB:
            DEBUGMSG("rb");
            res = ReadMailBox(pDwnLd, &p);
            if(res == OK)
               p = pbase + pDwnLd->read_mbox.true_ptr;
            else if(res == TIMEOUT) 
               p = pbase + pDwnLd->glob_var.timeout_ptr;
            else 
               p = pbase + pDwnLd->read_mbox.false_ptr;
            break;

         case FLAG_SM:
            DEBUGMSG("sm");
            if(SendMessage(pDwnLd, pMBDev, &p))
               p = pbase + pDwnLd->glob_var.timeout_ptr;
            else
               p = pbase + pDwnLd->next_ptr;
            break;

         case FLAG_RM:
            DEBUGMSG("rm");
            res = ReceiveMessage(pDwnLd, pMBDev, &p);
            if(res == OK)
               p = pbase + pDwnLd->rcv_msg.true_ptr;
            else if(res == TIMEOUT) 
               p = pbase + pDwnLd->glob_var.timeout_ptr;
            else 
               p = pbase + pDwnLd->rcv_msg.false_ptr;
            break;

         case FLAG_RW:
            DEBUGMSG("rw");
            if(ReceiveMessage_wait(pDwnLd, pMBDev, &p)) 
               p = pbase + pDwnLd->glob_var.timeout_ptr;
            else 
               p = pbase + pDwnLd->rcv_msg.true_ptr;
            break;

#ifdef NEVER
         case FLAG_DM:
         case FLAG_PM:
         case FLAG_ED:
         case FLAG_EP:
            DEBUGMSG("dm | pm | ed | ep");
            res = WriteMemory(pDwnLd, pMBDev, &p);
            if(res == OK)
               p = pbase + pDwnLd->write_mem.true_ptr;
            else if(res == TIMEOUT) 
               p = pbase + pDwnLd->glob_var.timeout_ptr;
            else 
               p = pbase + pDwnLd->write_mem.false_ptr;
            break;
#endif /* NEVER */            

#ifdef NEVER
         case FLAG_WD:
         case FLAG_WP:
            DEBUGMSG("wd | wp");
            res = WriteMemory_plus(pDwnLd, pMBDev, &p);
            if(res == OK)
               p = pbase + pDwnLd->write_mem.true_ptr;
            else if(res == TIMEOUT) 
               p = pbase + pDwnLd->glob_var.timeout_ptr;
            else 
               p = pbase + pDwnLd->write_mem.false_ptr;
            break;
#endif /* NEVER */            
         default:
            p = pbase + pDwnLd->block_hdr.unknown_ptr;
      }

      if(p == pbase)
         run_flag = FALSE;
      else if (p >= pend)
      {
         retvalue = (ERROR);
         run_flag = FALSE;
      }
   }

   if (pDwnLd->status.status)
      retvalue = (ERROR);

   Free_Mem(pDwnLd);

   if (oak_jump0_dsp(pMBDev) == (ERROR))
      return (ERROR);

   return retvalue;
#endif
}





