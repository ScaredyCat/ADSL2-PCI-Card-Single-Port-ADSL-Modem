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

#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/fs.h>     /* everything... */
#include <linux/errno.h>  /* error codes */
#include <linux/types.h>  /* size_t */
#include <linux/fcntl.h>        /* O_ACCMODE */
#include <linux/hdreg.h>  /* HDIO_GETGEO */
#include <asm/byteorder.h>
#undef CONFIG_DEVFS_FS
#ifdef CONFIG_DEVFS_FS
#include <linux/devfs_fs_kernel.h>
#endif

#include <asm/system.h>   /* cli(), *_flags */
#include <asm/uaccess.h>
#include <asm/io.h>

#define IFX_SD_VERSION "0.90"

#define MAJOR_NR sd_memory_major /* force definitions on in blk.h */
int sd_memory_major; /* must be declared before including blk.h */

#define SD_MEMORY_SHIFT 4                         /* max 16 partitions  */
#define SD_MEMORY_MAXNRDEV 2                      /* max 4 device units */
#define DEVICE_NR(device) (MINOR(device)>>SD_MEMORY_SHIFT)
#define DEVICE_NAME "sd"                      /* name for messaging */

#include <linux/blk.h>
#include <linux/blkpg.h>

#include <asm/danube/infineon_sdio_card.h>
#include <asm/danube/infineon_sdio.h>  
#include <asm/danube/ifx_sd_card.h>        /* local definitions */
#ifdef HAVE_BLKPG_H
#include <linux/blkpg.h>  /* blk_ioctl() */
#endif
 
#define SD_MEMORY_MAJOR 0
static int major    = SD_MEMORY_MAJOR;

MODULE_DESCRIPTION("Infineon SD Memory Card kernel module");
MODULE_AUTHOR("TC.Chen (tai-cheng.chen@infineon.com)");

/* The following items are obtained through kmalloc() in sd_memory_init() */

static IFXSDCard_t *sd_memory_cards;
static int *sd_memory_sizes = NULL;

static int sd_memory_open (struct inode *inode, struct file *filp);
static int sd_memory_release (struct inode *inode, struct file *filp);
static int sd_memory_ioctl (struct inode *inode, struct file *filp,
                     unsigned int cmd, unsigned long arg);
static int sd_memory_revalidate(kdev_t i_rdev);
static int sd_memory_check_change(kdev_t i_rdev);

/*
 * The file operations
 */

static struct block_device_operations sd_memory_bdops =
    {
    open:
        sd_memory_open,
    release:
        sd_memory_release,
    ioctl:
        sd_memory_ioctl,
    revalidate:
        sd_memory_revalidate,
    check_media_change:
        sd_memory_check_change,
    };
    
/*
 * Time for our genhd structure.
 */
static struct gendisk sd_memory_gendisk =
{
    major:
        0,              /* Major number assigned later */
    major_name:         "sd"
        ,           /* Name of the major device */
    minor_shift:
        SD_MEMORY_SHIFT,    /* Shift to get device number */
    max_p:
        1 << SD_MEMORY_SHIFT, /* Number of partitions */
    fops:
        &sd_memory_bdops,   /* Block dev operations */
        /* everything else is dynamic */
};



static struct hd_struct *sd_memory_partitions = NULL;

/**
 * Card Read/Write function
 * This function read/write data from/to sd card
 * 
 * \param  	memory_card	Pointer to the card structure
 * \param  	ptr		Pointer to the data 
 * \param  	addr		Address for reading/writing
 * \param  	size		Number of byte to read/write
 * \param  	mode		read or write
 * \return	Success or failure.  
 * \ingroup	Internal
 */
