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
 * File Name:   ifx_adsl_ceoc.c
 * Author :     Taicheng
 *
 * ===========================================================================
 *
 * Project: Danube
 *
 * ===========================================================================
 * Contents:This file implements the MEI driver for Danube ADSL/ADSL2+
 *  controller.
 *
 * ===========================================================================
 * References:
 *
 */
/* Change log:
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

static char IFX_ADSL_CEOC_VERSION[]="2.00.01"; // clear eoc module version


#ifdef IFX_ADSL_DEBUG
#define IFX_ADSL_DMSG(fmt, args...) printk( KERN_INFO  "%s: " fmt,__FUNCTION__, ## args)
#else
#define IFX_ADSL_DMSG(fmt, args...) do { } while(0)
#endif //ifdef IFX_ADSL_DEBUG

#define IFX_ADSL_EMSG(fmt, args...) printk( KERN_ERR  "%s: " fmt,__FUNCTION__, ## args)

extern void ifx_push_eoc(struct sk_buff * pkt);
static wait_queue_head_t wait_queue_hdlc_poll;	///wait queue for clear eoc 
static u16 ceoc_read_idx=0; // the current reading index for RX buffer
static int stop_ceoc_module=0;	//wakeup and clean ceoc module
static struct completion ifx_adsl_ceoc_thread_exit;
static int IFX_ADSL_CEOC_Push(struct sk_buff *ceoc_buff);
static int (* ifx_atm_push_ceoc_cb)(struct sk_buff *ceoc_buff);
/**
 * \brief Send a clear eoc packet to CO.
 * This function allocate a buffer to hold the eoc packet, swap the data, and fill the clear eoc header and snmp header, then call IFX_ADSL_SendHdlc to send the HDLC packet
 * \return	0 success else fail
 * \ingroup	Internal
 */ 
static int IFX_ADSL_CEOC_Send(struct sk_buff *eoc_pkt)
{
	int ret,pkt_len=0;
	unsigned char *pkt_data_ptr;
	int offset=0;
	int swap_idx=0;
	// check the adsl mode, if adsl 2/2+ mode, it need to fill the clear eoc header.
	if (adsl_mode <=8 && adsl_mode_extend==0) // adsl mode
	{
		pkt_len = eoc_pkt->len;

		pkt_data_ptr = kmalloc(pkt_len+3,GFP_KERNEL);

		offset=2; // snmp header
		// SNMP header
		pkt_data_ptr[0]=0x4c;
		pkt_data_ptr[1]=0x81;
		pkt_len +=2; // 2: snmp header
	}else
	{
		pkt_len = eoc_pkt->len + 4; // 4: eoc header and snmp header
		pkt_data_ptr = kmalloc(pkt_len+1+2,GFP_KERNEL);
		memset(pkt_data_ptr,0,pkt_len+1+2);
		//fill clear eoc header
		pkt_data_ptr[0]=0x1;
		pkt_data_ptr[1]=0x8;
		// SNMP header
		pkt_data_ptr[2]=0x4c;
		pkt_data_ptr[3]=0x81;
		offset=4; // skip eoc header + snmp header
	}
	// swap data
	for(swap_idx=0;swap_idx<(eoc_pkt->len/2)*2;swap_idx+=2)
	{
		//printk("%02X %02X ",eoc_pkt->data[swap_idx],eoc_pkt->data[swap_idx+1]);
		pkt_data_ptr[swap_idx+offset] = eoc_pkt->data[swap_idx+1];
		pkt_data_ptr[swap_idx+1+offset] = eoc_pkt->data[swap_idx];
	}
	if (eoc_pkt->len%2)
	{
		//printk("%02X ",eoc_pkt->data[eoc_pkt->len-1]);
		pkt_data_ptr[eoc_pkt->len-1+offset] = eoc_pkt->data[eoc_pkt->len-1];
		pkt_data_ptr[eoc_pkt->len+offset] = eoc_pkt->data[eoc_pkt->len-1];
	}
	// send to CO
	ret = IFX_ADSL_SendHdlc(pkt_data_ptr,pkt_len);

	if (pkt_data_ptr != eoc_pkt->data)
	{
		kfree(pkt_data_ptr);
	}
	dev_kfree_skb(eoc_pkt);
	return ret;
}

