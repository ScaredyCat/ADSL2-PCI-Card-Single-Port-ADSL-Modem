/*
 * ########################################################################
 *
 *  This program is free softwavre; you can distribute it and/or modify it
 *  under the terms of the GNU General Public License (Version 2) as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 *
 * ########################################################################
 *
 *
 */

/* ===========================================================================
 *
 * File Name:   ifx_adsl_basic.c
 * Author :     Taicheng
 *
 * ===========================================================================
 *
 * Project: Danube/Amazon
 *
 * ===========================================================================
 * Contents:This file implements the MEI driver for Danube/Amazon ADSL/ADSL2+
 *  controller.
 *
 * ===========================================================================
 * References:
 *
 */
/* Change log:
1.00.01
   Fixed cell rate calculation issue
   Fixed pvovider_id of adsl mib swapping issue
1.00.02
   Added L3 Low Poewr Mode support.
1.00.03
   Fixed Clear Eoc transmit issue.
1.00.04 31/08/2006
   Add ADSL Link/Data Led
   Add Dual Latency Path
   Add AUTOBOOT_ENABLE_SET ioctl for autoboot mode enable/disable
   Fix fast path cell rate calculation
2.00.00 02/10/2006
   First version for the separation of DSL and MEI
2.00.01 12/10/2006
   Fix some compiling warnings
*/

/*
 * ===========================================================================
 *                           INCLUDE FILES
 * ===========================================================================
 */

/** \defgroup Internal MEI driver internals
 */

//#define IFX_ADSL_PORT_RTEMS 1

#if defined(IFX_ADSL_PORT_RTEMS)
#include "danube_mei_rtems.h"
#else
#ifndef EXPORT_SYMTAB
#define EXPORT_SYMTAB
#endif

#include <ifx/ifx_adsl_linux.h>

#endif //!defined(IFX_ADSL_PORT_RTEMS)

static char IFX_ADSL_VERSION[]="2.00.01";

#ifdef IFX_ADSL_DEBUG
#define IFX_ADSL_DMSG(fmt, args...) printk( KERN_INFO  "%s: " fmt,__FUNCTION__, ## args)
#else
#define IFX_ADSL_DMSG(fmt, args...) do { } while(0)
#endif //ifdef IFX_ADSL_DEBUG

#define IFX_ADSL_EMSG(fmt, args...) printk( KERN_ERR  "%s: " fmt,__FUNCTION__, ## args)

#define IFX_ADSL_CMV_EXTRA //WINHOST debug
#define IFX_ADSL_L3_MODE_SUPPORT //L3 Low Power Mode Support
#define IFX_ADSL_DUAL_LATENCY_SUPPORT

// for ARC memory access
#define WHILE_DELAY 20000

#if defined(__LINUX__) 
#define IFX_ADSL_DEVNAME "dsl"

#endif // __LINUX__

#define IFX_ADSL_DYING_GASP_SUPPORT

/************************************************************************
 *  Function declaration
 ************************************************************************/
static void IFX_ADSL_MemoryWrite(u32 ul_address, u32 ul_data);
static void IFX_ADSL_MemoryRead(u32 ul_address, u32 *pul_data);
static MEI_ERROR IFX_ADSL_HaltDFE(void);
static MEI_ERROR IFX_ADSL_RunDFE(void);
static MEI_ERROR IFX_ADSL_RunAdslModem(void);
static ssize_t IFX_ADSL_Write(MEI_file_t *filp, char * buf, size_t size, loff_t * loff);
int IFX_ADSL_Ioctls(MEI_inode_t * ino, MEI_file_t * fil, unsigned int command, unsigned long lon);
static MEI_ERROR IFX_ADSL_GetUsRate(ifx_adsl_device_t *dev,u32 *rate_fast,u32 *rate_intl);
int IFX_ADSL_IsADSLShowtime(ifx_adsl_device_t *dev);
int IFX_ADSL_IsModemReady(ifx_adsl_device_t *pDev);
int IFX_ADSL_DoBspIoctl(ifx_adsl_device_t *pDev, unsigned int command, unsigned long lon);

#if defined(CONFIG_IFX_ADSL_LED)
#endif
#ifdef IFX_ADSL_DYING_GASP_SUPPORT
static int IFX_ADSL_GetDyingGaspAddr(ifx_adsl_device_t *pDev);
#endif
extern int ifx_adsl_autoboot_ioctl(MEI_inode_t * ino, MEI_file_t * fil, unsigned int command, unsigned long lon);

#if defined(__LINUX__)
#ifdef CONFIG_DEVFS_FS
static devfs_handle_t ifx_adsl_devfs_base_dir;
static devfs_handle_t ifx_adsl_devfs_handle;
#endif

#endif //defined(__LINUX__)


/************************************************************************
 *  variable declaration
 ************************************************************************/

static ifx_adsl_device_t *ifx_adsl_dev = NULL; 
 
static struct timeval time_disconnect,time_showtime;
static u16 unavailable_seconds = 0;

int showtime=0;
static int major=IFX_ADSL_DEV_MAJOR;
MEI_mutex_t mei_sema;

int loop_diagnostics_mode=0;
wait_queue_head_t wait_queue_loop_diagnostic;
int loop_diagnostics_completed=0;
u32 adsl_mode,adsl_mode_extend; // adsl mode : adsl/ 2/ 2+

#ifdef IFX_ADSL_DUAL_LATENCY_SUPPORT
static u8 bDualLatency=0;
#endif

#ifdef IFX_ADSL_L3_MODE_SUPPORT
static wait_queue_head_t wait_queue_l3; // l3 power mode
static int l3_shutdown = 0;
#endif

#if defined(__LINUX__)
#define MEI_DIRNAME "mei"
#define PROC_ITEMS 2
static reg_entry_t regs[PROC_ITEMS]; //total items to be monitored by /proc/mei
static struct proc_dir_entry *meidir;
#define NUM_OF_REG_ENTRY	(sizeof(regs)/sizeof(reg_entry_t))
#ifdef CONFIG_PROC_FS
static int IFX_ADSL_ProcRead(struct file * file, char * buf, size_t nbytes, loff_t *ppos);
static struct file_operations IFX_ADSL_ProcOperations = {
	read: 	IFX_ADSL_ProcRead,
	write:	NULL,
};
#endif

static struct file_operations mei_operations = {
	write:	IFX_ADSL_Write,
	ioctl:	IFX_ADSL_Ioctls,
};


#endif //defined(__LINUX__)


/////////////////               mei access Rd/Wr methods       ///////////////
/**
 * Write a value to register
 * This function writes a value to MIPS register
 *
 * \param  	ul_address	The address to write
 * \param  	ul_data		The value to write
 * \ingroup	Internal
 */
static void IFX_ADSL_MemoryWrite(u32 ul_address, u32 ul_data)
{
	*((volatile u32 *)ul_address) = ul_data;
	asm("SYNC");
	return;
} //	end of "IFX_ADSL_MemoryWrite(..."

/**
 * Read the register
 * This function read the value from MIPS register
 *
 * \param  	ul_address	The address to write
 * \param  	pul_data	Pointer to the data
 * \ingroup	Internal
 */
static void IFX_ADSL_MemoryRead(u32 ul_address, u32 *pul_data)
{
	*pul_data = *((volatile u32 *)ul_address);
	asm("SYNC");
	return;
} //	end of "IFX_ADSL_MemoryRead(..."

ifx_adsl_device_t *IFX_ADSL_GetAdslDevice(void)
{
	if (ifx_adsl_dev == NULL)
	{
		printk("Get adsl device fail!\n");
	}
	return ifx_adsl_dev;
}

//#endif

/**
 * Halt the ARC.
 * This function halts the ARC.
 *
 * \return	MEI_SUCCESS or MEI_FAILURE
 * \ingroup	Internal
 */
