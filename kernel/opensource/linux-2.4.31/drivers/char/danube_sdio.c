/* ============================================================================
 * Copyright (C) 2004 - Infineon Technologies.
 *
 * All rights reserved.
 * ============================================================================
 *
 * ============================================================================
 *
 * This document contains proprietary information belonging to Infineon
 * Technologies. Passing on and copying of this document, and communication
 * of its contents is not permitted without prior written authorisation.
 *
 * ============================================================================
 *
 * Revision History:
 *
 * ============================================================================
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/completion.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <linux/proc_fs.h>

#include <asm/danube/danube.h>
#include <asm/danube/danube_dma.h>
#include <asm/danube/irq.h>

#include <asm/danube/infineon_sdio_card.h>
#include <asm/danube/infineon_sdio.h>
#include <asm/danube/infineon_sdio_cmds.h>
#include <asm/danube/infineon_sdio_controller.h>
#include <asm/danube/danube_sdio_controller.h>
#include <asm/danube/danube_sdio_controller_registers.h>

#include <linux/delay.h>

#define DANUBE_SDIO_VERSION "0.50"

// MCLCMD 	GPIO 3 or 20
#define MCLCMD		20	
// MCLCLK	GPIO 0 or 19
#define MCLCLK		19
// MCLDATA0	GPIO 17 or 28
#define MCLDATA0	17
// MCLDATA1	GPIO 18 or 27
#define MCLDATA1	18
// MCLDATA2	GPIO 16 or 26
#define MCLDATA2	16
// MCLDATA3	GPIO 15 or 25
#define MCLDATA3	15

#define DMA_CLASS 0x0
//#define DANUBE_SDIO_DEBUG

#undef DANUBE_SDIO_DMSG
#ifdef DANUBE_SDIO_DEBUG
#define DANUBE_SDIO_DMSG(fmt, args...) printk( KERN_INFO  "%s: " fmt,__FUNCTION__, ## args)
#else
#define DANUBE_SDIO_DMSG(fmt, args...) 
#endif

#define DANUBE_SDIO_EMSG(fmt, args...) printk( KERN_ERR  "%s: " fmt,__FUNCTION__, ## args)

MODULE_DESCRIPTION("DANUBE sdio controller module");
MODULE_AUTHOR("tc.chen (tai-cheng.chen@infineon.com)");

#define SD_BLOCK_SIZE 512

static int danube_sd_controller_send_data(struct sd_controller *pDev,sdio_card_t *pCard, sd_block_request_t *pRequest, uint32_t timeout);
static int danube_sd_controller_read_data_pre(struct sd_controller *pDev, sdio_card_t *pCard,sd_block_request_t *pRequest);
static int danube_sd_controller_read_data(struct sd_controller *pDev, sdio_card_t *pCard,sd_block_request_t *pRequest, uint32_t timeout);
static int danube_sd_controller_send_cmd(struct sd_controller *pDev,struct sd_cmd *pCmd);
static void danube_sd_controller_sdio_mask_and_ack_irq(void);
static int danube_sd_controller_set_ops(int type,uint32_t data);
static int setup_dma_driver(danube_sd_controller_priv_t *priv);
static int cleanup_dma_driver(danube_sd_controller_priv_t *priv);

static struct completion comp_danube_sd_controller_cmd;
static wait_queue_head_t wait_queue_danube_sd_controller_cmd;
static wait_queue_head_t wait_queue_danube_sd_controller_data_cmd;
static struct completion comp_danube_sd_controller_data_cmd;
static sd_block_request_t *data_request=NULL;
static int cmd_access = 0,data_access=0;
#define DANUBE_SDIO_MAJOR 201
static int major = DANUBE_SDIO_MAJOR;
static uint32_t decode_taac(uint8_t taac);

static sdio_controller_t danube_sd_controller =
    {
        .name=DEVICE_NAME,
        .send_data	= danube_sd_controller_send_data,
        .read_data_pre  = danube_sd_controller_read_data_pre,
        .read_data	= danube_sd_controller_read_data,
        .send_cmd	= danube_sd_controller_send_cmd,
        .mask_ack_irq	= danube_sd_controller_sdio_mask_and_ack_irq,
        .set_ops	= danube_sd_controller_set_ops,
        //.VDD		= 0xffff00,
        .VDD		= (BIT20 | BIT21), //onlys support 3.3V
        .blklen		= 9,
       };

static int danube_sdio_ioctl(struct inode * ino, struct file * fil, unsigned int command, unsigned long lon);

static struct file_operations danube_sdio_operations = {
	ioctl:          danube_sdio_ioctl,
};


#define DANUBE_SDIO_PROC_DIRNAME     "danube_sdio"
#define PROC_ITEMS 12

static struct proc_dir_entry *danube_sdio_dir;

static int danube_sdio_proc_read(struct file * file, char * buf, size_t nbytes, loff_t *ppos);

static struct file_operations proc_operations = {
	read:	danube_sdio_proc_read,
};

typedef struct reg_entry {
	int * flag;
	char name[30];          // big enough to hold names
	char description[100];      // big enough to hold description
	unsigned short low_ino;
} reg_entry_t;
uint32_t cmd_response_counter=0;
uint32_t cmd_counter=0;
uint32_t cmd_error_counter=0;
uint32_t data_read_counter=0;
uint32_t data_read_total_size=0;
uint32_t data_write_counter=0;
uint32_t data_write_total_size=0;
uint32_t data_error_counter=0;

static reg_entry_t regs[PROC_ITEMS];       // total items to be monitored by /proc/mei

#define NUM_OF_REG_ENTRY	(sizeof(regs)/sizeof(reg_entry_t))

#define DANUBE_SD_CONTROLLER_WRITEL(data,addr)      do{ *((volatile u32*)(addr)) = (u32)(data); asm("SYNC");} while (0)
#define DANUBE_SD_CONTROLLER_READL(addr)    (*((volatile u32*)(addr)))

/**
    Send several blocks data to sd card
\param pDev	ptr to actual SD Controller
\param pCard	ptr to card device
\param pRequest	ptr to the request command
\return 	IFX_SUCCESS or IFX_ERROR if send command fail
\remark		Send a block data to sd card
*/
static int danube_sd_controller_send_data(struct sd_controller *pDev, sdio_card_t *pCard, sd_block_request_t *pRequest,uint32_t timeout)
{
	danube_sd_controller_priv_t *priv=pDev->priv;
	struct dma_device_info* dma_dev=priv->dma_device;
	uint32_t reg=0;
	uint32_t data_ctrl_reg;
	uint8_t *pData=NULL;
	block_data_t *blk_data;
	int need_to_alloc = 0;
	uint32_t clk_timeout;
	
#if 0
	if (pDev->card_transfer_state != pCard)
	{
		return ERROR_WRONG_CARD_STATE;
	}
#endif
	data_write_counter++;
	data_request = pRequest;
	dma_dev->current_tx_chan=0;
	data_access = 1;
	DANUBE_SD_CONTROLLER_WRITEL(pRequest->request_size,MCI_DLGTH);
    	data_ctrl_reg= (pRequest->block_size_shift << 4) |0x9;//blksize,dir=ctrl to card,dma enable 
    	DANUBE_SD_CONTROLLER_WRITEL(DANUBE_SD_CONTROLLER_READL(MCI_IM0)|MCI_IM_DTO | MCI_IM_DBE | MCI_IM_DCF,MCI_IM0);
    	if (pCard)
    	{
		clk_timeout = pCard->csd.nsac*100*100 + (uint32_t)((decode_taac(pCard->csd.taac)/10)* (danube_sd_controller.current_speed/10000)/100);   
		clk_timeout = clk_timeout << pCard->csd.r2w_factor;
	}
	else
	{
		clk_timeout = 0xFFFFFFFF;
	}
	DANUBE_SD_CONTROLLER_WRITEL(clk_timeout ,MCI_DTIM);
    	DANUBE_SD_CONTROLLER_WRITEL(data_ctrl_reg,MCI_DCTRL);
	blk_data = pRequest->blocks;
	pData = pRequest->blocks->data;
	
	//make sure the address is continuous
	while(pRequest->blocks)
	{
		if (pRequest->blocks->next)
		{
			if (pRequest->blocks->data+(1<< pRequest->block_size_shift)!=pRequest->blocks->next->data)
			{
				need_to_alloc = 1;
			} 
		}
    		pRequest->blocks = pRequest->blocks->next;  		
	}
	if (need_to_alloc)
	{
		int idx=0;
		char *pData_copy;
		pData_copy = kmalloc(pRequest->request_size, GFP_ATOMIC);
		if (pData_copy == NULL)
		{
			data_error_counter++;
			return -ENOMEM;
		}
		while(blk_data)
		{
			memcpy(pData_copy+idx,blk_data->data,(1<< pRequest->block_size_shift));
			idx+=(1<< pRequest->block_size_shift);
			blk_data = blk_data->next;
		}
		kfree(pData);
		pData=pData_copy;
	}
       	if (dma_device_write(dma_dev,pData, pRequest->request_size, NULL)!=pRequest->request_size)
	{
		printk("buffer full\n");
		pRequest->error = -EBUSY;
		data_error_counter++;
		goto send_data_end;
	}

	data_write_total_size+=pRequest->request_size;
	if (data_access)
    		interruptible_sleep_on_timeout(&wait_queue_danube_sd_controller_data_cmd, timeout); 
	if (data_access != 0)
	{
		DANUBE_SDIO_EMSG("data access timeout\n");
		pRequest->error = ERROR_TIMEOUT;
		data_access = 0;
	}
	if (pRequest->error)
	{
		printk("Error:pRequest->error=%d\n",pRequest->error);
		goto send_data_end;
    	}
send_data_end:
    data_request = NULL;
    reg = DANUBE_SD_CONTROLLER_READL(MCI_IM0);
    reg &= ~(MCI_STAT_DTO | MCI_STAT_DBE );
    DANUBE_SD_CONTROLLER_WRITEL(reg,MCI_IM0);
    return pRequest->error;
}