/**
 * \brief Read a clear eoc packet from DSL Clear EoC buffer.
 * This function read the eoc packet from DSL clear eoc buffer, update the read index and swap the data.
 * \return	0 success else fail
 * \ingroup	Internal
 */ 
static int IFX_ADSL_CEOC_ReadData(int pkt_len,int rx_buffer_addr,int rx_buffer_len,u8 *data_ptr1)
{
	int ret;
	MEI_ERROR dma_ret;
	u16 CMVMSG[MSG_LENGTH];
	int read_size,aread_size;
	int offset = 0;
	u8 *data=NULL,*data_ptr=NULL;
	int i,j;
	int over_read=0;
	ifx_adsl_device_t *adsl_dev;

	adsl_dev = IFX_ADSL_GetAdslDevice();
	if (adsl_dev == NULL)
	{
		return -EIO;
	}	

	i=j=0;

	read_size = (pkt_len /4) + 4; //size to read
	offset = ceoc_read_idx % 4; // offset to read
	over_read = read_size *4 - pkt_len - offset;// over reading

	ceoc_read_idx = (ceoc_read_idx & 0xFFFFFFFC );

	// allocate memory
	data = kmalloc(read_size *4,GFP_KERNEL);
	if (data == NULL)
		goto error;
	data_ptr = kmalloc(read_size *4,GFP_KERNEL);
	if (data_ptr == NULL)
		goto error;

	// calculate size for read 	
	if ( ceoc_read_idx + read_size*4 >= rx_buffer_len)
	{
		// data size want to read > dsl ceoc buffer size, need to read twice
		aread_size = (rx_buffer_len - ceoc_read_idx)/4;
	}
	else
	{
		aread_size = read_size;
	}

	// read data
	dma_ret =IFX_ADSL_BSP_MemoryDebugAccess(adsl_dev, IFX_ADSL_MEMORY_READ,rx_buffer_addr+ceoc_read_idx, (u32*)(data), aread_size);

	ceoc_read_idx += aread_size*4;
	// read second time for the read > dsl ceoc buffer size case
	if (aread_size != read_size)
	{
		dma_ret = IFX_ADSL_BSP_MemoryDebugAccess(adsl_dev, IFX_ADSL_MEMORY_READ,rx_buffer_addr, (u32*)(data)+aread_size, read_size - aread_size);
		ceoc_read_idx = (read_size - aread_size)*4;
	}
	// update read_index
	if (ceoc_read_idx < over_read)
		ceoc_read_idx =rx_buffer_len + ceoc_read_idx - over_read;
	else
		ceoc_read_idx -=over_read;
	// swap data from little endian to big endian
	if(offset == 0 || offset ==2){
		for(i=0; i<read_size; i++){
			// 3412 --> 1234

			for (j=0;j<4;j++)
			{
				if (i*4+j -offset >= 0)
				data_ptr[i*4+j-offset] = data[i*4+(3-j)];
			}
		}

	}else if (offset == 1){
		for(i=0; i<pkt_len; i=i+4){

			data_ptr[i+1] = data[i+1];
			data_ptr[i  ] = data[i+2];
			data_ptr[i+3] = data[i+7];
			data_ptr[i+2] = data[i];
		}
	}else if(offset ==3){
		for(i=0; i<pkt_len; i=i+4){
			data_ptr[i+1] = data[i+7];
			data_ptr[i+0] = data[i];
			data_ptr[i+3] = data[i+5];
			data_ptr[i+2] = data[i+6];
		}
	}
	if (pkt_len %2 ==1)
		data_ptr[pkt_len-1] = data_ptr[pkt_len];

#if 0
	//dump packet
	for(i=0; i<pkt_len; i++){
		printk("%02X ",data_ptr[i]);
		if (i+1%8==0)
			printk("\n");
	}
	printk("\n");
#endif
	
	kfree(data);
	memcpy(data_ptr1,data_ptr,pkt_len);
	kfree(data_ptr);

	// update read index
	makeCMV(H2D_CMV_WRITE, INFO, INFO_RX_CLEAR_EOC, 3 , 1, &ceoc_read_idx,CMVMSG);
	ret = IFX_ADSL_Ioctls((struct inode *)0,NULL, IFX_ADSL_IOC_CMV_WINHOST,(unsigned long) CMVMSG);
	if (ret != 0)
	{
		goto error;
	}

	return dma_ret;
error:
	kfree(data);
	kfree(data_ptr);
	return -1;
}