static int sd_memory_rw(IFXSDCard_t *memory_card, char *ptr, unsigned long addr, int size, int mode)
{
    int ret;
    sd_block_request_t request = { 0 };
    block_data_t *blk_data;
    int nBlock;
    int index;
#if defined(__BIG_ENDIAN_BITFIELD)
    char *data=NULL;
#endif
    nBlock = (size+ (1<<memory_card->block_size_shift)-1) >> memory_card->block_size_shift;
    
    blk_data =(block_data_t *) kmalloc( nBlock * sizeof(block_data_t), GFP_ATOMIC);
    memset(blk_data,0,nBlock*sizeof(block_data_t));

    request.type = mode;
    request.nBlocks = nBlock;
    request.block_size_shift =  memory_card->block_size_shift;;
    request.request_size = nBlock * (1 << memory_card->block_size_shift);;
    
    request.addr = addr;
#if defined(__BIG_ENDIAN_BITFIELD)
    if (mode == SD_WRITE_BLOCK)
    {
	data = kmalloc(size,GFP_ATOMIC);
	if (data == NULL)
	{
		return -ENOMEM;
	}
	for(index=0;index<size/4;index++)
		((uint32_t *)data)[index] = le32_to_cpu(((uint32_t *)ptr)[index]);
	ptr = data;
    }
#endif
    for (index=0;index < nBlock ;index++)
    {
	(blk_data+index)->data = ptr + (index * (1<<memory_card->block_size_shift));
	if (index >0)
		(blk_data+index-1)->next = (blk_data+index);
    }
    (blk_data+index-1)->next=NULL;
    request.blocks = blk_data;
    ret = ifx_sdio_block_transfer(memory_card->card, &request);
    kfree(blk_data);

#if defined(__BIG_ENDIAN_BITFIELD)
    if (ret == 0 && mode == SD_READ_BLOCK)
    {
	for(index=0;index<size/4;index++)
		((uint32_t *)ptr)[index] = le32_to_cpu(((uint32_t *)ptr)[index]);
    }
    if (mode == SD_WRITE_BLOCK)
    {
	kfree(data);
    }
#endif
    return ret;
}

/**
 * Check if the card is still valid(inserted)
 * This function check the card status
 * 
 * \param  	memory_card	Pointer to the card structure
 * \return	0: Valid 
 		1: Invalid. 
 * \ingroup	Internal
 */
static int check_sd_valid(IFXSDCard_t *memory_card)
{
    if (memory_card->load)// still valid
    {
	return 0;
    }
    return 1;
}

/**
 * Open function for linux operation
 * 
 * \param  	inode	Pointer to the inode structure
 * \param  	filp	Pointer to the file structure
 * \return	Success or failure. 
 * \ingroup	Internal
 */
static int sd_memory_open (struct inode *inode, struct file *filp)
{
    IFXSDCard_t *card; /* device information */
    int num = DEVICE_NR(inode->i_rdev);
    if (num >= SD_MEMORY_DEVS || !sd_memory_cards)
        return -ENODEV;
    card = sd_memory_cards + num;
    spin_lock(&card->lock) ;
    if (check_sd_valid(card))
    {
        spin_unlock(&card->lock);
        return -ENOMEM;
    }

    card->usage++;
    MOD_INC_USE_COUNT;
    spin_unlock(&card->lock) ;

    return 0;          /* success */
}

/**
 * Close function for linux operation
 * 
 * \param  	inode	Pointer to the inode structure
 * \param  	filp	Pointer to the file structure
 * \return	Success or failure.
 * \ingroup	Internal
 */
static int sd_memory_release (struct inode *inode, struct file *filp)
{
    IFXSDCard_t *card = NULL;
    if (sd_memory_cards)
    {
    	card = sd_memory_cards + DEVICE_NR(inode->i_rdev);
    	spin_lock(&card->lock);
        card->usage--;
    	spin_unlock(&card->lock);
    }

    MOD_DEC_USE_COUNT;

    return 0;
}


/**
 * IOCTL function for linux operation
 * 
 * \param  	inode	Pointer to the inode structure
 * \param  	filp	Pointer to the file structure
 * \param	cmd	The ioctl command.
 * \param	arg	The address of data.
 * \return	Success or failure.
 * \ingroup	Internal
 */

