/*
 *  drivers/mtd/nand.c
 *
 *  Overview:
 *   This is the generic MTD driver for NAND flash devices. It should be
 *   capable of working with almost all NAND chips currently available.
 *   Basic support for AG-AND chips is provided.
 *   
 *	Additional technical information is available on
 *	http://www.linux-mtd.infradead.org/tech/nand.html
 *	
 *  Copyright (C) 2000 Steven J. Hill (sjhill@realitydiluted.com)
 * 		  2002 Thomas Gleixner (tglx@linutronix.de)
 *
 *  02-08-2004  tglx: support for strange chips, which cannot auto increment 
 *		pages on read / read_oob
 *
 *  03-17-2004  tglx: Check ready before auto increment check. Simon Bayes
 *		pointed this out, as he marked an auto increment capable chip
 *		as NOAUTOINCR in the board driver.
 *		Make reads over block boundaries work too
 *
 *  04-14-2004	tglx: first working version for 2k page size chips
 *  
 *  05-19-2004  tglx: Basic support for Renesas AG-AND chips
 *
 * Credits:
 *	David Woodhouse for adding multichip support  
 *	
 *	Aleph One Ltd. and Toby Churchill Ltd. for supporting the
 *	rework for 2K page size chips
 *
 * TODO:
 *	Enable cached programming for 2k page size chips
 *	Check, if mtd->ecctype should be set to MTD_ECC_HW
 *	if we have HW ecc support.
 *	The AG-AND chips have nice features for speed improvement,
 *	which are not supported yet. Read / program 4 pages in one go.
 *
 * $Id: nand.c,v 1.86 2004/05/19 20:17:40 gleixner Exp $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/mtd/compatmac.h>
#include <linux/interrupt.h>
#include <linux/bitops.h>
#include <asm/io.h>

/* Define default oob placement schemes for large and small page devices */
static struct nand_oobinfo nand_oob_8 = {
	.useecc = MTD_NANDECC_AUTOPLACE,
	.eccbytes = 3,
	.eccpos = {0, 1, 2},
	.oobfree = { {3, 2}, {6, 2} }
};

static struct nand_oobinfo nand_oob_16 = {
	.useecc = MTD_NANDECC_AUTOPLACE,
	.eccbytes = 6,
	.eccpos = {0, 1, 2, 3, 6, 7},
	.oobfree = { {8, 8} }
};

static struct nand_oobinfo nand_oob_64 = {
	.useecc = MTD_NANDECC_AUTOPLACE,
	.eccbytes = 24,
	.eccpos = {
		40, 41, 42, 43, 44, 45, 46, 47, 
		48, 49, 50, 51, 52, 53, 54, 55, 
		56, 57, 58, 59, 60, 61, 62, 63},
	.oobfree = { {2, 38} }
};

/* This is used for padding purposes in nand_write_oob */
static u_char ffchars[] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};

/*
 * NAND low-level MTD interface functions
 */
static void nand_write_buf(struct mtd_info *mtd, const u_char *buf, int len);
static void nand_read_buf(struct mtd_info *mtd, u_char *buf, int len);
static int nand_verify_buf(struct mtd_info *mtd, const u_char *buf, int len);

static int nand_read (struct mtd_info *mtd, loff_t from, size_t len, size_t * retlen, u_char * buf);
static int nand_read_ecc (struct mtd_info *mtd, loff_t from, size_t len,
			  size_t * retlen, u_char * buf, u_char * eccbuf, struct nand_oobinfo *oobsel);
static int nand_read_oob (struct mtd_info *mtd, loff_t from, size_t len, size_t * retlen, u_char * buf);
static int nand_write (struct mtd_info *mtd, loff_t to, size_t len, size_t * retlen, const u_char * buf);
static int nand_write_ecc (struct mtd_info *mtd, loff_t to, size_t len,
			   size_t * retlen, const u_char * buf, u_char * eccbuf, struct nand_oobinfo *oobsel);
static int nand_write_oob (struct mtd_info *mtd, loff_t to, size_t len, size_t * retlen, const u_char *buf);
static int nand_writev (struct mtd_info *mtd, const struct iovec *vecs,
			unsigned long count, loff_t to, size_t * retlen);
static int nand_writev_ecc (struct mtd_info *mtd, const struct iovec *vecs,
			unsigned long count, loff_t to, size_t * retlen, u_char *eccbuf, struct nand_oobinfo *oobsel);
static int nand_erase (struct mtd_info *mtd, struct erase_info *instr);
static void nand_sync (struct mtd_info *mtd);

/* Some internal functions */
static int nand_write_page (struct mtd_info *mtd, struct nand_chip *this, int page, u_char *oob_buf,
		struct nand_oobinfo *oobsel, int mode);
#ifdef CONFIG_MTD_NAND_VERIFY_WRITE
static int nand_verify_pages (struct mtd_info *mtd, struct nand_chip *this, int page, int numpages, 
	u_char *oob_buf, struct nand_oobinfo *oobsel, int chipnr, int oobmode);
#else
#define nand_verify_pages(...) (0)
#endif
		
static void nand_get_chip (struct nand_chip *this, struct mtd_info *mtd, int new_state, int *erase_state);

/* Deselect and wake up anyone waiting on the device */
static void nand_release_chip (struct mtd_info *mtd)
{
	struct nand_chip *this = mtd->priv;

	/* De-select the NAND device */
	this->select_chip(mtd, -1);
	/* Release the chip */
	spin_lock_bh (&this->chip_lock);
	this->state = FL_READY;
	wake_up (&this->wq);
	spin_unlock_bh (&this->chip_lock);
}

static u_char nand_read_byte(struct mtd_info *mtd)
{
	struct nand_chip *this = mtd->priv;
	return readb(this->IO_ADDR_R);
}

static void nand_write_byte(struct mtd_info *mtd, u_char byte)
{
	struct nand_chip *this = mtd->priv;
	writeb(byte, this->IO_ADDR_W);
}

static u_char nand_read_byte16(struct mtd_info *mtd)
{
	struct nand_chip *this = mtd->priv;
	return (u_char) cpu_to_le16(readw(this->IO_ADDR_R));
}

static void nand_write_byte16(struct mtd_info *mtd, u_char byte)
{
	struct nand_chip *this = mtd->priv;
	writew(le16_to_cpu((u16) byte), this->IO_ADDR_W);
}

static u16 nand_read_word(struct mtd_info *mtd)
{
	struct nand_chip *this = mtd->priv;
	return readw(this->IO_ADDR_R);
}

static void nand_write_word(struct mtd_info *mtd, u16 word)
{
	struct nand_chip *this = mtd->priv;
	writew(word, this->IO_ADDR_W);
}

static void nand_select_chip(struct mtd_info *mtd, int chip)
{
	struct nand_chip *this = mtd->priv;
	switch(chip) {
	case -1:
		this->hwcontrol(mtd, NAND_CTL_CLRNCE);	
		break;
	case 0:
		this->hwcontrol(mtd, NAND_CTL_SETNCE);
		break;

	default:
		BUG();
	}
}

static void nand_write_buf(struct mtd_info *mtd, const u_char *buf, int len)
{
	int i;
	struct nand_chip *this = mtd->priv;

	for (i=0; i<len; i++)
		writeb(buf[i], this->IO_ADDR_W);
}

static void nand_read_buf(struct mtd_info *mtd, u_char *buf, int len)
{
	int i;
	struct nand_chip *this = mtd->priv;

	for (i=0; i<len; i++)
		buf[i] = readb(this->IO_ADDR_R);
}

static int nand_verify_buf(struct mtd_info *mtd, const u_char *buf, int len)
{
	int i;
	struct nand_chip *this = mtd->priv;

	for (i=0; i<len; i++)
		if (buf[i] != readb(this->IO_ADDR_R))
			return -EFAULT;

	return 0;
}

static void nand_write_buf16(struct mtd_info *mtd, const u_char *buf, int len)
{
	int i;
	struct nand_chip *this = mtd->priv;
	u16 *p = (u16 *) buf;
	len >>= 1;
	
	for (i=0; i<len; i++)
		writew(p[i], this->IO_ADDR_W);
		
}

static void nand_read_buf16(struct mtd_info *mtd, u_char *buf, int len)
{
	int i;
	struct nand_chip *this = mtd->priv;
	u16 *p = (u16 *) buf;
	len >>= 1;

	for (i=0; i<len; i++)
		p[i] = readw(this->IO_ADDR_R);
}

static int nand_verify_buf16(struct mtd_info *mtd, const u_char *buf, int len)
{
	int i;
	struct nand_chip *this = mtd->priv;
	u16 *p = (u16 *) buf;
	len >>= 1;

	for (i=0; i<len; i++)
		if (p[i] != readw(this->IO_ADDR_R))
			return -EFAULT;

	return 0;
}

static int nand_block_bad(struct mtd_info *mtd, loff_t ofs, int getchip)
{
	int page, chipnr, res = 0;
	struct nand_chip *this = mtd->priv;
	u16 bad;

	if (getchip) {
		/* Shift to get page */
		page = ((int) ofs) >> this->page_shift;
		chipnr = (int)((unsigned long) ofs / this->chipsize);

		/* Grab the lock and see if the device is available */
		nand_get_chip (this, mtd, FL_READING, NULL);

		/* Select the NAND device */
		this->select_chip(mtd, chipnr);
	} else 
		page = (int) ofs;	

	if (this->options & NAND_BUSWIDTH_16) {
		this->cmdfunc (mtd, NAND_CMD_READOOB, this->badblockpos & 0xFE, page & this->pagemask);
		bad = cpu_to_le16(this->read_word(mtd));
		if (this->badblockpos & 0x1)
			bad >>= 1;
		if ((bad & 0xFF) != 0xff)
			res = 1;
	} else {
		this->cmdfunc (mtd, NAND_CMD_READOOB, this->badblockpos, page & this->pagemask);
		if (this->read_byte(mtd) != 0xff)
			res = 1;
	}
		
	if (getchip) {
		/* Deselect and wake up anyone waiting on the device */
		nand_release_chip(mtd);
	}	
	
	return res;
}