/**
    Read several blocks data from sd card
\param pDev	ptr to actual SD Controller
\param pCard	ptr to card device
\param pRequest	ptr to the request command
\return - IFX_SUCCESS 
	- IFX_ERROR if an error occured
\remark	Read a block data to sd card
 */
static int danube_sd_controller_read_data_pre(struct sd_controller *pDev,sdio_card_t *pCard,sd_block_request_t *pRequest)
{
    uint32_t data_ctrl_reg;

#if 0
    if (pDev->card_transfer_state != pCard)
    {
        return ERROR_WRONG_CARD_STATE;
    }
#endif

    DANUBE_SDIO_DMSG("pRequest->nBlocks=%d,pRequest->request_size=%d,block_size_shift=%d\n",pRequest->nBlocks,pRequest->request_size,pRequest->block_size_shift); 
    data_request = pRequest;

// dma
     DANUBE_SD_CONTROLLER_WRITEL(0xFFFFffff ,MCI_DTIM); // need to check
     DANUBE_SD_CONTROLLER_WRITEL(DANUBE_SD_CONTROLLER_READL(MCI_IM0)|MCI_IM_DTO | MCI_STAT_DCF |MCI_IM_SBE ,MCI_IM0);
     data_ctrl_reg= (pRequest->block_size_shift << 4) |0xB;//blksize,dir=card to ctrl,dma enable 

     DANUBE_SD_CONTROLLER_WRITEL((1<<pRequest->block_size_shift)*pRequest->nBlocks,MCI_DLGTH);
     DANUBE_SDIO_DMSG("DLENGTH=%d\n",DANUBE_SD_CONTROLLER_READL(MCI_DLGTH));

    data_access = 1;
    DANUBE_SD_CONTROLLER_WRITEL(data_ctrl_reg,MCI_DCTRL);
	
    return OK;
}

static uint32_t decode_taac(uint8_t taac)
{
	uint32_t ret = 0;
	static const unsigned int tacc_exp[] = {
	1,	10,	100,	1000,	10000,	100000,	1000000, 10000000,
	};

	static const unsigned int tacc_mant[] = {
	0,	10,	12,	13,	15,	20,	25,	30,
	35,	40,	45,	50,	55,	60,	70,	80,
	};

	ret	 = (tacc_exp[taac&0x7] * tacc_mant[(taac>>3)&0xF] + 9) / 10;
	return ret;
}

static int danube_sd_controller_read_data(struct sd_controller *pDev,sdio_card_t *pCard,sd_block_request_t *pRequest, uint32_t timeout)
{
    uint32_t reg=0;
    uint32_t clk_timeout = 0;
    uint32_t data_size = pRequest->request_size;

    DANUBE_SDIO_DMSG("Data reading,total length=%d!\n",pRequest->request_size);
#if 0
    if (pDev->card_transfer_state != pCard)
    {
        return ERROR_WRONG_CARD_STATE;
    }
#endif
    if (pCard)
    {
    	clk_timeout = pCard->csd.nsac*100*100 + (uint32_t)((decode_taac(pCard->csd.taac)/10)* (danube_sd_controller.current_speed/10000)/100);   
    }else{
	clk_timeout = 0xFFFFFFFF;
    }
   
    DANUBE_SD_CONTROLLER_WRITEL(clk_timeout ,MCI_DTIM);
    data_read_counter++;
    
    if (data_access == 1)
    {
    	// wait for reading
    	interruptible_sleep_on_timeout(&wait_queue_danube_sd_controller_data_cmd, timeout); 

    }
    // read finish
    reg = DANUBE_SD_CONTROLLER_READL(MCI_IM0);
    reg &= ~(MCI_IM_DTO | MCI_STAT_DCF);
    DANUBE_SD_CONTROLLER_WRITEL(reg,MCI_IM0);
    if (data_access != 0)
    {
    	data_request = NULL;
	DANUBE_SDIO_EMSG("Data read timeout!\n");
	data_access = 0;
	data_error_counter++;
	return ERROR_TIMEOUT;
    }else
    {
    	data_read_total_size+=data_size;
    }
    data_request = NULL;
    return pRequest->error;
}

/**
    Send a command to sd card
\param pDev	ptr to actual SD Controller
\param pCmd	ptr to the request command
\return - IFX_SUCCESS 
	- IFX_ERROR if an error occured
\remark	Send a command to sd card
 */