static int sd_memory_ioctl (struct inode *inode, struct file *filp,
                     unsigned int cmd, unsigned long arg)
{
    int err, size;
    struct hd_geometry geo;
    IFXSDCard_t *memory_card = sd_memory_cards + DEVICE_NR(inode->i_rdev);
    PDEBUG("ioctl 0x%x 0x%lx\n", cmd, arg);
    switch(cmd)
    {

    case BLKGETSIZE:
        /* Return the device size, expressed in sectors */
        err = ! access_ok (VERIFY_WRITE, arg, sizeof(long));
        if (err)
            return -EFAULT;
        size = sd_memory_gendisk.part[MINOR(inode->i_rdev)].nr_sects;
        if (copy_to_user((long *) arg, &size, sizeof (long)))
            return -EFAULT;
        return 0;

    case BLKFLSBUF: /* flush */
        if (! capable(CAP_SYS_RAWIO))
            return -EACCES; /* only root */
        fsync_dev(inode->i_rdev);
        invalidate_buffers(inode->i_rdev);
        return 0;

    case BLKRAGET: /* return the readahead value */
        err = ! access_ok(VERIFY_WRITE, arg, sizeof(long));
        if (err)
            return -EFAULT;
        put_user(read_ahead[MAJOR(inode->i_rdev)],(long *) arg);
        return 0;

    case BLKRASET: /* set the readahead value */
        if (!capable(CAP_SYS_RAWIO))
            return -EACCES;
        if (arg > 0xff)
            return -EINVAL; /* limit it */
        read_ahead[MAJOR(inode->i_rdev)] = arg;
        return 0;

    case BLKRRPART: /* re-read partition table */
        return sd_memory_revalidate(inode->i_rdev);

    case HDIO_GETGEO:
        /*
         * get geometry: we have to fake one...  trim the size to a
         * multiple of 64 (32k): tell we have 16 sectors, 4 heads,
         * whatever cylinders. Tell also that data starts at sector. 4.
         */

        err = ! access_ok(VERIFY_WRITE, arg, sizeof(geo));
        if (err)
            return -EFAULT;
        size = memory_card->nBlock ;
        geo.cylinders = (size & ~0x3f) >> 6;
        geo.heads = 4;
        geo.sectors = 16;
        geo.start = sd_memory_partitions[MINOR(inode->i_rdev)].start_sect;
        if (copy_to_user((void *) arg, &geo, sizeof(geo)))
            return -EFAULT;

        return 0;

    default:
        /*
         * For ioctls we don't understand, let the block layer handle them.
         */
        return blk_ioctl(inode->i_rdev, cmd, arg);
    }

    return -ENOTTY; /* unknown command */
}

/**
 * IOCTL function for linux operation
 * 
 * \param  	inode	Pointer to the inode structure
 * \param  	filp	Pointer to the file structure
 * \param	cmd	The ioctl command.
 * \param	arg	The address of data.
 * \return	Success or failure.
 * \ingroup	Internal
 */
static int sd_memory_check_change(kdev_t i_rdev)
{
    int minor = DEVICE_NR(i_rdev);
    IFXSDCard_t *card;
    
    if (minor >= SD_MEMORY_DEVS || !sd_memory_cards) /* paranoid */
        return 0;
    card = sd_memory_cards + minor;
    if (check_sd_valid(card))
        return 0; /* expired */
    return 1; /* valid */
}

/**
 * Revalidate function for linux operation
 *
 * \param  	i_rdev	
 * \return	Success or failure.
 * \ingroup	Internal
 */

static int sd_memory_revalidate(kdev_t i_rdev)
{
    int minor = DEVICE_NR(i_rdev);
    IFXSDCard_t *memory_card = sd_memory_cards + minor;
    unsigned long size ;

    if(sd_memory_cards && memory_card->nBlock )
    {
	size = memory_card->nBlock;
    }else
    {
	size = 0;
    }
    /* then fill new info */
    register_disk(&sd_memory_gendisk, i_rdev, SD_MEMORY_MAXNRDEV, &sd_memory_bdops, size);
    return 0;
}

/*
 * Block-driver specific functions
 */

/**
 * Find the device for this request.
 *
 * \param  	req	Pointer to the request structure
 * \return	NULL if not found or Pointer to the IFXSDCard_t structure
 * \ingroup	Internal
 */
static IFXSDCard_t *sd_memory_locate_device(const struct request *req)
{
    int devno;
    IFXSDCard_t *card;
    /* Check if the minor number is in range */
    
    devno = DEVICE_NR(req->rq_dev);
    if (devno >= SD_MEMORY_DEVS)
    {
        static int count = 0;
        if (count++ < 5) /* print the message at most five times */
            printk(KERN_WARNING "sd_memory: request for unknown device\n");
        return NULL;
    }
    if (!sd_memory_cards)
    	return NULL;
    card = sd_memory_cards + devno;
    return card;
}


