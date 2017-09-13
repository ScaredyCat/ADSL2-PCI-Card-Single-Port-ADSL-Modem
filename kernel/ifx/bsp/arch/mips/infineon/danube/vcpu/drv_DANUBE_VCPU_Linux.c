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
   File        : $RCSfile: drv_DANUBE_VCPU_Linux.c,v $
   Revision    : $Revision: 1.1 $
   Date        : $Date: 2005/08/30 10:42:55 $
   Description : Linux driver for the DANUBE OAK DSP 
   
******************************************************************************/
#ifdef LINUX
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <asm/irq.h>
#include <asm/danube/irq.h>

/* traces */
#ifndef ENABLE_TRACE
#define ENABLE_TRACE
#endif
#ifndef ENABLE_LOG
#define ENABLE_LOG
#endif

#include <asm/danube/danube.h>
#include "drv_api.h"
#include "sys_drv_linux.h"
/* #include "RegIncaIP.h"   */
#include "drv_DANUBE_VCPU_Api.h"
#include "drv_DANUBE_VCPU_Interface.h"
#include "DANUBE_VCPU_DeviceDriver.h"
#include "DANUBE_VCPU_DeviceDriver_Ext.h"
#include "DANUBE_VCPU_Version.h"

/* external function declaration */

extern void DANUBE_MbxUpstrmISR(int irq, void *pDev, struct pt_regs *regs);

/* local function declaration */
int DANUBE_MPS_DSP_Open(struct inode *inode, struct file *filp);
int DANUBE_MPS_DSP_Close(struct inode *inode, struct file *filp);
static ssize_t DANUBE_MPS_DSP_write(struct file *file_p, const char *buf, size_t count, loff_t * ppos);
static ssize_t DANUBE_MPS_DSP_read(struct file *file_p, char *buf, size_t length, loff_t * ppos);
static unsigned int DANUBE_MPS_DSP_poll(struct file *file_p, poll_table *wait);
int DANUBE_MPS_DSP_Ioctl(struct inode *inode, struct file *file_p, unsigned int nCmd, unsigned long arg);

/* Declarations for debug interface */
CREATE_TRACE_GROUP(DANUBE_MPS_DSP_DRV)
CREATE_LOG_GROUP(DANUBE_MPS_DSP_DRV)

/* what string support, driver version string */
const char DANUBE_VCPU_WHATVERSION[] = DRV_DANUBE_VCPU_WHAT_STR;

static BYTE DANUBE_VCPU_major_number = 0;
MODULE_PARM(DANUBE_VCPU_major_number, "b");
MODULE_PARM_DESC(DANUBE_VCPU_major_number, "to override automatic major number");

/* the driver callbacks */
static struct file_operations DANUBE_MPS_DSP_fops =
{
   owner:   THIS_MODULE,
   read:    DANUBE_MPS_DSP_read,
   write:   DANUBE_MPS_DSP_write,
   poll:    DANUBE_MPS_DSP_poll,
   ioctl:   DANUBE_MPS_DSP_Ioctl,
   open:    DANUBE_MPS_DSP_Open,
   release: DANUBE_MPS_DSP_Close
};

/* device structure for the OAK DSP */
MPS_COMM_DEV DANUBE_MPS_dev = {};

#if CONFIG_PROC_FS 
static struct proc_dir_entry *DANUBE_vcpu_proc_dir;
#endif


/**
 * Open the device from user mode (e.g. application) or kernel mode
 * 
 * 
 * 
 * 
 * \param inode  - pointer to the inode
 * \param file_p - pointer to the file descriptor
 * 
 * \return  0             - if no error
 *          -error code   - in case of error
 */
s32 DANUBE_MPS_DSP_Open(struct inode *inode, struct file *file_p)
{
    MPS_COMM_DEV      *pDev = &DANUBE_MPS_dev;
    MPS_MB_DEV_STR    *pMBDev;
    BOOL              bcommand = FALSE;
    int               from_kernel = 0;
    DEVTYPE           num;

    /* Check whether called from user or kernel mode */
    if ( (inode == (struct inode *)1) || (inode == (struct inode *)2) 
      || (inode == (struct inode *)3) || (inode == (struct inode *)4)
      || (inode == (struct inode *)5) )
    {
        from_kernel = 1;
        num = (int)inode;
    } 
    else 
    {
        num = (DEVTYPE)MINOR(inode->i_rdev);  /* the real device */
    }
         
    /* check the device number */
    switch(num) 
    {
        case command:
            /* command type */
            TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW,\
            ("DANUBE_MPS_DSP_DRV: open command mailbox\n"));
            pMBDev = &(pDev->CommandMB);
            bcommand = TRUE;
            break;
        case voice0:
            /* voice channel 0 */
            TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW,\
            ("DANUBE_MPS_DSP_DRV: open voice0 mailbox\n"));
            pMBDev = &(pDev->VoiceMB[0]);
            break;
        case voice1:
            /* voice channel 1 */
            TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW,\
            ("DANUBE_MPS_DSP_DRV: open voice1 mailbox\n"));
            pMBDev = &pDev->VoiceMB[1];
            break;
        case voice2:
            /* voice channel 2 */
            TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW,\
            ("DANUBE_MPS_DSP_DRV: open voice2 mailbox\n"));
            pMBDev = &pDev->VoiceMB[2];
            break;
        case voice3:
            /* voice channel 3 */
            TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW,\
            ("DANUBE_MPS_DSP_DRV: open voice3 mailbox\n"));
            pMBDev = &pDev->VoiceMB[3];
            break;      
        default:
            TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_HIGH,\
            ("DANUBE_MPS_DSP_DRV ERROR: max. device number exceed!\n"));
            return -ENOENT;
    }
   
    if ((OK) == DANUBE_MPS_DSP_CommonOpen(pDev, pMBDev, bcommand)) 
    {
        if (!from_kernel) 
        {
            /* installation was successfull */
            /* and use filp->private_data to point to the device data */
            file_p->private_data = pMBDev;
#ifdef MODULE
            /* increment module use counter */
            MOD_INC_USE_COUNT;
#endif
        }
        TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW, \
             ("DANUBE_MPS_DSP_DRV: Device open successful\n"));
        return 0;
    } 
    else 
    {
        /* installation failed */
        TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_HIGH,\
             ("DANUBE_MPS_DSP_DRV ERROR: Device is already open!\n"));
        return -EMFILE;
    }
}



/**
 * Release the device 
 * 
 * 
 * 
 * 
 * \param inode  - pointer to the inode
 * \param filep - pointer to the file descriptor
 * 
 * \return  0             - if no error
 *          -error code   - in case of error
 */