/* This is the default implementation, which can be overridden by
 * a hardware specific driver.
*/
static int nand_default_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
	struct nand_chip *this = mtd->priv;
	u_char buf[2] = {0, 0};
	size_t	retlen;
	
	ofs += mtd->oobsize + (this->badblockpos & ~0x01);
	
	/* We write two bytes, so we dont have to mess with 16 bit access */
	return nand_write_oob (mtd, ofs , 2, &retlen, buf);
}

/* Check, if the device is write protected 
 * The function expects, that the device is already selected 
 */
static int nand_check_wp (struct mtd_info *mtd)
{
	struct nand_chip *this = mtd->priv;
	/* Check the WP bit */
	this->cmdfunc (mtd, NAND_CMD_STATUS, -1, -1);
	return (this->read_byte(mtd) & 0x80) ? 0 : 1; 
}

/*
 * Send command to NAND device
 */
static void nand_command (struct mtd_info *mtd, unsigned command, int column, int page_addr)
{
	register struct nand_chip *this = mtd->priv;

	/* Adjust columns for 16 bit buswidth */
	if (this->options & NAND_BUSWIDTH_16)
		column >>= 1;

	/* Begin command latch cycle */
	this->hwcontrol(mtd, NAND_CTL_SETCLE);
	/*
	 * Write out the command to the device.
	 */
	if (command == NAND_CMD_SEQIN) {
		int readcmd;

		if (column >= mtd->oobblock) {
			/* OOB area */
			column -= mtd->oobblock;
			readcmd = NAND_CMD_READOOB;
		} else if (column < 256) {
			/* First 256 bytes --> READ0 */
			readcmd = NAND_CMD_READ0;
		} else {
			column -= 256;
			readcmd = NAND_CMD_READ1;
		}
		this->write_byte(mtd, readcmd);
	}
	this->write_byte(mtd, command);

	/* Set ALE and clear CLE to start address cycle */
	this->hwcontrol(mtd, NAND_CTL_CLRCLE);

	if (column != -1 || page_addr != -1) {
		this->hwcontrol(mtd, NAND_CTL_SETALE);

		/* Serially input address */
		if (column != -1)
			this->write_byte(mtd, column);
		if (page_addr != -1) {
			this->write_byte(mtd, (unsigned char) (page_addr & 0xff));
			this->write_byte(mtd, (unsigned char) ((page_addr >> 8) & 0xff));
			/* One more address cycle for higher density devices */
			if (this->chipsize & 0x0c000000) 
				this->write_byte(mtd, (unsigned char) ((page_addr >> 16) & 0x0f));
		}
		/* Latch in address */
		this->hwcontrol(mtd, NAND_CTL_CLRALE);
	}
	
	/* 
	 * program and erase have their own busy handlers 
	 * status and sequential in needs no delay
	*/
	switch (command) {
			
	case NAND_CMD_PAGEPROG:
	case NAND_CMD_ERASE1:
	case NAND_CMD_ERASE2:
	case NAND_CMD_SEQIN:
	case NAND_CMD_STATUS:
		return;

	case NAND_CMD_RESET:
		if (this->dev_ready)	
			break;
		udelay(this->chip_delay);
		this->hwcontrol(mtd, NAND_CTL_SETCLE);
		this->write_byte(mtd, NAND_CMD_STATUS);
		this->hwcontrol(mtd, NAND_CTL_CLRCLE);
		while ( !(this->read_byte(mtd) & 0x40));
		return;

	/* This applies to read commands */	
	default:
		/* 
		 * If we don't have access to the busy pin, we apply the given
		 * command delay
		*/
		if (!this->dev_ready) {
			udelay (this->chip_delay);
			return;
		}	
	}
	
	/* wait until command is processed */
	while (!this->dev_ready(mtd));
}

/*
 * Send command to NAND device. This is the version for the new large page devices
 * We dont have the seperate regions as we have in the small page devices.
 * We must emulate NAND_CMD_READOOB to keep the code compatible
 */
static void nand_command_lp (struct mtd_info *mtd, unsigned command, int column, int page_addr)
{
	register struct nand_chip *this = mtd->priv;

	/* Emulate NAND_CMD_READOOB */
	if (command == NAND_CMD_READOOB) {
		column += mtd->oobblock;
		command = NAND_CMD_READ0;
	}
	
	/* Adjust columns for 16 bit buswidth */
	if (this->options & NAND_BUSWIDTH_16)
		column >>= 1;
		
	/* Begin command latch cycle */
	this->hwcontrol(mtd, NAND_CTL_SETCLE);
	/* Write out the command to the device. */
	this->write_byte(mtd, command);
	/* End command latch cycle */
	this->hwcontrol(mtd, NAND_CTL_CLRCLE);

	if (column != -1 || page_addr != -1) {
		this->hwcontrol(mtd, NAND_CTL_SETALE);

		/* Serially input address */
		if (column != -1) {
			this->write_byte(mtd, column & 0xff);
			this->write_byte(mtd, column >> 8);
		}	
		if (page_addr != -1) {
			this->write_byte(mtd, (unsigned char) (page_addr & 0xff));
			this->write_byte(mtd, (unsigned char) ((page_addr >> 8) & 0xff));
			/* One more address cycle for devices > 128MiB */
			if (this->chipsize > (128 << 20))
				this->write_byte(mtd, (unsigned char) ((page_addr >> 16) & 0xff));
		}
		/* Latch in address */
		this->hwcontrol(mtd, NAND_CTL_CLRALE);
	}
	
	/* 
	 * program and erase have their own busy handlers 
	 * status and sequential in needs no delay
	*/
	switch (command) {
			
	case NAND_CMD_CACHEDPROG:
	case NAND_CMD_PAGEPROG:
	case NAND_CMD_ERASE1:
	case NAND_CMD_ERASE2:
	case NAND_CMD_SEQIN:
	case NAND_CMD_STATUS:
		return;


	case NAND_CMD_RESET:
		if (this->dev_ready)	
			break;
		udelay(this->chip_delay);
		this->hwcontrol(mtd, NAND_CTL_SETCLE);
		this->write_byte(mtd, NAND_CMD_STATUS);
		this->hwcontrol(mtd, NAND_CTL_CLRCLE);
		while ( !(this->read_byte(mtd) & 0x40));
		return;

	case NAND_CMD_READ0:
		/* Begin command latch cycle */
		this->hwcontrol(mtd, NAND_CTL_SETCLE);
		/* Write out the start read command */
		this->write_byte(mtd, NAND_CMD_READSTART);
		/* End command latch cycle */
		this->hwcontrol(mtd, NAND_CTL_CLRCLE);
		/* Fall through into ready check */
		
	/* This applies to read commands */	
	default:
		/* 
		 * If we don't have access to the busy pin, we apply the given
		 * command delay
		*/
		if (!this->dev_ready) {
			udelay (this->chip_delay);
			return;
		}	
	}
	
	/* wait until command is processed */
	while (!this->dev_ready(mtd));
}

/*
 *	Get chip for selected access
 */
static void nand_get_chip (struct nand_chip *this, struct mtd_info *mtd, int new_state, int *erase_state)
{

	DECLARE_WAITQUEUE (wait, current);

	/* 
	 * Grab the lock and see if the device is available 
	*/
retry:
	spin_lock_bh (&this->chip_lock);

	if (this->state == FL_READY) {
		this->state = new_state;
		spin_unlock_bh (&this->chip_lock);
		return;
	}

	set_current_state (TASK_UNINTERRUPTIBLE);
	add_wait_queue (&this->wq, &wait);
	spin_unlock_bh (&this->chip_lock);
	schedule ();
	remove_wait_queue (&this->wq, &wait);
	goto retry;
}

/*
 * Wait for command done. This applies to erase and program only
 * Erase can take up to 400ms and program up to 20ms according to 
 * general NAND and SmartMedia specs
 *
 * @mtd:	MTD device structure
 * @this:	NAND chip structure
 * @state:	state to select the max. timeout value
 *
*/
static int nand_wait(struct mtd_info *mtd, struct nand_chip *this, int state)
{

	unsigned long	timeo = jiffies;
	int	status;
	
	if (state == FL_ERASING)
		 timeo += (HZ * 400) / 1000;
	else
		 timeo += (HZ * 20) / 1000;

	spin_lock_bh (&this->chip_lock);
	this->cmdfunc (mtd, NAND_CMD_STATUS, -1, -1);

	while (time_before(jiffies, timeo)) {		
		/* Check, if we were interrupted */
		if (this->state != state) {
			spin_unlock_bh (&this->chip_lock);
			return 0;
		}
		if (this->dev_ready) {
			if (this->dev_ready(mtd))
				break;
		}
		if (this->read_byte(mtd) & 0x40)
			break;
						
		spin_unlock_bh (&this->chip_lock);
		yield ();
		spin_lock_bh (&this->chip_lock);
	}
	status = (int) this->read_byte(mtd);
	spin_unlock_bh (&this->chip_lock);

	return status;
}