/**
 * Perform an actual transfer.
 *
 * \param  	memory_card	Pointer to the IFXSDCard_t structure
 * \param  	req		Pointer to the request structure
 * \return	Success or failure.
 * \ingroup	Internal
 */ 

static int sd_memory_transfer(IFXSDCard_t *memory_card, const struct request *req)
{
    int size, minor = MINOR(req->rq_dev);
    unsigned long sd_addr;
    int ret=0;

    sd_addr = (sd_memory_partitions[minor].start_sect + req->sector)*SD_MEMORY_HARDSECT;
    size = req->current_nr_sectors * (1 << memory_card->block_size_shift);

    /*
     * Make sure that the transfer fits within the device.
     */
    if (req->sector + req->current_nr_sectors >
            sd_memory_partitions[minor].nr_sects)
    {
        static int count = 0;
        if (count++ < 5)
            printk(KERN_WARNING "sd_memory: request past end of partition\n");
        return -EFAULT;
    }
    /*
     * Looks good, do the transfer.
     */

    switch(req->cmd)
    {
    case READ:
       	ret = sd_memory_rw(memory_card,req->buffer,sd_addr,size,SD_READ_BLOCK);
        break;
    case WRITE:
        ret = sd_memory_rw(memory_card,req->buffer,sd_addr,size,SD_WRITE_BLOCK);
        break;
    default:
	printk("%s unknown command\n",__FUNCTION__);
        /* can't happen */
        return -EIO;
    }

    return ret;
}

static void sd_memory_request(request_queue_t *q)
{
    IFXSDCard_t *card;
    int status;
    long flags;
    while(1)
    {
        INIT_REQUEST;  /* returns when queue is empty */
        /* Which "device" are we using?  (Is returned locked) */
        card = sd_memory_locate_device (CURRENT);
        if (card == NULL)
        {
            end_request(0);
            continue;
        }
        spin_lock_irqsave(&card->lock, flags);

        /* Perform the transfer and clean up. */
        status = sd_memory_transfer(card, CURRENT);
        spin_unlock_irqrestore(&card->lock, flags);
	if (status == 0)
        	end_request(1); /* success */
	else
        	end_request(0); /* fail */
    }
}


static int sd_memory_load(sdio_card_t *card)
{

    IFXSDCard_t *memory_card; /* device information */
    int slot=0,free_slot=-1;
	
	
    for (slot=0;slot<SD_MEMORY_DEVS;slot++)
    {
    	memory_card = sd_memory_cards + slot;
    	if (memory_card->card  == card)
		break;
	if (free_slot<0 && memory_card->card == NULL)
	{
		free_slot = slot;
	}
    }
   
    if (slot == SD_MEMORY_DEVS)
    {
	if (free_slot < 0 )
        	return -ENODEV;
    	memory_card = sd_memory_cards + free_slot;
	slot = free_slot;
    }
    
    if (memory_card->load)
    {
	printk("Driver has been load on slot %d yet!",slot);
	return -EBUSY;
    }
    memory_card->card = card;
    memory_card->load = 1;
    memory_card->block_size_shift = card->csd.read_bl_len;
    memory_card->nBlock = (1 << (card->csd.c_size_mult+2)) * (card->csd.c_size+1);

    sd_memory_revalidate(MKDEV(major,slot<<SD_MEMORY_SHIFT));

#if 0
    /* dump the partition table to see it */
    for (i=0; i < SD_MEMORY_DEVS << SD_MEMORY_SHIFT; i++)
        printk("part %i: beg %lx, size %lx\n", i,
               sd_memory_partitions[i].start_sect,
               sd_memory_partitions[i].nr_sects);

    printk ("<1>sd_memory: init complete, %d devs, size %d blks %d\n",
            SD_MEMORY_DEVS, memory_card->nBlock, 1<<memory_card->block_size_shift);
#endif

    return 0;
}