static MEI_ERROR IFX_ADSL_HaltDFE(void)
{
	ifx_adsl_device_t *adsl_dev;
	MEI_ERROR err=MEI_SUCCESS;
	
	adsl_dev = IFX_ADSL_GetAdslDevice();
	if (adsl_dev == NULL)
		return MEI_FAILURE;
	//return adsl_dev->CpuModeSet(adsl_dev,IFX_ADSL_CPU_HALT);
	if (IFX_ADSL_DoBspIoctl(adsl_dev,IFX_ADSL_BSP_IOC_HALT,(unsigned long)NULL)!=0)
		err = MEI_FAILURE;
	return err;

} //	end of "meiHalt(..."

/**
 * Run the ARC.
 * This function runs the ARC.
 *
 * \return	MEI_SUCCESS or MEI_FAILURE
 * \ingroup	Internal
 */
static MEI_ERROR IFX_ADSL_RunDFE(void)
{
	ifx_adsl_device_t *adsl_dev;
	MEI_ERROR err=MEI_SUCCESS;
	
	adsl_dev = IFX_ADSL_GetAdslDevice();	
	if (adsl_dev == NULL)
		return MEI_FAILURE;
	//return adsl_dev->CpuModeSet(adsl_dev,IFX_ADSL_CPU_RUN);
	if (IFX_ADSL_DoBspIoctl(adsl_dev,IFX_ADSL_BSP_IOC_RUN,(unsigned long)NULL)!=0)
		err = MEI_FAILURE;
	return err;


} //	end of "meiActivate(..."

/**
 * Reset the ARC.
 * This function resets the ARC.
 *
 * \return	MEI_SUCCESS or MEI_FAILURE
 * \ingroup	Internal
 */
static MEI_ERROR IFX_ADSL_ResetDFE(void)
{
	ifx_adsl_device_t *adsl_dev=NULL;
	MEI_ERROR err=MEI_SUCCESS;
	
	adsl_dev = IFX_ADSL_GetAdslDevice();	
	if (adsl_dev == NULL)
		return MEI_FAILURE;
	MEI_MUTEX_INIT(mei_sema, 1);
	//return adsl_dev->CpuModeSet(adsl_dev,IFX_ADSL_CPU_RESET);
	if (IFX_ADSL_DoBspIoctl(adsl_dev,IFX_ADSL_BSP_IOC_RESET,(unsigned long)NULL)!=0)
		err = MEI_FAILURE;
	return err;
}

/**
 * Reset the ARC, download boot codes, and run the ARC.
 * This function resets the ARC, downloads boot codes to ARC, and runs the ARC.
 *
 * \return	MEI_SUCCESS or MEI_FAILURE
 * \ingroup	Internal
 */
static MEI_ERROR IFX_ADSL_RunAdslModem(void)
{
	ifx_adsl_device_t *adsl_dev;
	MEI_ERROR meierr=MEI_FAILURE;
	
	adsl_dev = IFX_ADSL_GetAdslDevice();
	if (adsl_dev == NULL)
		return MEI_FAILURE;
	
	if (IFX_ADSL_DoBspIoctl(adsl_dev,IFX_ADSL_BSP_IOC_DSLSTART,(unsigned long)NULL)!=0)
		meierr = MEI_FAILURE;
	else
		meierr = MEI_SUCCESS;

	if (meierr == MEI_FAILURE)
	{
		IFX_ADSL_EMSG("Running ADSL modem firmware fail!\n");
		return MEI_FAILURE;
	}
	printk("ADSL firmware is running\n");
	MEI_MUTEX_UNLOCK(mei_sema);	// allow cmv access
	return MEI_SUCCESS;
}


////////////////makeCMV(Opcode, Group, Address, Index, Size, Data), CMV in u16 TxMessage[MSG_LENGTH]///////////////////////////

/**
 * Compose a message.
 * This function compose a message from opcode, group, address, index, size, and data
 *
 * \param	opcode		The message opcode
 * \param	group		The message group number
 * \param	address		The message address.
 * \param	index		The message index.
 * \param	size		The number of words to read/write.
 * \param	data		The pointer to data.
 * \param	CMVMSG		The pointer to message buffer.
 * \ingroup	Internal
 */
void makeCMV(u8 opcode, u8 group, u16 address, u16 index, int size, u16 * data,u16 *CMVMSG)
{
	memset(CMVMSG, 0, MSG_LENGTH*2);
	CMVMSG[0]= (opcode<<4) + (size&0xf);
	CMVMSG[1]= (((index==0)?0:1)<<7) + (group&0x7f);
	CMVMSG[2]= address;
	CMVMSG[3]= index;
	if((opcode == H2D_CMV_WRITE) && (data != NULL))
		memcpy(CMVMSG+4, data, size*2);
	return;
}


/**
 * Send a message to ARC and read the response
 * This function sends a message to arc, waits the response, and reads the responses.
 *
 * \param	request		Pointer to the request
 * \param	reply		Wait reply or not.
 * \param	response	Pointer to the response
 * \return	MEI_SUCCESS or MEI_FAILURE
 * \ingroup	Internal
 */
MEI_ERROR meiCMV(u16 * request, int reply, u16 *response)            // write cmv to arc, if reply needed, wait for reply
{
	ifx_adsl_device_t *adsl_dev;
	
	adsl_dev = IFX_ADSL_GetAdslDevice();
	return IFX_ADSL_BSP_SendCMV(adsl_dev,request,YES_REPLY,response);		
}


////////////////////////hdlc ////////////////

/**
 * Get the hdlc status
 *
 * \return	HDLC status
 * \ingroup	Internal
 */
static unsigned int IFX_ADSL_GetHdlcStatus(void)
{
	u16 CMVMSG[MSG_LENGTH];
	int ret;

	if (showtime!=1)
		return -ENETRESET;

	makeCMV(H2D_CMV_READ, STAT, STAT_ME_HDLC, 0, 1, NULL,CMVMSG);	//Get HDLC status
	ret = IFX_ADSL_Ioctls((struct inode *)0,NULL, IFX_ADSL_IOC_CMV_WINHOST, (unsigned long) CMVMSG);
	if (ret != 0)
	{
		return -EIO;
	}
	return CMVMSG[4]&0x0F;
}

/**
 * Check if the me is reslved.
 *
 * \param	status		the me status
 * \return	ME_HDLC_UNRESOLVED or ME_HDLC_RESOLVED
 * \ingroup	Internal
 */
int IFX_ADSL_IsHdlcResolved(int status)
{
	u16 CMVMSG[MSG_LENGTH];
	int ret;
	if (adsl_mode <=8 && adsl_mode_extend==0) // adsl mode
	{
		makeCMV(H2D_CMV_READ, CNTL, CNTL_ME_HDLC, 0, 1, NULL,CMVMSG);	//Get ME-HDLC Control
		ret = IFX_ADSL_Ioctls((struct inode *)0,NULL, IFX_ADSL_IOC_CMV_WINHOST, (unsigned long)CMVMSG);
		if (ret != 0)
		{
			return ME_HDLC_UNRESOLVED;
		}
		if (CMVMSG[4]&(1<<0))
		{
			return ME_HDLC_UNRESOLVED;
		}
	}
	else
	{
		if (status == ME_HDLC_MSG_QUEUED || status == ME_HDLC_MSG_SENT)
			return ME_HDLC_UNRESOLVED;
		if (status == ME_HDLC_IDLE)
		{
			makeCMV(H2D_CMV_READ, CNTL, CNTL_ME_HDLC, 0, 1, NULL,CMVMSG);	//Get ME-HDLC Control
			ret = IFX_ADSL_Ioctls((struct inode *)0,NULL, IFX_ADSL_IOC_CMV_WINHOST, (unsigned long)CMVMSG);
			if (ret != 0)
			{
				return IFX_POP_EOC_FAIL;
			}
			if (CMVMSG[4]&(1<<0))
			{
				return ME_HDLC_UNRESOLVED;
			}
		}
	}
	return ME_HDLC_RESOLVED;
}