/*
 *	Nand_page_program function is used for write and writev !
 *	This function will always program a full page of data
 *	If you call it with a non page aligned buffer, you're lost :)
 *
 * @mtd:	MTD device structure
 * @this:	NAND chip structure
 * @page: 	startpage inside the chip, must be called with (page & this->pagemask)
 * @oob_buf:	out of band data buffer
 * @oobsel:	out of band selecttion structre
 * @cached:	1 = enable cached programming if supported by chip
 */
static int nand_write_page (struct mtd_info *mtd, struct nand_chip *this, int page, 
	u_char *oob_buf,  struct nand_oobinfo *oobsel, int cached)
{
	int 	i, status;
	u_char	ecc_code[6];
	int	eccmode = oobsel->useecc ? this->eccmode : NAND_ECC_NONE;
	int  	*oob_config = oobsel->eccpos;
	int	datidx = 0, eccidx = 0, eccsteps = this->eccsteps;
	
	/* FIXME: Enable cached programming */
	cached = 0;
	
	/* Send command to begin auto page programming */
	this->cmdfunc (mtd, NAND_CMD_SEQIN, 0x00, page);

	/* Write out complete page of data, take care of eccmode */
	switch (eccmode) {
	/* No ecc, write all */
	case NAND_ECC_NONE:
		printk (KERN_WARNING "Writing data without ECC to NAND-FLASH is not recommended\n");
		this->write_buf(mtd, this->data_poi, mtd->oobblock);
		break;
		
	/* Software ecc 3/256, write all */
	case NAND_ECC_SOFT:
		for (; eccsteps; eccsteps--) {
			this->calculate_ecc(mtd, &this->data_poi[datidx], ecc_code);
			for (i = 0; i < 3; i++, eccidx++)
				oob_buf[oob_config[eccidx]] = ecc_code[i];
			datidx += this->eccsize;
		}
		this->write_buf(mtd, this->data_poi, mtd->oobblock);
		break;
		
	/* Hardware ecc 3 byte / 256 data, write 256 byte */	
	/* Hardware ecc 3 byte / 512 byte data, write 512 bytee */	
	case NAND_ECC_HW3_256:		
	case NAND_ECC_HW3_512:
		for (; eccsteps; eccsteps--) {
			this->enable_hwecc(mtd, NAND_ECC_WRITE);	/* enable hardware ecc logic for write */
			this->write_buf(mtd, &this->data_poi[datidx], this->eccsize);
			this->calculate_ecc(mtd, NULL, ecc_code);
			for (i = 0; i < 3; i++, eccidx++)
				oob_buf[oob_config[eccidx]] = ecc_code[i];
			datidx += this->eccsize;
		}
		break;
				
	/* Hardware ecc 6 byte / 512 byte data, write 512 byte */	
	case NAND_ECC_HW6_512:	
		for (; eccsteps; eccsteps--) {
			this->enable_hwecc(mtd, NAND_ECC_WRITE);	/* enable hardware ecc logic for write */
			this->write_buf(mtd, &this->data_poi[datidx], this->eccsize);
			this->calculate_ecc(mtd, NULL, ecc_code);
			for (i = 0; i < 6; i++, eccidx++)
				oob_buf[oob_config[eccidx]] = ecc_code[i];
			datidx += this->eccsize;
		}
		break;
		
	default:
		printk (KERN_WARNING "Invalid NAND_ECC_MODE %d\n", this->eccmode);
		BUG();	
	}
										
	/* Write out OOB data */
	this->write_buf(mtd, oob_buf, mtd->oobsize);

	/* Send command to actually program the data */
	this->cmdfunc (mtd, cached ? NAND_CMD_CACHEDPROG : NAND_CMD_PAGEPROG, -1, -1);

	if (!cached) {
		/* call wait ready function */
		status = this->waitfunc (mtd, this, FL_WRITING);
		/* See if device thinks it succeeded */
		if (status & 0x01) {
			DEBUG (MTD_DEBUG_LEVEL0, "%s: " "Failed write, page 0x%08x, ", __FUNCTION__, page);
			return -EIO;
		}
	} else {
		/* FIXME: Implement cached programming ! */
		/* wait until cache is ready*/
		// status = this->waitfunc (mtd, this, FL_CACHEDRPG);
	}
	return 0;	
}

#ifdef CONFIG_MTD_NAND_VERIFY_WRITE
/*
 * The NAND device assumes that it is always writing to a cleanly erased page.
 * Hence, it performs its internal write verification only on bits that 
 * transitioned from 1 to 0. The device does NOT verify the whole page on a
 * byte by byte basis. It is possible that the page was not completely erased 
 * or the page is becoming unusable due to wear. The read with ECC would catch 
 * the error later when the ECC page check fails, but we would rather catch 
 * it early in the page write stage. Better to write no data than invalid data.
 *
 * @mtd:	MTD device structure
 * @this:	NAND chip structure
 * @page: 	startpage inside the chip, must be called with (page & this->pagemask)
 * @numpages:	number of pages to verify
 * @oob_buf:	out of band data buffer
 * @oobsel:	out of band selecttion structre
 * @chipnr:	number of the current chip
 * @oobmode:	1 = full buffer verify, 0 = ecc only
 */
static int nand_verify_pages (struct mtd_info *mtd, struct nand_chip *this, int page, int numpages, 
	u_char *oob_buf, struct nand_oobinfo *oobsel, int chipnr, int oobmode)
{
	int i, datidx = 0, oobofs = 0, res = -EIO;
	u_char oobdata[64];

	/* Send command to read back the first page */
	this->cmdfunc (mtd, NAND_CMD_READ0, 0, page);

	for(;;) {	

		/* Loop through and verify the data */
		if (this->verify_buf(mtd, &this->data_poi[datidx], mtd->oobblock)) {
			DEBUG (MTD_DEBUG_LEVEL0, "%s: " "Failed write verify, page 0x%08x ", __FUNCTION__, page);
			goto out;
		}
		datidx += mtd->oobblock;
		/* check, if we must compare all data or if we just have to
		 * compare the ecc bytes
		 */
		if (oobmode) {
			if (this->verify_buf(mtd, &oob_buf[oobofs], mtd->oobsize)) {
				DEBUG (MTD_DEBUG_LEVEL0, "%s: " "Failed write verify, page 0x%08x ", __FUNCTION__, page);
				goto out;
			}
		} else {
			/* Read always, else autoincrement fails */
			this->read_buf(mtd, oobdata, mtd->oobsize);

			if (oobsel->useecc != MTD_NANDECC_OFF) {
				int ecc_bytes = oobsel->eccbytes;
		
				for (i = 0; i < ecc_bytes; i++) {
					int idx = oobsel->eccpos[i];
					if (oobdata[idx] != oob_buf[oobofs + idx] ) {
						DEBUG (MTD_DEBUG_LEVEL0,
					       	"%s: Failed ECC write "
						"verify, page 0x%08x, " "%6i bytes were succesful\n", __FUNCTION__, page, i);
						goto out;
					}
				}
			}	
		}
		oobofs += mtd->oobsize;
		page++;
		numpages--;
		if (!numpages)
			break;
		
		/* Apply delay or wait for ready/busy pin 
		 * Do this before the AUTOINCR check, so no problems
		 * arise if a chip which does auto increment
		 * is marked as NOAUTOINCR by the board driver.
		*/
		if (!this->dev_ready) 
			udelay (this->chip_delay);
		else
			while (!this->dev_ready(mtd));	
			
		/* Check, if the chip supports auto page increment */ 
		if (!NAND_CANAUTOINCR(this))
			this->cmdfunc (mtd, NAND_CMD_READ0, 0x00, page);
	}
	res = 0;
	/* 
	 * Terminate the read command. This is faster than sending a reset command or 
	 * applying a 20us delay before issuing the next programm sequence.
	 * This is not a problem for all chips, but I have found a bunch of them.
	 */
out:	 
	this->select_chip(mtd, -1);
	this->select_chip(mtd, chipnr);
	return res;
}
#endif

/*
 * This function simply calls nand_read_ecc with oob buffer and oobsel = NULL
 *
 * @mtd:	MTD device structure
 * @from:	offset to read from
 * @len:	number of bytes to read
 * @retlen:	pointer to variable to store the number of read bytes
 * @buf:	the databuffer to put data
*/
static int nand_read (struct mtd_info *mtd, loff_t from, size_t len, size_t * retlen, u_char * buf)
{
	return nand_read_ecc (mtd, from, len, retlen, buf, NULL, NULL);
}			   


/*
 * NAND read with ECC
 *
 * @mtd:	MTD device structure
 * @from:	offset to read from
 * @len:	number of bytes to read
 * @retlen:	pointer to variable to store the number of read bytes
 * @buf:	the databuffer to put data
 * @oob_buf:	filesystem supplied oob data buffer
 * @oobsel:	oob selection structure
 */