s32 DANUBE_MPS_DSP_Close(struct inode *inode, struct file *filp)
{
    MPS_MB_DEV_STR *pMBDev;
    int            from_kernel = 0;
    
    /* Check whether called from user or kernel mode */
    if ((inode == (struct inode *)1) || (inode == (struct inode *)2) || 
        (inode == (struct inode *)3) || (inode == (struct inode *)4) ||
        (inode == (struct inode *)5) ) 
    {
        from_kernel = 1;
        switch((int)inode) 
        {
            case command:
                pMBDev = &DANUBE_MPS_dev.CommandMB;
                break;
            case voice0:
                pMBDev = &DANUBE_MPS_dev.VoiceMB[0];
                break;
            case voice1:
                pMBDev = &DANUBE_MPS_dev.VoiceMB[1];
                break;
            case voice2:
                pMBDev = &DANUBE_MPS_dev.VoiceMB[2];
                break;
            case voice3:
                pMBDev = &DANUBE_MPS_dev.VoiceMB[3];
                break;                
            default:
                return (-EINVAL);
        }
    } 
    else 
    {
        pMBDev = filp->private_data;
    }

   TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW,("DANUBE_MPS_DSP_DRV: close\n"));  
   
   if (NULL != pMBDev)
   {
      /* device is still available */
      DANUBE_MPS_DSP_CommonClose(pMBDev);

#ifdef MODULE
      if (!from_kernel) 
      {
         /* increment module use counter */
         MOD_DEC_USE_COUNT;
      }
#endif
      
      return 0;      
   }
   else
   {
      /* something went totally wrong */
      TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_HIGH,\
         ("DANUBE_MPS_DSP_DRV ERROR: pMBDev pointer is NULL!\n"));
      return -ENODEV;
   }
}


/**
 * Writing data to the dsp device. This function is a dummy function. It
 * returns the given size. 
 * 
 * \param    inode -
 * \param    filp -
 * \param    buf - source buffer
 * \param    count - data length * 
 * \param    ppos * 
 * 
 * 
 * \return  input parameter count
 */
static ssize_t DANUBE_MPS_DSP_write(struct file *file_p, const char *buf, 
   size_t count, loff_t * ppos)
{
   TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW,("DANUBE_MPS_DSP_DRV: write\n"));

   return count;
}


/**
 * Reads data from the dsp device. This function is a dummy function. It
 * does not read any data from the device. It always returns zero.
 * 
 * \param    inode -
 * \param    file_p -
 * \param    buf 
 * \param    count 
 * \param    ppos * 
 * 
 * \return  0 always
 */
static ssize_t DANUBE_MPS_DSP_read(struct file * file_p, char *buf, 
   size_t count, loff_t * ppos)
{
   TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW,("DANUBE_MPS_DSP_DRV: read\n"));

   return 0;
}


/**
 * The select function of the driver. A user space program may sleep until
 * the driver wakes it up.
 * 
 * \param    file_p -
 * \param    wait -
 * 
 * \return  POLLIN  - data available
 *          POLLERR - device pointer is zero
 */
static u32 DANUBE_MPS_DSP_poll(struct file *file_p, poll_table *wait)
{
   MPS_MB_DEV_STR    *pMBDev = file_p->private_data;
   unsigned int      mask;
   
   TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW,("DANUBE_MPS_DSP_DRV: oak_poll\n"));  

   /* add to poll queue */
   poll_wait(file_p, &(pMBDev->oak_wklist), wait);

   mask = 0;

   /* upstream queue */
   if(pMBDev->upstrm_fifo.pWrite != pMBDev->upstrm_fifo.pRead)
   {
      TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW,\
         ("DANUBE_MPS_DSP_DRV: oak_poll: upstream queue filled\n"));
      
      /* queue is not empty */
      mask = POLLIN | POLLRDNORM;
   }
   
   /* downstream queue */
   if(DANUBE_MPS_MbxFifoMemAvailable(&pMBDev->dwstrm_fifo) != 0)
   {
      TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW,\
         ("DANUBE_MPS_DSP_DRV: DANUBE_MPS_DSP_poll: downstream queue space\n"));  
      
      /* queue is not full */
      mask |= POLLOUT | POLLWRNORM;
   }
   return mask;
}


/**
 * Configuration / Control for the MPS device
 * 
 * 
 * \param     inode    -
 * \param     file_p   - 
 * \param     nCmd     - function id's
 * \param     arg      - additional optional argument 
 *          
 * \return  negative value - ioctl failed
 *          0              - ioctl succeeded
 *          
 */
s32 DANUBE_MPS_DSP_Ioctl(struct inode *inode, struct file *file_p, 
                          unsigned int nCmd, unsigned long arg)
{
#if 0    
    void              *pTemp;
    unsigned long     flags;
    VCPU_GLOBAL_REG   *pBaseGlobal;
    VCPU_FWDWNLD       dwnld_struct;
    u32               reg;    
#endif    
    int               retvalue = -EINVAL;
    DSP_READWRITE     rw_struct;
    MPS_MB_DEV_STR    *pMBDev;
    int               from_kernel = 0;

    if ((inode == (struct inode *)1) || (inode == (struct inode *)2) || 
        (inode == (struct inode *)3) || (inode == (struct inode *)4) ||
        (inode == (struct inode *)5) ) 
    {
        from_kernel = 1;
        switch((int)inode) 
        {
            case command:
                pMBDev = &DANUBE_MPS_dev.CommandMB;
                break;
            case voice0:
                pMBDev = &DANUBE_MPS_dev.VoiceMB[0];
                break;
            case voice1:
                pMBDev = &DANUBE_MPS_dev.VoiceMB[1];
                break;
            case voice2:
                pMBDev = &DANUBE_MPS_dev.VoiceMB[2];
                break;         
            case voice3: 
                pMBDev = &DANUBE_MPS_dev.VoiceMB[3];
                break;
            default:
                return (-EINVAL);
        }
    }
    else 
    {
        pMBDev = file_p->private_data;
    }

    switch(nCmd) 
    {
        case FIO_DANUBE_VCPU_EVENT_REG:
            pMBDev->event_mask = arg;
            retvalue = OK;
            break;
            
        case FIO_DANUBE_VCPU_EVENT_UNREG:
            pMBDev->event_mask = 0xFFFFFFFF;
            retvalue = OK;
            break;
            
        case FIO_DANUBE_MPS_MB_READ:
            /* Read the data from mailbox stored in local FIFO */
            TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW,
                ("DANUBE_MPS_DSP_DRV: vcpu_ioctl MB read\n"));  
          
            if (from_kernel) 
            {
                retvalue = DANUBE_MPS_MbxReadUpstrmFifo(pMBDev,(DSP_READWRITE*)arg, 0);
            }
            else 
            {
                u32 *pUserBuf;
                /* Initialize destination and copy DSP_READWRITE from usermode */               
                memset(&rw_struct, 0, sizeof(DSP_READWRITE));
                copy_from_user((char*)&rw_struct, (char*)arg, sizeof(DSP_READWRITE));
                
                pUserBuf = rw_struct.pData;  /* Remember usermode buffer */
                
                /* Allocate kernelmode buffer for fetching data */
                rw_struct.pData = kmalloc(rw_struct.nDataBytes, GFP_KERNEL);
                if (rw_struct.pData == NULL) 
                {
                    return -ENOMEM; 
                }
                /* read data from from upstream mailbox FIFO */
                retvalue = DANUBE_MPS_MbxReadUpstrmFifo(pMBDev,&rw_struct, 0);
                if(retvalue)
                    return -ENOMSG;
                /* Copy data to usermode buffer... */
                copy_to_user((char*)pUserBuf, (char*)rw_struct.pData, rw_struct.nDataBytes);
                kfree(rw_struct.pData);                 
                /* ... and finally restore the buffer pointer and copy DSP_READWRITE back! */
                rw_struct.pData = pUserBuf;                
                copy_to_user((char*)arg, (char*)&rw_struct, sizeof(DSP_READWRITE));
            }
            break;
            
        case FIO_DANUBE_MPS_MB_WRITE:  
            /* Write data to send to the mailbox into the local FIFO */
            TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW,("DANUBE_MPS_DSP_DRV: oak_ioctl MB write\n"));  
      
            if (from_kernel) 
            {
                if(pMBDev->devID == command) 
                {
                    return( DANUBE_MPS_MbxWriteDwnstrCmd(pMBDev, (DSP_READWRITE*)arg) );
                } 
                else 
                {
                    return( DANUBE_MPS_MbxWriteDwnstrData( pMBDev, (DSP_READWRITE*)arg) );
                }   
            }
            else 
            {
                u32 *pUserBuf;
                copy_from_user((char*)&rw_struct, (char*)arg, sizeof(DSP_READWRITE));

                /* Remember usermode buffer */
                pUserBuf = rw_struct.pData;
                /* Allocate kernelmode buffer for writing data */
                rw_struct.pData = kmalloc(rw_struct.nDataBytes, GFP_KERNEL);
                if (rw_struct.pData == NULL) 
                {
                     return(-ENOMEM);
                }

                /* copy data to kernelmode buffer and write to mailbox FIFO */
                copy_from_user((char*)rw_struct.pData, (char*)pUserBuf, rw_struct.nDataBytes);
                if(pMBDev->devID == command) 
                {
                    retvalue = DANUBE_MPS_MbxWriteDwnstrCmd(pMBDev, &rw_struct);
                }
                else 
                {
                    retvalue = DANUBE_MPS_MbxWriteDwnstrData(pMBDev, &rw_struct);
                } 
                /* ... and finally restore the buffer pointer and copy DSP_READWRITE back! */
                kfree(rw_struct.pData);
                rw_struct.pData = pUserBuf;
                copy_to_user((char*)arg, (char*)&rw_struct, sizeof(DSP_READWRITE));
            }
            break;   