int _IFX_ADSL_SendHdlc(unsigned char *hdlc_pkt,int pkt_len,int max_length)
{
	int ret;
	u16 CMVMSG[MSG_LENGTH];
	u16 data=0;
	u16 len=0;
	int rx_length=0;
	int write_size=0;

	if (pkt_len > max_length)
	{
		makeCMV(H2D_CMV_READ, INFO, INFO_RX_CLEAR_EOC, 2, 1, NULL,CMVMSG);	//Get ME-HDLC Control
		ret = IFX_ADSL_Ioctls((struct inode *)0,NULL, IFX_ADSL_IOC_CMV_WINHOST,(unsigned long) CMVMSG);
		if (ret != 0)
		{
			return -EIO;
		}
		rx_length = CMVMSG[4];
		if (rx_length + max_length < pkt_len)
		{
			printk("Exceed maximum eoc rx(%d)+tx(%d) message length\n",rx_length, max_length);
			return -EMSGSIZE;
		}
		data = 1;
		makeCMV(H2D_CMV_WRITE, INFO, INFO_RX_CLEAR_EOC, 6, 1, &data, CMVMSG);	//disable RX Eoc
		ret = IFX_ADSL_Ioctls((struct inode *)0,NULL, IFX_ADSL_IOC_CMV_WINHOST, (unsigned long)CMVMSG);
		if (ret != 0)
		{
			return -EIO;
		}
	}
	while(len < pkt_len)
	{
		write_size = pkt_len - len;
		if (write_size > 24)
			write_size = 24;
		//printk("len=%d,write_size=%d,pkt_len=%d\n",len,write_size,pkt_len);
		memset(CMVMSG,0,sizeof(CMVMSG));
		makeCMV(H2D_CMV_WRITE, INFO, INFO_ME_HDLC_TxBuffer, len/2, (write_size+1)/2,(u16 *)(hdlc_pkt+len),CMVMSG);	//Write clear eoc message to ARC
		ret = IFX_ADSL_Ioctls((struct inode *)0,NULL, IFX_ADSL_IOC_CMV_WINHOST, (unsigned long)CMVMSG);
		if (ret != 0)
		{
			return -EIO;
		}
		len+=write_size;
	}
	makeCMV(H2D_CMV_WRITE, INFO, INFO_ME_HDLC_Params, 2, 1,&len,CMVMSG);	//Update tx message length
	ret = IFX_ADSL_Ioctls((struct inode *)0,NULL, IFX_ADSL_IOC_CMV_WINHOST, (unsigned long)CMVMSG);
	if (ret != 0)
	{
		return -EIO;
	}
	data = (1<<0);
	makeCMV(H2D_CMV_WRITE, CNTL, CNTL_ME_HDLC, 0, 1,&data,CMVMSG);	//Start to send
	ret = IFX_ADSL_Ioctls((struct inode *)0,NULL, IFX_ADSL_IOC_CMV_WINHOST, (unsigned long)CMVMSG);
	if (ret != 0)
	{
		return -EIO;
	}
	return 0;
}


/**
 * Send hdlc packets
 *
 * \param	hdlc_pkt	Pointer to hdlc packet
 * \param	hdlc_pkt_len	The number of bytes to send
 * \return	success or failure.
 * \ingroup	Internal
 */
int IFX_ADSL_SendHdlc(unsigned char *hdlc_pkt,int hdlc_pkt_len)
{
	int hdlc_status=0;
	u16 CMVMSG[MSG_LENGTH];
	int max_hdlc_tx_length=0,ret=0,retry=0;

	while(retry<10)
	{

		hdlc_status = IFX_ADSL_GetHdlcStatus();

		if (IFX_ADSL_IsHdlcResolved(hdlc_status)==ME_HDLC_RESOLVED) // arc ready to send HDLC message
		{
			makeCMV(H2D_CMV_READ, INFO, INFO_ME_HDLC_Params, 0, 1, NULL,CMVMSG);	//Get Maximum Allowed HDLC Tx Message Length
			ret = IFX_ADSL_Ioctls((struct inode *)0,NULL, IFX_ADSL_IOC_CMV_WINHOST, (unsigned long)&CMVMSG[0]);
			if (ret != 0)
			{
				IFX_ADSL_DMSG("IFX_ADSL_SendHdlc failed. Return -EIO");
				return -EIO;
			}
			max_hdlc_tx_length = CMVMSG[4];
			ret = _IFX_ADSL_SendHdlc(hdlc_pkt,hdlc_pkt_len,max_hdlc_tx_length);
			return ret;
		}
		retry++;
		MEI_WAIT(1);
	}
	IFX_ADSL_DMSG("IFX_ADSL_SendHdlc failed. Return -EBUSY");
	return -EBUSY;
}

/**
 * Read the hdlc packets
 *
 * \param	hdlc_pkt	Pointer to hdlc packet
 * \param	hdlc_pkt_len	The maximum number of bytes to read
 * \return	The number of bytes which reads.
 * \ingroup	Internal
 */
int IFX_ADSL_ReadHdlc(char *hdlc_pkt,int max_hdlc_pkt_len)
{
	u16 CMVMSG[MSG_LENGTH];
	int msg_read_len,ret=0,pkt_len=0,retry = 0;

	while(retry<10)
	{
		ret = IFX_ADSL_GetHdlcStatus();
		if (ret == ME_HDLC_RESP_RCVD)
		{
			int current_size=0;
			makeCMV(H2D_CMV_READ, INFO, INFO_ME_HDLC_Params, 3, 1, NULL,CMVMSG);	//Get EoC packet length
			ret = IFX_ADSL_Ioctls((MEI_inode_t *)0,NULL, IFX_ADSL_IOC_CMV_WINHOST, (unsigned long)CMVMSG);
			if (ret != 0)
			{
				return -EIO;
			}

			pkt_len = CMVMSG[4];
			if (pkt_len > max_hdlc_pkt_len)
			{
				ret = -ENOMEM;
				goto error;
			}
			while( current_size < pkt_len)
			{
				if (pkt_len - current_size >(MSG_LENGTH*2-8))
					msg_read_len = (MSG_LENGTH*2-8);
				else
					msg_read_len = pkt_len - (current_size);
				makeCMV(H2D_CMV_READ, INFO, INFO_ME_HDLC_RxBuffer, 0 + (current_size/2), (msg_read_len+1)/2, NULL,CMVMSG);	//Get hdlc packet
				ret = IFX_ADSL_Ioctls((MEI_inode_t *)0,NULL, IFX_ADSL_IOC_CMV_WINHOST, (unsigned long)CMVMSG);
				if (ret != 0)
				{
					goto error;
				}
				memcpy(hdlc_pkt+current_size,&CMVMSG[4],msg_read_len);
				current_size +=msg_read_len;
			}
			ret = current_size;
			break;
		}else
		{
			ret = -ENODATA;
		}

		retry++;

		MEI_WAIT(10);
	}
error:
	return ret;
}

static ssize_t IFX_ADSL_Write(MEI_file_t * filp, char * buf, size_t size, loff_t * loff)
{
	MEI_ERROR mei_error = MEI_FAILURE;
	long offset=0;
	ifx_adsl_device_t *adsl_dev;
	
	adsl_dev = IFX_ADSL_GetAdslDevice();	
	if (adsl_dev == NULL)
		return -EPERM;
	mei_error = IFX_ADSL_BSP_FWDownload(adsl_dev, buf,size, (long *)loff,&offset);	
	if (mei_error == MEI_FAILURE)
		return -EIO;
	return (ssize_t)offset;
}