static int nand_read_ecc (struct mtd_info *mtd, loff_t from, size_t len,
			  size_t * retlen, u_char * buf, u_char * oob_buf, struct nand_oobinfo *oobsel)
{
	int i, j, col, realpage, page, end, ecc, chipnr, sndcmd = 1;
	int erase_state = 0;
	int read = 0, oob = 0, ecc_status = 0, ecc_failed = 0;
	struct nand_chip *this = mtd->priv;
	u_char *data_poi, *oob_data = oob_buf;
	u_char ecc_calc[24];
	u_char ecc_code[24];
        int eccmode, eccsteps;
	int	*oob_config, datidx;
	int	blockcheck = (mtd->erasesize >> this->page_shift) - 1;


	DEBUG (MTD_DEBUG_LEVEL3, "nand_read_ecc: from = 0x%08x, len = %i\n", (unsigned int) from, (int) len);

	/* Do not allow reads past end of device */
	if ((from + len) > mtd->size) {
		DEBUG (MTD_DEBUG_LEVEL0, "nand_read_ecc: Attempt read beyond end of device\n");
		*retlen = 0;
		return -EINVAL;
	}

	/* Grab the lock and see if the device is available */
	nand_get_chip (this, mtd ,FL_READING, &erase_state);

	/* use userspace supplied oobinfo, if zero */
	if (oobsel == NULL)
		oobsel = &mtd->oobinfo;
	
	/* Autoplace of oob data ? Use the default placement scheme */
	if (oobsel->useecc == MTD_NANDECC_AUTOPLACE)
		oobsel = this->autooob;
		
	eccmode = oobsel->useecc ? this->eccmode : NAND_ECC_NONE;
	oob_config = oobsel->eccpos;

	/* Select the NAND device */
	chipnr = (int)((unsigned long)from / this->chipsize);
	this->select_chip(mtd, chipnr);

	/* First we calculate the starting page */
	realpage = from >> this->page_shift;
	page = realpage & this->pagemask;

	/* Get raw starting column */
	col = from & (mtd->oobblock - 1);

	end = mtd->oobblock;
	ecc = this->eccsize;

	/* Loop until all data read */
	while (read < len) {
		
		int aligned = (!col && (len - read) >= end);
		/* 
		 * If the read is not page aligned, we have to read into data buffer
		 * due to ecc, else we read into return buffer direct
		 */
		if (aligned)
			data_poi = &buf[read];
		else 
			data_poi = this->data_buf;
		
		/* Check, if we have this page in the buffer 
		 *
		 * FIXME: Make it work when we must provide oob data too,
		 * check the usage of data_buf oob field
		 */
		if (realpage == this->pagebuf && !oob_buf) {
			/* aligned read ? */
			if (aligned)
				memcpy (data_poi, this->data_buf, end);
			goto readdata;
		}

		/* Check, if we must send the read command */
		if (sndcmd) {
			this->cmdfunc (mtd, NAND_CMD_READ0, 0x00, page);
			sndcmd = 0;
		}	

		/* get oob area, if we have no oob buffer from fs-driver */
		if (!oob_buf || oobsel->useecc == MTD_NANDECC_AUTOPLACE)
			oob_data = &this->data_buf[end];

		eccsteps = this->eccsteps;
		
		switch (eccmode) {
		case NAND_ECC_NONE: {	/* No ECC, Read in a page */
			static unsigned long lastwhinge = 0;
			if ((lastwhinge / HZ) != (jiffies / HZ)) {
				printk (KERN_WARNING "Reading data from NAND FLASH without ECC is not recommended\n");
				lastwhinge = jiffies;
			}
			this->read_buf(mtd, data_poi, end);
			break;
		}
			
		case NAND_ECC_SOFT:	/* Software ECC 3/256: Read in a page + oob data */
			this->read_buf(mtd, data_poi, end);
			for (i = 0, datidx = 0; eccsteps; eccsteps--, i+=3, datidx += ecc) 
				this->calculate_ecc(mtd, &data_poi[datidx], &ecc_calc[i]);
			break;	
			
		case NAND_ECC_HW3_256: /* Hardware ECC 3 byte /256 byte data */
		case NAND_ECC_HW3_512: /* Hardware ECC 3 byte /512 byte data */	
			for (i = 0, datidx = 0; eccsteps; eccsteps--, i+=3, datidx += ecc) {
				this->enable_hwecc(mtd, NAND_ECC_READ);	
				this->read_buf(mtd, &data_poi[datidx], ecc);
				this->calculate_ecc(mtd, &data_poi[datidx], &ecc_calc[i]);	/* read from hardware */
			}
			break;						

		case NAND_ECC_HW6_512: /* Hardware ECC 6 byte / 512 byte data  */
			for (i = 0, datidx = 0; eccsteps; eccsteps--, i+=6, datidx += ecc) {
				this->enable_hwecc(mtd, NAND_ECC_READ);	
				this->read_buf(mtd, &data_poi[datidx], ecc);
				this->calculate_ecc(mtd, &data_poi[datidx], &ecc_calc[i]);	/* read from hardware */
			}
			break;						

		default:
			printk (KERN_WARNING "Invalid NAND_ECC_MODE %d\n", this->eccmode);
			BUG();	
		}

		/* read oobdata */
		this->read_buf(mtd, oob_data, mtd->oobsize);
		
		/* Skip ECC, if not active */
		if (eccmode == NAND_ECC_NONE)
			goto readdata;	
		
		/* Pick the ECC bytes out of the oob data */
		for (j = 0; j < oobsel->eccbytes; j++)
			ecc_code[j] = oob_data[oob_config[j]];

		/* correct data, if neccecary */
		for (i = 0, j = 0, datidx = 0; i < this->eccsteps; i++, datidx += ecc) {
			ecc_status = this->correct_data(mtd, &data_poi[datidx], &ecc_code[j], &ecc_calc[j]);
			
			/* Get next chunk of ecc bytes */
			j += eccmode == NAND_ECC_HW6_512 ? 6 : 3;
			
			/* check, if we have a fs supplied oob-buffer, 
			 * This is the legacy mode. Used by YAFFS1
			 */
			if (oob_buf && oobsel->useecc != MTD_NANDECC_AUTOPLACE) { 
				int *p = (int *)(&oob_data[mtd->oobsize]);
				p[i] = ecc_status;
			}
			
			if (ecc_status == -1) {	
				DEBUG (MTD_DEBUG_LEVEL0, "nand_read_ecc: " "Failed ECC read, page 0x%08x\n", page);
				ecc_failed++;
			}
		}		

		/* check, if we have a fs supplied oob-buffer */
		if (oob_buf) {
			/* without autoplace. Legacy mode used by YAFFS1 */
			if (oobsel->useecc != MTD_NANDECC_AUTOPLACE) {
				oob_data += mtd->oobsize + this->eccsteps * sizeof (int);
			} else {
				/* Walk through the autoplace chunks */
				for (i = 0, j = 0; j < mtd->oobavail; i++) {
					int from = oobsel->oobfree[i][0];
					int num = oobsel->oobfree[i][1];
					memcpy(&oob_buf[oob], &oob_data[from], num);
					j+= num;
				}
				oob += mtd->oobavail;
			}
		}
readdata:
		/* Partial page read, transfer data into fs buffer */
		if (!aligned) { 
			for (j = col; j < end && read < len; j++)
				buf[read++] = data_poi[j];
			this->pagebuf = realpage;	
		} else		
			read += mtd->oobblock;

		if (read == len)
			break;	

		/* For subsequent reads align to page boundary. */
		col = 0;
		/* Increment page address */
		realpage++;

		/* Apply delay or wait for ready/busy pin 
		 * Do this before the AUTOINCR check, so no problems
		 * arise if a chip which does auto increment
		 * is marked as NOAUTOINCR by the board driver.
		*/
		if (!this->dev_ready) 
			udelay (this->chip_delay);
		else
			while (!this->dev_ready(mtd));	
			
		page = realpage & this->pagemask;
		/* Check, if we cross a chip boundary */
		if (!page) {
			chipnr++;
			this->select_chip(mtd, -1);
			this->select_chip(mtd, chipnr);
		}
		/* Check, if the chip supports auto page increment 
		 * or if we have hit a block boundary. 
		*/ 
		if (!NAND_CANAUTOINCR(this) || !(page & blockcheck))
			sndcmd = 1;				
	}

	/* Deselect and wake up anyone waiting on the device */
	nand_release_chip(mtd);

	/*
	 * Return success, if no ECC failures, else -EIO
	 * fs driver will take care of that, because
	 * retlen == desired len and result == -EIO
	 */
	*retlen = read;
	return ecc_failed ? -EIO : 0;
}

/*
 * NAND read out-of-band
 *
 * @mtd:	MTD device structure
 * @from:	offset to read from
 * @len:	number of bytes to read
 * @retlen:	pointer to variable to store the number of read bytes
 * @buf:	the databuffer to put data
 */