#if 0
        case FIO_DANUBE_VCPU_DWNLOAD:
            /* Download of OAK DSP firmware file */
            TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW,("DANUBE_MPS_DSP_DRV: oak_ioctl download\n"));  

            if(pMBDev->devID == command) 
            {
                if (from_kernel) 
                {
                    retvalue = DANUBE_MPS_VCPU_DownloadFirmware(pMBDev,(VCPU_FWDWNLD*)arg);
                } 
                else 
                {
                    u32 *pUserBuffer;
                    copy_from_user((char*)&dwnld_struct, (char*)arg, sizeof(VCPU_FWDWNLD));
                    printk("DANUBE_VCPU_DEV: Download firmware (size %li bytes)... ", dwnld_struct.Length);
                    pUserBuffer = dwnld_struct.pData;
                    dwnld_struct.pData = vmalloc(dwnld_struct.Length);
                    retvalue = copy_from_user((char*)dwnld_struct.pData,(char*)pUserBuffer, dwnld_struct.Length);
                    retvalue = DANUBE_MPS_VCPU_DownloadFirmware(pMBDev, &dwnld_struct);
                    if (retvalue != 0) 
                    {
                        printk(" error (%i)!\n", retvalue);
                    }
                    else 
                    {
                        printk(" ok!\n");
                    }
                    vfree(dwnld_struct.pData);
                }
            } 
            else 
            {
                retvalue = -EINVAL;
            }
            break;

        case FIO_DANUBE_VCPU_GETVERSION:
            TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW, ("DANUBE_MPS_DSP_DRV: oak_ioctl get version\n"));  

            if (from_kernel) 
            {
                memcpy((char*)arg, (char*)string_vcpu_version, strlen(string_vcpu_version));
            } 
            else 
            {
                copy_to_user((char*)arg, (char*)string_vcpu_version, strlen(string_vcpu_version));
            }
            retvalue = OK;
            break;

        case FIO_DANUBE_MPS_MB_RST_QUEUE :
            /* Reset the QUEUE Upstream/downstream Note : The interrupts and the
            tasks should be not be running if the queues have to be disabled */
            TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW, ("DANUBE_MPS_DSP_DRV: oak_ioctl get version\n"));  

            if(arg == UPSTREAM) 
            {
                pTemp = &(pMBDev->upstrm_fifo);
            } 
            else 
            {
                pTemp = &(pMBDev->dwstrm_fifo);
            }
            DANUBE_MPS_DSP_MbxClearFifo((MailboxParam_s *)pTemp);
            retvalue = OK;
            break;      

        case  FIO_DANUBE_VCPU_RESET:
            /* Reset of the DSP */
            TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW, ("DANUBE_MPS_DSP_DRV: oak_ioctl dsp reset\n"));  
            retvalue = DANUBE_MPS_DSP_Reset(pMBDev->pBaseGlobal);
            break;

        case  FIO_DANUBE_VCPU_RESTART:
            /* Restart of the DSP */
            TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW, ("DANUBE_MPS_DSP_DRV: oak_ioctl dsp restart\n"));  
            retvalue = DANUBE_MPS_DSP_Restart(pMBDev->pBaseGlobal);
            break;

#ifdef OAK_FIFO_BLOCKING_WRITE
        case  FIO_DANUBE_TXFIFO_SET:
            /* Set the mailbox TX FIFO to blocking */
            TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW, ("DANUBE_MPS_DSP_DRV: oak_ioctl tx fifo set\n"));  

            if(pMBDev->devID == command) 
            {
                retvalue = -EINVAL;  /* not supported for this command MB */
            } 
            else 
            {
                if (arg > 0) 
                {
                    pMBDev->bBlockWriteMB = TRUE;
                } 
                else 
                {
                    pMBDev->bBlockWriteMB = FALSE;
                    Sem_Unlock(pMBDev->sem_write_fifo);
                }
                retvalue = OK;
            }
            break;

        case FIO_DANUBE_TXFIFO_GET:
            /* Get the mailbox TX FIFO to blocking */
            TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW, ("DANUBE_MPS_DSP_DRV: oak_ioctl tx fifo get\n"));  

            if(pMBDev->devID == command) 
            {
                retvalue = -EINVAL;                    
            } 
            else 
            {
                if (!from_kernel) 
                {
                    copy_to_user((char*)arg, (char*)&pMBDev->bBlockWriteMB, sizeof(BOOL));
                }
                retvalue = OK;
            }
            break;
#endif /* OAK_FIFO_BLOCKING_WRITE */
#endif
        default:
            TRACE(DANUBE_MPS_DSP_DRV,DBG_LEVEL_HIGH,
                  ("DANUBE_MPS_DSP_Ioctl: Invalid IOCTL handle %d passed.\n", nCmd));
            break ;
    }
    return retvalue;
}


