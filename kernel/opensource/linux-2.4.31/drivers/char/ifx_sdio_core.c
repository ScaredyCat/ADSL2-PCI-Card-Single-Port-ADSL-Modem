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
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

MODULE_DESCRIPTION("infineon sdio core module");
MODULE_AUTHOR("tc.chen (tai-cheng.chen@infineon.com)");

#include <asm/danube/infineon_sdio_card.h>
#include <asm/danube/infineon_sdio.h>
#include <asm/danube/infineon_sdio_cmds.h>
#include <asm/danube/infineon_sdio_controller.h>

#define IFX_SDIO_CORE_VERSION "0.50"

static int sd_send_acmd(struct sd_controller *dev,uint32_t op_code,uint32_t args,uint32_t response_type,struct sd_cmd *cmd,uint16_t rca);
static int sd_card_eject(sdio_card_t *card);

static LIST_HEAD(sdio_controllers);
static LIST_HEAD(sdio_card_drivers);

#ifdef SDIO_SUPPORT 
static int stop_sdio_irq_thread = 0;
static wait_queue_head_t wait_queue_sdio_irq;
static struct completion sdio_irq_thread_exit;
#endif //#ifdef SDIO_SUPPORT 

static struct completion sdio_init_thread_exit;
struct semaphore sd_data_sema,sd_cmd_sema;
static int acmd_access=0;
static wait_queue_head_t wait_queue_init_thread;
static int stop_sdio_init_thread;

#ifdef CONFIG_PROC_FS
static struct proc_dir_entry *infineon_sdio_dir;

typedef struct reg_entry {
	int * flag;
	char name[30];          // big enough to hold names
	char description[100];      // big enough to hold description
	unsigned short low_ino;
	sdio_card_t *card;
} reg_entry_t;

#define INFINEON_SDIO_PROC_DIRNAME     "infineon_sdio"
#define SDIO_PROC_ITEMS 2
static reg_entry_t regs_sdio[SDIO_PROC_ITEMS];       // total items to be monitored by /proc/mei
#define MAX_CARDS 2
static reg_entry_t regs_cards[MAX_CARDS][6];       // total items to be monitored by /proc/mei
#define NUM_OF_REG_CARD_ENTRY	(sizeof(regs_cards[0])/sizeof(reg_entry_t))
static struct proc_dir_entry *infineon_card_dir[MAX_CARDS];
#endif

// enable 4 bit data access
#define ENABLE_WIDE_BUS 

// enable frequency change
#define FREQUENCY_CHANGE
#undef SDIO_SUPPORT

static int sd_send_cmd(struct sd_controller *dev,uint32_t op_code,uint32_t args,uint32_t response_type,struct sd_cmd *cmd, int aquire_lock)
{
    int i=0,ret;
    cmd->op_code = op_code;
    cmd->args = args;
    cmd->response_type = response_type;

    // clear response
    cmd->error=0;
    for (i=0;i<4;i++)
        cmd->response[i]=0;

    if (aquire_lock)
    	down_interruptible(&dev->sd_cmd_sema);
    ret = dev->send_cmd(dev,cmd);
    if (aquire_lock)
    	up(&dev->sd_cmd_sema);
    return ret;
}

static int sd_send_acmd(struct sd_controller *dev,uint32_t op_code,uint32_t args,uint32_t response_type,struct sd_cmd *cmd,uint16_t rca)
{
    struct sd_cmd app_cmd=
        {
            0
        };
    int ret=OK;
    down_interruptible(&dev->sd_cmd_sema);
    ret = sd_send_cmd(dev,SD_CMD_APP_CMD,rca<<16,SD_RSP_R1,&app_cmd,0);
    if (ret != OK)
	goto end;
    if ( app_cmd.response[0]&R1_SD_CS_ERROR)
    {
    	SDIOERRMSG("%s %d: Got R1_SD_CS_ERROR,response=%08X\n",__FUNCTION__,__LINE__,app_cmd.response[0]);
        ret = app_cmd.response[0]&R1_SD_CS_ERROR;
	goto end;
    }
    ret = sd_send_cmd(dev,op_code,args,response_type,cmd,0);
end:
    up(&dev->sd_cmd_sema);
    return ret;
}

static int setup_width_bus(struct sd_controller *dev,sdio_card_t *card)
{
	int ret;
	struct sd_cmd cmd ={0};
      
	ret = sd_send_acmd(dev,SD_CMD_SET_BUS_WIDTH,2,SD_RSP_R1,&cmd,card->rca);//4bit
	if (ret == OK)
	{
		dev->set_ops(SD_SET_BUS_WIDTH,SD_BUS_4BITS);
	}else
	{
		SDIOERRMSG("update 4 bit fail.\n");
	}
	return ret;
}

static int sd_card_select(struct sd_controller *dev,sdio_card_t *card,uint32_t rca, int state)
{
    struct sd_cmd cmd =
        {
            0
        };
    int ret = OK;
    if ( (dev->card_transfer_state != card && state ==  SD_CS_STATE_TRAN) || (dev->card_transfer_state == card && state ==  SD_CS_STATE_STBY))
    {
	if (state == SD_CS_STATE_TRAN)
	{
		if (rca == 0)
		{
            		return -EINVAL;
		}
        ret = sd_send_cmd(dev,SD_CMD_SELECT_CARD,rca << 16,SD_RSP_R1b,&cmd,1);
	}
	else
	{
        	ret = sd_send_cmd(dev,SD_CMD_SELECT_CARD,0,SD_RSP_NONE,&cmd,1);
	}
	if (ret != OK)
	{
            	SDIOERRMSG("%s %d: Busy response=%08X\n",__FUNCTION__,__LINE__,cmd.response[0]);
            	return ret;
	}
	// read the status to make sure card select success	
	if (rca != 0)
	{
        ret = sd_send_cmd(dev,SD_CMD_SEND_STATUS,rca << 16,SD_RSP_R1,&cmd,1);
        if (ret == OK)
        {
            if ( ((cmd.response[0] >> SD_CS_STATE_SHIFT ) & SD_CS_STATE_MASK) == SD_CS_STATE_TRAN  )
	    {
		if ( state ==  SD_CS_STATE_TRAN )
		{
			dev->card_transfer_state = card;
			if ( (card->scr.sd_bus_width & SCR_SD_BUS_WIDTH_4BIT) == SCR_SD_BUS_WIDTH_4BIT)
			{
#ifdef ENABLE_WIDE_BUS
				ret = setup_width_bus(dev,card);
				if (ret != OK)
					SDIOERRMSG("Setup bus with 4 bits fail.\n");
#endif
			}
		}else
		{
			return ERROR_WRONG_CARD_STATE;
		}
	    }
            else if ( ((cmd.response[0] >> SD_CS_STATE_SHIFT ) & SD_CS_STATE_MASK) == SD_CS_STATE_STBY  )
	    {
		if ( state ==  SD_CS_STATE_STBY )
		{
			dev->card_transfer_state = NULL;
		}else
		{
			return ERROR_WRONG_CARD_STATE;
		}
	    }
            else
            {
            	SDIOERRMSG("%s %d: Busy response=%08X\n",__FUNCTION__,__LINE__,cmd.response[0]);
            	return ERROR_WRONG_CARD_STATE;
            }
	    }
        }
    }
    return ret;
}