static int danube_sd_controller_send_cmd(struct sd_controller *pDev,struct sd_cmd *pCmd)
{
    uint32_t sd_cmd=0;
    uint32_t reg =0;
    int nRetry=0;
    
send_retry:

    DANUBE_SDIO_DMSG("Sending command :op_code=%d,type=0x%X args=0x%X\n",pCmd->op_code,pCmd->response_type,pCmd->args);
   
    DANUBE_SD_CONTROLLER_WRITEL(pCmd->args,MCI_ARG);
    sd_cmd = pCmd->op_code;
    switch(pCmd->response_type)
    {
    case SD_RSP_R1:
    case SD_RSP_R1b:
    case SD_RSP_R3:
    case SD_RSP_R4:
    case SD_RSP_R5:
    case SD_RSP_R6:
        sd_cmd |= MCI_CMD_SHORT_RSP;
        DANUBE_SD_CONTROLLER_WRITEL(DANUBE_SD_CONTROLLER_READL(MCI_IM0)|MCI_IM_CRE|MCI_IM_CTO|MCI_IM_CCF,MCI_IM0);
        break;
    case SD_RSP_R2:
        sd_cmd |= MCI_CMD_LONG_RSP;
        DANUBE_SD_CONTROLLER_WRITEL(DANUBE_SD_CONTROLLER_READL(MCI_IM0)|MCI_IM_CRE|MCI_IM_CTO|MCI_IM_CCF,MCI_IM0);
        break;
    default:
        DANUBE_SD_CONTROLLER_WRITEL(DANUBE_SD_CONTROLLER_READL(MCI_IM0)|MCI_IM_CS,MCI_IM0);
        break;
    }
    pDev->cmd = pCmd;

    pDev->cmd->error = OK;
    sd_cmd |= MCI_CMD_EN ;

    cmd_access = 1;

    DANUBE_SD_CONTROLLER_WRITEL(sd_cmd,MCI_CMD);
    cmd_counter ++;
    
    interruptible_sleep_on_timeout(&wait_queue_danube_sd_controller_cmd, 100); 	

    reg = DANUBE_SD_CONTROLLER_READL(MCI_IM0);
    reg &= ~(MCI_IM_CRE|MCI_IM_CTO|MCI_IM_CCF|MCI_IM_CS);
    DANUBE_SD_CONTROLLER_WRITEL(reg,MCI_IM0);

    if (cmd_access != 0)
    {
	DANUBE_SDIO_DMSG("cmd access timeout\n");
	pDev->cmd->error = ERROR_TIMEOUT;
    	cmd_access = 0;
    	nRetry++;
    	if (nRetry<2)
    		goto send_retry;
    }

    return pDev->cmd->error;
}


/**
    SD Controller interrupt handle
\param pDev	ptr to actual SD Controller
\return - IFX_SUCCESS 
	- IFX_ERROR if an error occured
\remark	SD Controller interrupt handle
 */
static int danube_sd_controller_handle_int(struct sd_controller *pDev)
{
    u32 status,irq_done=0;
    struct sd_cmd *cmd = pDev->cmd;
    u32 reg_im0=0;
    
    status = DANUBE_SD_CONTROLLER_READL(MCI_STAT);
    DANUBE_SDIO_DMSG("Got interrupt, STATUS=%08X\n",status);
    reg_im0 = DANUBE_SD_CONTROLLER_READL(MCI_IM0);
    irq_done = status & (~reg_im0);
    status &= reg_im0;

	irq_done=0;
	
	//Receive Data Available
	//Data available in receive FIFO.
	if (status & MCI_STAT_RXDA) 
	{
		irq_done |= MCI_STAT_RXDA;
	}
	
	//Transmit Data Available
	//Data available in transmit FIFO.
	if (status & MCI_STAT_TXDA)
	{
		irq_done |= MCI_STAT_TXDA;
	}
	
	//Receive FIFO Empty
	//Returns the Receive FIFO status.
	if (status & MCI_STAT_RXFE)
	{
		irq_done |= MCI_STAT_RXFE;
		printk("MCI_STAT_RXFE\n");
	}
	
	//Transmit FIFO Empty
	//Returns the Receive FIFO status.
	if (status & MCI_STAT_TXFE)
	{
		irq_done |= MCI_STAT_TXFE;
	}
	
	//Receive FIFO Full
	//Returns the Receive FIFO status.
	if (status & MCI_STAT_RXFF)
	{
		irq_done |= MCI_STAT_RXFF;
		printk("MCI_STAT_RXFF\n");
	}
	
	//Transmit FIFO Full
	//Returns the Transmit FIFO status.
	if (status & MCI_STAT_TXFF)
	{
		irq_done |= MCI_STAT_TXFF;
	}
	
	//Receive FIFO half full
	//Returns the FIFO status.
	if (status & MCI_STAT_RXHF)
	{
		irq_done |= MCI_STAT_RXHF;
		printk("MCI_STAT_RXHF\n");
	}
	
	//Transmit FIFO half empty
	//Returns the FIFO status.
	if (status & MCI_STAT_TXHF)
	{
		irq_done |= MCI_STAT_TXHF;
	}
	
	//Receive Active
	//Data receive in progress.
	if (status & MCI_STAT_RXA)
	{
		printk("MCI_STAT_RXA\n");
		irq_done |= MCI_STAT_RXA;
	}
	
	//Transmit Active
	//Data transmit in progress.
	if (status & MCI_STAT_TXA)
	{
		irq_done |= MCI_STAT_TXA;
	}
	
	//Command Active
	//Command transfer in progress.
	if (status & MCI_STAT_CMDA)
	{
		irq_done |= MCI_STAT_CMDA;
	}
	
	//Data Block End
	//Data block sent. CRC check passed.
	if (status & MCI_STAT_DBE)
	{
		irq_done |= MCI_STAT_DBE;
		//DANUBE_SDIO_EMSG("MCI_STAT_DBE\n");
		if (DANUBE_SD_CONTROLLER_READL(MCI_DCNT) ==0)
		{
			data_access = 0;
			//DANUBE_SDIO_EMSG("MCI_STAT_DBE,wake up!\n");
			wake_up_interruptible(&wait_queue_danube_sd_controller_data_cmd);
		}
	}
	
	//Start Bit Error		
	//Start bit not detected on all data signals in wide bus mode.
	if (status & MCI_STAT_SBE)
	{
		irq_done |= MCI_STAT_SBE;
		DANUBE_SDIO_EMSG("Error:MCI_STAT_SBE\n");
		data_request->error = ERROR_CRC_ERROR;
		wake_up_interruptible(&wait_queue_danube_sd_controller_data_cmd);
	}
	
	//Data End
	//Data end indication means that the data counter is zero.
	if (status & MCI_STAT_DE)
	{
		irq_done |= MCI_STAT_DE;
	}
	
	//Command Sent
	//Command was sent. No response required.
	if (status & MCI_STAT_CS)
	{
		if (cmd->response_type == SD_RSP_NONE)
		{
			cmd_access = 0;
			wake_up_interruptible(&wait_queue_danube_sd_controller_cmd);			
		}
		irq_done |= MCI_STAT_CS;
	}
	
	//Command Response End
	//Command response received. CRC check passed.
	if (status & MCI_STAT_CRE ) 
	{
		if (DANUBE_SD_CONTROLLER_READL(MCI_REPCMD) == pDev->cmd->op_code)
		{
			
			switch ( cmd->response_type )
			{
			case SD_RSP_R1:
			case SD_RSP_R1b:
			case SD_RSP_R3:
			case SD_RSP_R4:
			case SD_RSP_R5:
			case SD_RSP_R6:
				cmd->response[0] = DANUBE_SD_CONTROLLER_READL(MCI_REP0);
				cmd_response_counter++;
				DANUBE_SDIO_DMSG("got interrupt,resp=%08X\n",cmd->response[0]);
			break;
			default:
				cmd->error = ERROR_WRONG_RESPONSE_TYPE;
			break;
			}
			cmd_access = 0;
			wake_up_interruptible(&wait_queue_danube_sd_controller_cmd);
			
		}
		else
		{
			if (DANUBE_SD_CONTROLLER_READL(MCI_REPCMD)==0x3F && cmd->response_type == SD_RSP_R2) // Response CMD of Response R2 is 3F
			{
				
				cmd->response[0] = DANUBE_SD_CONTROLLER_READL(MCI_REP0);
				cmd->response[1] = DANUBE_SD_CONTROLLER_READL(MCI_REP1);
				cmd->response[2] = DANUBE_SD_CONTROLLER_READL(MCI_REP2);
				cmd->response[3] = DANUBE_SD_CONTROLLER_READL(MCI_REP3);
				cmd_access = 0;
				cmd_response_counter++;
				DANUBE_SDIO_DMSG("got interrupt,resp[0]=0x%8X,resp[1]=0x%08X,resp[2]=0x%08X,resp[3]=0x%08X\n",cmd->response[0],cmd->response[1],cmd->response[2],cmd->response[3]);
				wake_up_interruptible(&wait_queue_danube_sd_controller_cmd);
			}else
			{
				//break;
				cmd->error = ERROR_WRONG_RESPONSE_CMD;
			}
		}
		irq_done |= MCI_STAT_CRE;	
	}
	
	//Receive Overrun
	//Receive FIFO overrun error.
	if (status & MCI_STAT_RO)
	{
		irq_done |= MCI_STAT_RO;
		data_request->error = ERROR_DATA_ERROR;
	}
	
	//Transmit Underrun
	//Transmit FIFO underrun error.
	if (status & MCI_STAT_TU)
	{
		irq_done |= MCI_STAT_TU;
		data_request->error = ERROR_DATA_ERROR;
	}
	
	//Data Time-out
	//Data time-out indication.
	if (status & MCI_STAT_DTO) // data timeout
	{
		DANUBE_SDIO_EMSG("Data Timeout.\n");
		data_request->error = ERROR_TIMEOUT;
		wake_up_interruptible(&wait_queue_danube_sd_controller_data_cmd);
		irq_done |= MCI_STAT_DTO;
	}
	
	//Command Time-out
	//Command time-out indication.
	if (status & MCI_STAT_CTO ) // command time-out | command crc fail
	{
		DANUBE_SDIO_DMSG("Command time-out | command crc fai.\n");
		cmd->error = ERROR_TIMEOUT;
		irq_done |= MCI_STAT_CTO;
	}
	
	//Data CRC Fail
	//Data block send/received CRC check failed.
	if (status & MCI_STAT_DCF)
	{
		irq_done |= MCI_STAT_DCF;
		DANUBE_SDIO_EMSG("ERROR_CRC_ERROR! MCI_STAT_DCF\n");
		data_request->error = ERROR_CRC_ERROR;
	}
	
	//Command CRC Fail
	//Command response received, CRC check failed.
	if (status & MCI_STAT_CCF)
	{
		if (cmd->response_type == SD_RSP_R3 && DANUBE_SD_CONTROLLER_READL(MCI_REPCMD)==0x3F) // Response R3 CRC is always 7F, Response CMD is always 3F
		{
			cmd->response[0] = DANUBE_SD_CONTROLLER_READL(MCI_REP0);
			DANUBE_SDIO_DMSG("got interrupt,resp=%08X\n",cmd->response[0]);
		}else
		{
			DANUBE_SDIO_EMSG("Command response received.Crc check failed!\n");
			cmd->error = ERROR_CRC_ERROR;
		}
		cmd_access = 0;
		wake_up_interruptible(&wait_queue_danube_sd_controller_cmd);
		irq_done |= MCI_STAT_CCF;
	}
	
	//DANUBE_SD_CONTROLLER_WRITEL(irq_done,MCI_CL);
	DANUBE_SD_CONTROLLER_WRITEL(status,MCI_CL);
	if (cmd->error != OK || (data_request && data_request->error != OK))
	{
		//DANUBE_SDIO_EMSG("Error!!,cmd->error=%d,data_request=%d\n",cmd->error,data_request?data_request->error:999);
		goto ERROR;
	}
    //status &= ~(irq_done);

    //DANUBE_SD_CONTROLLER_WRITEL(DANUBE_SD_CONTROLLER_READL(SDIO_ICR)|SDIO_ICR_INTR0,SDIO_ICR);
    return OK;
ERROR:    
    //DANUBE_SD_CONTROLLER_WRITEL(DANUBE_SD_CONTROLLER_READL(SDIO_ICR)|SDIO_ICR_INTR0,SDIO_ICR);
    if (cmd->error != OK)
    {
    	cmd_error_counter++;
    	return cmd->error;
    }
    if (data_request && data_request->error != OK)
    {
    	data_error_counter++;
    	return data_request->error;
    }
    return -1;
}