#ifdef CONFIG_PROC_FS
static int IFX_ADSL_ProcRead(struct file * file, char * buf, size_t nbytes, loff_t *ppos)
{
	int i_ino = (file->f_dentry->d_inode)->i_ino;
	char outputbuf[64];
	int count=0;
	int i;
	reg_entry_t* current_reg=NULL;
	ifx_adsl_device_t *adsl_dev;
	
	u32 version=0;
	u16 RxMessage[MSG_LENGTH]__attribute__ ((aligned(4)));
	u16 TxMessage[MSG_LENGTH]__attribute__ ((aligned(4)));
	
	adsl_dev = IFX_ADSL_GetAdslDevice();
		
	for (i=0;i<NUM_OF_REG_ENTRY;i++) {
		if (regs[i].low_ino==i_ino) {
			current_reg = &regs[i];
			break;
		}
	}
	if (current_reg==NULL)
		return -EINVAL;

	if (current_reg->flag == (int *) 1){	
		///proc/mei/version
		//format:
		//Firmware version: major.minor.sub_version.int_version.rel_state.spl_appl
		///Firmware Date Time Code: date/month min:hour
		if (*ppos>0) /* Assume reading completed in previous read*/
			return 0;               // indicates end of file
		if (adsl_dev == NULL)
			return -EPERM;

		if (IFX_ADSL_IsModemReady(adsl_dev) != 1)
		{
			return -EAGAIN;
		}
		
		if(MEI_MUTEX_LOCK(mei_sema))
			return -ERESTARTSYS;
		
		//major:bits 0-7
		//minor:bits 8-15
		makeCMV(H2D_CMV_READ, INFO, INFO_Version, 0, 1, NULL,TxMessage);
		if(meiCMV(TxMessage, YES_REPLY,RxMessage)!=MEI_SUCCESS)
		{
			MEI_MUTEX_UNLOCK(mei_sema);
			return -EIO;
		}
		version = RxMessage[4];
		count = sprintf(outputbuf, "%d.%d.",(version)&0xff,(version>>8)&0xff);

		//sub_version:bits 4-7
		//int_version:bits 0-3
		//spl_appl:bits 8-13
		//rel_state:bits 14-15
		makeCMV(H2D_CMV_READ, INFO, INFO_Version, 1, 1, NULL,TxMessage);
		if(meiCMV(TxMessage, YES_REPLY,RxMessage)!=MEI_SUCCESS)
		{
			MEI_MUTEX_UNLOCK(mei_sema);
			return -EFAULT;
		}
		version =RxMessage[4];
		count += sprintf(outputbuf+count, "%d.%d.%d.%d",
				(version>>4)&0xf,
				version&0xf,
				(version>>14)&0x3,
				(version>>8)&0x3f);
		//Date:bits 0-7
		//Month:bits 8-15
		makeCMV(H2D_CMV_READ, INFO, INFO_TimeStamp, 0, 1, NULL,TxMessage);
		if(meiCMV(TxMessage, YES_REPLY,RxMessage)!=MEI_SUCCESS){
			MEI_MUTEX_UNLOCK(mei_sema);
			return -EIO;
		}
		version = RxMessage[4];

		//Hour:bits 0-7
		//Minute:bits 8-15
		makeCMV(H2D_CMV_READ, INFO, INFO_TimeStamp, 1, 1, NULL,TxMessage);
		if(meiCMV(TxMessage, YES_REPLY,RxMessage)!=MEI_SUCCESS){
			MEI_MUTEX_UNLOCK(mei_sema);
			return -EFAULT;
		}
		version += (RxMessage[4]<<16);
		count += sprintf(outputbuf+count, " %d/%d %d:%d\n"
				,version&0xff
				,(version>>8)&0xff
				,(version>>25)&0xff
				,(version>>16)&0xff);
		MEI_MUTEX_UNLOCK(mei_sema);
		*ppos+=count;
	}else if (current_reg->flag == (int *) 2)
	{	
		if (*ppos>0) /* Assume reading completed in previous read*/
			return 0;  
			  
		if (adsl_dev == NULL)
			return -EPERM;
			
		count = sprintf(outputbuf, "%d\n",showtime);
		*ppos+=count;
	}else{
		if((int)(*ppos)/((int)7)==16)
			return 0;  // indicate end of the message
		count = sprintf(outputbuf, "0x%04X\n\n", *(((u16 *)(current_reg->flag))+ (int)(*ppos)/((int)7)));
		*ppos+=count;
	}
	if (copy_to_user(buf, outputbuf, count))
		return -EFAULT;
	return count;
}
#endif // CONFIG_PROC_FS

/********************************************************
 * L3 Power Mode                                        *
 ********************************************************/
/**
 * Send a CMV message.
 * This function sends a CMV message to ARC
 *
 * \param	opcode		The message opcode
 * \param	group		The message group number
 * \param	address		The message address.
 * \param	index		The message index.
 * \param	size		The number of words to read/write.
 * \param	data		The pointer to data.
 * \param	CMVMSG		The pointer to message buffer.
 * \return	0: success
 * \ingroup	Internal
 */
int IFX_ADSL_SendCmv(u8 opcode, u8 group, u16 address, u16 index, int size, u16 * data, u16 *CMVMSG)
{
	int ret;

	makeCMV(opcode, group, address, index, size, data,CMVMSG);
	ret = IFX_ADSL_Ioctls((struct inode *)0,NULL, IFX_ADSL_IOC_CMV_WINHOST, (unsigned long)CMVMSG);
	return ret;
}

#ifdef IFX_ADSL_L3_MODE_SUPPORT

/**
 * Check the L3 request from CO
 * This function Check if CPE received the L3 request from CO
 * \return	1: got L3 request.
 * \ingroup	Internal
 */
int IFX_ADSL_IsCoL3Request(void)
{
	u16 CMVMSG[MSG_LENGTH];
	ifx_adsl_device_t *adsl_dev;
	
	adsl_dev = IFX_ADSL_GetAdslDevice();	
	if (IFX_ADSL_IsModemReady(adsl_dev) == 1)
	{
		if (IFX_ADSL_SendCmv(H2D_CMV_READ, STAT, 4, 0, 1, NULL,CMVMSG)!= 0)
		{
			return -EBUSY;
		}
		if (CMVMSG[4]& (1<<14))
		{
			return 1;
		}
	}
	return 0;
}

/**
 * Check the L3 status
 * This function get the CPE Power Management Mode status
 * \return	0: L0 Mode
 *		2: L2 Mode
 *		3: L3 Mode
 * \ingroup	Internal
 */
int IFX_ADSL_GetL3Status(void)
{
	u16 CMVMSG[MSG_LENGTH];
	ifx_adsl_device_t *adsl_dev;
	
	adsl_dev = IFX_ADSL_GetAdslDevice();	
	if (!adsl_dev)
		return 0;
	if (IFX_ADSL_IsModemReady(adsl_dev) == 0)
	{
		return L3_POWER_MODE;
	}else
	{
		if (IFX_ADSL_SendCmv(H2D_CMV_READ, STAT, 18, 0, 1, NULL,CMVMSG)!= 0)
		{
			return -EBUSY;
		}
		return ((int)CMVMSG[4]);

	}
	return 0;
}

/**
 * Send a L3 request to CO
 * This function send a L3 request to CO and check the CO response.
 * \return	0: Success. Others: Fail.
 * \ingroup	Internal
 */
int IFX_ADSL_SendL3Request(void)
{
	u16 cmd = 0x1;
	int nRetry = 0;
	u16 CMVMSG[MSG_LENGTH];
	ifx_adsl_device_t *adsl_dev;
	
	adsl_dev = IFX_ADSL_GetAdslDevice();	

	if (IFX_ADSL_IsModemReady(adsl_dev) == 0)
	{
		return -EBUSY;
	}
	// send l3 request to CO
	if (IFX_ADSL_SendCmv(H2D_CMV_WRITE, CNTL, 3, 0, 1, &cmd, CMVMSG)!= 0)
	{
		return -EBUSY;
	}
retry:
	MEI_WAIT(10);

        // check CO response
	if (IFX_ADSL_SendCmv(H2D_CMV_READ, STAT, 20, 0, 1, NULL, CMVMSG)!= 0)
	{
		return -EBUSY;
	}
	if (CMVMSG[4] == 0)
	{
		nRetry ++;
		if (nRetry < 10)
		{
			goto retry;
		}else
		{
			return -EBUSY;
		}

	}else if (CMVMSG[4] == 1) // reject
	{
		return -EPERM;
	}else if (CMVMSG[4] == 2) // ok
	{
		return 0;
	}else if (CMVMSG[4] == 3) // failure
	{
		return -EAGAIN;
	}
	return 0;
}