/**
 * Allows the upper layer to register a callback function either for
 * downstream (tranmsit mailbox space available) or for upstream (read data 
 * available)
 * 
 * 
 * \param   type     - DSP device entity ( 1 - command, 2 - voice0, 3 - voice1, 
 *                     4 - voice2, 5 - voice3 )
 * \param   dir      - Direction (1 - upstream, 2 - downstream)
 * \param   callback - Callback function to register
 *          
 * \return     OK    - Callback registered successfully
 *             ENXIO - Wrong DSP device entity (only 1-3 supported)
 *             EBUSY - Callback already registered
 *          
 */
s32 DANUBE_MPS_DSP_RegisterDataCallback(DEVTYPE type, UINT dir, void (*callback)(DEVTYPE type))
{
    MPS_MB_DEV_STR    *pMBDev;

    if (callback == NULL) 
    {
        return (-EINVAL);
    }
    
    /* Get corresponding mailbox device structure */
    switch(type) 
    {
        case command:
            pMBDev = &DANUBE_MPS_dev.CommandMB;
            break;
        case voice0:
            pMBDev = &DANUBE_MPS_dev.VoiceMB[0];
            break;
        case voice1:
            pMBDev = &DANUBE_MPS_dev.VoiceMB[1];
            break;
        case voice2:
            pMBDev = &DANUBE_MPS_dev.VoiceMB[2];
            break;
        case voice3:
            pMBDev = &DANUBE_MPS_dev.VoiceMB[3];
            break;            
        default:
            return (-ENXIO);
    }
    

    /* Enter the desired callback function */
    switch(dir) 
    {
        case 1:   /* register upstream callback function */
            if (pMBDev->up_callback != NULL) 
            {
                return (-EBUSY);
            } 
            else 
            {
                pMBDev->up_callback = callback;
            }
            break;
        case 2: /* register downstream callback function */
            if (pMBDev->down_callback != NULL) 
            {
                return (-EBUSY);
            }
            else 
            {
                pMBDev->down_callback = callback;
            }
            break;
        default:
            break;
    }
    return(OK);
}


/**
 * Allows the upper layer to un-register the callback function previously
 * registered using DANUBE_MPS_DSP_RegisterDataCallback
 * 
 * 
 * \param   type     - DSP device entity ( 1 - command, 2 - voice0, 3 - voice1)
 * \param   dir      - Direction (1 - upstream, 2 - downstream)
 *          
 * \return     OK    - Callback registered successfully
 *             ENXIO - Wrong DSP device entity (only 1-3 supported)
 *             EINVAL- Nothing to unregister
 *          
 */
s32 DANUBE_MPS_DSP_UnregisterDataCallback(DEVTYPE type, UINT dir)
{
    MPS_MB_DEV_STR    *pMBDev;
    /* Get corresponding mailbox device structure */
    switch(type) 
    {
        case command:
            pMBDev = &DANUBE_MPS_dev.CommandMB;
            break;
        case voice0:
            pMBDev = &DANUBE_MPS_dev.VoiceMB[0];
            break;
        case voice1:
            pMBDev = &DANUBE_MPS_dev.VoiceMB[1];
            break;
        case voice2:
            pMBDev = &DANUBE_MPS_dev.VoiceMB[2];
            break;
        case voice3:
            pMBDev = &DANUBE_MPS_dev.VoiceMB[3];
            break;            
        default:
            return (-ENXIO);
    }
    /* Delete the desired callback function */
    switch(dir) 
    {
        case 1:
            if (pMBDev->up_callback == NULL) 
            {
                return (-EINVAL);
            } 
            else 
            {
                pMBDev->up_callback = NULL;
            }
            break;
        case 2:
            if (pMBDev->down_callback == NULL) 
            {
                return (-EINVAL);
            } 
            else 
            {
                pMBDev->down_callback = NULL;
            }
            break;
        default:
            printk("DANUBE_MPS_DSP_UnregisterDataCallback: Invalid Direction %d\n",dir);
            return (-ENXIO);
    }
    return(OK);
}

/****************************************************************************
Description:
   Allows the upper layer to register a callback function for event
   notification. 
Arguments:
   type     - DSP device entity ( 1 - command, 2 - voice0, 3 - voice1)
   mask     - mask according to MBC_ISR content
   callback - Callback function to register
Return Value:
   OK    - Callback registered successfully
   ENXIO - Wrong DSP device entity (only 1-3 supported)
   EBUSY - Callback already registered
Remarks:
   None.
****************************************************************************/
/**
 * 
 * 
 * 
 * \param   
 * \param   
 *          
 * \return  
 *          
 */
s32 DANUBE_MPS_DSP_RegisterEventCallback(DEVTYPE type, UINT mask, void (*callback)(UINT mask))
{
    MPS_MB_DEV_STR    *pMBDev;

    if (callback == NULL) 
    {
        return (-EINVAL);
    }
    
    /* Get corresponding mailbox device structure */
    switch(type) 
    {
        case command:
            pMBDev = &DANUBE_MPS_dev.CommandMB;
            break;
        case voice0:
            pMBDev = &DANUBE_MPS_dev.VoiceMB[0];
            break;
        case voice1:
            pMBDev = &DANUBE_MPS_dev.VoiceMB[1];
            break;
        case voice2:
            pMBDev = &DANUBE_MPS_dev.VoiceMB[2];
            break;
        case voice3:
            pMBDev = &DANUBE_MPS_dev.VoiceMB[3];
            break;            
        default:
            return (-ENXIO);
    }

    /* Enter the desired callback function */
    if (pMBDev->event_callback != NULL) 
    {
        return (-EBUSY);
    } 
    else 
    {
        pMBDev->event_mask = mask;
        pMBDev->event_callback = callback;
    }
    return(OK);
}

/****************************************************************************
Description:
   Allows the upper layer to un-register the callback function previously
   registered using DANUBE_MPS_DSP_RegisterEventCallback
Arguments:
   type     - DSP device entity ( 1 - command, 2 - voice0, 3 - voice1)
   mask     - mask according to MBC_ISR content
Return Value:
   OK    - Callback registered successfully
   ENXIO - Wrong DSP device entity (only 1-3 supported)
   EINVAL- Callback not registered
Remarks:
   None.
****************************************************************************/
/**
 * 
 * 
 * 
 * \param   
 * \param   
 *          
 * \return  
 *          
 */
s32 DANUBE_MPS_DSP_UnregisterEventCallback(DEVTYPE type, UINT mask)
{
    MPS_MB_DEV_STR    *pMBDev;
    /* Get corresponding mailbox device structure */
    switch(type) 
    {
        case command:
            pMBDev = &DANUBE_MPS_dev.CommandMB;
            break;
        case voice0:
            pMBDev = &DANUBE_MPS_dev.VoiceMB[0];
            break;
        case voice1:
            pMBDev = &DANUBE_MPS_dev.VoiceMB[1];
            break;
        case voice2:
            pMBDev = &DANUBE_MPS_dev.VoiceMB[2];
            break;
        case voice3:
            pMBDev = &DANUBE_MPS_dev.VoiceMB[3];
            break;            
        default:
            return (-ENXIO);
    }
    /* Delete the desired callback function */
    if (pMBDev->event_callback == NULL) 
    {
        return (-EINVAL);
    } 
    else 
    {
        pMBDev->event_callback = NULL;
    }
    return(OK);
}