/**
    SDIO interrupt handle
\param irq irq number
\param dev_id ptr to the Device 
\param pt_regs ptr to the registers
\return - IFX_SUCCESS 
	- IFX_ERROR if an error occured
\remark	SD Controller interrupt handle
 */
static void danube_sd_controller_handle_sdio_int(int irq, void *dev_id, struct pt_regs *regs)
{
    uint32_t imc=0;
    unsigned long flags;
    struct sd_controller *pDev = (struct sd_controller *) dev_id;
    u32 status = DANUBE_SD_CONTROLLER_READL(SDIO_MIS);
    
    DANUBE_SDIO_DMSG("got sdio interrupt,irq=%d.\n",irq);
    save_flags(flags);
    cli();
    if (status & SDIO_MIS_SDIO)
    {
        // disable sdio interrupt until sdio inturrupt finish
        imc = DANUBE_SD_CONTROLLER_READL(SDIO_IMC);
        imc &= ~(SDIO_IMC_SDIO);
        DANUBE_SD_CONTROLLER_WRITEL(imc,SDIO_IMC);
#ifdef SDIO_SUPPORT
        sd_core_sdio_int(pDev);
#endif        
        DANUBE_SD_CONTROLLER_WRITEL(DANUBE_SD_CONTROLLER_READL(SDIO_ICR)|SDIO_ICR_INTR0,SDIO_ICR);
    }
    restore_flags(flags);

}

/**
    Mask and ACK SDIO interrupt
\return - IFX_SUCCESS 
	- IFX_ERROR if an error occured
\remark	    Mask and ACK SDIO interrupt
 */
static void danube_sd_controller_sdio_mask_and_ack_irq(void)
{
    DANUBE_SD_CONTROLLER_WRITEL(DANUBE_SD_CONTROLLER_READL(SDIO_ICR)|SDIO_ICR_SDIO,SDIO_IMC);
}

/**
    SD interrupt handle
\param irq irq number
\param dev_id ptr to the Device 
\param pt_regs ptr to the registers
\return - IFX_SUCCESS 
	- IFX_ERROR if an error occured
\remark	SD Controller interrupt handle
 */
static void danube_sd_controller_irq(int irq, void *dev_id, struct pt_regs *regs)
{
    struct sd_controller *pDev = (struct sd_controller *) dev_id;
    unsigned long flags;

    save_flags(flags);
    cli();

    danube_sd_controller_handle_int( pDev);
    restore_flags(flags);
}

/**
   Chagne the VDD or FREQUENCY 
\param type type of operation
\param data data to update
\return - IFX_SUCCESS 
	- IFX_ERROR if an error occured
\remark	SD Controller interrupt handle
 */