/**
 * Enable L3 Power Mode
 * This function send a L3 request to CO and check the CO response. Then reboot the CPE to enter L3 Mode.
 * \return	0: Success. Others: Fail.
 * \ingroup	Internal
 */
int IFX_ADSL_SetL3Shutdown(void)
{
	int ret = 0;
	if (l3_shutdown==0)
	{
		// send l3 request to CO
		ret = IFX_ADSL_SendL3Request();
		if (ret == 0)//got CO ACK
		{
			//reboot adsl and block autoboot daemon
			ret = IFX_ADSL_Ioctls((struct inode *)0,NULL, IFX_ADSL_IOC_REBOOT, (unsigned long)NULL);
			l3_shutdown = 1;
		}
	}
	return ret;
}

/**
 * Disable L3 Power Mode
 * This function disable L3 Mode and wake up the autoboot daemon.
 * \return	0: Success.
 * \ingroup	Internal
 */
//l3 power mode disable
int IFX_ADSL_SetL3Poweron(void)
{
	if (l3_shutdown==1)
	{
		l3_shutdown = 0;
		// wakeup autoboot daemon
		MEI_WAKEUP_EVENT(wait_queue_l3);

	}
	return 0;
}
/********************************************************
 * End of L3 Power Mode                                 *
 ********************************************************/
#endif //IFX_ADSL_L3_MODE_SUPPORT


#ifdef IFX_ADSL_DUAL_LATENCY_SUPPORT
/*
 * Dual Latency Path Initialization function
 */
int IFX_ADSL_DualLatency_Init(void)
{
	u16 nDual = 0;
	u16 CMVMSG[MSG_LENGTH];

        // setup up stream path
	if (bDualLatency&DUAL_LATENCY_US_ENABLE)
	{
		nDual = 2;
	}else
	{
		nDual = 1;
	}

	if (IFX_ADSL_SendCmv(H2D_CMV_WRITE, CNFG, 10, 0, 1, &nDual, CMVMSG)!= 0)
	{
		return -EBUSY;
	}

	if (IFX_ADSL_SendCmv(H2D_CMV_WRITE, CNFG, 11, 0, 1, &nDual, CMVMSG)!= 0)
	{
		return -EBUSY;
	}

        // setup down stream path
	if (bDualLatency&DUAL_LATENCY_DS_ENABLE)
	{
		nDual = 2;
	}else
	{
		nDual = 1;
	}

	if (IFX_ADSL_SendCmv(H2D_CMV_WRITE, CNFG, 21, 0, 1, &nDual, CMVMSG)!= 0)
	{
		return -EBUSY;
	}
	if (IFX_ADSL_SendCmv(H2D_CMV_WRITE, CNFG, 22, 0, 1, &nDual, CMVMSG)!= 0)
	{
		return -EBUSY;
	}
	return 0;
}

int IFX_ADSL_IsDualLatencyEnabled(void)
{
	return bDualLatency;
}
#endif

int IFX_ADSL_AdslStart(void)
{
	int meierr=MEI_SUCCESS;
	ifx_adsl_device_t *adsl_dev;
	
	adsl_dev = IFX_ADSL_GetAdslDevice();	
	if (adsl_dev == NULL)
		return -EPERM;
	showtime=0;
	loop_diagnostics_completed = 0;
	if (time_disconnect.tv_sec == 0)
		do_gettimeofday(&time_disconnect);

	IFX_ADSL_DMSG("time_disconnect %d sec\n", (int)time_disconnect.tv_sec);
	if(MEI_MUTEX_LOCK(mei_sema))	//disable CMV access until ARC ready
	{
		IFX_ADSL_EMSG("-ERESTARTSYS\n");
		return -ERESTARTSYS;
	}

	if(IFX_ADSL_RunAdslModem()!= MEI_SUCCESS){
		IFX_ADSL_EMSG("IFX_ADSL_RunAdslModem() error...");
		meierr=MEI_FAILURE;
	}
#ifdef IFX_ADSL_L3_MODE_SUPPORT
        /* L3 Power Mode Start*/
	if (l3_shutdown==1)
	{
		// block autoboot daemon until l3 power mode disable
		MEI_WAIT_EVENT(wait_queue_l3);
	}
	/* L3 Power Mode End*/
#endif //IFX_ADSL_L3_MODE_SUPPORT

#ifdef IFX_ADSL_INCLUDE_AUTOBOOT
	ifx_adsl_autoboot_thread_restart();
#endif /* IFX_ADSL_INCLUDE_AUTOBOOT */

	return meierr;
}

/**
 * All actions which are necessary once showtime is reached
 *
 * \remarks Assumes that the caller is holding the mei_sema !
 * \ingroup	Internal
 */
int IFX_ADSL_AdslShowtime(void)
{
	int meierr=MEI_SUCCESS;
	ifx_adsl_device_t *adsl_dev;
	
	adsl_dev = IFX_ADSL_GetAdslDevice();	
	u16 RxMessage[MSG_LENGTH]__attribute__ ((aligned(4)));
	u16 TxMessage[MSG_LENGTH]__attribute__ ((aligned(4)));

	do_gettimeofday(&time_showtime);
	unavailable_seconds += time_showtime.tv_sec - time_disconnect.tv_sec;
	IFX_ADSL_DMSG("unavailable_seconds = %d\n", unavailable_seconds);
	time_disconnect.tv_sec = 0;

// read adsl mode
	meierr = IFX_ADSL_ReadAdslMode();

#ifdef IFX_ADSL_DYING_GASP_SUPPORT
	meierr = IFX_ADSL_GetDyingGaspAddr(adsl_dev);
#endif //IFX_ADSL_DYING_GASP_SUPPORT	

#ifdef CONFIG_IFX_ADSL_MIB
	mei_mib_adsl_link_up();
#endif

	makeCMV(H2D_CMV_WRITE, PLAM, PLAM_NearEndUASLCnt, 0, 1, &unavailable_seconds,TxMessage);
	if(meiCMV(TxMessage, YES_REPLY,RxMessage)!=MEI_SUCCESS){
		IFX_ADSL_EMSG("\n\nCMV fail, Group 7 Address 10 Index 0");
	}
	

	showtime=1;
	if (adsl_dev==NULL)
		meierr = MEI_FAILURE;
	else
	{
		u32 rate_fast=0;
		u32 rate_intl=0;
		IFX_ADSL_GetUsRate(adsl_dev,&rate_fast,&rate_intl);
		meierr = IFX_ADSL_BSP_Showtime(adsl_dev,rate_fast,rate_intl);
	}
	
	
	return meierr;
}

/**
 * Read out the ADSL mode after training (showtime or diagnostic)
 *
 * \remarks Assumes that the caller is holding the mei_sema !
 * \ingroup	Internal
 */
int IFX_ADSL_ReadAdslMode(void)
{
	int meierr=MEI_SUCCESS;
	u16 RxMessage[MSG_LENGTH]__attribute__ ((aligned(4)));
	u16 TxMessage[MSG_LENGTH]__attribute__ ((aligned(4)));

	makeCMV(H2D_CMV_READ, STAT, STAT_Mode, 0, 1, NULL,TxMessage);
	if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
		IFX_ADSL_DMSG("\n\nCMV fail, Group STAT Address 1 Index 0");
	}
	adsl_mode = RxMessage[4];

	makeCMV(H2D_CMV_READ, STAT, STAT_Mode1, 0, 1, NULL, TxMessage);
	if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
		IFX_ADSL_DMSG("\n\nCMV fail, Group STAT Address 17 Index 0");
	}
	adsl_mode_extend = RxMessage[4];

	IFX_ADSL_DMSG("ADSL Mode: 0x%04X 0x%04X\n", adsl_mode, adsl_mode_extend);

	return meierr;
}