/**
 * Read from mailbox upstream FIFO
 * This function reads from the mailbox upstream FIFO selected by type.
 * 
 * \param   type - channel select (command, voice0, ..., voice3)
 * \param   *rw  - pointer to received data 
 *          
 * \return   0 in case of successful write operation
 *          < 0 in case of error
 *          
 */
s32 DANUBE_MPS_DSP_ReadMailbox(DEVTYPE type, DSP_READWRITE *rw)
{
    s32 ret;

    switch(type) 
    {
        case command:
            ret = DANUBE_MPS_MbxReadUpstrmFifo( &DANUBE_MPS_dev.CommandMB, 
                                                 rw, 0);
            break;
        case voice0:
            ret = DANUBE_MPS_MbxReadUpstrmFifo( &DANUBE_MPS_dev.VoiceMB[0], 
                                                 rw, 0);
            break;
        case voice1:
            ret = DANUBE_MPS_MbxReadUpstrmFifo( &DANUBE_MPS_dev.VoiceMB[1], 
                                                 rw, 0);
            break;
        case voice2:
            ret = DANUBE_MPS_MbxReadUpstrmFifo( &DANUBE_MPS_dev.VoiceMB[2], 
                                                 rw, 0);
            break;
        case voice3:
            ret = DANUBE_MPS_MbxReadUpstrmFifo( &DANUBE_MPS_dev.VoiceMB[3], 
                                                 rw, 0);
            break;            
        default:
            ret = -ENXIO;
    }
    return(ret);
}


/**
 * Write to downstream mailbox buffer
 * This function writes data to either the command or to the voice downstream FIFO 
 * 
 * \param   type - channel select (command, voice0, ..., voice3)
 * \param   *rw  - pointer to data container to be sent 
 *          
 * \return  0 in case of successful write operation
 *          < 0 in case of error
 */
s32 DANUBE_MPS_DSP_WriteMailbox(DEVTYPE type, DSP_READWRITE *rw)
{
    int ret;

    switch(type) 
    {
        case command:
            ret = DANUBE_MPS_MbxWriteDwnstrCmd(&DANUBE_MPS_dev.CommandMB, rw);
            break;
        case voice0:
            ret = DANUBE_MPS_MbxWriteDwnstrData(&DANUBE_MPS_dev.VoiceMB[0], rw);
            break;
        case voice1:
            ret = DANUBE_MPS_MbxWriteDwnstrData(&DANUBE_MPS_dev.VoiceMB[1], rw);
            break;
        case voice2:
            ret = DANUBE_MPS_MbxWriteDwnstrData(&DANUBE_MPS_dev.VoiceMB[2], rw);
            break;
        case voice3:
            ret = DANUBE_MPS_MbxWriteDwnstrData(&DANUBE_MPS_dev.VoiceMB[3], rw);
            break;            
        default:
            ret = -ENXIO;
    }
    return(ret);
}

#if CONFIG_PROC_FS 

/**
 * Read the version information from the driver.
 * 
 * 
 * \param   buf - destination buffer
 * \param   
 *          
 * \return  length
 *          
 */
static int DANUBE_MPS_DSP_GetVersionProc(char *buf)
{
   int len;

   len = sprintf(buf, "%s\n", &DANUBE_VCPU_WHATVERSION[4]);

   len += sprintf(buf + len, "Compiled on %s, %s for Linux kernel %s\n", 
      __DATE__, __TIME__, UTS_RELEASE);

   return len;
}


/**
 * Read the status information from the driver.
 * 
 * 
 * \param   buf - destination buffer
 * \param   
 *          
 * \return  length
 *          
 */