static int decode_cid (sd_cid_t *cid,uint32_t *cid_raw)
{
    memcpy(cid,cid_raw,16);
    cid->prv = cid_raw[2]>>24;
    ((uint8_t *)&cid->pnm)[0] = (cid_raw[0])&0xFF;
    ((uint8_t *)&cid->pnm)[1] = (cid_raw[1]>>24)&0xFF;
    ((uint8_t *)&cid->pnm)[2] = (cid_raw[1]>>16)&0xFF;
    ((uint8_t *)&cid->pnm)[3] = (cid_raw[1]>>8)&0xFF;
    ((uint8_t *)&cid->pnm)[4] = (cid_raw[1])&0xFF;
    ((uint8_t *)&cid->psn)[0] = (cid_raw[2]>>16)&0xFF;
    ((uint8_t *)&cid->psn)[1] = (cid_raw[2]>>8)&0xFF;
    ((uint8_t *)&cid->psn)[2] = (cid_raw[2])&0xFF;
    ((uint8_t *)&cid->psn)[3] = (cid_raw[3]>>24)&0xFF;
    cid->mdt = (cid_raw[3]>>8)&0xFFF;	
    return 0;
}

static int read_card_cid(struct sd_controller *dev, sd_cid_t *cid)
{
    int ret=OK;
    struct sd_cmd cmd = { 0 };
    
    // send ALL_SEND_CID (CMD2) to get the card's CID
    ret = sd_send_cmd(dev,SD_CMD_ALL_SEND_CID,0,SD_RSP_R2,&cmd,1);
    if (ret!= OK)
        return ret;

    decode_cid(cid,&cmd.response[0]);
  
    return ret;
}

static int sd_memcard_init(struct sd_controller *dev,uint32_t *vdd,sd_cid_t *cid, uint32_t *ocr)
{
    struct sd_cmd cmd = { 0 };
    int ret=OK;
    uint32_t operation_vdd=0;
    int retry = 0;

    *ocr = 0;
    // send GO_IDLE_STATE (CMD0) to set card to idle state
    sd_send_cmd(dev,SD_CMD_GO_IDLE_STATE,0,SD_RSP_NONE,&cmd,1);
    mdelay(1);

    // send SD_APP_OP_COND (ACMD41) with argument 0 to ask the card to send it's OCR content
    ret = sd_send_acmd(dev,SD_CMD_SD_APP_OP_COND,0,SD_RSP_R3,&cmd,0);
    if (ret != OK)
    {
        return ret;
    }
    
    *ocr = cmd.response[0] & 0xffffff;
    operation_vdd = *ocr & dev->VDD;

    if (operation_vdd == 0) //VDD not supported
    {
        goto inactive_state;
    }

    retry = 0;
   
    // wait the card ready : send APP_OP_COND (ACMD41) and check the ready bit in the response
    do {

        ret = sd_send_acmd(dev,SD_CMD_SD_APP_OP_COND,operation_vdd,SD_RSP_R3,&cmd,0);
	retry ++;
	set_current_state (TASK_INTERRUPTIBLE);
     	schedule_timeout(1);
    }while( ret == OK && !(cmd.response[0] & SD_MEMRDY) && retry < MAX_RETRY);

    if( ret != OK ) // go to inactive state
    {
        if (ret == ERROR_TIMEOUT)
            return ret;
        goto inactive_state;
    }

    *vdd = operation_vdd;
    
    ret = read_card_cid(dev,cid);  
    
    if (ret!= OK)
        return ret;
          
    ret = MP_CARD_FOUND;

    // card-identification mode finish
    return ret;


inactive_state:
    ret = FOUND_NONE;
    return ret;
}

#ifdef SDIO_SUPPORT
static int _sdio_read_byte(struct sd_controller *dev, uint8_t func_no, uint32_t addr,uint8_t *data)
{
    struct sd_cmd cmd=
        {
            0
        };
    int ret;

    ret = sd_send_cmd(dev,SD_CMD_IO_RW_REDIRECT,(func_no << 28) | ((SDIO_CIS_PTR +addr) << 9),SD_RSP_R5,&cmd,1);
    if (ret != OK)
    {
        SDIOERRMSG("send SD_CMD_IO_RW_REDIRECT cmd fail address:%X in read_card_cis function!\n",cmd.args);
        return ret;
    }
    return OK;
}

static int _sdio_write_byte(struct sd_controller *dev, uint8_t func_no, uint32_t addr, uint8_t *data)
{
    struct sd_cmd cmd=
        {
            0
        };
    int ret;

    ret = sd_send_cmd(dev,SD_CMD_IO_RW_REDIRECT,SDIO_IO_RW_DIRECT_WRITE | (func_no << 28) | ((SDIO_CIS_PTR + addr) << 9),SD_RSP_R5,&cmd, 1);
    if (ret != OK)
    {
        SDIOERRMSG("send IO_RW_DIRECT cmd fail address:%X in read_card_cis function!\n",cmd.args);
        return ret;
    }
    return OK;
}

static struct sd_cis_tuple *read_cis_tuple(struct sd_controller *dev,uint32_t cis_ptr)
{
    uint8_t tpl_code;
    uint8_t tpl_size;
    int ret;
    struct sd_cis_tuple *tuple;

    ret = _sdio_read_byte(dev,0,cis_ptr,&tpl_code);
    if (ret != OK)
    {
        SDIOERRMSG("read_cis_tuple fail address:%X !\n",cis_ptr);
        goto error;
    }
    if (tpl_code == SD_CISTPL_NULL || tpl_code == SD_CISTPL_END)
    {
        SDIODBGMSG("read_cis_tuple got TPL_NULL or TPL_END tuple code with address %X!\n",cis_ptr);
        goto error;
    }
    ret = _sdio_read_byte(dev,0,cis_ptr+1,&tpl_size);
    if (ret != OK)
    {
        SDIOERRMSG("read_cis_tuple fail address:%X !\n",cis_ptr+1);
        goto error;
    }
    if (tpl_size == 0xff || tpl_size == 0)
    {
        SDIOERRMSG("read_cis_tuple got size %X, return NULL!\n",tpl_size);
        goto error;
    }
    tuple = kmalloc(sizeof(struct sd_cis_tuple),GFP_KERNEL);
    tuple->code = tpl_code;
    tuple->size = tpl_size;
    tuple->data = kmalloc(tpl_size,GFP_KERNEL);
    for (tpl_size=0;tpl_size < tuple->size;tpl_size++)
    {
        ret = _sdio_read_byte(dev,0,cis_ptr+2+tpl_size,&tuple->data[tpl_size]);
        if (ret != OK)
        {
            SDIOERRMSG("read_cis_tuple fail address:%X !\n",cis_ptr+1);
            goto free_and_error;
        }
    }
    return tuple;

free_and_error:
    kfree(tuple->data);
    kfree(tuple);
error:
    return NULL;

}

int sdio_readb( sdio_card_t *card, uint8_t func_no,uint32_t addr, uint8_t *data)
{
    int ret=0;
    struct sd_controller *dev=card->controller;

    ret = sd_card_select(dev,card,card->rca, SD_CS_STATE_TRAN);
    if (ret != OK)
        return ret;
    ret = _sdio_read_byte(dev,func_no,addr,data);
    return ret;
}

int sdio_writeb(sdio_card_t *card, uint8_t func_no, uint32_t addr,uint8_t *data)
{
    int ret=0;
    struct sd_controller *dev=card->controller;

    ret = sd_card_select(dev,card,card->rca, SD_CS_STATE_TRAN);
    if (ret != OK)
        return ret;
    ret = _sdio_write_byte(dev,func_no, addr,data);
    return ret;
}
#endif //#ifdef SDIO_SUPPORT