int IFX_ADSL_AdslReset(void)
{
	int meierr=MEI_SUCCESS;
#ifdef CONFIG_IFX_ADSL_MIB
	mei_mib_adsl_link_down();
#endif

#ifdef IFX_ADSL_L3_MODE_SUPPORT
	/* L3 Power Mode start */
	if (IFX_ADSL_IsCoL3Request()==1) //co request
	{
		// cpe received co L3 request
		l3_shutdown=1;
	}
	/* L3 Power Mode end */
#endif //IFX_ADSL_L3_MODE_SUPPORT

	IFX_ADSL_ResetDFE();
	IFX_ADSL_HaltDFE();
	return meierr;
}

static int IFX_ADSL_IoctlCopyFrom(int from_kernel,char * dest,char * from,int size)
{
	int ret = 0;
	
	if (!from_kernel )
		ret = copy_from_user((char *)dest, (char *)from, size);
	else
		ret = memcpy((char *)dest,(char *)from, size);
	return ret;
}

static int IFX_ADSL_IoctlCopyTo(int from_kernel,char * dest,char * from,int size)
{
	int ret = 0;
	
	if (!from_kernel )
		ret = copy_to_user((char *)dest, (char *)from, size);
	else
		ret = memcpy((char *)dest,(char *)from, size);
	return 0;
}

int IFX_ADSL_HandleIoctls(MEI_inode_t * ino, MEI_file_t * fil, unsigned int command, unsigned long lon)
{
	int i;

	int meierr=MEI_SUCCESS;
	meireg regrdwr;
	meidebug debugrdwr;
	u16 RxMessage[MSG_LENGTH]__attribute__ ((aligned(4)));
	u16 TxMessage[MSG_LENGTH]__attribute__ ((aligned(4)));
	ifx_adsl_device_t *adsl_dev;
	
	adsl_dev = IFX_ADSL_GetAdslDevice();	

	int from_kernel = 0;//joelin
	if (ino ==(MEI_inode_t *) 0) from_kernel = 1;//joelin
	switch(command){
	case IFX_ADSL_IOC_START:
		meierr = IFX_ADSL_AdslStart();
		break;

	case IFX_ADSL_IOC_SHOWTIME:
		if(MEI_MUTEX_LOCK(mei_sema))
			return -ERESTARTSYS;
		meierr = IFX_ADSL_AdslShowtime();
		MEI_MUTEX_UNLOCK(mei_sema);
		break;

	case IFX_ADSL_IOC_HALT:
		IFX_ADSL_ResetDFE();
		IFX_ADSL_HaltDFE();
		break;
	case IFX_ADSL_IOC_RUN:
		IFX_ADSL_RunDFE();
		break;
	case IFX_ADSL_IOC_CMV_WINHOST:
		if(MEI_MUTEX_LOCK(mei_sema))
			return -ERESTARTSYS;
		IFX_ADSL_IoctlCopyFrom(from_kernel,(char *)TxMessage, (char *)lon, MSG_LENGTH*2);//joelin
		if(meiCMV(TxMessage, YES_REPLY,RxMessage)!=MEI_SUCCESS){
			IFX_ADSL_EMSG("\n\nWINHOST CMV fail :TxMessage:%X %X %X %X, RxMessage:%X %X %X %X %X\n",TxMessage[0],TxMessage[1],TxMessage[2],TxMessage[3],RxMessage[0],RxMessage[1],RxMessage[2],RxMessage[3],RxMessage[4]);
			meierr = MEI_FAILURE;
		}
		else
		{
			IFX_ADSL_IoctlCopyTo(from_kernel, (char *)lon, (char *)RxMessage, MSG_LENGTH*2);
		}
		MEI_MUTEX_UNLOCK(mei_sema);
		break;
#ifdef IFX_ADSL_CMV_EXTRA
	case IFX_ADSL_IOC_CMV_READ:
		IFX_ADSL_IoctlCopyFrom(from_kernel,(char *)(&regrdwr), (char *)lon, sizeof(meireg));
		IFX_ADSL_MemoryRead((u32)regrdwr.iAddress, (u32 *)&(regrdwr.iData));

		IFX_ADSL_IoctlCopyTo(from_kernel, (char *)lon, (char *)(&regrdwr), sizeof(meireg));
		break;

	case IFX_ADSL_IOC_CMV_WRITE:
		IFX_ADSL_IoctlCopyFrom(from_kernel, (char *)&regrdwr, (char *)lon, sizeof(meireg));
		IFX_ADSL_MemoryWrite((u32)regrdwr.iAddress, regrdwr.iData);
		break;

	case IFX_ADSL_IOC_GET_BASE_ADDRESS:
		IFX_ADSL_IoctlCopyTo(from_kernel, (char *)lon, (char *)(&adsl_dev->base_address), sizeof(adsl_dev->base_address));
		break;
	case IFX_ADSL_IOC_REMOTE:
		IFX_ADSL_IoctlCopyFrom(from_kernel, (char *)(&i), (char *)lon, sizeof(int));
		if (IFX_ADSL_DoBspIoctl(adsl_dev,IFX_ADSL_BSP_IOC_REMOTE,(unsigned long)&i)!=0)
			meierr = MEI_FAILURE;
		else
		{
			if(i==0){
				MEI_MUTEX_UNLOCK(mei_sema);
				}
			else if(i==1){
				if(MEI_MUTEX_LOCK(mei_sema))
					return -ERESTARTSYS;
			}
			else{
				IFX_ADSL_EMSG("\n\n IFX_ADSL_IOC_REMOTE argument error");
				meierr=MEI_FAILURE;
			}
		}			
		break;

	case IFX_ADSL_IOC_READDEBUG:
	case IFX_ADSL_IOC_WRITEDEBUG:
		IFX_ADSL_IoctlCopyFrom(from_kernel,(char *)(&debugrdwr), (char *)lon, sizeof(debugrdwr));
		
		if(command==IFX_ADSL_IOC_READDEBUG)
		{
			if (IFX_ADSL_DoBspIoctl(adsl_dev,IFX_ADSL_BSP_IOC_DEBUG_READ,(unsigned long)&debugrdwr)!=0)
				meierr=MEI_FAILURE;
			//adsl_dev->MemoryDebugAccess(adsl_dev, IFX_ADSL_MEMORY_READ, debugrdwr.iAddress, debugrdwr.buffer, debugrdwr.iCount);
		}
		else
		{
			//adsl_dev->MemoryDebugAccess(adsl_dev, IFX_ADSL_MEMORY_WRITE,debugrdwr.iAddress, debugrdwr.buffer, debugrdwr.iCount);
			if (IFX_ADSL_DoBspIoctl(adsl_dev,IFX_ADSL_BSP_IOC_DEBUG_WRITE,(unsigned long)&debugrdwr)!=0)
				meierr=MEI_FAILURE;
		}

		IFX_ADSL_IoctlCopyTo(from_kernel, (char *)lon, (char*)(&debugrdwr), sizeof(debugrdwr));//dying gasp
		break;
	case IFX_ADSL_IOC_RESET:
	case IFX_ADSL_IOC_REBOOT:
		IFX_ADSL_AdslReset();
		break;
	case IFX_ADSL_IOC_DOWNLOAD:
		// DMA the boot code page(s)
		IFX_ADSL_DMSG("Start download pages");
		if (IFX_ADSL_DoBspIoctl(adsl_dev,IFX_ADSL_BSP_IOC_BOOTDOWNLOAD,(unsigned long)NULL)!=0)
			meierr = MEI_FAILURE;
		break;
		
#endif //IFX_ADSL_CMV_EXTRA
	//for clearEoC
	case IFX_ADSL_IOC_JTAG_ENABLE:
		i=1;
		if (IFX_ADSL_DoBspIoctl(adsl_dev,IFX_ADSL_BSP_IOC_JTAG_ENABLE,(unsigned long)&i)!=0)
			meierr = MEI_FAILURE;
		break;
	case IFX_ADSL_IOC_GET_LOOP_DIAGNOSTICS_MODE:
		if ((void *)lon != NULL) {
			if (IFX_ADSL_IoctlCopyTo(from_kernel, (char *)lon, (char *)&loop_diagnostics_mode, sizeof(int)))
				return -EFAULT;
		}
		else
			meierr = loop_diagnostics_mode;
		break;
	case IFX_ADSL_IOC_LOOP_DIAGNOSTIC_MODE_COMPLETE:
		loop_diagnostics_completed = 1;
		// read adsl mode
		meierr = IFX_ADSL_ReadAdslMode();
		MEI_WAKEUP_EVENT(wait_queue_loop_diagnostic);
		break;
	case IFX_ADSL_IOC_SET_LOOP_DIAGNOSTICS_MODE:
		if (lon != loop_diagnostics_mode)
		{
			loop_diagnostics_completed = 0;
			loop_diagnostics_mode = lon;
			meierr = IFX_ADSL_AdslStart();
		}
		break;
	case IFX_ADSL_IOC_IS_LOOP_DIAGNOSTICS_MODE_COMPLETE:
		if ((void *)lon != NULL) {
			if (IFX_ADSL_IoctlCopyTo(from_kernel, (char *)lon, (char *)&loop_diagnostics_completed, sizeof(int)))
				return -EFAULT;
		}
		else
			meierr = loop_diagnostics_completed;
		break;
#ifdef IFX_ADSL_L3_MODE_SUPPORT
	/* L3 Power Mode Start*/
	case IFX_ADSL_IOC_GET_POWER_MANAGEMENT_MODE:
		i = IFX_ADSL_GetL3Status();
		IFX_ADSL_IoctlCopyTo(from_kernel, (char *)lon, (char *)&i, sizeof(int));
		break;
	case IFX_ADSL_IOC_SET_L3_POWER_MODE:
		i=1;
		IFX_ADSL_IoctlCopyFrom(from_kernel, (char *)&i, (char *)lon, sizeof(int));
		if (i == 0)
		{
			return IFX_ADSL_SetL3Shutdown();
		}else
		{
			return IFX_ADSL_SetL3Poweron();
		}
		break;
	/* L3 Power Mode End*/
#endif //IFX_ADSL_L3_MODE_SUPPORT
#ifdef IFX_ADSL_DUAL_LATENCY_SUPPORT
	case IFX_ADSL_IOC_GET_DUAL_LATENCY:
		i = IFX_ADSL_IsDualLatencyEnabled();
		if (i<0)
			return i;
		IFX_ADSL_IoctlCopyTo(from_kernel, (char *)lon, (char *)&i, sizeof(int));
		break;
	case IFX_ADSL_IOC_SET_DUAL_LATENCY:
		i=0;
		IFX_ADSL_IoctlCopyFrom(from_kernel, (char *)&i, (char *)lon, sizeof(int));
		if (i >DUAL_LATENCY_US_DS_ENABLE)
		{
			return -EINVAL;
		}
		if (i!=bDualLatency)
		{
			bDualLatency = i;
			i = 1; // DualLatency update,need to reboot arc
		}else
		{
			i = 0;// DualLatency is the same
		}
		if ((IFX_ADSL_IsModemReady(adsl_dev)) && i ) // modem is already start, reboot arc to apply Dual Latency changed
		{
			IFX_ADSL_Ioctls((MEI_inode_t *)0,NULL, IFX_ADSL_IOC_REBOOT, (unsigned long)NULL);
		}
		break;

#endif
	default:
		IFX_ADSL_EMSG("The ioctl command(0x%X is not supported!\n", command);
		meierr = -ENOIOCTLCMD;
	}
	return meierr;
}