static int danube_sd_controller_set_ops(int type,uint32_t data)
{
    uint32_t reg=0;
    uint32_t div=0;
    switch (type)
    {
    case SD_SET_VDD:
        break;
    case SD_SET_FREQENCY:
        reg = DANUBE_SD_CONTROLLER_READL(MCI_CLK);
	reg &= ~(MCI_CLK_BY | 0xFF); //BYPASS, CLK_DIV
	if (data == 0)
	{
	    DANUBE_SDIO_DMSG("Warning! The clock is 0.\n");
	    reg &= ~(MCI_CLK_EN);
            DANUBE_SD_CONTROLLER_WRITEL(reg,MCI_CLK);
            danube_sd_controller.current_speed = 0;
	    break;
	}

	if (data >= ((danube_sd_controller_priv_t *)danube_sd_controller.priv)->mclk_speed)
	{
	    if (data > ((danube_sd_controller_priv_t *)danube_sd_controller.priv)->mclk_speed)
	    	DANUBE_SDIO_EMSG("Error! Clock is too large. clock = %d, danube mclk_speed=%d\n",data,((danube_sd_controller_priv_t *)danube_sd_controller.priv)->mclk_speed);

		div = MCI_CLK_BY;
	}else
	{
		div = ((danube_sd_controller_priv_t *)danube_sd_controller.priv)->mclk_speed / (data * 2); 
		div -=1;
		// make sure the clock <= data
	        if ( (((danube_sd_controller_priv_t *)danube_sd_controller.priv)->mclk_speed  / ((div +1)*2)) > data)
		 	div++;
	}
	
	DANUBE_SDIO_DMSG("div=%d,mclk_speed=%d\n",div,((danube_sd_controller_priv_t *)(danube_sd_controller.priv))->mclk_speed);

	reg |= div;
	reg |= MCI_CLK_EN;
        DANUBE_SD_CONTROLLER_WRITEL(reg,MCI_CLK);
        if (div == MCI_CLK_BY)
        	danube_sd_controller.current_speed = ((danube_sd_controller_priv_t *)danube_sd_controller.priv)->mclk_speed;
        else
		danube_sd_controller.current_speed = ((danube_sd_controller_priv_t *)danube_sd_controller.priv)->mclk_speed / ((div + 1)*2);
	DANUBE_SDIO_DMSG("current_speed = %08X\n",danube_sd_controller.current_speed);
	
    case SD_SET_BUS_WIDTH:
    	reg = DANUBE_SD_CONTROLLER_READL(MCI_CLK);
    	switch(data)
    	{
    		case SD_BUS_4BITS:
    			reg |= MCI_CLK_WD;
    		break;
    		case SD_BUS_1BITS:
    			reg &= ~(MCI_CLK_WD);
    		break;
    		default:
            		return -EINVAL;
    	}
    	DANUBE_SD_CONTROLLER_WRITEL(reg,MCI_CLK);
    	DANUBE_SDIO_DMSG("MCI_CLK=%08X\n", DANUBE_SD_CONTROLLER_READL(MCI_CLK));
        break;
    case SD_SET_BLOCK_LEN:
    	if (data != danube_sd_controller.blklen)
    	{
    		danube_sd_controller_priv_t *priv;
    		priv = danube_sd_controller.priv;
		cleanup_dma_driver(priv);  
    		danube_sd_controller.blklen = data;
    		setup_dma_driver(priv);
    	}
    	break;
    }
    
    return OK;
}

static int danube_data_recv(struct dma_device_info* dma_dev)
{
    uint8_t* buf=NULL;
    int len=0,i=0;

    len=dma_device_read(dma_dev,&buf,NULL);
    DANUBE_SDIO_DMSG("danube_data_recv,len=%d\n",len);

    if ( !data_request )
    {
    	kfree(buf);
    	return OK;
    }

    while(i< len && data_request->blocks && data_request->request_size>0)
    {
    	memcpy(data_request->blocks->data,((uint8_t *)buf)+i,1 <<data_request->block_size_shift );
	i+=(1<<data_request->block_size_shift);;
    	data_request->request_size -= (1<<data_request->block_size_shift);;
    	data_request->blocks=data_request->blocks->next;
    }
    kfree(buf);
    if (data_request->request_size ==0 || data_request->blocks == NULL)
    {
	data_access = 0;
        wake_up_interruptible(&wait_queue_danube_sd_controller_data_cmd);
    }

    return OK;
}


static int dma_intr_handler(struct dma_device_info* dma_dev,int status)
{

    DANUBE_SDIO_DMSG("got dma interrupt!\n");
    switch(status)
    {
    case RCV_INT:
        danube_data_recv(dma_dev);
        break;
    case TX_BUF_FULL_INT:
	DANUBE_SDIO_EMSG("TX_BUF_FULL_INT\n");
        break;
    }
    return OK;
}



static u8* dma_buffer_alloc(int len, int* byte_offset,void** opt)
{
    u8* buffer=NULL;
    buffer = kmalloc(1<<danube_sd_controller.blklen,GFP_ATOMIC);

    if (buffer == NULL)
    {
        return NULL;
    }
    //*(int*)opt=(int)buffer;
    memset(buffer,0,1<<danube_sd_controller.blklen);
    *byte_offset=0;
    return buffer;
}

static int dma_buffer_free(u8* dataptr,void* opt)
{
    if(dataptr==NULL)
    {}
    else
    {
        //kfree(dataptr);
    }
    return OK;

}

static int danube_sdio_ioctl(struct inode * ino, struct file * fil, unsigned int command, unsigned long lon)
{
	struct sd_cmd sdio_ioctl_cmd={0};
	int ret = 0;
	sd_block_request_t sdio_ioctl_data_request;
	danube_sdio_ioctl_block_request user_data_request;
    	block_data_t blk_data;

	switch(command){
        case DANUBE_SDIO_SEND_CMD:
		copy_from_user((char *)&sdio_ioctl_cmd, (char *)lon, sizeof(struct sd_cmd));
		ret = danube_sd_controller_send_cmd(&danube_sd_controller,&sdio_ioctl_cmd);
		copy_to_user((char *)lon, (char *)&sdio_ioctl_cmd, sizeof(struct sd_cmd));
		break;
        case DANUBE_SDIO_READ_DATA:
		copy_from_user((char *)&user_data_request, (char *)lon, sizeof(danube_sdio_ioctl_block_request));
		sdio_ioctl_data_request.block_size_shift = user_data_request.block_length;
		sdio_ioctl_data_request.nBlocks = 1;
		sdio_ioctl_data_request.request_size=user_data_request.data_length;
		blk_data.data = user_data_request.data;
		blk_data.next = NULL;
    		sdio_ioctl_data_request.blocks = &blk_data;
	
		ret = danube_sd_controller_read_data_pre(&danube_sd_controller,NULL,&sdio_ioctl_data_request);
		ret = danube_sd_controller_send_cmd(&danube_sd_controller,&user_data_request.cmd);

		ret = danube_sd_controller_read_data(&danube_sd_controller,NULL, &sdio_ioctl_data_request, 1000);
		copy_to_user((char *)lon, (char *)&user_data_request, sizeof(danube_sdio_ioctl_block_request));
		break;
        case DANUBE_SDIO_SEND_DATA:
		copy_from_user((char *)&user_data_request, (char *)lon, sizeof(danube_sdio_ioctl_block_request));
		sdio_ioctl_data_request.block_size_shift = user_data_request.block_length;
		sdio_ioctl_data_request.nBlocks = 1;
		sdio_ioctl_data_request.request_size=user_data_request.data_length;
		blk_data.data = user_data_request.data;
		blk_data.next = NULL;
    		sdio_ioctl_data_request.blocks = &blk_data;
		ret = danube_sd_controller_send_cmd(&danube_sd_controller,&user_data_request.cmd);
		ret = danube_sd_controller_send_data(&danube_sd_controller, NULL , &sdio_ioctl_data_request,1000);
		break;
	case DANUBE_SDIO_SET_OPS_WBUS:
		if (lon == 0)
		{
	    	      ret = danube_sd_controller_set_ops(SD_SET_BUS_WIDTH,SD_BUS_1BITS);
		}else if (lon == 1)
		{
	    	      ret = danube_sd_controller_set_ops(SD_SET_BUS_WIDTH,SD_BUS_4BITS);
		}else
		{
			ret = -EINVAL;
		}
		break;
	case DANUBE_SDIO_SET_OPS_FREQUENCY:
    		ret = danube_sd_controller_set_ops(SD_SET_FREQENCY,lon);
		break;
	case DANUBE_SDIO_GET_OPS_WBUS:
		break;
	default:
		return -ENOIOCTLCMD;			
	}
	return ret;
}