static int sd_set_blocklen(sdio_card_t *card,uint8_t block_len_bits)
{
    struct sd_cmd cmd=
        {
            0
        };
    int ret=0;
    struct sd_controller *dev = card->controller;
    int retry=0;

retry:
    ret = sd_send_cmd(dev,SD_CMD_SET_BLOCKLEN,1<<block_len_bits,SD_RSP_R1,&cmd, 1);

    if (ret != OK || cmd.response[0] & R1_SD_CS_ERROR )
    {
        SDIOERRMSG("sd_set_blocklen fail erron:%d response:%X!\n",ret,cmd.response[0]);
	retry++;
	if (retry<MAX_RETRY)
		goto retry;
    }
    dev->set_ops(SD_SET_BLOCK_LEN,block_len_bits);
    card->current_block_len_shift = block_len_bits;
    return ret;
}

int ifx_sdio_block_transfer(sdio_card_t  *card, sd_block_request_t *request)
{

    int ret=OK,retry_count=0;
    struct sd_controller *dev;
    struct sd_cmd cmd={0 };

    dev=card->controller;
    request->error = 0;
    if (request->addr > ((1 << (card->csd.c_size_mult+2)) * (card->csd.c_size+1) )<< card->csd.read_bl_len)
    {
	SDIOERRMSG("size> requset_addr addr=%08X,size=%08X\n",request->addr,((1 << (card->csd.c_size_mult+2)) * (card->csd.c_size+1) )<< card->csd.read_bl_len);
	return -EFAULT;
    }
    if (!card->valid)
    {
    	return -EIO;
    }

    down_interruptible(&dev->sd_data_sema);
    
retry:
    ret = sd_send_cmd(dev,SD_CMD_SEND_STATUS,card->rca << 16,SD_RSP_R1,&cmd, 1);
    if (ret != OK)
    {
        goto error;
    }

    if ( (cmd.response[0] & SD_CS_READY_FOR_DATA) != SD_CS_READY_FOR_DATA)
    {
	retry_count++;
	if (retry_count > MAX_DATA_WAIT_RETRY)
	{
		ret = ERROR_TIMEOUT;
		SDIOERRMSG("Timeout for CS_READY_FOR_DATA!\n");
        	goto error;
	}
	if (retry_count > MAX_DATA_WAIT_RETRY-50)
	{
		set_current_state (TASK_INTERRUPTIBLE);
     		schedule_timeout(1);
	}
	goto retry;
    }
  
    ret = sd_card_select(dev,card,card->rca, SD_CS_STATE_TRAN);

    if (ret!=OK)
    {
	SDIOERRMSG("sd_card_select fail!\n");
        goto error;
    }
    
    if (request->block_size_shift != card->current_block_len_shift)
    {
    	ret = sd_set_blocklen(card,request->block_size_shift);
        if (ret != OK)
        {
	    SDIOERRMSG("sd_set_blocklen fail!\n");
            goto error;
        }
        
    }

    request->error = 0;
    switch(request->type)
    {
        // read
    case SD_READ_BLOCK:
	
    	ret= dev->read_data_pre(dev,card,request);
        if (ret != OK)
        {
	    SDIOERRMSG("read_data_pre fail!\n");
            goto error;
        }
        if (request->nBlocks == 1) // single block read
        {
            ret = sd_send_cmd(dev,SD_CMD_READ_SINGLE_BLOCK,request->addr,SD_RSP_R1,&cmd, 1);
        }else
        {
            ret = sd_send_cmd(dev,SD_CMD_READ_MULTIPLE_BLOCK,request->addr,SD_RSP_R1,&cmd, 1);
        }
        if (ret != OK)
        {
	    SDIOERRMSG("send read block cmd fail!\n");
            goto error;
        }
         
        ret= dev->read_data(dev,card,request,150*request->nBlocks);
        if (ret != OK)
        {
	    SDIOERRMSG("read block fail!\n");
            goto error;
        }

        if (ret != OK)
        	goto error;
        break;

        // write
    case SD_WRITE_BLOCK:
        //if (request->addr != blk_boundly && !card->write_bl_partial)
        {
            // read first
            // update
        }
        if (request->nBlocks == 1) // single block write
        {
            ret = sd_send_cmd(dev,SD_CMD_WRITE_BLOCK,request->addr,SD_RSP_R1,&cmd, 1);
        }else
        {
            ret = sd_send_cmd(dev,SD_CMD_WRITE_MULTIPLE_BLOCK,request->addr,SD_RSP_R1,&cmd, 1);
        }
        if (ret != OK)
        {
	    SDIOERRMSG("send write block cmd fail!,error=%d\n",ret);
            goto error;
        }
        ret = dev->send_data(dev,card,request,300*request->nBlocks);
        if (ret != OK)
        {
	    SDIOERRMSG("send block fail!\n");
            goto error;
        }
        break;
    }

error:
    if (request->nBlocks > 1)
    {
        ret = sd_send_cmd(dev,SD_CMD_STOP_TRANSMISSION,0,SD_RSP_R1,&cmd, 1);
        if (ret != OK)
        {
	    SDIOERRMSG("send stop transmission fail!\n");
            goto error;
        }
    }

    up(&dev->sd_data_sema);
    if (ret == ERROR_TIMEOUT)
    {
       	card -> valid = 0;
	wake_up_interruptible(&wait_queue_init_thread);
    }
    return ret;
}

#ifdef SDIO_SUPPORT
static struct sd_cis_tuple * read_card_cis(struct sd_controller *dev,uint32_t rca)
{
    struct sd_cmd cmd= { 0 };
    int ret;
    uint32_t cis_ptr_addr=0;
    struct sd_cis_tuple *cis,*cis_tmp;
    int i=0;
    struct sd_cis_tuple *cis_tuple;

    ret = sd_card_select(dev,NULL,rca,SD_CS_STATE_TRAN);
    if (ret != OK)
    {
        SDIOERRMSG("send SD_CMD_SELECT_CARD cmd fail in read_card_cis function!\n");
        return NULL;
    }

    cmd.op_code = SD_CMD_IO_RW_REDIRECT;
    for (i=0;i<3;i++)
    {
        cmd.args = (SDIO_CIS_PTR + i) << 9;
        cmd.response_type = SD_RSP_R5;
        ret = dev->send_cmd(dev,&cmd);
        if (ret != OK)
        {
            SDIOERRMSG("send SD_CMD_IO_RW_REDIRECT cmd fail address:%X in read_card_cis function!\n",cmd.args);
            return NULL;
        }
        if (cmd.response[0]&R5_SD_CS_ERROR)
        {
            SDIOERRMSG("send SD_CMD_IO_RW_REDIRECT cmd fail address:%X response %X in read_card_cis function!\n",cmd.args,cmd.response[0]);
            return NULL;
        }
        ((uint8_t *)&cis_ptr_addr)[4-i]=cmd.response[0]&0xff;
    }
    
    cis=cis_tmp=NULL;
    do
    {
        cis_tuple = read_cis_tuple(dev,cpu_to_le32(cis_ptr_addr));
        if (cis_tuple==NULL)
            break;
        if (cis == NULL)
        {
            cis_tuple = cis;
            cis_tmp = cis;
        }
        else
        {
            cis_tmp->next = cis_tuple;
            cis_tmp = cis_tmp->next;
        }
        cis_ptr_addr += cis_tuple->size +2; // tuple code + tpl_linke + size
    }
    while(cis_tuple && cis_tuple->size != 0);
    return cis;
}