/**
 * \brief Read a clear eoc packet from DSL Clear EoC buffer.
 * This function read the eoc packet from DSL clear eoc buffer, update the read index and swap the data.
 * \return	0 success else fail
 * \ingroup	Internal
 */ 
static int IFX_ADSL_CEOC_Receive(int ceoc_write_idx ,int rx_buffer_len,struct sk_buff **eoc_pkt)
{
	u16 CMVMSG[MSG_LENGTH];
	int pkt_len,ret;
	u16 lsw_addr,msw_addr;
	u32 rx_buffer_addr = 0;
	MEI_ERROR dma_ret;

	//printk("rx_buffer_len=%d,ceoc_read_idx=%d,ceoc_write_idx=%d\n",rx_buffer_len,ceoc_read_idx,ceoc_write_idx);
	if (ceoc_write_idx > ceoc_read_idx)
	{
		pkt_len = ceoc_write_idx-ceoc_read_idx ;
	}else
	{
		pkt_len = rx_buffer_len - ceoc_read_idx + ceoc_write_idx ;
	}

	*eoc_pkt = dev_alloc_skb(pkt_len);
	if(*eoc_pkt == NULL)
	{
		printk("Out of memory!\n");
		ret = -ENOMEM;
		goto error;
	}

	makeCMV(H2D_CMV_READ, INFO, INFO_RX_CLEAR_EOC, 0, 1, NULL,CMVMSG);	//Get HDLC packet
	ret = IFX_ADSL_Ioctls((struct inode *)0,NULL, IFX_ADSL_IOC_CMV_WINHOST, (unsigned long)CMVMSG);
	if (ret != 0)
	{
		goto error;
	}
	lsw_addr = CMVMSG[4];

	makeCMV(H2D_CMV_READ, INFO, INFO_RX_CLEAR_EOC, 1, 1, NULL,CMVMSG);	//Get HDLC packet
	ret = IFX_ADSL_Ioctls((struct inode *)0,NULL, IFX_ADSL_IOC_CMV_WINHOST,(unsigned long)CMVMSG);
	if (ret != 0)
	{
		goto error;
	}
	msw_addr = CMVMSG[4];
	rx_buffer_addr = msw_addr<<16 | lsw_addr;

	dma_ret = IFX_ADSL_CEOC_ReadData(pkt_len,rx_buffer_addr, rx_buffer_len,(u8 *)skb_put(*eoc_pkt,pkt_len));
	if(dma_ret != MEI_SUCCESS)
	{
		ret = -EIO;
		goto error;
	}

	return 0;
error:
	if (*eoc_pkt!=NULL)
		dev_kfree_skb(*eoc_pkt);
	return ret;
}

/**
 * \brief Clear Eoc Rx Interrupt bottom half Handler
 * \return	0 success else fail
 * \ingroup	Internal
 */ 