static int danube_sdio_gpio_configure(void)
{
	uint32_t gpio_p0_dir,gpio_p1_dir;
	uint32_t gpio_p0_alt0,gpio_p1_alt0;
	uint32_t gpio_p0_alt1,gpio_p1_alt1;
	uint32_t gpio_p0_od,gpio_p1_od;
	uint32_t gpio_p0_out,gpio_p1_out;
	uint32_t gpio_p0_pudsel,gpio_p1_pudsel;
	uint32_t gpio_p0_puden,gpio_p1_puden;


	gpio_p0_out = *(DANUBE_GPIO_P0_IN);
	gpio_p1_out = *(DANUBE_GPIO_P1_IN);
	gpio_p0_dir = *(DANUBE_GPIO_P0_DIR);
	gpio_p1_dir = *(DANUBE_GPIO_P1_DIR);

	gpio_p0_alt0 = *(DANUBE_GPIO_P0_ALTSEL0);
	gpio_p1_alt0 = *(DANUBE_GPIO_P1_ALTSEL0);
	gpio_p0_alt1 = *(DANUBE_GPIO_P0_ALTSEL1);
	gpio_p1_alt1 = *(DANUBE_GPIO_P1_ALTSEL1);
	gpio_p0_od = *(DANUBE_GPIO_P0_OD);
	gpio_p1_od = *(DANUBE_GPIO_P1_OD);
	gpio_p0_pudsel = *(DANUBE_GPIO_P0_PUDSEL);
	gpio_p1_pudsel = *(DANUBE_GPIO_P1_PUDSEL);
	gpio_p0_puden = *(DANUBE_GPIO_P0_PUDEN);
	gpio_p1_puden = *(DANUBE_GPIO_P1_PUDEN);
	
	//////////////////////// MCLCLK//////////////////////////////
#if MCLCLK == 0	
	//GPIO0 (P0.0) : DIR=1, ALT0=0, ALT1=1, OUT=1, OD=1
	DANUBE_SDIO_DMSG("using GPIO 0 as MCLCLK.\n");
	gpio_p0_dir |=(BIT0);
	gpio_p0_alt0 &= ~(BIT0);
	gpio_p0_alt1 |= (BIT0);
	gpio_p0_od |= (BIT0);
	gpio_p0_pudsel |= (BIT0);
	gpio_p0_puden |= (BIT0);
#elif MCLCLK == 19
	//GPIO19 (P1.3) : DIR=1,ALT0=0,ALT1=1,OUT=1, OD=1
	DANUBE_SDIO_DMSG("using GPIO 19 as MCLCLK.\n");
	gpio_p1_dir   |=(BIT3);
	gpio_p1_alt0 &= ~(BIT3);
	gpio_p1_alt1 |= (BIT3);
	gpio_p1_od  |= (BIT3);
	gpio_p1_pudsel |= (BIT3);
	gpio_p1_puden |= (BIT3);
	
#else 
#error MCLCLK not defined
#endif	

	/////////////////////////MCLCMD////////////////////////////
#if MCLCMD == 3	
	//GPIO3 (P0.3): DIR=0,ALT0=0,ALT1=1,OUT=1, OD=1
	DANUBE_SDIO_DMSG("using GPIO 3 as MCLCMD.\n");
	//gpio_p0_dir &= ~(1<<3);
	gpio_p0_dir &= ~(1<<3);
	gpio_p0_alt0 &= ~(1<<3);
	gpio_p0_alt1 |= (1<<3);
	gpio_p0_od |= (1<<3);
	gpio_p0_pudsel |= (1<<3);
	gpio_p0_puden |= (1<<3);
#elif MCLCMD == 20
	//GPIO20 (P1.4): DIR=0,ALT0=0,ALT1=1,OUT=1, OD=1
	DANUBE_SDIO_DMSG("using GPIO 20 as MCLCMD.\n");
	gpio_p1_dir &= ~(1<<4);
	gpio_p1_alt0 &= ~(1<<4);
	gpio_p1_alt1 |= (1<<4);
	gpio_p1_od |= (1<<4);
	gpio_p1_pudsel |= (1<<4);
	gpio_p1_puden |= (1<<4);

#else 
#error MCLCMD not defined
#endif

	/////////////////////////DATA0/////////////////////////////
#if MCLDATA0 == 17
	//GPIO17 (P1.1): DIR=0,ALT0=0,ALT1=1,OUT=1, OD=1
	DANUBE_SDIO_DMSG("using GPIO 17 as DATA0.\n");
	gpio_p1_dir &= ~(1<<1);
	gpio_p1_alt0 &= ~(1<<1);
	gpio_p1_alt1 |= (1<<1);
	gpio_p1_od |= (1<<1);
	gpio_p1_pudsel |= (1<<1);
	gpio_p1_puden |= (1<<1);
#elif MCLDATA0 == 28	
	//GPIO28 (P1.12): DIR=0,ALT0=1,ALT1=1,OUT=1, OD=1
	DANUBE_SDIO_DMSG("using GPIO 28 as DATA0.\n");
	gpio_p1_dir &= ~(1<<12);
	gpio_p1_alt0 |= (1<<12);
	gpio_p1_alt1 |= (1<<12);
	gpio_p1_od |= (1<<12);
	gpio_p1_pudsel |= (1<<12);
	gpio_p1_puden |= (1<<12);
#else 
#error MCLDATA0 not defined	
#endif	
#if MCLDATA1 == 18	
	/////////////////////////DATA1/////////////////////////////
	//GPIO18 (P1.2): DIR=0,ALT0=0,ALT1=1,OUT=1, OD=1
	DANUBE_SDIO_DMSG("using GPIO 18 as DATA1.\n");

	gpio_p1_alt0 &= ~(1<<2);
	gpio_p1_alt1 |= (1<<2);
	gpio_p1_od |= (1<<2);
	gpio_p1_pudsel |= (1<<2);
	gpio_p1_puden |= (1<<2);
#elif MCLDATA1 == 27		
	//GPIO27 (P1.11): DIR=0,ALT0=0,ALT1=1,OUT=1, OD=1
	DANUBE_SDIO_DMSG("using GPIO 27 as DATA1.\n");
	gpio_p1_dir &= ~(1<<11);
	gpio_p1_alt0 &= ~(1<<11);
	gpio_p1_alt1 |= (1<<11);
	gpio_p1_od |= (1<<11);
	gpio_p1_pudsel |= (1<<11);
	gpio_p1_puden |= (1<<11);
#else 
#error MCLDATA1 not defined		
#endif	

	/////////////////////////DATA2/////////////////////////////
#if MCLDATA2 == 16		
	//GPIO16 (P1.0): DIR=0,ALT0=0,ALT1=1,OUT=1, OD=1
	DANUBE_SDIO_DMSG("using GPIO 16 as DATA2.\n");
	gpio_p1_dir  |= (1);
	gpio_p1_alt0 &= ~(1);
	gpio_p1_alt1 |= (1);
	gpio_p1_od |= (1);
	gpio_p1_pudsel |= (1);
	gpio_p1_puden |= ~(1);
#elif MCLDATA2 == 26	
	//GPIO26 (P1.10): DIR=0,ALT0=1,ALT1=1,OUT=1, OD=1
	DANUBE_SDIO_DMSG("using GPIO 26 as DATA2.\n");
	gpio_p1_dir  &= ~(1<<10);
	gpio_p1_alt0 |= (1<<10);
	gpio_p1_alt1 |= (1<<10);
	gpio_p1_od |= (1<<10);
	gpio_p1_pudsel |= (1<<10);
	gpio_p1_puden |= (1<<10);
#else 
#error MCLDATA2 not defined		
#endif	
#if MCLDATA3 == 15	
	/////////////////////////DATA3/////////////////////////////
	//GPIO15 (P0.15): DIR=0,ALT0=0,ALT1=1,OUT=1, OD=1
	DANUBE_SDIO_DMSG("using GPIO 15 as DATA3.\n");
	gpio_p0_alt0 &= ~(1<<15);
	gpio_p0_alt1 |= (1<<15);
	gpio_p0_od |= (1<<15);
	gpio_p0_pudsel |= (1<<15);
	gpio_p0_puden |= (1<<15);
#elif MCLDATA3 == 25		
	//GPIO25 (P1.9): DIR=0,ALT0=0,ALT1=1,OUT=1, OD=1
	DANUBE_SDIO_DMSG("using GPIO 25 as DATA3.\n");
	gpio_p1_dir &= ~(1<<9);
	gpio_p1_alt0 &= ~(1<<9);
	gpio_p1_alt1 |= (1<<9);
	gpio_p1_od |= (1<<9);
	gpio_p1_pudsel |= (1<<9);
	gpio_p1_puden |= (1<<9);

#else 
#error MCLDATA3 not defined		
#endif
 	*(DANUBE_GPIO_P0_DIR) =gpio_p0_dir;
  	*(DANUBE_GPIO_P1_DIR) =gpio_p1_dir;
    	*(DANUBE_GPIO_P0_ALTSEL0) = gpio_p0_alt0;
    	*(DANUBE_GPIO_P1_ALTSEL0) = gpio_p1_alt0;
    	*(DANUBE_GPIO_P0_ALTSEL1) = gpio_p0_alt1;
    	*(DANUBE_GPIO_P1_ALTSEL1) = gpio_p1_alt1;   
    	*(DANUBE_GPIO_P0_OD)		= gpio_p0_od;    
    	*(DANUBE_GPIO_P1_OD)		= gpio_p1_od;
   	*(DANUBE_GPIO_P0_OUT)		= gpio_p0_out;    
    	*(DANUBE_GPIO_P1_OUT)	= gpio_p1_out; 
   	*(DANUBE_GPIO_P0_PUDSEL)		= gpio_p0_pudsel;    
   	*(DANUBE_GPIO_P1_PUDSEL)		= gpio_p1_pudsel;    
   	*(DANUBE_GPIO_P0_PUDEN)		= gpio_p0_puden;    
   	*(DANUBE_GPIO_P1_PUDEN)		= gpio_p1_puden;    
   	return 0;
}