static int sd_memory_unload(sdio_card_t *card)
{

    IFXSDCard_t *memory_card; /* device information */
    int i;
    int slot =0;
    
    for (slot=0;slot<SD_MEMORY_DEVS;slot++)
    {
    	memory_card = sd_memory_cards + slot;
    	if (memory_card->card  == card)
		break;
    }

    if (slot == SD_MEMORY_DEVS || !memory_card->load)
    {
	printk("Driver did not load on slot %d!",slot);
	return -ENODEV;
    }


    for ( i = slot<<SD_MEMORY_SHIFT ; i <(slot+1)<<SD_MEMORY_SHIFT ; i++ ) {
	invalidate_device(MKDEV(major,i),1);
	sd_memory_gendisk.part[i].start_sect = 0;
	sd_memory_gendisk.part[i].nr_sects   = 0;
    }
    devfs_register_partitions(&sd_memory_gendisk, slot<<SD_MEMORY_SHIFT, 1);
    memory_card->card = NULL;
    memory_card->load = 0;
    memory_card->block_size_shift = 0;
    memory_card->nBlock = 0;

    sd_memory_revalidate(MKDEV(major,slot<<SD_MEMORY_SHIFT));
    return 0;
}

static int sd_memory_probe(void)
{
    int result, i;
    IFXSDCard_t *memory_card;
    /*
     * Copy the (static) cfg variables to public prefixed ones to allow
     * snoozing with a debugger.
     */

    if (sd_memory_cards != NULL)// already probe
    	return 0; 
    
    sd_memory_major    = major;

    /*
     * Register your major, and accept a dynamic number
     */
#ifdef CONFIG_DEVFS_FS
    if ((result = devfs_register_blkdev(sd_memory_major, "ifx_sd", &sd_memory_bdops))) {
       	printk(KERN_WARNING "sd_memory: can't get major %d,result=%d\n",sd_memory_major,result);
	return -EIO;
    }
#else
    result = register_blkdev(sd_memory_major, "ifx_sd", &sd_memory_bdops);
    if (result < 0)
    {
       	printk(KERN_WARNING "sd_memory: can't get major %d,result=%d\n",sd_memory_major,result);
        return result;
    }
#endif
    if (sd_memory_major == 0)
        sd_memory_major = result; /* dynamic */
    major = sd_memory_major; /* Use `major' later on to save typing */
    sd_memory_gendisk.major = major; /* was unknown at load time */

    /*
     * allocate the devices -- we can't have them static, as the number
     * can be specified at load time
     */

    sd_memory_cards = kmalloc(SD_MEMORY_DEVS * sizeof (IFXSDCard_t), GFP_ATOMIC);
    if (!sd_memory_cards)
        goto fail_malloc;
    memset(sd_memory_cards, 0, SD_MEMORY_DEVS * sizeof (IFXSDCard_t));
    for (i=0; i < SD_MEMORY_DEVS; i++)
    {
        /* data and usage remain zeroed */
        memory_card = sd_memory_cards + i;
	 //memory_card->card->size = 0;
        spin_lock_init(&memory_card.lock);
    }
    /*
     * Assign the other needed values: request, rahead, size, blksize,
     * hardsect. All the minor devices feature the same value.
     * Note that `sd_memory' defines all of them to allow testing non-default
     * values. A real device could well avoid setting values in global
     * arrays if it uses the default values.
     */

    read_ahead[major] = SD_MEMORY_RAHEAD;
    result = -ENOMEM; /* for the possible errors */

    sd_memory_sizes = kmalloc( (SD_MEMORY_DEVS << SD_MEMORY_SHIFT) * sizeof(int),
                               GFP_ATOMIC);
    if (!sd_memory_sizes)
        goto fail_malloc;
    /* Start with zero-sized partitions, and correctly sized units */
    memset(sd_memory_sizes, 0, (SD_MEMORY_DEVS << SD_MEMORY_SHIFT) * sizeof(int));
    for (i=0; i< SD_MEMORY_DEVS; i++)
	sd_memory_sizes[i<<SD_MEMORY_SHIFT] = 0;

    blk_size[MAJOR_NR] = sd_memory_gendisk.sizes = sd_memory_sizes;

    /* Allocate the partitions array. */
    sd_memory_partitions = kmalloc( (SD_MEMORY_DEVS << SD_MEMORY_SHIFT) *
                                    sizeof(struct hd_struct), GFP_ATOMIC);
    if (!sd_memory_partitions)
        goto fail_malloc;
    memset(sd_memory_partitions, 0, (SD_MEMORY_DEVS << SD_MEMORY_SHIFT) *
           sizeof(struct hd_struct));

    sd_memory_gendisk.part = sd_memory_partitions;
    sd_memory_gendisk.nr_real = SD_MEMORY_DEVS;
    /*
     * Put our gendisk structure on the list.
     */
    add_gendisk(&sd_memory_gendisk);

    blk_init_queue(BLK_DEFAULT_QUEUE(major), sd_memory_request);
    EXPORT_NO_SYMBOLS; /* otherwise, leave global symbols visible */

    return 0; /* succeed */

fail_malloc:
    read_ahead[major] = 0;
    if (sd_memory_sizes)
    {
        kfree(sd_memory_sizes);
        sd_memory_sizes = NULL;
    }
    if (sd_memory_partitions)
    {
        kfree(sd_memory_partitions);
        sd_memory_partitions = NULL;
    }
    blk_size[major] = NULL;
    if (sd_memory_cards)
    {
        kfree(sd_memory_cards);
        sd_memory_cards = NULL;
    }
#ifdef CONFIG_DEVFS_FS
    devfs_unregister_blkdev(major, "ifx_sd");
#else
    unregister_blkdev(major, "ifx_sd");
#endif    
    return result;
}