int IFX_ADSL_CEOC_RX(void)
{
	u16 CMVMSG[MSG_LENGTH];
	int rx_buffer_len=0,ret=0;
	struct sk_buff * eoc_pkt;
	u16 ceoc_write_idx=0;

	makeCMV(H2D_CMV_READ, INFO, INFO_RX_CLEAR_EOC, 2, 1, NULL,CMVMSG);	//Get EoC packet length
	ret = IFX_ADSL_Ioctls((struct inode *)0,NULL, IFX_ADSL_IOC_CMV_WINHOST,(unsigned long) CMVMSG);
	if (ret != 0)
	{
		printk("ioctl fail!!\n");
	}
	rx_buffer_len = CMVMSG[4];
	makeCMV(H2D_CMV_READ, INFO, INFO_RX_CLEAR_EOC, 4, 1, NULL,CMVMSG);	//Get write index
	ret = IFX_ADSL_Ioctls((struct inode *)0,NULL, IFX_ADSL_IOC_CMV_WINHOST, (unsigned long)CMVMSG);
	if (ret != 0)
	{
		return -EIO;
	}
	ceoc_write_idx = CMVMSG[4];
	ret = IFX_ADSL_CEOC_Receive(ceoc_write_idx,rx_buffer_len,&eoc_pkt);
#if defined (CONFIG_ATM)
	if (ret == 0)
	{
		skb_pull(eoc_pkt,2); // skip 4c 81 header
		IFX_ADSL_CEOC_Push(eoc_pkt);	//pass data to higher layer
	}
#endif
	return ret;
}

/**
 * \brief Clear Eoc Rx Interrupt Handler
 * This function wakeup the hdlc poll thread to read the Rx data.
 * \return	0 success else fail
 * \ingroup	Internal
 */ 
MEI_ERROR IFX_ADSL_CEOC_IRQHandler(ifx_adsl_device_t *dev,u32 message)
{
	if (message & OMB_CLEAREOC_INTERRUPT_CODE) // clear eoc message interrupt
	{
		MEI_WAKEUP_EVENT(wait_queue_hdlc_poll);
	}
	return MEI_SUCCESS;	
}

/**
 * \brief Main function for Polling Clear Eoc Rx.
 * \return	0 success else fail
 * \ingroup	Internal
 */ 
static int IFX_ADSL_CEOC_Poll(void *unused)
{
	struct task_struct *tsk = current;
	
	stop_ceoc_module = 0;
	
	daemonize();
	reparent_to_init();
	strcpy(tsk->comm, "adsl_ceoc_poll");
	sigfillset(&tsk->blocked);

	while(stop_ceoc_module==0){
		// sleep until CEoC Rx interrupt
		MEI_WAIT_EVENT (wait_queue_hdlc_poll);
		if (stop_ceoc_module)
			break;
		if (showtime)
		{
			// read the date
			IFX_ADSL_CEOC_RX();
		}
	}
	complete_and_exit(&ifx_adsl_ceoc_thread_exit,0);
	return 0;
}

/**
 * \brief Clear EoC send Ioctl function
 * \return	0 success else fail
 * \ingroup	Internal
 */ 
static int IFX_ADSL_CEOC_SendIoctl(MEI_inode_t * ino, MEI_file_t * fil, unsigned int command, unsigned long lon)
{
	int from_kernel = 0;//joelin
	meidebug debugrdwr;
	struct sk_buff * eoc_skb;
	
	if (ino ==(MEI_inode_t *) 0) from_kernel = 1;//joelin	
	
	if (!showtime)
	{
		return -EIO;
	}
	if (!from_kernel)
	{
	    copy_from_user((char *)(&debugrdwr), (char *)lon, sizeof(debugrdwr));
	    eoc_skb = dev_alloc_skb(debugrdwr.iCount*4);
	    if(eoc_skb==NULL){
		IFX_ADSL_EMSG("\n\nskb alloc fail");
		return -ENOMEM;
	    }

	    eoc_skb->len=debugrdwr.iCount*4;
	    memcpy(skb_put(eoc_skb, debugrdwr.iCount*4), (char *)debugrdwr.buffer, debugrdwr.iCount*4);
	}else
	{
	    eoc_skb = (struct sk_buff *)lon;
	}
	return IFX_ADSL_CEOC_Send(eoc_skb);	//pass data to higher layer
}

/**
 * \brief Ioctl function for Clear Eoc module.
 * \return	0 success else fail
 * \ingroup	Internal
 */ 