static int danube_sdio_proc_read(struct file * file, char * buf, size_t nbytes, loff_t *ppos)
{
        int i_ino = (file->f_dentry->d_inode)->i_ino;
	char outputbuf[128];
	int count=0;
	int i;
	reg_entry_t* current_reg=NULL;
	for (i=0;i<NUM_OF_REG_ENTRY;i++) {
		if (regs[i].low_ino==i_ino) {
			current_reg = &regs[i];
			break;
		}
	}
	if (current_reg==NULL)
		return -EINVAL;
	if (current_reg->flag == (int *) 1){
	///proc/danube_sdio/gpio
		if (*ppos>0) /* Assume reading completed in previous read*/
			return 0;               // indicates end of file
		count = sprintf(outputbuf, "\nFunction\tGPIO Number\n");
		count += sprintf(outputbuf+count, "Command:\t%d\nClock  :\t%d\nDATA 0 :\t%d\nDATA 1 :\t%d\nDATA 2 :\t%d\nDATA 3 :\t%d\n"
				,MCLCMD, MCLCLK, MCLDATA0, MCLDATA1, MCLDATA2, MCLDATA3);
		*ppos+=count;
	}
	else if (current_reg->flag == (int *) 2){
	///proc/danube_sdio/frequency
		if (*ppos>0) /* Assume reading completed in previous read*/
			return 0;               // indicates end of file
		count += sprintf(outputbuf+count, "Frequency: %d\n",danube_sd_controller.current_speed);
		*ppos+=count;
	}
	else if (current_reg->flag == (int *) 3){
		uint32_t reg;
		
		if (*ppos>0) /* Assume reading completed in previous read*/
			return 0;               // indicates end of fil

	    	reg = DANUBE_SD_CONTROLLER_READL(MCI_CLK);
	    	if (reg & MCI_CLK_WD)
	    	{
	    		count += sprintf(outputbuf+count, "4 Bits\n");
	    	}else
	    	{
	    		count += sprintf(outputbuf+count, "1 Bit\n");
	    	}
		*ppos+=count;
	}
	else{
        	if (*ppos>0) /* Assume reading completed in previous read*/
			return 0;               // indicates end of file
		count = sprintf(outputbuf, "0x%08X\n\n", *(current_reg->flag));
	        *ppos+=count;
	        if (count>nbytes)  /* Assume output can be read at one time */
			return -EINVAL;
	}
	
	
	if (copy_to_user(buf, outputbuf, count))
	{
		return -EFAULT;
	}
	return count;
}

static int setup_dma_driver(danube_sd_controller_priv_t *priv)
{
    int i=0;
    int ret;
	
    priv->dma_device = dma_device_reserve("SDIO");
    
    if(!priv->dma_device) return -ENOMEM;
    priv->dma_device->intr_handler=dma_intr_handler;
    priv->dma_device->buffer_alloc=dma_buffer_alloc;
    priv->dma_device->buffer_free=dma_buffer_free;
    priv->dma_device->num_rx_chan=1;/*turn on all the receive channels*/
    
    for(i=0;i<priv->dma_device->num_rx_chan;i++)
    {
      priv->dma_device->rx_chan[i]->packet_size=1<<danube_sd_controller.blklen;
      priv->dma_device->rx_chan[i]->control=DANUBE_DMA_CH_ON;     
    }
    ret=dma_device_register(priv->dma_device);
    for(i=0;i<priv->dma_device->num_rx_chan;i++)
    	priv->dma_device->rx_chan[i]->open(priv->dma_device->rx_chan[i]);
    return ret;    	
   
}
static int cleanup_dma_driver(danube_sd_controller_priv_t *priv)
{
    if (priv == NULL)
    	return -EINVAL;
    	
    if(priv->dma_device)
    {
        dma_device_unregister(priv->dma_device);
        kfree(priv->dma_device);
    }
    return 0;
}