static uint8_t *sd_cis_get_tuple_body(uint8_t code,struct sd_cis_tuple *cis)
{
    while(cis)
    {
        if (cis->code == code)
            return cis->data;
        cis=cis->next;
    }
    return NULL;
}
#endif //#ifdef SDIO_SUPPORT

static sdio_card_driver_t * search_card_driver(uint8_t *data,int type)
{
    struct list_head *lh;
    sdio_card_driver_t *card_driver;
    uint8_t *manfid;

    if (type == SD_TYPE) // manufacture identification exist in cid data
    {
        manfid = data;
    }
    else	// manufacture identification exist in cis data
    {
#ifdef SDIO_SUPPORT    	
        manfid=sd_cis_get_tuple_body(SD_CISTPL_MANFID,(struct sd_cis_tuple *)data);
#endif //#ifdef SDIO_SUPPORT        
        if (manfid == NULL)
        {
            SDIODBGMSG("No MANFID found!\n");
            return NULL;
        }
    }
    list_for_each(lh, &sdio_card_drivers)
    {
        card_driver = (sdio_card_driver_t *)lh;
        if (card_driver->type == type)
        {
            if (type == SD_TYPE)
            {
            	return card_driver;
            }
#ifdef SDIO_SUPPORT            
            if (!memcmp(card_driver->cid,manfid,16))
                return card_driver;
#endif //#ifdef SDIO_SUPPORT                

        }
    }
    return NULL;
}
static uint32_t decode_tran_speed(uint8_t tran_speed)
{
	uint32_t speed=0;
	uint32_t rate_unit=100000; //100K
	uint32_t time_value=0;
	int temp=0;
	temp = tran_speed&0x7;
	while(temp>0)
	{
		rate_unit*=10;
		temp --;
	}
	temp = (tran_speed >> 3) &0xF;
	if( temp ==0)
	{
		return 0;
	}else if( temp == 1)
	{
		time_value = 10;
	}else if( temp < 4)
	{
		time_value = 10 + temp;
	}else if( temp < 0xE)
	{
		time_value = (temp -1 )*5;
	}else if( temp == 0xE)
	{
		time_value = 70;
	}else if( temp == 0xF)
	{
		time_value = 80;
	}
	speed = time_value * rate_unit / 10;
	return speed;
}

static int decode_csd(sdio_csd_t *csd,uint32_t *csd_raw)
{
	    
    memcpy(csd,csd_raw,16);  
    csd->c_size = (csd_raw[1]&0x3FF)<<2 | (csd_raw[2]>>30 &0x3 );
    csd->vdd_r_curr_min = csd_raw[2]>>27&0x7;
    csd->vdd_r_curr_max = csd_raw[2]>>24&0x7;
    csd->vdd_w_curr_min = csd_raw[2]>>21&0x7;
    csd->vdd_w_curr_max = csd_raw[2]>>18&0x7;
    csd->c_size_mult = csd_raw[2]>>15&0x7;
    csd->erase_blk_en = csd_raw[2]>>14&0x1;
    csd->sector_size = csd_raw[2]>>7&0x7F;
    csd->wp_grp_size = csd_raw[2]&0x7F;
    return 0;    
}

static int read_card_csd(struct sd_controller *dev,sdio_card_t *card)
{
    struct sd_cmd cmd= { 0 };
    int ret=OK;

    ret = sd_send_cmd(dev,SD_CMD_SEND_CSD,card->rca << 16,SD_RSP_R2,&cmd, 1);
    if (ret != OK)
    {
        SDIOERRMSG("read_card_csd fail\n");
        return ret;
    }
    
    decode_csd(&card->csd,&cmd.response[0]);
  
    card->max_speed = decode_tran_speed(card->csd.tran_speed);
   
    return ret;
}

static int decode_scr(sd_scr_t *scr,uint8_t *scr_raw)
{
	scr->reserved32 = le32_to_cpu(((uint32_t *)scr_raw)[1]);
        scr->reserved16 = le16_to_cpu(((uint16_t *)scr_raw)[0]);
	scr->sd_bus_width = (scr_raw[2])&0xf;
	scr->sd_security = (scr_raw[2]>>4)&0x7;
	scr->data_stat_after_erase = (scr_raw[2]>>7)&0x1;
	scr->sd_spec = (scr_raw[3])&0xf;
	scr->scr_structure = (scr_raw[3]>>4)&0xf;
	return 0;
}

static int read_card_scr(struct sd_controller *dev,sdio_card_t *card)
{
    sd_block_request_t request;
    struct sd_cmd cmd={0};
    int ret=OK;
    unsigned char scr[8];
    block_data_t block = { 0 };

    down_interruptible(&dev->sd_data_sema);
    
    // fill request structure
    request.nBlocks = 1;
    request.request_size = 8;
    request.addr = 0;
    request.block_size_shift = 3; //8 Bytes
    block.data = scr;
    request.blocks = &block;
    
    ret = sd_card_select(dev,card,card->rca,SD_CS_STATE_TRAN);
    if (ret != OK)
    {
	SDIOERRMSG("sd_card_select fail\n");
       	goto error;
    }
    ret= dev->read_data_pre(dev,card,&request);
    if (ret != OK)
    {
	SDIOERRMSG("read_data_pre fail\n");
	goto error;
    }
    
    request.error = 0;
    ret = sd_send_acmd(dev,SD_CMD_SEND_SCR,0,SD_RSP_R1,&cmd, card->rca);
    if (ret != OK)
    {
	SDIOERRMSG("send_acmd fail\n");
	goto error;
    }
    ret= dev->read_data(dev,card,&request,100);

    if (ret != OK)
    {
	SDIOERRMSG("read_data fail\n");
	goto error;
    }
    decode_scr(&card->scr,scr);

    ret = sd_card_select(dev,card,card->rca,SD_CS_STATE_STBY);
    if (ret != OK){
	SDIOERRMSG("sd_card_select fail\n");
	goto error;
    }

error:
    up(&dev->sd_data_sema);
    return ret;

}

#ifdef SDIO_SUPPORT
// ret: 0 not found
// ret: 2 found SDIO card
// ret: 1 found MP card
// ret: - error
static int sd_iocard_init(struct sd_controller *dev,uint32_t *vdd, uint32_t *ocr)
{
    struct sd_cmd cmd={0};
    int ret=OK;
    unsigned char NO_IO=0;
    uint32_t operation_vdd=0;
    uint8_t	 MP=0;
    int retry=0;

    *ocr = 0;
    // send IO_SEND_OP_COND (CMD5) with argument 0 to get IO card's OCR content
    ret = sd_send_cmd(dev,SD_CMD_IO_SEND_OP_COND,0,SD_RSP_R4,&cmd, 1);
    if (ret != OK)
        return ret;

    // Number of I/O Functions
    NO_IO = cmd.response[0]>>28;
    NO_IO &=0x7;

    // check if the card also contains SD memory.
    if (cmd.response[0]&SD_MP_PRESENT)
    {
        MP=1;
    }

    // No any I/O function supported
    if (NO_IO == 0)
    {
        goto inactive_state;
    }

    *ocr = cmd.response[0] & 0xffffff;
    operation_vdd = *ocr & dev->VDD;
    if (operation_vdd == 0) // OCR invalid
    {
        goto inactive_state;
    }

    // wait the card ready : send IO_SEND_OP_COND (CMD5) and check the ready bit in the response
    do
    {
        ret = sd_send_cmd(dev,SD_CMD_IO_SEND_OP_COND,dev->VDD,SD_RSP_R4,&cmd,1);
        retry++;
    }
    while ( ret == OK && !(cmd.response[0] & SDIO_IORDY) && retry < MAX_RETRY);

    if( ((ret != OK)|| retry == MAX_RETRY ) ) // go to inactive state
    {
        if (ret == ERROR_TIMEOUT)
            return ret;
        goto inactive_state;
    }

    *vdd = operation_vdd;

    if (MP)
    {
        ret = COMBO_CARD_FOUND;
    }
    else
    {
        ret = SDIO_CARD_FOUND;
    }
    // card initialization finish
    return ret;


inactive_state:
    if (MP)
    {
        ret = MP_CARD_FOUND;
    }
    else
    {
        sd_send_cmd(dev,SD_CMD_GO_INACTIVE_STATE,0,SD_RSP_R1,&cmd, 1);
        ret = FOUND_NONE;
    }
    return ret;
}
#endif //#ifdef SDIO_SUPPORT