static int nand_read_oob (struct mtd_info *mtd, loff_t from, size_t len, size_t * retlen, u_char * buf)
{
	int i, col, page, chipnr;
	int erase_state = 0;
	struct nand_chip *this = mtd->priv;
	int	blockcheck = (mtd->erasesize >> this->page_shift) - 1;

	DEBUG (MTD_DEBUG_LEVEL3, "nand_read_oob: from = 0x%08x, len = %i\n", (unsigned int) from, (int) len);

	/* Shift to get page */
	page = ((int) from) >> this->page_shift;
	chipnr = (int)((unsigned long)from / this->chipsize);
	
	/* Mask to get column */
	col = from & (mtd->oobsize - 1);

	/* Initialize return length value */
	*retlen = 0;

	/* Do not allow reads past end of device */
	if ((from + len) > mtd->size) {
		DEBUG (MTD_DEBUG_LEVEL0, "nand_read_oob: Attempt read beyond end of device\n");
		*retlen = 0;
		return -EINVAL;
	}

	/* Grab the lock and see if the device is available */
	nand_get_chip (this, mtd , FL_READING, &erase_state);

	/* Select the NAND device */
	this->select_chip(mtd, chipnr);

	/* Send the read command */
	this->cmdfunc (mtd, NAND_CMD_READOOB, col, page & this->pagemask);
	/* 
	 * Read the data, if we read more than one page
	 * oob data, let the device transfer the data !
	 */
	i = 0;
	while (i < len) {
		int thislen = (mtd->oobsize - col) & (mtd->oobsize - 1);
		if (!thislen)
			thislen = mtd->oobsize;
		thislen = min_t(int, thislen, len);
		this->read_buf(mtd, &buf[i], thislen);
		i += thislen;
		col += thislen;
		/* Read more ? */
		if (i < len) {
			page++;
			/* Apply delay or wait for ready/busy pin 
			 * Do this before the AUTOINCR check, so no problems
			 * arise if a chip which does auto increment
			 * is marked as NOAUTOINCR by the board driver.
			*/
			if (!this->dev_ready) 
				udelay (this->chip_delay);
			else
				while (!this->dev_ready(mtd));	

			/* Check, if we cross a chip boundary */
			if (!(page & this->pagemask)) {
				chipnr++;
				this->select_chip(mtd, -1);
				this->select_chip(mtd, chipnr);
			}
				
			/* Check, if the chip supports auto page increment 
			 * or if we have hit a block boundary. 
			*/ 
			if (!NAND_CANAUTOINCR(this) || !(page & blockcheck)) {
				/* For subsequent page reads set offset to 0 */
			        this->cmdfunc (mtd, NAND_CMD_READOOB, 0x0, page & this->pagemask);
			}
		}
	}

	/* Deselect and wake up anyone waiting on the device */
	nand_release_chip(mtd);

	/* Return happy */
	*retlen = len;
	return 0;
}

/* Prepare the out of band buffer 
 *
 * Return:
 * 1. Filesystem buffer available and autoplacement is off,
 *    return filesystem buffer
 * 2. No filesystem buffer or autoplace is off, return internal
 *    buffer
 * 3. Filesystem buffer is given and autoplace selected
 *    put data from fs buffer into internal buffer and
 *    retrun internal buffer
 *
 * Note: The internal buffer is filled with 0xff. This must
 * be done only once, when no autoplacement happens
 * Autoplacement sets the buffer dirty flag, which
 * forces the 0xff fill before using the buffer again.
 *
 * @mtd:	MTD device structure
 * @fsbuf:	buffer given by fs driver
 * @oobsel:	out of band selection structre
 * @autoplace:	1 = place given buffer into the oob bytes
 * @numpages:	number of pages to prepare
*/
static u_char * nand_prepare_oobbuf (struct mtd_info *mtd, u_char *fsbuf, struct nand_oobinfo *oobsel,
		int autoplace, int numpages)
{
	struct nand_chip *this = mtd->priv;
	int i, len, ofs;

	/* Zero copy fs supplied buffer */
	if (fsbuf && !autoplace) 
		return fsbuf;

	/* Check, if the buffer must be filled with ff again */
	if (this->oobdirty) {	
		memset (this->oob_buf, 0xff, 
			mtd->oobsize << (this->erase_shift - this->page_shift));
		this->oobdirty = 0;
	}	
	
	/* If we have no autoplacement or no fs buffer use the internal one */
	if (!autoplace || !fsbuf)
		return this->oob_buf;
	
	/* Walk through the pages and place the data */
	this->oobdirty = 1;
	ofs = 0;
	while (numpages--) {
		for (i = 0, len = 0; len < mtd->oobavail; i++) {
			int to = ofs + oobsel->oobfree[i][0];
			int num = oobsel->oobfree[i][1];
			memcpy (&this->oob_buf[to], fsbuf, num);
			len += num;
			fsbuf += num;
		}
		ofs += mtd->oobavail;
	}
	return this->oob_buf;
}

#define NOTALIGNED(x) (x & (mtd->oobblock-1)) != 0

/*
 * This function simply calls nand_write_ecc with oob buffer and oobsel = NULL
 *
 * @mtd:	MTD device structure
 * @to:		offset to write to
 * @len:	number of bytes to write
 * @retlen:	pointer to variable to store the number of written bytes
 * @buf:	the data to write
*/
static int nand_write (struct mtd_info *mtd, loff_t to, size_t len, size_t * retlen, const u_char * buf)
{
	return (nand_write_ecc (mtd, to, len, retlen, buf, NULL, NULL));
}			   
/*
 * NAND write with ECC
 *
 * @mtd:	MTD device structure
 * @to:		offset to write to
 * @len:	number of bytes to write
 * @retlen:	pointer to variable to store the number of written bytes
 * @buf:	the data to write
 * @eccbuf:	filesystem supplied oob data buffer
 * @oobsel:	oob selection structure
 */
static int nand_write_ecc (struct mtd_info *mtd, loff_t to, size_t len,
			   size_t * retlen, const u_char * buf, u_char * eccbuf, struct nand_oobinfo *oobsel)
{
	int startpage, page, ret = -EIO, oob = 0, written = 0, chipnr;
	int autoplace = 0, numpages, totalpages;
	struct nand_chip *this = mtd->priv;
	u_char *oobbuf, *bufstart;
	int ppblock = mtd->erasesize >> this->page_shift;

	DEBUG (MTD_DEBUG_LEVEL3, "nand_write_ecc: to = 0x%08x, len = %i\n", (unsigned int) to, (int) len);

	/* Initialize retlen, in case of early exit */
	*retlen = 0;

	/* Do not allow write past end of device */
	if ((to + len) > mtd->size) {
		DEBUG (MTD_DEBUG_LEVEL0, "nand_write_ecc: Attempt to write past end of page\n");
		return -EINVAL;
	}

	/* reject writes, which are not page aligned */	
	if (NOTALIGNED (to) || NOTALIGNED(len)) {
		printk (KERN_NOTICE "nand_write_ecc: Attempt to write not page aligned data\n");
		return -EINVAL;
	}

	/* Grab the lock and see if the device is available */
	nand_get_chip (this, mtd, FL_WRITING, NULL);

	/* Calculate chipnr */
	chipnr = (int)((unsigned long) to / this->chipsize);
	/* Select the NAND device */
	this->select_chip(mtd, chipnr);

	/* Check, if it is write protected */
	if (nand_check_wp(mtd))
		goto out;

	/* if oobsel is NULL, use chip defaults */
	if (oobsel == NULL) 
		oobsel = &mtd->oobinfo;		
		
	/* Autoplace of oob data ? Use the default placement scheme */
	if (oobsel->useecc == MTD_NANDECC_AUTOPLACE) {
		oobsel = this->autooob;
		autoplace = 1;
	}	

	/* Setup variables and oob buffer */
	totalpages = len >> this->page_shift;
	page = ((int) to) >> this->page_shift;
	/* Invalidate the page cache, if we write to the cached page */
	if (page <= this->pagebuf && this->pagebuf < (page + totalpages))  
		this->pagebuf = -1;
	
	/* Set it relative to chip */
	page &= this->pagemask;
	startpage = page;
	/* Calc number of pages we can write in one go */
	numpages = min (ppblock - (startpage  & (ppblock - 1)), totalpages);
	oobbuf = nand_prepare_oobbuf (mtd, eccbuf, oobsel, autoplace, numpages);
	bufstart = (u_char *)buf;

	/* Loop until all data is written */
	while (written < len) {

		this->data_poi = (u_char*) &buf[written];
		/* Write one page. If this is the last page to write
		 * or the last page in this block, then use the
		 * real pageprogram command, else select cached programming
		 * if supported by the chip.
		 */
		ret = nand_write_page (mtd, this, page, &oobbuf[oob], oobsel, (--numpages > 0));
		if (ret) {
			DEBUG (MTD_DEBUG_LEVEL0, "nand_write_ecc: write_page failed %d\n", ret);
			goto out;
		}	
		/* Next oob page */
		oob += mtd->oobsize;
		/* Update written bytes count */
		written += mtd->oobblock;
		if (written == len) 
			goto cmp;
		
		/* Increment page address */
		page++;

		/* Have we hit a block boundary ? Then we have to verify and
		 * if verify is ok, we have to setup the oob buffer for
		 * the next pages.
		*/
		if (!(page & (ppblock - 1))){
			int ofs;
			this->data_poi = bufstart;
			ret = nand_verify_pages (mtd, this, startpage, 
				page - startpage,
				oobbuf, oobsel, chipnr, (eccbuf != NULL));
			if (ret) {
				DEBUG (MTD_DEBUG_LEVEL0, "nand_write_ecc: verify_pages failed %d\n", ret);
				goto out;
			}	
			*retlen = written;

			ofs = autoplace ? mtd->oobavail : mtd->oobsize;
			if (eccbuf)
				eccbuf += (page - startpage) * ofs;
			totalpages -= page - startpage;
			numpages = min (totalpages, ppblock);
			page &= this->pagemask;
			startpage = page;
			oobbuf = nand_prepare_oobbuf (mtd, eccbuf, oobsel, 
					autoplace, numpages);
			/* Check, if we cross a chip boundary */
			if (!page) {
				chipnr++;
				this->select_chip(mtd, -1);
				this->select_chip(mtd, chipnr);
			}
		}
	}
	/* Verify the remaining pages */
cmp:
	this->data_poi = bufstart;
 	ret = nand_verify_pages (mtd, this, startpage, totalpages,
		oobbuf, oobsel, chipnr, (eccbuf != NULL));
	if (!ret)
		*retlen = written;
	else	
		DEBUG (MTD_DEBUG_LEVEL0, "nand_write_ecc: verify_pages failed %d\n", ret);

out:
	/* Deselect and wake up anyone waiting on the device */
	nand_release_chip(mtd);

	return ret;
}