static void sd_memory_cleanup(void)
{
    int i;
    struct gendisk **gdp;
    IFXSDCard_t *memory_card;

    for (i=0;i<SD_MEMORY_DEVS;i++)
    {
    	memory_card = sd_memory_cards + i;

	if (memory_card->card)
		return;
    }

    for (i = 0; i < SD_MEMORY_DEVS; i++)
    {
        memory_card = sd_memory_cards + i;
        spin_lock(&memory_card->lock);
        memory_card->usage++;
        spin_unlock(&memory_card->lock);
    }

    /* flush it all and reset all the data structures */
    /*
     * Unregister the device now to avoid further operations during cleanup.
     */
#ifdef CONFIG_DEVFS_FS
    //devfs_register_partitions(&sd_memory_gendisk, 0, 1);
    
    if (devfs_unregister_blkdev(major, "ifx_sd")) {
       	printk(KERN_WARNING "sd_memory: can't unregister blkdev major %d\n",major);
    }
#else
    unregister_blkdev(major, "ifx_sd");
#endif    
    for (i = 0; i < (SD_MEMORY_DEVS << SD_MEMORY_SHIFT); i++)
        fsync_dev(MKDEV(major, i)); /* flush the devices */
    blk_cleanup_queue(BLK_DEFAULT_QUEUE(major));
    read_ahead[major] = 0;
    kfree(blk_size[major]); /* which is gendisk->sizes as well */
    blk_size[major] = NULL;
    kfree(sd_memory_gendisk.part);
    kfree(blksize_size[major]);
    blksize_size[major] = NULL;

    /*
     * Get our gendisk structure off the list.
     */
    for (gdp = &gendisk_head; *gdp; gdp = &((*gdp)->next))
        if (*gdp == &sd_memory_gendisk)
        {
            *gdp = (*gdp)->next;
            break;
        }
   kfree(sd_memory_cards);
   sd_memory_cards = NULL;
   sync_inodes(MKDEV(major,0<<SD_MEMORY_SHIFT));
   sync_unlocked_inodes();

}


static sdio_card_driver_t sd_memory_card_driver = 
{
 .probe = sd_memory_probe,
 .insert = sd_memory_load,
 .eject = sd_memory_unload,
 .remove = sd_memory_cleanup,
 .type = SD_TYPE,
 .name = "Infineon SD Card Driver",
};

/**
 * Init function 
 * 
 * \return	Success or failure.
 * \ingroup	Internal
 */
static int sd_memorycard_init_module(void)
{
    printk( KERN_INFO "Infineon SD card driver version %s\n",IFX_SD_VERSION);
    register_sd_card_driver(&sd_memory_card_driver);
    return 0;
}

/**
 * Destory function 
 * 
 * \ingroup	Internal
 */
static void sd_memorycard_exit_module(void)
{
    unregister_sd_card_driver(&sd_memory_card_driver);
    sd_memory_cleanup();
}

module_init(sd_memorycard_init_module);
module_exit(sd_memorycard_exit_module);