static int DANUBE_MPS_DSP_GetStatusProc(char *buf)
{
   int len;

   len = sprintf(buf, "%d connections of the driver are used\n", MOD_IN_USE);
   
   /* Print internals of the command mailbox fifo */
   len += sprintf(buf+len, 
      "Fifo size CMD fifo:\t\tread %4ld bytes\n", 
      DANUBE_MPS_dev.CommandMB.upstrm_fifo.FifoSize);

   len += sprintf(buf+len, 
      "Fill level CMD fifo:\t\tread %4ld bytes\n", 
      DANUBE_MPS_dev.CommandMB.upstrm_fifo.FifoSize - 
      DANUBE_MPS_MbxFifoMemAvailable(&DANUBE_MPS_dev.CommandMB.upstrm_fifo));

   len += sprintf(buf+len, 
      "Free level CMD fifo:\t\tread %4ld bytes\n", 
      DANUBE_MPS_MbxFifoMemAvailable(&DANUBE_MPS_dev.CommandMB.upstrm_fifo));

   /* Print internals of the voice0  fifo */
   len += sprintf(buf+len, 
      "Fifo size voice0 fifo:\t\twrite %4ld bytes,\tread %4ld bytes\n", 
      DANUBE_MPS_dev.VoiceMB[0].dwstrm_fifo.FifoSize, 
      DANUBE_MPS_dev.VoiceMB[0].upstrm_fifo.FifoSize);

   len += sprintf(buf+len, 
      "Fill level voice0 fifo\t\twrite %4ld bytes,\tread %4ld bytes\n", 
      DANUBE_MPS_dev.VoiceMB[0].dwstrm_fifo.FifoSize - 
      DANUBE_MPS_MbxFifoMemAvailable(&DANUBE_MPS_dev.VoiceMB[0].dwstrm_fifo),
      DANUBE_MPS_dev.VoiceMB[0].upstrm_fifo.FifoSize - 
      DANUBE_MPS_MbxFifoMemAvailable(&DANUBE_MPS_dev.VoiceMB[0].upstrm_fifo));

   len += sprintf(buf+len, 
      "Free level voice0 fifo:\t\twrite %4ld bytes,\tread %4ld bytes\n", 
      DANUBE_MPS_MbxFifoMemAvailable(&DANUBE_MPS_dev.VoiceMB[0].dwstrm_fifo),
      DANUBE_MPS_MbxFifoMemAvailable(&DANUBE_MPS_dev.VoiceMB[0].upstrm_fifo));

   /* Print internals of the voice1  fifo */
   len += sprintf(buf+len, 
      "Fifo size voice1 fifo:\t\twrite %4ld bytes,\tread %4ld bytes\n", 
      DANUBE_MPS_dev.VoiceMB[1].dwstrm_fifo.FifoSize, 
      DANUBE_MPS_dev.VoiceMB[1].upstrm_fifo.FifoSize);

   len += sprintf(buf+len, 
      "Fill level voice1 fifo\t\twrite %4ld bytes,\tread %4ld bytes\n", 
      DANUBE_MPS_dev.VoiceMB[1].dwstrm_fifo.FifoSize - 
      DANUBE_MPS_MbxFifoMemAvailable(&DANUBE_MPS_dev.VoiceMB[1].dwstrm_fifo),
      DANUBE_MPS_dev.VoiceMB[1].upstrm_fifo.FifoSize - 
      DANUBE_MPS_MbxFifoMemAvailable(&DANUBE_MPS_dev.VoiceMB[1].upstrm_fifo));

   len += sprintf(buf+len, 
      "Free level voice1 fifo:\t\twrite %4ld bytes,\tread %4ld bytes\n", 
      DANUBE_MPS_MbxFifoMemAvailable(&DANUBE_MPS_dev.VoiceMB[1].dwstrm_fifo),
      DANUBE_MPS_MbxFifoMemAvailable(&DANUBE_MPS_dev.VoiceMB[1].upstrm_fifo));

#if 0
   /* get the fifo pointer */
   len += sprintf(buf+len, 
      "CMD read fifo pointer:\t\twrite 0x%08lX,\tread 0x%08lX,"
      "\n\t\t\t\tstart 0x%08lX,\tend  0x%08lX\n", 
      (u32)DANUBE_MPS_dev.CommandMB.upstrm_fifo.pWrite,
      (u32)DANUBE_MPS_dev.CommandMB.upstrm_fifo.pRead,
      (u32)DANUBE_MPS_dev.CommandMB.upstrm_fifo.pStart,
      (u32)DANUBE_MPS_dev.CommandMB.upstrm_fifo.pEnd);

   len += sprintf(buf+len, 
      "Voice0 write fifo pointer:\twrite 0x%08lX,\tread 0x%08lX,"
      "\n\t\t\t\tstart 0x%08lX,\tend  0x%08lX\n", 
      (u32)DANUBE_MPS_dev.VoiceMB[0].dwstrm_fifo.pWrite,
      (u32)DANUBE_MPS_dev.VoiceMB[0].dwstrm_fifo.pRead,
      (u32)DANUBE_MPS_dev.VoiceMB[0].dwstrm_fifo.pStart,
      (u32)DANUBE_MPS_dev.VoiceMB[0].dwstrm_fifo.pEnd);

   len += sprintf(buf+len, 
      "Voice0 read fifo pointer:\twrite 0x%08lX,\tread 0x%08lX,"
      "\n\t\t\t\tstart 0x%08lX,\tend  0x%08lX\n", 
      (u32)DANUBE_MPS_dev.VoiceMB[0].upstrm_fifo.pWrite,
      (u32)DANUBE_MPS_dev.VoiceMB[0].upstrm_fifo.pRead,
      (u32)DANUBE_MPS_dev.VoiceMB[0].upstrm_fifo.pStart,
      (u32)DANUBE_MPS_dev.VoiceMB[0].upstrm_fifo.pEnd);

   len += sprintf(buf+len, 
      "Voice1 write fifo pointer:\twrite 0x%08lX,\tread 0x%08lX,"
      "\n\t\t\t\tstart 0x%08lX,\tend  0x%08lX\n", 
      (u32)DANUBE_MPS_dev.VoiceMB[1].dwstrm_fifo.pWrite,
      (u32)DANUBE_MPS_dev.VoiceMB[1].dwstrm_fifo.pRead,
      (u32)DANUBE_MPS_dev.VoiceMB[1].dwstrm_fifo.pStart,
      (u32)DANUBE_MPS_dev.VoiceMB[1].dwstrm_fifo.pEnd);

   len += sprintf(buf+len, 
      "Voice1 read fifo pointer:\twrite 0x%08lX,\tread 0x%08lX,"
      "\n\t\t\t\tstart 0x%08lX,\tend  0x%08lX\n", 
      (u32)DANUBE_MPS_dev.VoiceMB[1].upstrm_fifo.pWrite,
      (u32)DANUBE_MPS_dev.VoiceMB[1].upstrm_fifo.pRead,
      (u32)DANUBE_MPS_dev.VoiceMB[1].upstrm_fifo.pStart,
      (u32)DANUBE_MPS_dev.VoiceMB[1].upstrm_fifo.pEnd);
#endif


   /* Printout the number of interrupts and fifo misses */
   len += sprintf(buf+len, 
      "Num Rx interrupts:\t\tCMD %4ld,\tVoice0 %4ld,\tVoice1 %4ld\n", 
      DANUBE_MPS_dev.CommandMB.RxnumIRQs, 
      DANUBE_MPS_dev.VoiceMB[0].RxnumIRQs, 
      DANUBE_MPS_dev.VoiceMB[1].RxnumIRQs);

   len += sprintf(buf+len, 
      "Num Tx interrupts:\t\tCMD %4ld,\tVoice0 %4ld,\tVoice1 %4ld\n", 
      DANUBE_MPS_dev.CommandMB.TxnumIRQs, 
      DANUBE_MPS_dev.VoiceMB[0].TxnumIRQs, 
      DANUBE_MPS_dev.VoiceMB[1].TxnumIRQs);

   len += sprintf(buf+len, 
      "Num Rx FIFO miss:\t\tCMD %4ld,\tVoice0 %4ld,\tVoice1 %4ld\n", 
      DANUBE_MPS_dev.CommandMB.RxnumMiss, 
      DANUBE_MPS_dev.VoiceMB[0].RxnumMiss, 
      DANUBE_MPS_dev.VoiceMB[1].RxnumMiss);

   len += sprintf(buf+len, 
      "Num Tx FIFO miss:\t\tCMD %4ld,\tVoice0 %4ld,\tVoice1 %4ld\n", 
      DANUBE_MPS_dev.CommandMB.TxnumMiss, 
      DANUBE_MPS_dev.VoiceMB[0].TxnumMiss, 
      DANUBE_MPS_dev.VoiceMB[1].TxnumMiss);

   len += sprintf(buf+len, 
      "Num Rx bytes:\t\tCMD %4ld,\tVoice0 %4ld,\tVoice1 %4ld\n", 
      DANUBE_MPS_dev.CommandMB.RxnumBytes, 
      DANUBE_MPS_dev.VoiceMB[0].RxnumBytes, 
      DANUBE_MPS_dev.VoiceMB[1].RxnumBytes);

   len += sprintf(buf+len, 
      "Num Tx bytes:\t\tCMD %4ld,\tVoice0 %4ld,\tVoice1 %4ld\n", 
      DANUBE_MPS_dev.CommandMB.TxnumBytes, 
      DANUBE_MPS_dev.VoiceMB[0].TxnumBytes, 
      DANUBE_MPS_dev.VoiceMB[1].TxnumBytes);

   len += sprintf(buf+len, "Events:\n");
   len += sprintf(buf+len, "\t\tCommand: 0x%08x (mask 0x%08x)\n", DANUBE_MPS_dev.CommandMB.event, DANUBE_MPS_dev.CommandMB.event_mask);
   len += sprintf(buf+len, "\t\tVoice 0: 0x%08x (mask 0x%08x)\n", DANUBE_MPS_dev.VoiceMB[0].event, DANUBE_MPS_dev.VoiceMB[0].event_mask);
   len += sprintf(buf+len, "\t\tVoice 1: 0x%08x (mask 0x%08x)\n", DANUBE_MPS_dev.VoiceMB[1].event, DANUBE_MPS_dev.VoiceMB[1].event_mask);

#if 0
   /* printout the content of the ICU*/
   len += sprintf(buf+len, "Content of the ICU modules:\n");
   len += sprintf(buf+len, "\t\tICU1_ISR Content:\t0x%08lX\n", 
      *DANUBE_ICU_IM1_ISR);
   len += sprintf(buf+len, "\t\tICU1_IER Content:\t0x%08lX\n", 
      *DANUBE_ICU_IM1_IER);
   len += sprintf(buf+len, "\t\tICU1_IOSR Content:\t0x%08lX\n", 
      *DANUBE_ICU_IM1_IOSR);

   /* printout the context of the mailbox interrupts */
   len += sprintf(buf+len, "Content of the mailbox interrupt module:\n");
   len += sprintf(buf+len, "\t\tMBC_ISR Content:\t0x%08lX\n", 
      *DANUBE_MBC_MBC_ISR);
   len += sprintf(buf+len, "\t\tMBC_MSK Content:\t0x%08lX\n", 
      *DANUBE_MBC_MBC_MSK);
   len += sprintf(buf+len, "\t\tMBC_MSK01 Content:\t0x%08lX\n", 
      *DANUBE_MBC_MBC_MSK01);
   len += sprintf(buf+len, "\t\tMBC_MSK10 Content:\t0x%08lX\n", 
      *DANUBE_MBC_MBC_MSK10);

   /* printout the space of the mailboxes */
   len += sprintf(buf+len, "Fill/Free level of the mailboxes:\n");
   len += sprintf(buf+len, "\tPackage\tMBC_FS0 Content:\t0x%08lX\n", 
      *DANUBE_MBC_MBC_FS0);
   len += sprintf(buf+len, "\tCommand\tMBC_FS1 Content:\t0x%08lX\n", 
      *DANUBE_MBC_MBC_FS1);
   len += sprintf(buf+len, "\tPackage\tMBC_DA2 Content:\t0x%08lX\n", 
      *DANUBE_MBC_MBC_DA2);
   len += sprintf(buf+len, "\tCommand\tMBC_DA3 Content:\t0x%08lX\n", 
      *DANUBE_MBC_MBC_DA3);
#endif
   TRACE(DANUBE_MPS_DSP_DRV, DBG_LEVEL_HIGH, ("%s: Mailbox and ICU states to be adapted !!!\n",__FUNCTION__)); 
   return len;
}