/*
 * NAND write out-of-band
 *
 * @mtd:	MTD device structure
 * @to:		offset to write to
 * @len:	number of bytes to write
 * @retlen:	pointer to variable to store the number of written bytes
 * @buf:	the data to write
 */
static int nand_write_oob (struct mtd_info *mtd, loff_t to, size_t len, size_t * retlen, const u_char * buf)
{
	int column, page, status, ret = -EIO, chipnr;
	struct nand_chip *this = mtd->priv;

	DEBUG (MTD_DEBUG_LEVEL3, "nand_write_oob: to = 0x%08x, len = %i\n", (unsigned int) to, (int) len);

	/* Shift to get page */
	page = ((int) to) >> this->page_shift;
	chipnr = (int)((unsigned long)to / this->chipsize);

	/* Mask to get column */
	column = to & (mtd->oobsize - 1);

	/* Initialize return length value */
	*retlen = 0;

	/* Do not allow write past end of page */
	if ((column + len) > mtd->oobsize) {
		DEBUG (MTD_DEBUG_LEVEL0, "nand_write_oob: Attempt to write past end of page\n");
		return -EINVAL;
	}

	/* Grab the lock and see if the device is available */
	nand_get_chip (this, mtd, FL_WRITING, NULL);

	/* Select the NAND device */
	this->select_chip(mtd, chipnr);

	/* Reset the chip. Some chips (like the Toshiba TC5832DC found
	   in one of my DiskOnChip 2000 test units) will clear the whole
	   data page too if we don't do this. I have no clue why, but
	   I seem to have 'fixed' it in the doc2000 driver in
	   August 1999.  dwmw2. */
	this->cmdfunc(mtd, NAND_CMD_RESET, -1, -1);

	/* Check, if it is write protected */
	if (nand_check_wp(mtd))
		goto out;
	
	/* Invalidate the page cache, if we write to the cached page */
	if (page == this->pagebuf)
		this->pagebuf = -1;

	if (NAND_MUST_PAD(this)) {
		/* Write out desired data */
		this->cmdfunc (mtd, NAND_CMD_SEQIN, mtd->oobblock, page & this->pagemask);
		/* prepad 0xff for partial programming */
		this->write_buf(mtd, ffchars, column);
		/* write data */
		this->write_buf(mtd, buf, len);
		/* postpad 0xff for partial programming */
		this->write_buf(mtd, ffchars, mtd->oobsize - (len+column));
	} else {
		/* Write out desired data */
		this->cmdfunc (mtd, NAND_CMD_SEQIN, mtd->oobblock + column, page & this->pagemask);
		/* write data */
		this->write_buf(mtd, buf, len);
	}
	/* Send command to program the OOB data */
	this->cmdfunc (mtd, NAND_CMD_PAGEPROG, -1, -1);

	status = this->waitfunc (mtd, this, FL_WRITING);

	/* See if device thinks it succeeded */
	if (status & 0x01) {
		DEBUG (MTD_DEBUG_LEVEL0, "nand_write_oob: " "Failed write, page 0x%08x\n", page);
		ret = -EIO;
		goto out;
	}
	/* Return happy */
	*retlen = len;

#ifdef CONFIG_MTD_NAND_VERIFY_WRITE
	/* Send command to read back the data */
	this->cmdfunc (mtd, NAND_CMD_READOOB, column, page & this->pagemask);

	if (this->verify_buf(mtd, buf, len)) {
		DEBUG (MTD_DEBUG_LEVEL0, "nand_write_oob: " "Failed write verify, page 0x%08x\n", page);
		ret = -EIO;
		goto out;
	}
#endif
	ret = 0;
out:
	/* Deselect and wake up anyone waiting on the device */
	nand_release_chip(mtd);

	return ret;
}


/*
 * NAND write with iovec. This just calls the ecc function
 *
 * @mtd:	MTD device structure
 * @vecs:	the iovectors to write
 * @count:	number of vectors
 * @to:		offset to write to
 * @retlen:	pointer to variable to store the number of written bytes
 */
static int nand_writev (struct mtd_info *mtd, const struct iovec *vecs, unsigned long count, 
		loff_t to, size_t * retlen)
{
	return (nand_writev_ecc (mtd, vecs, count, to, retlen, NULL, 0));	
}

/*
 * NAND write with iovec with ecc
 *
 * @mtd:	MTD device structure
 * @vecs:	the iovectors to write
 * @count:	number of vectors
 * @to:		offset to write to
 * @retlen:	pointer to variable to store the number of written bytes
 * @eccbuf:	filesystem supplied oob data buffer
 * @oobsel:	oob selection structure
 */
static int nand_writev_ecc (struct mtd_info *mtd, const struct iovec *vecs, unsigned long count, 
		loff_t to, size_t * retlen, u_char *eccbuf, struct nand_oobinfo *oobsel)
{
	int i, page, len, total_len, ret = -EIO, written = 0, chipnr;
	int oob, numpages, autoplace = 0, startpage;
	struct nand_chip *this = mtd->priv;
	int ppblock = mtd->erasesize >> this->page_shift;
	u_char *oobbuf, *bufstart;

	/* Preset written len for early exit */
	*retlen = 0;

	/* Calculate total length of data */
	total_len = 0;
	for (i = 0; i < count; i++)
		total_len += (int) vecs[i].iov_len;

	DEBUG (MTD_DEBUG_LEVEL3,
	       "nand_writev: to = 0x%08x, len = %i, count = %ld\n", (unsigned int) to, (unsigned int) total_len, count);

	/* Do not allow write past end of page */
	if ((to + total_len) > mtd->size) {
		DEBUG (MTD_DEBUG_LEVEL0, "nand_writev: Attempted write past end of device\n");
		return -EINVAL;
	}

	/* reject writes, which are not page aligned */	
	if (NOTALIGNED (to) || NOTALIGNED(total_len)) {
		printk (KERN_NOTICE "nand_write_ecc: Attempt to write not page aligned data\n");
		return -EINVAL;
	}

	/* Grab the lock and see if the device is available */
	nand_get_chip (this, mtd, FL_WRITING, NULL);

	/* Get the current chip-nr */
	chipnr = (int)((unsigned long) to / this->chipsize);
	/* Select the NAND device */
	this->select_chip(mtd, chipnr);

	/* Check, if it is write protected */
	if (nand_check_wp(mtd))
		goto out;

	/* if oobsel is NULL, use chip defaults */
	if (oobsel == NULL) 
		oobsel = &mtd->oobinfo;		

	/* Autoplace of oob data ? Use the default placement scheme */
	if (oobsel->useecc == MTD_NANDECC_AUTOPLACE) {
		oobsel = this->autooob;
		autoplace = 1;
	}	

	/* Setup start page */
	page = ((int) to) >> this->page_shift;
	/* Invalidate the page cache, if we write to the cached page */
	if (page <= this->pagebuf && this->pagebuf < ((to + total_len) >> this->page_shift))  
		this->pagebuf = -1;

	startpage = page & this->pagemask;

	/* Loop until all iovecs' data has been written */
	len = 0;
	while (count) {
		/* If the given tuple is >= pagesize then
		 * write it out from the iov
		 */
		if ((vecs->iov_len - len) >= mtd->oobblock) {
			/* Calc number of pages we can write
			 * out of this iov in one go */
			numpages = (vecs->iov_len - len) >> this->page_shift;
			/* Do not cross block boundaries */
			numpages = min (ppblock - (startpage & (ppblock - 1)), numpages);
			oobbuf = nand_prepare_oobbuf (mtd, NULL, oobsel, autoplace, numpages);
			bufstart = (u_char *)vecs->iov_base;
			bufstart += len;
			this->data_poi = bufstart;
			oob = 0;
			for (i = 1; i <= numpages; i++) {
				/* Write one page. If this is the last page to write
				 * then use the real pageprogram command, else select 
				 * cached programming if supported by the chip.
				 */
				ret = nand_write_page (mtd, this, page & this->pagemask, 
					&oobbuf[oob], oobsel, i != numpages);
				if (ret)
					goto out;
				this->data_poi += mtd->oobblock;
				len += mtd->oobblock;
				oob += mtd->oobsize;
				page++;
			}
			/* Check, if we have to switch to the next tuple */
			if (len >= (int) vecs->iov_len) {
				vecs++;
				len = 0;
				count--;
			}
		} else {
			/* We must use the internal buffer, read data out of each 
			 * tuple until we have a full page to write
			 */
			int cnt = 0;
			while (cnt < mtd->oobblock) {
				if (vecs->iov_base != NULL && vecs->iov_len) 
					this->data_buf[cnt++] = ((u_char *) vecs->iov_base)[len++];
				/* Check, if we have to switch to the next tuple */
				if (len >= (int) vecs->iov_len) {
					vecs++;
					len = 0;
					count--;
				}
			}
			this->pagebuf = page;	
			this->data_poi = this->data_buf;	
			bufstart = this->data_poi;
			numpages = 1;		
			oobbuf = nand_prepare_oobbuf (mtd, NULL, oobsel, autoplace, numpages);
			ret = nand_write_page (mtd, this, page & this->pagemask,
				oobbuf, oobsel, 0);
			if (ret)
				goto out;
			page++;
		}

		this->data_poi = bufstart;
		ret = nand_verify_pages (mtd, this, startpage, numpages, oobbuf, oobsel, chipnr, 0);
		if (ret)
			goto out;
			
		written += mtd->oobblock * numpages;
		/* All done ? */
		if (!count)
			break;

		startpage = page & this->pagemask;
		/* Check, if we cross a chip boundary */
		if (!startpage) {
			chipnr++;
			this->select_chip(mtd, -1);
			this->select_chip(mtd, chipnr);
		}
	}
	ret = 0;
out:
	/* Deselect and wake up anyone waiting on the device */
	nand_release_chip(mtd);

	*retlen = written;
	return ret;
}