/**
 * MEI IO controls for user space accessing
 *
 * \param	ino		Pointer to the stucture of inode.
 * \param	fil		Pointer to the stucture of file.
 * \param	command		The ioctl command.
 * \param	lon		The address of data.
 * \return	Success or failure.
 * \ingroup	Internal
 */
int IFX_ADSL_Ioctls(MEI_inode_t * ino, MEI_file_t * fil, unsigned int command, unsigned long lon)
{
	int meierr=0;
	if (command < IFX_ADSL_IOC_BASIC_BASE)
	{
#ifdef CONFIG_IFX_ADSL_MIB
		return mei_mib_ioctl(ino,fil,command,lon);
#else
		IFX_ADSL_EMSG("No such ioctl command (0x%X)! MEI ADSL MIB is not supported!\n",command);
		meierr = -ENOIOCTLCMD;
#endif //CONFIG_IFX_ADSL_MIB
	}
	else if (command >= IFX_ADSL_IOC_BASIC_BASE && command < IFX_ADSL_IOC_CEOC_BASE)
	{
		meierr = IFX_ADSL_HandleIoctls(ino,fil,command,lon);
	}
#ifdef CONFIG_IFX_ADSL_CEOC	
	else if (command >= IFX_ADSL_IOC_CEOC_BASE && command < IFX_ADSL_IOC_AUTOBOOT_BASE)
	{
		meierr = IFX_ADSL_CEOC_Ioctl(ino,fil,command,lon);
	}
#endif	
#ifdef IFX_ADSL_INCLUDE_AUTOBOOT		
	else if (command >= IFX_ADSL_IOC_AUTOBOOT_BASE && command < IFX_ADSL_IOC_END)
	{
		meierr = ifx_adsl_autoboot_ioctl(ino,fil,command,lon);
	}
#endif	
	else
	{
		IFX_ADSL_EMSG("The ioctl command(0x%X is not supported!\n", command);
		meierr = -ENOIOCTLCMD;
	}
	return meierr;
}//IFX_ADSL_Ioctls

#ifdef IFX_ADSL_DYING_GASP_SUPPORT
static int IFX_ADSL_GetDyingGaspAddr(ifx_adsl_device_t *pDev)
{
	u16 RxMessage[MSG_LENGTH]__attribute__ ((aligned(4)));
	u16 TxMessage[MSG_LENGTH]__attribute__ ((aligned(4)));
	
	makeCMV(H2D_CMV_READ, INFO, INFO_SelfTestResult, 4, 1, NULL, TxMessage);
	if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
		IFX_ADSL_EMSG("\n\nCMV fail, Group 3 Address 66 Index 4");
	}
	pDev->lop_debugwr.iAddress=(u32)RxMessage[4];
	makeCMV(H2D_CMV_READ, INFO, INFO_SelfTestResult, 5, 1, NULL, TxMessage);
	if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
		IFX_ADSL_EMSG("\n\nCMV fail, Group 3 Address 66 Index 5");
	}
	
	// fill lop_debugwr information
	pDev->lop_debugwr.iAddress+=((u32)RxMessage[4])<<16;
	pDev->lop_debugwr.buffer[0]=0xffffffff;		
	pDev->lop_debugwr.iCount=1;	
	return 0;	
}
MEI_ERROR IFX_ADSL_SendDyingGasp(ifx_adsl_device_t *pDev)
{
	if (IFX_ADSL_IsADSLShowtime(pDev))
		IFX_ADSL_Ioctls((struct inode *)0,NULL, IFX_ADSL_IOC_WRITEDEBUG, (unsigned long)&pDev->lop_debugwr);
	return 0;		
}
#endif //IFX_ADSL_DYING_GASP_SUPPORT

int IFX_ADSL_IsADSLShowtime(ifx_adsl_device_t *dev)
{

	if (showtime)
		return 1;
	return 0;
}

int IFX_ADSL_DoBspIoctl(ifx_adsl_device_t *pDev, unsigned int command, unsigned long lon)
{
	if (pDev==NULL)
	{
		printk("%s: device == NULL!\n",__FUNCTION__);
		return -EIO;
	}
	return IFX_ADSL_BSP_KernelIoctls(pDev,command,lon);
}

int IFX_ADSL_IsModemReady(ifx_adsl_device_t *pDev)
{
	int modem_ready=0;
	if (IFX_ADSL_DoBspIoctl(pDev,IFX_ADSL_BSP_IOC_IS_MODEM_READY,(unsigned long)&modem_ready)!=0)
		return 0;
	return modem_ready;
}