int IFX_ADSL_CEOC_Ioctl(MEI_inode_t * ino, MEI_file_t * fil, unsigned int command, unsigned long lon)
{
	int meierr = 0;
	switch(command)
	{
	case IFX_ADSL_IOC_CEOC_SEND:
		meierr = IFX_ADSL_CEOC_SendIoctl(ino,fil,command,lon);
		break;
	}
	return meierr;
}

static int IFX_ADSL_CEOC_Push(struct sk_buff *ceoc_buff)
{
	if (ifx_atm_push_ceoc_cb)
	{
		return ifx_atm_push_ceoc_cb(ceoc_buff);
	}
	return -EIO;
}

int IFX_ADSL_CEOC_RXCBRegister( int (* ifx_push_ceoc)(struct sk_buff *ceoc_buff))
{
	int error = 0;
	if (ifx_push_ceoc && ifx_atm_push_ceoc_cb == NULL)
	{
		ifx_atm_push_ceoc_cb = ifx_push_ceoc;
	}else
	{
		error = -EIO;
	}
	MOD_INC_USE_COUNT;
	return error;
}
int IFX_ADSL_CEOC_RXCBUnregister( int (* ifx_push_ceoc)(struct sk_buff *ceoc_buff))
{
	int error = 0;
	if (ifx_push_ceoc && ifx_atm_push_ceoc_cb == ifx_push_ceoc)
	{
		ifx_atm_push_ceoc_cb = NULL;
	}else
	{
		error = -EIO;
	}
	MOD_DEC_USE_COUNT;
	return error;}

/**
 * \brief Setup ADSL firmware to enable the Clear EoC RX interrupt
 * \return	0 success else fail
 * \ingroup	Internal
 */ 
int IFX_ADSL_CEOC_Init(void)
{
	u16 data;
	u16 RxMessage[MSG_LENGTH]__attribute__ ((aligned(4)));
	u16 TxMessage[MSG_LENGTH]__attribute__ ((aligned(4)));	
	
	data=1;
	makeCMV(H2D_CMV_WRITE, OPTN , 24, 0, 1, &data, TxMessage);
	if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
             printk("Enable clear eoc fail!\n");
	}
	
	return 0;
}


/**
 * \brief Clear Eoc Module initialization 
 * This function init the Clear Eoc module. It creates a thread to poll/read clear eoc Rx buffer.
 * \return	0 success else fail
 * \ingroup	Internal
 */ 
int IFX_ADSL_CEOC_ModuleInit(void)
{
	printk("Infineon ADSL ClearEOC Module Version:%s\n",IFX_ADSL_CEOC_VERSION);	
#ifdef __LINUX__
	init_completion(&ifx_adsl_ceoc_thread_exit);
#endif
	MEI_INIT_WAKELIST("arceoc",wait_queue_hdlc_poll);
	kernel_thread(IFX_ADSL_CEOC_Poll , NULL, CLONE_FS | CLONE_FILES | CLONE_SIGNAL);
	return 0;
}


/**
 * \brief Clear Eoc  Module cleanup 
 * This function clean up the Clear Eoc module.
 * \return	0 success else fail
 * \ingroup	Internal
 */ 
int IFX_ADSL_CEOC_ModuleCleanup(void)
{
	stop_ceoc_module = 1;			
        MEI_WAKEUP_EVENT(wait_queue_hdlc_poll); //wake up and clean led module
        wait_for_completion(&ifx_adsl_ceoc_thread_exit);
        return 0;
}

EXPORT_SYMBOL(IFX_ADSL_CEOC_IRQHandler);
EXPORT_SYMBOL(IFX_ADSL_CEOC_Ioctl);
EXPORT_SYMBOL(IFX_ADSL_CEOC_Init);
EXPORT_SYMBOL(IFX_ADSL_CEOC_ModuleInit);
EXPORT_SYMBOL(IFX_ADSL_CEOC_ModuleCleanup);


EXPORT_SYMBOL(IFX_ADSL_CEOC_RXCBRegister);
EXPORT_SYMBOL(IFX_ADSL_CEOC_RXCBUnregister);