/* 
 * NAND standard block erase command function
 * 
 */
static void single_erase_cmd (struct mtd_info *mtd, int page)
{
	/* Send commands to erase a block */
	this->cmdfunc (mtd, NAND_CMD_ERASE1, -1, page);
	this->cmdfunc (mtd, NAND_CMD_ERASE2, -1, -1);
}

/* 
 * AND multi block erase command function
 * Erase 4 consecutive blocks
 */
static void multi_erase_cmd (struct mtd_info *mtd, int page)
{
	/* Send commands to erase a block */
	this->cmdfunc (mtd, NAND_CMD_ERASE1, -1, page++);
	this->cmdfunc (mtd, NAND_CMD_ERASE1, -1, page++);
	this->cmdfunc (mtd, NAND_CMD_ERASE1, -1, page++);
	this->cmdfunc (mtd, NAND_CMD_ERASE1, -1, page);
	this->cmdfunc (mtd, NAND_CMD_ERASE2, -1, -1);
}
 
/*
 * NAND erase a block
 * 
 * @mtd:	MTD device structure
 * @instr:	erase instruction
 */
static int nand_erase (struct mtd_info *mtd, struct erase_info *instr)
{
	int page, len, status, pages_per_block, ret, chipnr;
	struct nand_chip *this = mtd->priv;

	DEBUG (MTD_DEBUG_LEVEL3,
	       "nand_erase: start = 0x%08x, len = %i\n", (unsigned int) instr->addr, (unsigned int) instr->len);

	/* Start address must align on block boundary */
	if (instr->addr & (mtd->erasesize - 1)) {
		DEBUG (MTD_DEBUG_LEVEL0, "nand_erase: Unaligned address\n");
		return -EINVAL;
	}

	/* Length must align on block boundary */
	if (instr->len & (mtd->erasesize - 1)) {
		DEBUG (MTD_DEBUG_LEVEL0, "nand_erase: Length not block aligned\n");
		return -EINVAL;
	}

	/* Do not allow erase past end of device */
	if ((instr->len + instr->addr) > mtd->size) {
		DEBUG (MTD_DEBUG_LEVEL0, "nand_erase: Erase past end of device\n");
		return -EINVAL;
	}

	/* Grab the lock and see if the device is available */
	nand_get_chip (this, mtd, FL_ERASING, NULL);

	/* Shift to get first page */
	page = (int) (instr->addr >> this->page_shift);
	chipnr = (int)((unsigned long)instr->addr / this->chipsize);

	/* Calculate pages in each block */
	pages_per_block = 1 << (this->erase_shift - this->page_shift);

	/* Select the NAND device */
	this->select_chip(mtd, chipnr);

	/* Check the WP bit */
	this->cmdfunc (mtd, NAND_CMD_STATUS, -1, -1);
	if (!(this->read_byte(mtd) & 0x80)) {
		DEBUG (MTD_DEBUG_LEVEL0, "nand_erase: Device is write protected!!!\n");
		instr->state = MTD_ERASE_FAILED;
		goto erase_exit;
	}

	/* Loop through the pages */
	len = instr->len;

	instr->state = MTD_ERASING;

	while (len) {
		/* Check if we have a bad block, we do not erase bad blocks ! */
		if (this->block_bad(mtd, (loff_t) page, 0)) {
			printk (KERN_WARNING "nand_erase: attempt to erase a bad block at page 0x%08x\n", page);
			instr->state = MTD_ERASE_FAILED;
			goto erase_exit;
		}
		
		/* Invalidate the page cache, if we erase the block which contains 
		   the current cached page */
		if (page >= this->pagebuf && this->pagebuf < (page + pages_per_block))  
			this->pagebuf = -1;

		this->erase_cmd (mtd, page & this->pagemask);
		
		status = this->waitfunc (mtd, this, FL_ERASING);

		/* See if block erase succeeded */
		if (status & 0x01) {
			DEBUG (MTD_DEBUG_LEVEL0, "nand_erase: " "Failed erase, page 0x%08x\n", page);
			instr->state = MTD_ERASE_FAILED;
			goto erase_exit;
		}
		
		/* Increment page address and decrement length */
		len -= mtd->erasesize;
		page += pages_per_block;

		/* Check, if we cross a chip boundary */
		if (len && !(page & this->pagemask)) {
			chipnr++;
			this->select_chip(mtd, -1);
			this->select_chip(mtd, chipnr);
		}
	}
	instr->state = MTD_ERASE_DONE;

erase_exit:

	ret = instr->state == MTD_ERASE_DONE ? 0 : -EIO;;
	/* Do call back function */
	if (!ret && instr->callback)
		instr->callback (instr);

	/* Deselect and wake up anyone waiting on the device */
	nand_release_chip(mtd);

	/* Return more or less happy */
	return ret;
}

/*
 * NAND sync
 *
 * @mtd:	MTD device structure
 */
static void nand_sync (struct mtd_info *mtd)
{
	struct nand_chip *this = mtd->priv;
	DECLARE_WAITQUEUE (wait, current);

	DEBUG (MTD_DEBUG_LEVEL3, "nand_sync: called\n");

retry:
	/* Grab the spinlock */
	spin_lock_bh (&this->chip_lock);

	/* See what's going on */
	switch (this->state) {
	case FL_READY:
	case FL_SYNCING:
		this->state = FL_SYNCING;
		spin_unlock_bh (&this->chip_lock);
		break;

	default:
		/* Not an idle state */
		add_wait_queue (&this->wq, &wait);
		spin_unlock_bh (&this->chip_lock);
		schedule ();

		remove_wait_queue (&this->wq, &wait);
		goto retry;
	}

	/* Lock the device */
	spin_lock_bh (&this->chip_lock);

	/* Set the device to be ready again */
	if (this->state == FL_SYNCING) {
		this->state = FL_READY;
		wake_up (&this->wq);
	}

	/* Unlock the device */
	spin_unlock_bh (&this->chip_lock);
}

/*
 * Check whether the block at the given offset is bad
 *
 * @mtd:	MTD device structure
 * @ofs:	offset relative to mtd start
 */
static int nand_block_isbad (struct mtd_info *mtd, loff_t ofs)
{
	struct nand_chip *this = mtd->priv;
	
	/* Check for invalid offset */
	if (ofs > mtd->size) 
		return -EINVAL;

	return this->block_bad(mtd, ofs, 1);
}

/*
 * Mark the block at the given offset as bad
 *
 * @mtd:	MTD device structure
 * @ofs:	offset relative to mtd start
 */
static int nand_block_markbad (struct mtd_info *mtd, loff_t ofs)
{
	struct nand_chip *this = mtd->priv;
		
	/* Check for invalid offset */
	if (ofs > mtd->size) 
		return -EINVAL;

	return this->block_markbad(mtd, ofs);
}

/*
 * Scan for the NAND device
 *
 * This fills out all the not initialized function pointers
 * with the defaults.
 * The flash ID is read and the mtd/chip structures are
 * filled with the appropriate values.
 *
 * @mtd:	MTD device structure
 * @maxchips:	Number of chips to scan for
 */