static MEI_ERROR IFX_ADSL_GetUsRate(ifx_adsl_device_t *dev,u32 *rate_fast,u32 *rate_intl)
{
	ifx_adsl_device_t *adsl_dev;
	u16 RxMessage[MSG_LENGTH]__attribute__ ((aligned(4)));
	u16 TxMessage[MSG_LENGTH]__attribute__ ((aligned(4)));
	
	if ( (adsl_dev = IFX_ADSL_GetAdslDevice()) != dev)
	{
		return MEI_FAILURE;
	}
	
	if (!showtime)
	{
		return MEI_FAILURE;
	}
	
// reading the datarate makes only sense if debug output is enabled
	makeCMV(H2D_CMV_READ, RATE, RATE_UsRate, 0, 4, NULL, TxMessage);
	if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
		IFX_ADSL_EMSG("\n\nCMV fail, Group RATE Address 0 Index 0");
		return MEI_FAILURE;
	}else
	{
		*rate_intl = RxMessage[4] | RxMessage[5]<<16;
		*rate_fast = RxMessage[6] | RxMessage[7]<<16;
		IFX_ADSL_DMSG("Datarate US intl = %d, fast = %d\n", *rate_intl, *rate_fast);
	}
	return MEI_SUCCESS;
}


static int IFX_ADSL_EventCB(ifx_adsl_cb_event_t *event)
{
	int error = 0;
	if (event == NULL)
	{
		return -EIO;
	}
	switch (event->ID)
	{
		case IFX_ADSL_BSP_EVENT_DYING_GASP:
			IFX_ADSL_SendDyingGasp(event->pDev);
		break;
		case IFX_ADSL_BSP_EVENT_CEOC_IRQ:
#ifdef CONFIG_IFX_ADSL_CEOC
			IFX_ADSL_CEOC_IRQHandler(NULL,event->param->irq_message);	
#endif
		break;		
	}
	return error;
}

#if defined(__LINUX__) 
static int IFX_ADSL_InitDevNode(void)
{
#ifdef CONFIG_DEVFS_FS
	memset(&ifx_adsl_devfs_handle,0,sizeof(ifx_adsl_devfs_handle));
	
	if ((ifx_adsl_devfs_handle = devfs_register( NULL , IFX_ADSL_DEVNAME, DEVFS_FL_DEFAULT,major,0,S_IFCHR | S_IRUGO | S_IWUGO, &mei_operations, (void *)0)) == NULL)
	{
		IFX_ADSL_EMSG("Register mei devfs error.\n");
		return -ENODEV;
	}
	
#else
	if (register_chrdev(major, IFX_ADSL_DEVNAME, &mei_operations)!=0) {
		IFX_ADSL_EMSG("\n\n unable to register major for %s!!!",IFX_ADSL_DEVNAME);
		return -ENODEV;
	}

#endif	
	return 0;
}

#ifdef CONFIG_PROC_FS
static int IFX_ADSL_InitProcFS(void)
{
	struct proc_dir_entry *entry=NULL;
	int i=0;
	reg_entry_t regs_temp[PROC_ITEMS] =    // Items being debugged
	{
	/*  {   flag,          	        name,              description } */
	    {(int *)1, 		     "version", 	"version of firmware", 0},
	    {(int *)2,		     "Modem_Status",	"Adsl Modem Status", 0},
	};
	memcpy((char *)regs, (char *)regs_temp, sizeof(regs_temp));
	
	// procfs
	meidir=proc_mkdir(MEI_DIRNAME, &proc_root);
	if ( meidir == NULL) {
		IFX_ADSL_EMSG(": can't create /proc/" MEI_DIRNAME "\n\n");
		return(-ENOMEM);
	}

	for(i=0;i<NUM_OF_REG_ENTRY;i++) {
		entry = create_proc_entry(regs[i].name,
				S_IWUSR |S_IRUSR | S_IRGRP | S_IROTH,
				meidir);
		if(entry) {
			regs[i].low_ino = entry->low_ino;
			entry->proc_fops = &IFX_ADSL_ProcOperations;
		} else {
			IFX_ADSL_EMSG(": can't create /proc/" MEI_DIRNAME "/%s\n\n", regs[i].name);
			return(-ENOMEM);
		}
	}
	return 0;	
}
#endif //CONFIG_PROC_FS
#endif //#if defined(__LINUX__)

////////////////////////////////////////////////////////////////////////////
int __init IFX_ADSL_ModuleInit(void)
{

	do_gettimeofday(&time_disconnect);
#ifdef CONFIG_DEVFS_FS
	char buf[10];
#endif

	printk("Infineon ADSL version:%s\n", IFX_ADSL_VERSION);

	ifx_adsl_dev = IFX_ADSL_BSP_DriverHandleGet(0,0);
	if (ifx_adsl_dev == NULL)
	{
		printk("Get BSP Driver Handle Fail!\n");
	}
	
	MEI_MUTEX_INIT(mei_sema, 1);  // semaphore initialization, mutex
	MEI_INIT_WAKELIST("arcldq",wait_queue_loop_diagnostic);		// for loop diagnostic function
#ifdef IFX_ADSL_L3_MODE_SUPPORT
	MEI_INIT_WAKELIST("arcl3q",wait_queue_l3); // for l3 power mode
#endif //IFX_ADSL_L3_MODE_SUPPORT

	if (IFX_ADSL_BSP_EventCBRegister(IFX_ADSL_EventCB)!=0)
	{
		printk("Regiser Event Callback Fail!\n");
	}
	
#if defined(CONFIG_IFX_ADSL_LED)
	IFX_ADSL_LED_ModuleInit();
#endif

#ifdef IFX_ADSL_INCLUDE_AUTOBOOT
	init_waitqueue_head(&wait_queue_autoboot);
	init_completion(&autoboot_thread_exit);
	ifx_adsl_autoboot_thread_start();
#endif /* IFX_ADSL_INCLUDE_AUTOBOOT */

#ifdef CONFIG_IFX_ADSL_MIB
	ifx_adsl_mib_init();
#endif

#ifdef CONFIG_IFX_ADSL_CEOC
	IFX_ADSL_CEOC_ModuleInit();
#endif

#if defined(__LINUX__)
	IFX_ADSL_InitDevNode();
#ifdef CONFIG_PROC_FS
	IFX_ADSL_InitProcFS();
#endif //CONFIG_PROC_FS
#endif
	///////////////////////////////// register net device ////////////////////////////

	return 0;
}

void __exit IFX_ADSL_ModuleCleanup(void)
{
#if defined(CONFIG_IFX_ADSL_LED) 
	IFX_ADSL_LED_ModuleCleanup();
#endif
	showtime=0;//joelin,clear task

#if defined(__LINUX__)
#ifdef CONFIG_DEVFS_FS
	devfs_unregister(ifx_adsl_devfs_handle);
#else
	unregister_chrdev(major, "ifx_adsl");
#endif
#endif
#ifdef CONFIG_IFX_ADSL_MIB
	ifx_adsl_mib_cleanup();
#endif
#ifdef CONFIG_IFX_ADSL_CEOC
	IFX_ADSL_CEOC_ModuleCleanup();
#endif
#ifdef IFX_ADSL_INCLUDE_AUTOBOOT
	ifx_adsl_autoboot_thread_stop();
#endif /* IFX_ADSL_INCLUDE_AUTOBOOT */

	if (IFX_ADSL_BSP_EventCBUnregister(IFX_ADSL_EventCB)!=0)
	{
		printk("Regiser Event Callback Fail!\n");
	}
	
	IFX_ADSL_BSP_DriverHandleDelete(ifx_adsl_dev);

	return;
}

EXPORT_SYMBOL(IFX_ADSL_SendHdlc);
EXPORT_SYMBOL(IFX_ADSL_ReadHdlc);
EXPORT_SYMBOL(IFX_ADSL_Ioctls);
EXPORT_SYMBOL(IFX_ADSL_IsADSLShowtime);

MODULE_LICENSE("GPL");

module_init(IFX_ADSL_ModuleInit);
module_exit(IFX_ADSL_ModuleCleanup);