/**
 * The proc filesystem: function to read and entry.
 * 
 * 
 * \param    page - 
 * \param    start -
 * \param    off -
 * \param    count -
 * \param    eof -
 * \param    data - 
 *          
 * \return  
 *          
 */
static INT DANUBE_MPS_DSP_ReadProc(char *page, char **start, off_t off,
   int count, int *eof, void *data)
{
   int len;

   int (*fn)(char *buf);

   if ( data != NULL )
   {
      fn = data;
      len = fn(page);
   }
   else
      return 0;

   if ( len <= off+count ) *eof = 1;
   *start = page + off;
   len -= off;
   if ( len>count ) len = count;
   if ( len<0 ) len = 0;
   return len;
}
#endif



/**
 * Initialize the module
 * 
 * 
 * \param    None
 *          
 * \return  Error code or 0 on success
 */
static int __init DANUBE_MPS_InitModule(void)
{
    int result;

    printk(KERN_INFO "%s\n", &DANUBE_VCPU_WHATVERSION[4]);
    printk(KERN_INFO "(c) Copyright 2005, Infineon Technologies AG\n");
   
    SetTraceLevel(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW);
    SetLogLevel(DANUBE_MPS_DSP_DRV, DBG_LEVEL_LOW);

    result = register_chrdev(DANUBE_VCPU_major_number , DRV_DANUBE_VCPU_NAME , &DANUBE_MPS_DSP_fops);

    if ( result < 0 )
    {
        TRACE(DANUBE_MPS_DSP_DRV,DBG_LEVEL_HIGH,\
            ("DANUBE_MPS_DSP_DRV: can't get major %d\n", DANUBE_VCPU_major_number));
        return result;
    }
   
     /* dynamic major                       */
     if ( DANUBE_VCPU_major_number == 0 )
        DANUBE_VCPU_major_number = result;

    /* reset the device before initializing the device driver */
    if( (ERROR) == DANUBE_MPS_DSP_Reset() )
    {
        TRACE(DANUBE_MPS_DSP_DRV,DBG_LEVEL_HIGH,\
            ("DANUBE_MPS_DSP_DRV: Hardware reset failed!\n"));
        return -EPERM;   
    }
   
    /* init the device driver structure*/
    if (0 != DANUBE_MPS_InitStructures(&DANUBE_MPS_dev))
        return -ENOMEM;

    /* AFE/DFE common status 0 */
	result = request_irq(INT_NUM_IM4_IRL18, DANUBE_MbxUpstrmISR, 
      0, "mbx_read_upstrm", &DANUBE_MPS_dev);
	if (result)
		return result;

#if 0
    /* voice channel 0 interrupt */
	result = request_irq(INT_NUM_IM5_IRL7, DANUBE_MbxVoiceChannel0ISR, 
      0, "voice_channel0", &DANUBE_MPS_dev);
	if (result)
		return result;
    /* voice channel 1 interrupt */    
	result = request_irq(INT_NUM_IM5_IRL8, DANUBE_MbxVoiceChannel1ISR, 
      0, "voice_channel1", &DANUBE_MPS_dev);
	if (result)
		return result;
    /* voice channel 2 interrupt */
	result = request_irq(INT_NUM_IM5_IRL9, DANUBE_MbxVoiceChannel2ISR, 
      0, "voice_channel2", &DANUBE_MPS_dev);
	if (result)
		return result;
    /* voice channel 3 interrupt */
	result = request_irq(INT_NUM_IM5_IRL10, DANUBE_MbxVoiceChannel3ISR, 
      0, "voice_channel3", &DANUBE_MPS_dev);
	if (result)
		return result;
    /* AFE/DFE common status 1 */   
	result = request_irq(INT_NUM_IM5_IRL12, DANUBE_MbxAFE_DFE_1ISR, 
      0, "afe_dfe_1", &DANUBE_MPS_dev);
	if (result)
		return result;    
    /* combined interrupt of above interrupt sources */
	result = request_irq(INT_NUM_IM5_IRL13, DANUBE_MbxAllISR, 
      0, "mbx_all", &DANUBE_MPS_dev);
	if (result)
		return result;  
    /* Semaphore / notification interrupt */
	result = request_irq(INT_NUM_IM5_IRL14, DANUBE_MbxSemNotifISR, 
      0, "mbx_sem_notif", &DANUBE_MPS_dev);
	if (result)
		return result;  
    /* MPS global interrupt*/
	result = request_irq(INT_NUM_IM5_IRL15, DANUBE_MPS_Global_ISR, 
      0, "mps_global", &DANUBE_MPS_dev);
	if (result)
		return result;  
      
#endif    
    /* Enable DFE/AFE 0 Interrupt at ICU0 */
    MPS_INTERRUPTS_ENABLE(DANUBE_MPS_AD0_IR4);  
#if 0
   MPS_INTERRUPTS_ENABLE(DANUBE_MPS_VC0_IR0);
   MPS_INTERRUPTS_ENABLE(DANUBE_MPS_VC1_IR1);
   MPS_INTERRUPTS_ENABLE(DANUBE_MPS_VC2_IR2);
   MPS_INTERRUPTS_ENABLE(DANUBE_MPS_VC3_IR3);
   MPS_INTERRUPTS_ENABLE(DANUBE_MPS_AD1_IR5);
   MPS_INTERRUPTS_ENABLE(DANUBE_MPS_VC_AD_ALL_IR6); 
   MPS_INTERRUPTS_ENABLE(DANUBE_MPS_SEM_IR7);
   MPS_INTERRUPTS_ENABLE(DANUBE_MPS_GLB_IR8);
   
#endif 

#if CONFIG_PROC_FS 
   /* install the proc entry */
   TRACE(DANUBE_MPS_DSP_DRV,DBG_LEVEL_LOW,("DANUBE_MPS_DSP_DRV: using proc fs\n"));

   DANUBE_vcpu_proc_dir = proc_mkdir("driver/" DRV_DANUBE_VCPU_NAME, NULL);
   if ( DANUBE_vcpu_proc_dir != NULL )
   {
      DANUBE_vcpu_proc_dir->owner = THIS_MODULE;
      create_proc_read_entry("version" , S_IFREG|S_IRUGO,
         DANUBE_vcpu_proc_dir, DANUBE_MPS_DSP_ReadProc, (void *)DANUBE_MPS_DSP_GetVersionProc);
      create_proc_read_entry("status" , S_IFREG|S_IRUGO,
         DANUBE_vcpu_proc_dir, DANUBE_MPS_DSP_ReadProc, (void *)DANUBE_MPS_DSP_GetStatusProc);
   }
   else
   {
      TRACE(DANUBE_MPS_DSP_DRV,DBG_LEVEL_HIGH,\
         ("DANUBE_MPS_DSP_DRV: cannotrun flash_nf create proc entry\n"));
   }
#endif

/*
 * Allocate memory for upstream messages sent by VCPU and send buffer 
 * provisioning message to VCPU
 *
 *
 */



   return 0;
}