int nand_scan (struct mtd_info *mtd, int maxchips)
{
	int i, nand_maf_id, nand_dev_id, busw;
	struct nand_chip *this = mtd->priv;

	/* Get buswidth to select the correct functions*/
	busw = this->options & NAND_BUSWIDTH_16;

	/* check for proper chip_delay setup, set 20us if not */
	if (!this->chip_delay)
		this->chip_delay = 20;

	/* check, if a user supplied command function given */
	if (this->cmdfunc == NULL)
		this->cmdfunc = nand_command;

	/* check, if a user supplied wait function given */
	if (this->waitfunc == NULL)
		this->waitfunc = nand_wait;

	if (!this->select_chip)
		this->select_chip = nand_select_chip;
	if (!this->write_byte)
		this->write_byte = busw ? nand_write_byte16 : nand_write_byte;
	if (!this->read_byte)
		this->read_byte = busw ? nand_read_byte16 : nand_read_byte;
	if (!this->write_word)
		this->write_word = nand_write_word;
	if (!this->read_word)
		this->read_word = nand_read_word;
	if (!this->block_bad)
		this->block_bad = nand_block_bad;
	if (!this->block_markbad)
		this->block_markbad = nand_default_block_markbad;
	if (!this->write_buf)
		this->write_buf = busw ? nand_write_buf16 : nand_write_buf;
	if (!this->read_buf)
		this->read_buf = busw ? nand_read_buf16 : nand_read_buf;
	if (!this->verify_buf)
		this->verify_buf = busw ? nand_verify_buf16 : nand_verify_buf;

	/* Select the device */
	this->select_chip(mtd, 0);

	/* Send the command for reading device ID */
	this->cmdfunc (mtd, NAND_CMD_READID, 0x00, -1);

	/* Read manufacturer and device IDs */
	nand_maf_id = this->read_byte(mtd);
	nand_dev_id = this->read_byte(mtd);

	/* Print and store flash device information */
	for (i = 0; nand_flash_ids[i].name != NULL; i++) {
				
		if (nand_dev_id != nand_flash_ids[i].id) 
			continue;
			
		mtd->name = nand_flash_ids[i].name;
		this->chipsize = nand_flash_ids[i].chipsize << 20;
		
		/* New devices have all the information in additional id bytes */
		if (!nand_flash_ids[i].pagesize) {
			int extid;
			/* The 3rd id byte contains non relevant data ATM */
			extid = this->read_byte(mtd);
			/* The 4th id byte is the important one */
			extid = this->read_byte(mtd);
			/* Calc pagesize */
			mtd->oobblock = 1024 << (extid & 0x3);
			extid >>= 2;
			/* Calc oobsize */
			mtd->oobsize = (8 << (extid & 0x03)) * (mtd->oobblock / 512);
			extid >>= 2;
			/* Calc blocksize. Blocksize is multiples of 64KiB */
			mtd->erasesize = (64 * 1024)  << (extid & 0x03);
			extid >>= 2;
			/* Get buswidth information */
			busw = (extid & 0x01) ? NAND_BUSWIDTH_16 : 0;
		
		} else {
			/* Old devices have this data hardcoded in the
			 * device id table */
			mtd->erasesize = nand_flash_ids[i].erasesize;
			mtd->oobblock = nand_flash_ids[i].pagesize;
			mtd->oobsize = mtd->oobblock / 32;
			busw = nand_flash_ids[i].options & NAND_BUSWIDTH_16;
		}

		/* Check, if buswidth is correct. Hardware drivers should set
		 * this correct ! */
		if (busw != (this->options & NAND_BUSWIDTH_16)) {
			printk (KERN_INFO "NAND device: Manufacturer ID:"
				" 0x%02x, Chip ID: 0x%02x (%s %s)\n", nand_maf_id, nand_dev_id, 
				nand_manuf_ids[i].name , mtd->name);
			printk (KERN_WARNING 
				"NAND bus width %d instead %d bit\n", 
					(this->options & NAND_BUSWIDTH_16) ? 16 : 8,
					busw ? 16 : 8);
			this->select_chip(mtd, -1);
			return 1;	
		}
		
		/* Calculate the address shift from the page size */	
		this->page_shift = ffs(mtd->oobblock) - 1;
		this->erase_shift = ffs(mtd->erasesize) - 1;

		/* Set the bad block position */
		this->badblockpos = mtd->oobblock > 512 ? 
			NAND_LARGE_BADBLOCK_POS : NAND_SMALL_BADBLOCK_POS;

		/* Get chip options */
		this->options = nand_flash_ids[i].options;
		/* Check if this is a not a samsung device. Do not clear the options
		 * for chips which are not having an extended id.
		 */	
		if (nand_maf_id != NAND_MFR_SAMSUNG && !nand_flash_ids[i].pagesize)
			this->options &= ~NAND_SAMSUNG_LP_OPTIONS;
		
		/* Check for AND chips with 4 page planes */
		if (this->options & NAND_4PAGE_ARRAY)
			this->erase_cmd = multi_erase_cmd;
		else
			this->erase_cmd = single_erase_cmd;

		/* Do not replace user supplied command function ! */
		if (this->pagesize > 512 && this->cmdfunc == nand_command)
			this->cmdfunc = nand_command_lp;
				
		/* Try to identify manufacturer */
		for (i = 0; nand_manuf_ids[i].id != 0x0; i++) {
			if (nand_manuf_ids[i].id == nand_maf_id)
				break;
		}
		printk (KERN_INFO "NAND device: Manufacturer ID:"
			" 0x%02x, Chip ID: 0x%02x (%s %s)\n", nand_maf_id, nand_dev_id, 
			nand_manuf_ids[i].name , mtd->name);
		break;
	}

	if (!mtd->name) {
		printk (KERN_WARNING "No NAND device found!!!\n");
		this->select_chip(mtd, -1);
		return 1;
	}

	for (i=1; i < maxchips; i++) {
		this->select_chip(mtd, i);

		/* Send the command for reading device ID */
		this->cmdfunc (mtd, NAND_CMD_READID, 0x00, -1);

		/* Read manufacturer and device IDs */
		if (nand_maf_id != this->read_byte(mtd) ||
		    nand_dev_id != this->read_byte(mtd))
			break;
	}
	if (i > 1)
		printk(KERN_INFO "%d NAND chips detected\n", i);
	
	if (!this->oob_buf || !this->data_buf) {
		printk (KERN_WARNING "nand_scan(): Buffers not set. Scan aborted\n"); 
		return 1;
	}

	/* Store the number of chips and calc total size for mtd */
	this->numchips = i;
	mtd->size = i * this->chipsize;
	/* Convert chipsize to number of pages per chip -1. */
	this->pagemask = (this->chipsize >> this->page_shift) - 1;
	/* Preset the internal oob buffer */
	memset(this->oob_buf, 0xff, mtd->oobsize << (this->erase_shift - this->page_shift));

	/* If no default placement scheme is given, select an
	 * appropriate one */
	if (!this->autooob) {
		/* Select the appropriate default oob placement scheme for
		 * placement agnostic filesystems */
		switch (mtd->oobsize) { 
		case 8:
			this->autooob = &nand_oob_8;
			break;
		case 16:
			this->autooob = &nand_oob_16;
			break;
		case 64:
			this->autooob = &nand_oob_64;
			break;
		default:
			printk (KERN_WARNING "No oob scheme defined for oobsize %d\n",
				mtd->oobsize);
			BUG();
		}
	}
	
	/* The number of bytes available for the filesystem to place fs dependend
	 * oob data */
	if (this->options & NAND_BUSWIDTH_16) {
		mtd->oobavail = mtd->oobsize - (this->autooob->eccbytes + 2);
		if (this->autooob->eccbytes & 0x01)
			mtd->oobavail--;
	} else
		mtd->oobavail = mtd->oobsize - (this->autooob->eccbytes + 1);

	/* 
	 * check ECC mode, default to software
	 * if 3byte/512byte hardware ECC is selected and we have 256 byte pagesize
	 * fallback to software ECC 
	*/
	this->eccsize = 256;	/* set default eccsize */	

	switch (this->eccmode) {

	case NAND_ECC_HW3_512: 
	case NAND_ECC_HW6_512: 
		if (mtd->oobblock == 256) {
			printk (KERN_WARNING "512 byte HW ECC not possible on 256 Byte pagesize, fallback to SW ECC \n");
			this->eccmode = NAND_ECC_SOFT;
			this->calculate_ecc = nand_calculate_ecc;
			this->correct_data = nand_correct_data;
			break;		
		} else 
			this->eccsize = 512; /* set eccsize to 512 and fall through for function check */

	case NAND_ECC_HW3_256:
		if (this->calculate_ecc && this->correct_data && this->enable_hwecc)
			break;
		printk (KERN_WARNING "No ECC functions supplied, Hardware ECC not possible\n");
		BUG();	

	case NAND_ECC_NONE: 
		printk (KERN_WARNING "NAND_ECC_NONE selected by board driver. This is not recommended !!\n");
		this->eccmode = NAND_ECC_NONE;
		break;

	case NAND_ECC_SOFT:	
		this->calculate_ecc = nand_calculate_ecc;
		this->correct_data = nand_correct_data;
		break;

	default:
		printk (KERN_WARNING "Invalid NAND_ECC_MODE %d\n", this->eccmode);
		BUG();	
	}	
	
	mtd->eccsize = this->eccsize;
	
	/* Set the number of read / write steps for one page to ensure ECC generation */
	switch (this->eccmode) {
	case NAND_ECC_HW3_512:
	case NAND_ECC_HW6_512:
		this->eccsteps = mtd->oobblock / 512;
		break;
	case NAND_ECC_HW3_256:
	case NAND_ECC_SOFT:	
		this->eccsteps = mtd->oobblock / 256;
		break;
		
	case NAND_ECC_NONE: 
		this->eccsteps = 1;
		break;
	}
	
	/* Initialize state, waitqueue and spinlock */
	this->state = FL_READY;
	init_waitqueue_head (&this->wq);
	spin_lock_init (&this->chip_lock);

	/* De-select the device */
	this->select_chip(mtd, -1);

	/* Invalidate the pagebuffer reference */
	this->pagebuf = -1;

	/* Fill in remaining MTD driver data */
	mtd->type = MTD_NANDFLASH;
	mtd->flags = MTD_CAP_NANDFLASH | MTD_ECC;
	mtd->ecctype = MTD_ECC_SW;
	mtd->erase = nand_erase;
	mtd->point = NULL;
	mtd->unpoint = NULL;
	mtd->read = nand_read;
	mtd->write = nand_write;
	mtd->read_ecc = nand_read_ecc;
	mtd->write_ecc = nand_write_ecc;
	mtd->read_oob = nand_read_oob;
	mtd->write_oob = nand_write_oob;
	mtd->readv = NULL;
	mtd->writev = nand_writev;
	mtd->writev_ecc = nand_writev_ecc;
	mtd->sync = nand_sync;
	mtd->lock = NULL;
	mtd->unlock = NULL;
	mtd->suspend = NULL;
	mtd->resume = NULL;
	mtd->block_isbad = nand_block_isbad;
	mtd->block_markbad = nand_block_markbad;

	/* and make the autooob the default one */
	memcpy(&mtd->oobinfo, this->autooob, sizeof(mtd->oobinfo));

	mtd->owner = THIS_MODULE;

	/* Return happy */
	return 0;
}

EXPORT_SYMBOL (nand_scan);

MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("Steven J. Hill <sjhill@realitydiluted.com>, Thomas Gleixner <tglx@linutronix.de>");
MODULE_DESCRIPTION ("Generic NAND flash driver code");