static int danube_sdio_controller_init_module(void)
{
    uint32_t sdio_id=0;
    int ret=0,i=0;
    danube_sd_controller_priv_t *priv;
    struct proc_dir_entry *entry;
    reg_entry_t regs_temp[PROC_ITEMS] =                                     // Items being debugged
    {
        /*	{       flag,          name,          description } */
        { (int *)1, "gpio", "gpio number to used", 0 },
        { (int *)2, "frequency", "sd controller frequency", 0},
        { (int *)3, "bus_width", "sd controller data bus width", 0},
        { &cmd_counter, "cmd_counter", "command count", 0},
        { &cmd_response_counter, "cmd_response_counter", "command response count", 0},
        { &cmd_error_counter, "cmd_error_counter", "command error count", 0},
        { &data_read_counter, "data_read_counter", "data read command counter", 0},
        { &data_read_total_size, "data_read_total_size", "total data size of reading", 0},
        { &data_write_counter, "data_write_counter", "data write command counter", 0},
        { &data_write_total_size, "data_write_total_size", "total data size of writing", 0},
        { &data_error_counter, "data_error_counter", "data error count", 0},
        { &danube_sd_controller.blklen, "block_len", "block length", 0},

    };

    printk( "Danube_sdio_controller Version:%s\n",DANUBE_SDIO_VERSION );

    sdio_id = DANUBE_SD_CONTROLLER_READL(SDIO_ID);
    if (sdio_id != 0xF041C030)
    {
        SDIOERRMSG("Danube SDIO Controller not found!!\n");
        return -ENODEV;
    }
    init_waitqueue_head(&wait_queue_danube_sd_controller_cmd);
    //init_completion(&comp_danube_sd_controller_cmd);
    init_waitqueue_head(&wait_queue_danube_sd_controller_data_cmd);
    //init_completion(&comp_danube_sd_controller_data_cmd);

    danube_sd_controller.priv = kmalloc(sizeof(danube_sd_controller_priv_t), GFP_ATOMIC);
    if (danube_sd_controller.priv == NULL)
        return -ENOMEM;
    priv = danube_sd_controller.priv;
    
    memset(priv, 0, sizeof(danube_sd_controller_priv_t));
#if 0
    // setup dma
    priv->dma_device = dma_device_reserve("SDIO");
    
    if(!priv->dma_device) return -ENOMEM;
    priv->dma_device->intr_handler=dma_intr_handler;
    priv->dma_device->buffer_alloc=dma_buffer_alloc;
    priv->dma_device->buffer_free=dma_buffer_free;
    priv->dma_device->num_rx_chan=1;/*turn on all the receive channels*/
    
    for(i=0;i<priv->dma_device->num_rx_chan;i++)
    {
      priv->dma_device->rx_chan[i]->packet_size=1<<danube_sd_controller.blklen;
      priv->dma_device->rx_chan[i]->control=DANUBE_DMA_CH_ON;     
    }
    ret=dma_device_register(priv->dma_device);
    for(i=0;i<priv->dma_device->num_rx_chan;i++)
    	priv->dma_device->rx_chan[i]->open(priv->dma_device->rx_chan[i]);
#endif   
    setup_dma_driver(priv);
    
    // power on SDIO module
    *(DANUBE_PMU_PWDCR) &= ~(1<<DANUBE_PMU_SDIO_SHIFT);

    // reset sdio 
    //*(DANUBE_RCU_RST_REQ) |= (DANUBE_RCU_RST_REQ_SDIO);

    danube_sdio_gpio_configure();
    
    DANUBE_SD_CONTROLLER_WRITEL(0x400,SDIO_CLC); // 100MHz / 4 = 25MHz       
    priv->mclk_speed = 25000000; //25   MHz
    
    danube_sd_controller_set_ops(SD_SET_FREQENCY,0);
    mdelay(1);     
    danube_sd_controller_set_ops(SD_SET_FREQENCY,SD_CLK_400K);
    
    //DANUBE_SD_CONTROLLER_WRITEL(MCI_PWR_ON | MCI_PWR_VDD_33 | MCI_PWR_ROD ,MCI_PWR);// fix to 3.3 V | Power On mode
    DANUBE_SD_CONTROLLER_WRITEL(MCI_PWR_ON | MCI_PWR_VDD_33 ,MCI_PWR);// fix to 3.3 V | Power On mode

    // enable DMA transmit/receive path
    DANUBE_SD_CONTROLLER_WRITEL(SDIO_DMACON_TXON|SDIO_DMACON_RXON | DMA_CLASS,SDIO_DMACON);

    // clear all interrupt flags
    DANUBE_SD_CONTROLLER_WRITEL(0x7FF,MCI_CL); 

    DANUBE_SD_CONTROLLER_WRITEL(SDIO_IMC_SDIO|SDIO_IMC_INTR0 ,SDIO_ICR);

    ret = request_irq(DANUBE_SD_CONTROLLER_I_IRQ, danube_sd_controller_handle_sdio_int, SA_INTERRUPT, DEVICE_NAME, &danube_sd_controller);
    ret = request_irq(DANUBE_SD_CONTROLLER_INTR0_IRQ, danube_sd_controller_irq, SA_INTERRUPT, DEVICE_NAME, &danube_sd_controller);
    if (ret != 0)
    {
        SDIOERRMSG("request_irq fail! errno:%d\n",ret);
        return ret;
    }

    // disable MMC interrupt
    DANUBE_SD_CONTROLLER_WRITEL(0,MCI_IM0);
    
    //DANUBE_SD_CONTROLLER_WRITEL(SDIO_IMC_SDIO|SDIO_IMC_INTR0,SDIO_IMC);
    DANUBE_SD_CONTROLLER_WRITEL(SDIO_IMC_INTR0,SDIO_IMC);

    // enable SDIO
    DANUBE_SD_CONTROLLER_WRITEL(SDIO_CTRL_SDIOEN ,SDIO_CTRL);
    
    register_sd_controller(&danube_sd_controller);
#ifdef CONFIG_DEVFS_FS
    if (devfs_register_chrdev(major, "danube_sdio", &danube_sdio_operations)!=0) {
        DANUBE_SDIO_EMSG("\n\n unable to register major for danube_sdio!!!");
	return -1;
    }
#else
    if (register_chrdev(major, "danube_sdio", &danube_sdio_operations)!=0) {
        DANUBE_SDIO_EMSG("\n\n unable to register major for danube_sdio!!!");
	return -1;
    }
#endif    


    // procfs
    danube_sdio_dir=proc_mkdir(DANUBE_SDIO_PROC_DIRNAME, NULL);
    if ( danube_sdio_dir == NULL) {
	printk(KERN_ERR ": can't create /proc/" DANUBE_SDIO_PROC_DIRNAME "\n\n");
	return(-ENOMEM);
    }


    memcpy((char *)regs, (char *)regs_temp, sizeof(regs_temp));
    
    for(i=0;i<NUM_OF_REG_ENTRY;i++) {
	entry = create_proc_entry(regs[i].name,	S_IWUSR |S_IRUSR | S_IRGRP | S_IROTH, danube_sdio_dir);
	if(entry) {
		regs[i].low_ino = entry->low_ino;
		entry->proc_fops = &proc_operations;
	} else {
			printk( KERN_ERR 
				": can't create /proc/" DANUBE_SDIO_PROC_DIRNAME
				"/%s\n\n", regs[i].name);
			return(-ENOMEM);
	}
    }

    return 0;
}

static void danube_sdio_controller__exit_module(void)
{
    danube_sd_controller_priv_t *priv;
#ifdef CONFIG_PROC_FS
    int i=0;
#endif    

    SDIODBGMSG( "Module danube_sdio_controller exit\n" );
    
#ifdef CONFIG_PROC_FS
        for(i=0;i<NUM_OF_REG_ENTRY;i++)
		remove_proc_entry(regs[i].name, danube_sdio_dir);

        remove_proc_entry(DANUBE_SDIO_PROC_DIRNAME, &proc_root);
#endif //CONFIG_PROC_FS
    
    free_irq(DANUBE_SD_CONTROLLER_I_IRQ,NULL);
    free_irq(DANUBE_SD_CONTROLLER_INTR0_IRQ,NULL);
    unregister_sd_controller(&danube_sd_controller);


    priv = danube_sd_controller.priv;
#if 0    
    if(priv->dma_device)
    {
        dma_device_unregister(priv->dma_device);
        kfree(priv->dma_device);
    }
#endif
    cleanup_dma_driver(priv);    

    kfree(priv);
#ifdef CONFIG_DEVFS_FS
    devfs_unregister_chrdev(major,"danube_sdio");
#else    
    unregister_chrdev(major,"danube_sdio");
#endif    
}

module_init(danube_sdio_controller_init_module);
module_exit(danube_sdio_controller__exit_module);