#ifdef CONFIG_PROC_FS
static int infineon_card_proc_read(struct file * file, char * buf, size_t nbytes, loff_t *ppos);

static struct file_operations infineon_card_proc_operations = {
	read:	infineon_card_proc_read,
};

static int infineon_card_proc_read(struct file * file, char * buf, size_t nbytes, loff_t *ppos)
{
        int i_ino = (file->f_dentry->d_inode)->i_ino;
	char outputbuf[1024];
	int count=0;
	int i,j;
	reg_entry_t* current_reg=NULL;

	for (j=0;j<MAX_CARDS;j++)
	{
		for (i=0;i<NUM_OF_REG_CARD_ENTRY;i++) {
			if (regs_cards[j][i].low_ino==i_ino) {
				current_reg = &regs_cards[j][i];
				break;
			}
		}
        }
	if (current_reg==NULL)
		return -EINVAL;
	if (current_reg->flag == (int *) 1){
 		
	///proc/infineon_sdio/CARD_X/type
		if (*ppos>0) /* Assume reading completed in previous read*/
			return 0;               // indicates end of file
                count = sprintf(outputbuf, "\nType\t\n");
                if (current_reg->card->type & SD_TYPE)
                {
                	count += sprintf(outputbuf+count, "SD\n");
                }
                if (current_reg->card->type & SDIO_TYPE)
                {
                	count += sprintf(outputbuf+count, "SDIO\n");
                }
		
		*ppos+=count;
	}else if (current_reg->flag == (int *) 2){
		
	///proc/infineon_sdio/CARD_X/scr
		if (*ppos>0) /* Assume reading completed in previous read*/
			return 0;               // indicates end of file
			
		count += sprintf(outputbuf+count,"SD_BUS_WIDTH          0x%01X\n",current_reg->card->scr.sd_bus_width);
		count += sprintf(outputbuf+count,"SD_SECURITY           0x%01X\n",current_reg->card->scr.sd_security);
		count += sprintf(outputbuf+count,"DATA_STAT_AFTER_ERASE 0x%01X\n",current_reg->card->scr.data_stat_after_erase);
		count += sprintf(outputbuf+count,"SD_SPEC               0x%01X\n",current_reg->card->scr.sd_spec);
		count += sprintf(outputbuf+count,"SCR_STRUCTURE         0x%01X\n",current_reg->card->scr.scr_structure);
		*ppos+=count;
	}else if (current_reg->flag == (int *) 3){
	///proc/infineon_sdio/CARD_X/csd
		if (*ppos>0) /* Assume reading completed in previous read*/
			return 0;               // indicates end of file
			
	    count += sprintf(outputbuf+count,"CSD                0x%01X\n",current_reg->card->csd.csd);
	    count += sprintf(outputbuf+count,"TAAC               0x%02X\n",current_reg->card->csd.taac);
	    count += sprintf(outputbuf+count,"NSAC               0x%02X\n",current_reg->card->csd.nsac);
	    count += sprintf(outputbuf+count,"TRAN_SPEED         0x%02X\n",current_reg->card->csd.tran_speed);
	    count += sprintf(outputbuf+count,"CCC                0x%03X\n",current_reg->card->csd.ccc);	
	    count += sprintf(outputbuf+count,"READ_BL_LEN        0x%01X\n",current_reg->card->csd.read_bl_len);	
	    count += sprintf(outputbuf+count,"READ_BL_PARTIAL    0x%01X\n",current_reg->card->csd.read_bl_partial);	
	    count += sprintf(outputbuf+count,"WRITE_BLK_MISALIGN 0x%01X\n",current_reg->card->csd.write_blk_misalign);
	    count += sprintf(outputbuf+count,"READ_BLK_MISALIGN  0x%01X\n",current_reg->card->csd.read_blk_misalign);
	    count += sprintf(outputbuf+count,"DSR_IMP            0x%01X\n",current_reg->card->csd.dsr_imp);
	    count += sprintf(outputbuf+count,"CSIZE              0x%03X\n", current_reg->card->csd.c_size);
	    count += sprintf(outputbuf+count,"VDD_R_CURR_MIN     0x%01X\n",current_reg->card->csd.vdd_r_curr_min);
	    count += sprintf(outputbuf+count,"VDD_R_CURR_MAX     0x%01X\n",current_reg->card->csd.vdd_r_curr_max);
	    count += sprintf(outputbuf+count,"VDD_W_CURR_MIN     0x%01X\n",current_reg->card->csd.vdd_w_curr_min);
	    count += sprintf(outputbuf+count,"VDD_W_CURR_MAX     0x%01X\n",current_reg->card->csd.vdd_w_curr_max);
	    count += sprintf(outputbuf+count,"C_SIZE_MULT        0x%01X\n",current_reg->card->csd.c_size_mult);
	    count += sprintf(outputbuf+count,"ERASE_BLK_EN       0x%01X\n",current_reg->card->csd.erase_blk_en);
	    count += sprintf(outputbuf+count,"SECTOR_SIZE        0x%01X\n",current_reg->card->csd.sector_size);
	    count += sprintf(outputbuf+count,"WP_GRP_SIZE        0x%01X\n",current_reg->card->csd.wp_grp_size);
	    count += sprintf(outputbuf+count,"WP_GRP_ENABLE      0x%01X\n",current_reg->card->csd.wp_grp_enable);
	    count += sprintf(outputbuf+count,"R2W_FACTOR         0x%01X\n",current_reg->card->csd.r2w_factor);
	    count += sprintf(outputbuf+count,"WRITE_BL_LEN       0x%01X\n",current_reg->card->csd.write_bl_len);
	    count += sprintf(outputbuf+count,"WRITE_BL_PARTIAL   0x%01X\n",current_reg->card->csd.write_bl_partial);
	    count += sprintf(outputbuf+count,"FILE_FORMAT_GRP    0x%01X\n",current_reg->card->csd.file_format_grp);
	    count += sprintf(outputbuf+count,"COPY               0x%01X\n",current_reg->card->csd.copy);
	    count += sprintf(outputbuf+count,"PERM_WRITE_PROTECT 0x%01X\n",current_reg->card->csd.perm_write_protect);
	    count += sprintf(outputbuf+count,"TMP_WRITE_PROTECT  0x%01X\n",current_reg->card->csd.tmp_write_protect);
	    count += sprintf(outputbuf+count,"FILE_FORMAT        0x%01X\n",current_reg->card->csd.file_format);
		*ppos+=count;
	}else if (current_reg->flag == (int *) 4){ 		
	///proc/infineon_sdio/CARD_X/ocr
		if (*ppos>0) /* Assume reading completed in previous read*/
			return 0;               // indicates end of file
                count = sprintf(outputbuf, "\nOCR:\n");	
                if (current_reg->card->type & SD_TYPE)
                {	
            		count += sprintf(outputbuf+count, "SD:0x%08X\n",current_reg->card->mp_ocr);
        	}
        	if (current_reg->card->type & SDIO_TYPE)
                {	
            		count += sprintf(outputbuf+count, "SD:0x%08X\n",current_reg->card->sdio_ocr);
        	}
		*ppos+=count;
	}else if (current_reg->flag == (int *) 5){ 		
	///proc/infineon_sdio/CARD_X/cid
		if (*ppos>0) /* Assume reading completed in previous read*/
			return 0;               // indicates end of file
                count = sprintf(outputbuf, "CID:\n");	
                if (current_reg->card->type & SD_TYPE)
                {	
            		count += sprintf(outputbuf+count, "MID:0x%02X\n",current_reg->card->cid.mid);
            		count += sprintf(outputbuf+count, "OID:%c%c\n",((char *)&current_reg->card->cid.oid)[0],((char *)&current_reg->card->cid.oid)[1]);
            		count += sprintf(outputbuf+count, "PNM:%c%c%c%c%c\n",current_reg->card->cid.pnm[0],current_reg->card->cid.pnm[1],current_reg->card->cid.pnm[2],current_reg->card->cid.pnm[3],current_reg->card->cid.pnm[4]);
            		count += sprintf(outputbuf+count, "PRV:%d.%d\n",(current_reg->card->cid.prv>>4)&0xf,current_reg->card->cid.prv&0xf);
            		count += sprintf(outputbuf+count, "PSN:0x%08X\n",current_reg->card->cid.psn);
            		count += sprintf(outputbuf+count, "MDT:%d %d\n",current_reg->card->cid.mdt&0xF,((current_reg->card->cid.mdt>>4)&0xFF)+2000);
                 	}
        	
		*ppos+=count;
	}else{
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

static int create_card_proc_entry (sdio_card_t *card) 
{
	char name[32];
	
	struct proc_dir_entry *entry;
	int i=0,slot=-1;
	
    reg_entry_t regs_temp[NUM_OF_REG_CARD_ENTRY] =                                     // Items being debugged
    {
        /*	{       flag,          name,          description } */
        { (int *)1, "TYPE", "sd card type", 0 , NULL},
        { (int *)2, "SCR", "scr register", 0, NULL},      
        { (int *)3, "CSD", "csd register", 0, NULL},  
        { (int *)4, "OCR", "ocr register", 0, NULL}, 
        { (int *)5, "CID", "cid register", 0, NULL},     
        { (int *)&card->rca, "RCA", "rca register", 0, NULL},
    };		

    for (i=0;i<MAX_CARDS;i++)
    {
    	if (card == regs_cards[i][0].card )
    		return(-ENOMEM);;
    	if (slot<0 && regs_cards[i][0].card == NULL)
    		slot = i;
    }	
    if (slot < 0)
    {
    	SDIOERRMSG("Error: exceed max cards!\n");
    	return(-ENOMEM);
     }
    sprintf(name,"CARD_%d",slot);
    infineon_card_dir[slot]=proc_mkdir(name, infineon_sdio_dir);
    if ( infineon_card_dir[slot] == NULL) {
	printk(KERN_ERR "%s: can't create /proc/%s/%s\n\n" ,__FUNCTION__,INFINEON_SDIO_PROC_DIRNAME,name);
	return(-ENOMEM);
    }
    
    memcpy((char *)regs_cards[slot], (char *)regs_temp, sizeof(regs_temp));
    
    for(i=0;i<NUM_OF_REG_CARD_ENTRY;i++) {
    	regs_cards[slot][i].card = card;
	entry = create_proc_entry(regs_cards[slot][i].name,	S_IWUSR |S_IRUSR | S_IRGRP | S_IROTH, infineon_card_dir[slot]);
	if(entry) {
		regs_cards[slot][i].low_ino = entry->low_ino;
		entry->proc_fops = &infineon_card_proc_operations;
	} else {
			printk(KERN_ERR "%s: can't create /proc/%s/%s/%s\n\n" ,__FUNCTION__,INFINEON_SDIO_PROC_DIRNAME,name,regs_cards[slot][i].name);
			return(-ENOMEM);
	}
    }
    return 0;
}
static int remove_card_proc_entry(sdio_card_t *card)
{
    int i=0,slot = -1;
    char name[32];
    for (i=0;i<MAX_CARDS;i++)
    {
    	if (card == regs_cards[i][0].card )
    	{
    	    slot = i;
    	    break;
    	}
    }	
    if (slot < 0)
    {
        SDIOERRMSG("Error: No card found!\n");
    	return(-ENOMEM);
    }
    sprintf(name,"CARD_%d",slot);
    for(i=0;i<NUM_OF_REG_CARD_ENTRY;i++)
    {
	remove_proc_entry(regs_cards[slot][i].name, infineon_card_dir[slot]);
	regs_cards[slot][i].card = NULL;
    }
    remove_proc_entry(name, infineon_sdio_dir);
    infineon_card_dir[slot]=NULL;
    return 0;
}
#endif //#ifdef CONFIG_PROC_FS

static int sd_card_scan(struct sd_controller *dev)
{
    int ret=OK;
    struct sd_cmd cmd={0};
    int sdio_timeout=0,mp_timeout=0;
    uint32_t rca=0;
    //uint32_t mp_cid[4];
    sdio_card_driver_t *sd_card_driver=NULL;
    sd_cid_t mp_cid;
    unsigned char mp=0;
    uint32_t mp_ocr=0,sd_vdd=0;

    unsigned char sdio=0;
    sdio_card_driver_t *sdio_card_driver=NULL;
    
#ifdef SDIO_SUPPORT    
    uint32_t sdio_ocr=0;
    uint32_t sdio_vdd=0;
    struct sd_cis_tuple *cis=NULL;    
#endif    
 
#ifdef SDIO_SUPPORT
    if (!sdio_timeout)
        ret = sd_iocard_init(dev,&sdio_vdd,&sdio_ocr);
    switch (ret)
    {
    case MP_CARD_FOUND:
        mp = 1;
        break;
    case SDIO_CARD_FOUND:
        sdio = 1;
        break;
    case COMBO_CARD_FOUND:
        mp = sdio = 1;
        break;
    default:
        sdio_timeout = 1;
        break;
    }
#else
    mp = 1;
#endif    
    if (sdio_timeout || mp == 1)
    {
	
        ret = sd_memcard_init(dev,&sd_vdd,&mp_cid,&mp_ocr);
        if (ret == MP_CARD_FOUND)
        {
            SDIODBGMSG("%s %d:found a mp card.mid=%X,oid=%X,PRV=%X,PSN=%X,MDT=%X\n",__FUNCTION__,__LINE__,mp_cid.mid,mp_cid.oid,mp_cid.prv,mp_cid.psn,mp_cid.mdt);
            mp = 1;
        }
        else
        {
             SDIODBGMSG("%s %d: no mp card found\n",__FUNCTION__,__LINE__);
             mp_timeout = 1;	// timeout or MMC card found!
        }
    }
    if (mp == 1 || sdio == 1)
    {
        ret = sd_send_cmd(dev,SD_CMD_SEND_RELATIVE_ADDR,0,SD_RSP_R6,&cmd, 1);
        if (ret != OK )
        {
            goto inactive_state;
        }
        else
        {
            if (cmd.response[0] & R6_SD_CS_ERROR)
            {
                goto inactive_state;
            }
            rca = cmd.response[0]>>16;
        }
        if (mp == 1)
        {
            sd_card_driver = search_card_driver((uint8_t *)&mp_cid,SD_TYPE);
        }
#ifdef SDIO_SUPPORT        
        if (sdio == 1)
        {
            cis = read_card_cis(dev,rca);
            sdio_card_driver = search_card_driver((uint8_t *)cis,SDIO_TYPE);
        }
#endif //#ifdef SDIO_SUPPORT
        
        if (sd_card_driver != NULL || sdio_card_driver != NULL)
        {
            // need to alloc card drivers
            sdio_card_t *new_card;
            new_card = kmalloc(sizeof(sdio_card_t),GFP_KERNEL);
            if (new_card == NULL)
            {
                SDIOERRMSG("malloc card memory fail!\n");
                return ERROR_NOMEM;
            }
            memset(new_card,0,sizeof(sdio_card_t));
            new_card->controller = dev;
            INIT_LIST_HEAD(&(new_card->list));
            new_card->rca = rca;
            create_card_proc_entry(new_card);
            if (mp == 1)
            {
                new_card->sd_driver = sd_card_driver;
                new_card->vdd = sd_vdd;
                new_card->mp_ocr = mp_ocr;
                memcpy(&new_card->cid, &mp_cid,sizeof(mp_cid));
                ret = read_card_csd(dev,new_card);
		if (ret != 0)
		    return ret;
		ret = read_card_scr(dev,new_card);
		if (ret != 0)
		    return ret;
#ifdef FREQUENCY_CHANGE
		// update tran_speed to max // need to update
		dev->set_ops(SD_SET_FREQENCY,new_card->max_speed);
#endif
		new_card->type |= SD_TYPE;
		new_card -> valid = 1;
                if (((sdio_card_driver_t *)new_card->sd_driver)->probe()== OK)
		{
               	    ((sdio_card_driver_t *)new_card->sd_driver)->insert(new_card);
       		}
            }
#ifdef SDIO_SUPPORT            
            if (sdio == 1)
            {
                new_card->vdd = sdio_vdd;
                new_card->cis = cis;
		new_card->type |= SDIO_TYPE;
                new_card->sdio_driver = sdio_card_driver;
                new_card->sdio_ocr = sdio_ocr;
                if (((sdio_card_driver_t *)new_card->sdio_driver)->probe()== OK)
		{
                    ((sdio_card_driver_t *)new_card->sdio_driver)->insert(new_card);
       		}
            }
#endif //#ifdef SDIO_SUPPORT            
            list_add(&(new_card->list), &(dev->cards.list));
        }
    }
    return OK;

inactive_state:
    sd_send_cmd(dev,SD_CMD_GO_INACTIVE_STATE,rca,SD_RSP_R1,&cmd, 1);
    return OK;
}

#ifdef SDIO_SUPPORT
static int free_cis(struct sd_cis_tuple *cis)
{
    struct sd_cis_tuple *tuple=NULL;
    while(cis)
    {
        tuple = cis;
        cis=cis->next;
        kfree(tuple->data);
        kfree(tuple);
    }
    return OK;
}
#endif //#ifdef SDIO_SUPPORT

static int sd_card_eject(sdio_card_t *card)
{
    if (card->type &SD_TYPE && card->sd_driver)
    {
    	((sdio_card_driver_t *)card->sd_driver)->eject(card);
    	((sdio_card_driver_t *)card->sd_driver)->remove(card);
    }
#ifdef SDIO_SUPPORT    
    if (card->type &SDIO_TYPE && card->sdio_driver)
    {
    	((sdio_card_driver_t *)card->sdio_driver)->eject(card);
    	((sdio_card_driver_t *)card->sdio_driver)->remove(card);
    }
#endif //#ifdef SDIO_SUPPORT    
    remove_card_proc_entry(card);
    card -> valid = 0;
#ifdef SDIO_SUPPORT    
    if (card->cis)
       free_cis(card->cis);
#endif //#ifdef SDIO_SUPPORT       
    list_del(&(card->list));
    kfree(card);
    return OK;
}

#ifdef SDIO_SUPPORT
int sd_core_sdio_int(struct sd_controller *dev)
{
    struct list_head *lh;
    struct sd_controller *controller;
    list_for_each(lh, &sdio_card_drivers)
    {
        controller = (struct sd_controller *)lh;
        if (controller == dev)
        {
            controller->sdio_irq_state=1;
            break;
        }
    }
    wake_up_interruptible(&wait_queue_sdio_irq);
    return 0;
}

static int sdio_irq_thread(void *data)
{
    struct task_struct *tsk = current;
    struct list_head *lh;
    struct list_head *card_lh;
    sdio_card_t *card;
    struct sd_controller *controller;

    daemonize();
    strcpy(tsk->comm, "sdio_irq_thread");
    sigfillset(&tsk->blocked);

    while (!stop_sdio_irq_thread)
    {
        interruptible_sleep_on(&wait_queue_sdio_irq);
        list_for_each(lh, &sdio_controllers)
        {
            controller = (struct sd_controller *)lh;
            if (controller->sdio_irq_state)
            {
                //controller->cards.list
                list_for_each(card_lh, &controller->cards.list)
                {
                    card = (sdio_card_t *)card_lh;
                    if (card->type & SDIO_TYPE)
                        ((sdio_card_driver_t *)card->sdio_driver)->sdio_irq();
                }
                // enable sdio interrupt
                controller->mask_ack_irq();
                controller->sdio_irq_state = 0;
            }
        }

    }
    complete_and_exit(&sdio_irq_thread_exit, 0);
}
#endif //#ifdef SDIO_SUPPORT

static int sdio_init_thread(void *data)
{
    struct task_struct *tsk = current;
    struct list_head *lh, *card_lh;
    sdio_card_t *card;
    struct sd_controller *controller;

    init_waitqueue_head(&wait_queue_init_thread);
    daemonize();
    strcpy(tsk->comm, "sdio_init_thread");
    sigfillset(&tsk->blocked);

    while (!stop_sdio_init_thread)
    {
    	interruptible_sleep_on_timeout(&wait_queue_init_thread, 5*HZ); 
    	 list_for_each(lh, &sdio_controllers)
	 {
		int ret;
    		struct sd_cmd cmd= { 0 };
	        controller = (struct sd_controller *)lh;
                if (list_empty(&(controller->cards.list)))
		{
#ifdef FREQUENCY_CHANGE
			controller->set_ops(SD_SET_FREQENCY,SD_CLK_400K);
#endif
#ifdef ENABLE_WIDE_BUS
			controller->set_ops(SD_SET_BUS_WIDTH,SD_BUS_1BITS);
#endif
			controller->card_transfer_state = NULL;
			
			sema_init(&controller->sd_cmd_sema,1);
			sema_init(&controller->sd_data_sema,1);
			sd_send_cmd(controller,SD_CMD_GO_IDLE_STATE,0,SD_RSP_NONE,&cmd,1);
	    		ret = sd_send_acmd(controller,SD_CMD_SD_APP_OP_COND,0,SD_RSP_R3,&cmd,0);
    			if (ret == OK || (ret!=ERROR_TIMEOUT && (cmd.response[0] & R1_SD_CS_ERROR)))
			{
    				sd_send_cmd(controller,SD_CMD_GO_IDLE_STATE,0,SD_RSP_NONE,&cmd,1);
	        		sd_card_scan(controller);
			}
		}else
		{
                	list_for_each(card_lh, &controller->cards.list)
                	{
                   	 	card = (sdio_card_t *)card_lh;
                    		if (card->type & SD_TYPE)
				{
					down_interruptible(&controller->sd_data_sema);
        				ret = sd_send_cmd(controller,SD_CMD_SEND_STATUS,card->rca << 16,SD_RSP_R1,&cmd,1);
    					up(&controller->sd_data_sema);
        				if (ret != OK)
					{
                    				sd_card_eject(card);
						break;
					}
				}
			}
		}
	 }
    		
    }
    complete_and_exit(&sdio_init_thread_exit, 0);
}

int register_sd_card_driver(sdio_card_driver_t *driver)
{
    struct list_head *driver_entry=(void *)driver;
    
    list_add(driver_entry, &sdio_card_drivers);
    MOD_INC_USE_COUNT;
    return OK;
}

void unregister_sd_card_driver(sdio_card_driver_t *driver)
{
    struct list_head *driver_entry = (void *)driver;
    list_del(driver_entry);
    MOD_DEC_USE_COUNT;

}

int register_sd_controller(sdio_controller_t *controller)
{
    struct list_head *driver_entry=(void *)controller;

    list_add(driver_entry, &sdio_controllers);
    INIT_LIST_HEAD(&controller->cards.list);

    controller->set_ops(SD_SET_FREQENCY,SD_CLK_400K);
     
    MOD_INC_USE_COUNT;
    return OK;
}

void unregister_sd_controller(sdio_controller_t *controller)
{
    struct list_head *driver_entry = (void *)controller;
    list_del(driver_entry);
    MOD_DEC_USE_COUNT;

}

#ifdef CONFIG_PROC_FS

static int infineon_sdio_proc_read(struct file * file, char * buf, size_t nbytes, loff_t *ppos);

static struct file_operations infineon_sdio_proc_operations = {
	read:	infineon_sdio_proc_read,
};

#define NUM_OF_REG_ENTRY	(sizeof(regs_sdio)/sizeof(reg_entry_t))

static int infineon_sdio_proc_read(struct file * file, char * buf, size_t nbytes, loff_t *ppos)
{
        int i_ino = (file->f_dentry->d_inode)->i_ino;
	char outputbuf[256];
	int count=0;
	int i;
	reg_entry_t* current_reg=NULL;
	for (i=0;i<NUM_OF_REG_ENTRY;i++) {
		if (regs_sdio[i].low_ino==i_ino) {
			current_reg = &regs_sdio[i];
			break;
		}
	}
	if (current_reg==NULL)
		return -EINVAL;
	if (current_reg->flag == (int *) 1){
		struct list_head *lh;
 		struct sd_controller *controller;
 		
	///proc/infinon_sdio/host_driver
		if (*ppos>0) /* Assume reading completed in previous read*/
			return 0;               // indicates end of file
                count = sprintf(outputbuf, "\nDriver Name\t\n");			
		list_for_each(lh, &sdio_controllers)
        	{
            		controller = (struct sd_controller *)lh;
            		count += sprintf(outputbuf+count, "%s\n",controller->name);
        	}
		*ppos+=count;
	}else if (current_reg->flag == (int *) 2){
		struct list_head *lh;
	        sdio_card_driver_t *card_driver;
 		
	///proc/infinon_sdio/card_driver
		if (*ppos>0) /* Assume reading completed in previous read*/
			return 0;               // indicates end of file
                count = sprintf(outputbuf, "\nType Card Driver Name\n");			
		list_for_each(lh, &sdio_card_drivers)
        	{
            		card_driver = (sdio_card_driver_t *)lh;
            		count += sprintf(outputbuf+count, "%s %s\n",card_driver->type==SD_TYPE?"SD  ":"SDIO",card_driver->name);
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
#endif //CONFIG_PROC_FS

static int sdio_core_init_module(void)
{
    int i=0;
#ifdef CONFIG_PROC_FS    
    struct proc_dir_entry *entry;
    reg_entry_t regs_temp[SDIO_PROC_ITEMS] =                                     // Items being debugged
    {
        /*	{       flag,          name,          description } */
        { (int *)1, "host_driver", "host driver", 0, NULL },
        { (int *)2, "card_driver", "card driver", 0, NULL},
    };	
#endif    
	
    printk( "Infineon SDIO Core Version: %s\n",IFX_SDIO_CORE_VERSION );
    
    INIT_LIST_HEAD(&sdio_controllers);
    INIT_LIST_HEAD(&sdio_card_drivers);

#ifdef SDIO_SUPPORT
    init_waitqueue_head(&wait_queue_sdio_irq);
    init_completion(&sdio_irq_thread_exit);

    stop_sdio_irq_thread=0;
    kernel_thread(sdio_irq_thread, NULL, CLONE_FS | CLONE_FILES | CLONE_SIGHAND);
#endif //#ifdef SDIO_SUPPORT
    
    kernel_thread(sdio_init_thread, NULL, CLONE_FS | CLONE_FILES | CLONE_SIGHAND);

#ifdef CONFIG_PROC_FS    
    // procfs
    infineon_sdio_dir=proc_mkdir(INFINEON_SDIO_PROC_DIRNAME, NULL);
    if ( infineon_sdio_dir == NULL) {
	printk(KERN_ERR "%s[%d]: can't create /proc/%s",__FUNCTION__,__LINE__,INFINEON_SDIO_PROC_DIRNAME "\n\n");
	return(-ENOMEM);
    }


    memcpy((char *)regs_sdio, (char *)regs_temp, sizeof(regs_temp));
    
    for(i=0;i<NUM_OF_REG_ENTRY;i++) {
	entry = create_proc_entry(regs_sdio[i].name,	S_IWUSR |S_IRUSR | S_IRGRP | S_IROTH, infineon_sdio_dir);
	if(entry) {
		regs_sdio[i].low_ino = entry->low_ino;
		entry->proc_fops = &infineon_sdio_proc_operations;
	} else {
			printk( KERN_ERR 
				"%s: can't create /proc/%s/%s\n" ,__FUNCTION__,INFINEON_SDIO_PROC_DIRNAME, regs_sdio[i].name);
			return(-ENOMEM);
	}
    }
#endif // CONFIG_PROC_FS    
    return 0;
}

static void sdio_core_exit_module(void)
{
    int i;


#ifdef CONFIG_PROC_FS
        for(i=0;i<NUM_OF_REG_ENTRY;i++)
		remove_proc_entry(regs_sdio[i].name, infineon_sdio_dir);

        remove_proc_entry(INFINEON_SDIO_PROC_DIRNAME, &proc_root);
#endif //CONFIG_PROC_FS

#ifdef SDIO_SUPPORT    
    stop_sdio_irq_thread = 1;
    wake_up_interruptible(&wait_queue_sdio_irq);
    wait_for_completion(&sdio_irq_thread_exit);
#endif //#ifdef SDIO_SUPPORT    
    stop_sdio_init_thread = 1;
    wake_up_interruptible(&wait_queue_init_thread);
    wait_for_completion(&sdio_init_thread_exit);
    
    SDIODBGMSG( "Module sdio_core exit\n" );
}

module_init(sdio_core_init_module);
module_exit(sdio_core_exit_module);