/**
 * Clean up the module when unloaded. The function is called by the kernel.
 * 
 * 
 * \param    None
 *          
 * \return  Error code or 0 on success
 */
static void __exit DANUBE_MPS_CleanupModule(void)
{
/*   VCPU_GLOBAL_REG *pBaseGlobal = &(DANUBE_MPS_dev.pBaseGlobal);  */

    /* disable Interrupts at ICU0 */   
    MPS_INTERRUPTS_DISABLE(DANUBE_MPS_AD0_IR4);  /* Disable DFE/AFE 0 Interrupts */
#if 0
    MPS_INTERRUPTS_DISABLE(DANUBE_MPS_VC0_IR0);
    MPS_INTERRUPTS_DISABLE(DANUBE_MPS_VC1_IR1);
    MPS_INTERRUPTS_DISABLE(DANUBE_MPS_VC2_IR2);
    MPS_INTERRUPTS_DISABLE(DANUBE_MPS_VC3_IR3);
    MPS_INTERRUPTS_DISABLE(DANUBE_MPS_AD1_IR5);
    MPS_INTERRUPTS_DISABLE(DANUBE_MPS_VC_AD_ALL_IR6); 
    MPS_INTERRUPTS_DISABLE(DANUBE_MPS_SEM_IR7);
    MPS_INTERRUPTS_DISABLE(DANUBE_MPS_GLB_IR8);
#endif 
   /* disable all MPS interrupts */
    *DANUBE_MPS_SAD0SR = 0x00000000;   
   
   unregister_chrdev(DANUBE_VCPU_major_number , DRV_DANUBE_VCPU_NAME);
   
   /* release the memory usage of the device driver structure */
   DANUBE_MPS_DSP_ReleaseStructures(&DANUBE_MPS_dev);

   /* release all interrupts at the system */
   free_irq(INT_NUM_IM4_IRL18, &DANUBE_MPS_dev);
#if 0 
    /**/  
   free_irq(INT_NUM_IM1_IRL7, &DANUBE_MPS_dev);
   free_irq(INT_NUM_IM1_IRL8, &DANUBE_MPS_dev);
   free_irq(INT_NUM_IM1_IRL9, &DANUBE_MPS_dev);
   free_irq(INT_NUM_IM1_IRL10, &DANUBE_MPS_dev);
   free_irq(INT_NUM_IM1_IRL12, &DANUBE_MPS_dev);
   free_irq(INT_NUM_IM1_IRL13, &DANUBE_MPS_dev); 
   free_irq(INT_NUM_IM1_IRL14, &DANUBE_MPS_dev);
   free_irq(INT_NUM_IM1_IRL15, &DANUBE_MPS_dev);   
#endif   
#if CONFIG_PROC_FS 
   remove_proc_entry("version", DANUBE_vcpu_proc_dir);
   remove_proc_entry("status", DANUBE_vcpu_proc_dir);
   remove_proc_entry("driver/" DRV_DANUBE_VCPU_NAME, NULL);
#endif

   TRACE(DANUBE_MPS_DSP_DRV,DBG_LEVEL_NORMAL,("DANUBE_MPS_DSP_DRV: cleanup done\n"));
}


/****************************************************************************/

module_init(DANUBE_MPS_InitModule);

#ifndef DEBUG
EXPORT_SYMBOL(DANUBE_MPS_DSP_RegisterDataCallback);
EXPORT_SYMBOL(DANUBE_MPS_DSP_UnregisterDataCallback);
EXPORT_SYMBOL(DANUBE_MPS_DSP_RegisterEventCallback);
EXPORT_SYMBOL(DANUBE_MPS_DSP_UnregisterEventCallback);
EXPORT_SYMBOL(DANUBE_MPS_DSP_ReadMailbox);
EXPORT_SYMBOL(DANUBE_MPS_DSP_WriteMailbox);
EXPORT_SYMBOL(DANUBE_MPS_DSP_Ioctl);
EXPORT_SYMBOL(DANUBE_MPS_DSP_Open);
EXPORT_SYMBOL(DANUBE_MPS_DSP_Close);
#endif

#ifdef MODULE

MODULE_AUTHOR("Infineon Technologies AG");
MODULE_DESCRIPTION("MPS/DSP driver for DANUBE");
MODULE_SUPPORTED_DEVICE("DANUBE MIPS24KEc DSP");
MODULE_LICENSE("GPL");

module_exit(DANUBE_MPS_CleanupModule);
#endif

#endif /* LINUX */


