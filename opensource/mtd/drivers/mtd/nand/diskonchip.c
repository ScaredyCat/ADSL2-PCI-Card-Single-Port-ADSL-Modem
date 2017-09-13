/* 
 * drivers/mtd/nand/diskonchip.c
 *
 * (C) 2003 Red Hat, Inc.
 *
 * Author: David Woodhouse <dwmw2@infradead.org>
 *
 * Interface to generic NAND code for M-Systems DiskOnChip devices
 *
 * $Id: diskonchip.c,v 1.9 2004/03/27 19:55:53 gleixner Exp $
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <asm/io.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/doc2000.h>
#include <linux/mtd/compatmac.h>

struct doc_priv {
	unsigned long virtadr;
	unsigned long physadr;
	u_char ChipID;
	u_char CDSNControl;
	int chips_per_floor; /* The number of chips detected on each floor */
	int curfloor;
	int curchip;
};

#define DoC_is_Millennium(doc) ((doc)->ChipID == DOC_ChipID_DocMil)
#define DoC_is_2000(doc) ((doc)->ChipID == DOC_ChipID_Doc2k)

static void doc200x_hwcontrol(struct mtd_info *mtd, int cmd);
static void doc200x_select_chip(struct mtd_info *mtd, int chip);

static int debug=0;
MODULE_PARM(debug, "i");

static int try_dword=1;
MODULE_PARM(try_dword, "i");

static void DoC_Delay(struct doc_priv *doc, unsigned short cycles)
{
	volatile char dummy;
	int i;
	
	for (i = 0; i < cycles; i++) {
		if (DoC_is_Millennium(doc))
			dummy = ReadDOC(doc->virtadr, NOP);
		else
			dummy = ReadDOC(doc->virtadr, DOCStatus);
	}
	
}
/* DOC_WaitReady: Wait for RDY line to be asserted by the flash chip */
static int _DoC_WaitReady(struct doc_priv *doc)
{
	unsigned long docptr = doc->virtadr;
	unsigned long timeo = jiffies + (HZ * 10);

	if(debug) printk("_DoC_WaitReady...\n");
	/* Out-of-line routine to wait for chip response */
	while (!(ReadDOC(docptr, CDSNControl) & CDSN_CTRL_FR_B)) {
		if (time_after(jiffies, timeo)) {
			printk("_DoC_WaitReady timed out.\n");
			return -EIO;
		}
		udelay(1);
		cond_resched();
	}

	return 0;
}

static inline int DoC_WaitReady(struct doc_priv *doc)
{
	unsigned long docptr = doc->virtadr;
	int ret = 0;

	DoC_Delay(doc, 4);

	if (!(ReadDOC(docptr, CDSNControl) & CDSN_CTRL_FR_B))
		/* Call the out-of-line routine to wait */
		ret = _DoC_WaitReady(doc);

	DoC_Delay(doc, 2);
	if(debug) printk("DoC_WaitReady OK\n");
	return ret;
}

static void doc2000_write_byte(struct mtd_info *mtd, u_char datum)
{
	struct nand_chip *this = mtd->priv;
	struct doc_priv *doc = (void *)this->priv;
	unsigned long docptr = doc->virtadr;

	if(debug)printk("write_byte %02x\n", datum);
	WriteDOC(datum, docptr, CDSNSlowIO);
	WriteDOC(datum, docptr, 2k_CDSN_IO);
}

static u_char doc2000_read_byte(struct mtd_info *mtd)
{
	struct nand_chip *this = mtd->priv;
	struct doc_priv *doc = (void *)this->priv;
	unsigned long docptr = doc->virtadr;

	ReadDOC(docptr, CDSNSlowIO);
	u_char ret = ReadDOC(docptr, 2k_CDSN_IO);
	if (debug) printk("read_byte returns %02x\n", ret);
	return ret;
}
static void doc2000_writebuf(struct mtd_info *mtd, 
			     const u_char *buf, int len)
{
	struct nand_chip *this = mtd->priv;
	struct doc_priv *doc = (void *)this->priv;
	unsigned long docptr = doc->virtadr;
	int i;
	if (debug)printk("writebuf of %d bytes: ", len);
	for (i=0; i < len; i++) {
		WriteDOC_(buf[i], docptr, DoC_2k_CDSN_IO + i);
		if (debug && i < 16)
			printk("%02x ", buf[i]);
	}
	if (debug) printk("\n");
}

static void doc2000_readbuf(struct mtd_info *mtd, 
			    u_char *buf, int len)
{
	struct nand_chip *this = mtd->priv;
	struct doc_priv *doc = (void *)this->priv;
	unsigned long docptr = doc->virtadr;
 	int i;

	if (debug)printk("readbuf of %d bytes: ", len);

	for (i=0; i < len; i++) {
		buf[i] = ReadDOC(docptr, 2k_CDSN_IO + i);
	}
}

static void doc2000_readbuf_dword(struct mtd_info *mtd, 
			    u_char *buf, int len)
{
	struct nand_chip *this = mtd->priv;
	struct doc_priv *doc = (void *)this->priv;
	unsigned long docptr = doc->virtadr;
 	int i;

	if (debug) printk("readbuf_dword of %d bytes: ", len);

	if (unlikely((((unsigned long)buf)|len) & 3)) {
		for (i=0; i < len; i++) {
			*(uint8_t *)(&buf[i]) = ReadDOC(docptr, 2k_CDSN_IO + i);
		}
	} else {
		for (i=0; i < len; i+=4) {
			*(uint32_t*)(&buf[i]) = readl(docptr + DoC_2k_CDSN_IO + i);
		}
	}
}

static int doc2000_verifybuf(struct mtd_info *mtd, 
			      const u_char *buf, int len)
{
	struct nand_chip *this = mtd->priv;
	struct doc_priv *doc = (void *)this->priv;
	unsigned long docptr = doc->virtadr;
	int i;

	for (i=0; i < len; i++)
		if (buf[i] != ReadDOC(docptr, 2k_CDSN_IO))
			return -EFAULT;
	return 0;
}

static uint16_t doc200x_ident_chip(struct mtd_info *mtd, int nr)
{
	struct nand_chip *this = mtd->priv;
	struct doc_priv *doc = (void *)this->priv;
	uint16_t ret;

	doc200x_select_chip(mtd, nr);
	doc200x_hwcontrol(mtd, NAND_CTL_SETCLE);
	this->write_byte(mtd, NAND_CMD_READID);
	doc200x_hwcontrol(mtd, NAND_CTL_CLRCLE);
	doc200x_hwcontrol(mtd, NAND_CTL_SETALE);
	this->write_byte(mtd, 0);
	doc200x_hwcontrol(mtd, NAND_CTL_CLRALE);
	
	ret = this->read_byte(mtd) << 8;
	ret |= this->read_byte(mtd);

	if (doc->ChipID == DOC_ChipID_Doc2k && try_dword && !nr) {
		/* First chip probe. See if we get same results by 32-bit access */
		union {
			uint32_t dword;
			uint8_t byte[4];
		} ident;
		struct nand_chip *this = mtd->priv;
		struct doc_priv *doc = (void *)this->priv;
		unsigned long docptr = doc->virtadr;

		doc200x_hwcontrol(mtd, NAND_CTL_SETCLE);
		doc2000_write_byte(mtd, NAND_CMD_READID);
		doc200x_hwcontrol(mtd, NAND_CTL_CLRCLE);
		doc200x_hwcontrol(mtd, NAND_CTL_SETALE);
		doc2000_write_byte(mtd, 0);
		doc200x_hwcontrol(mtd, NAND_CTL_CLRALE);

		ident.dword = readl(docptr + DoC_2k_CDSN_IO);
		if (((ident.byte[0] << 8) | ident.byte[1]) == ret) {
			printk(KERN_INFO "DiskOnChip 2000 responds to DWORD access\n");
			this->read_buf = &doc2000_readbuf_dword;
		}
	}
		
	return ret;
}

static void doc2000_count_chips(struct mtd_info *mtd)
{
	struct nand_chip *this = mtd->priv;
	struct doc_priv *doc = (void *)this->priv;
	uint16_t mfrid;
	int i;

	/* Max 4 chips per floor on DiskOnChip 2000 */
	doc->chips_per_floor = 4;

	/* Find out what the first chip is */
	mfrid = doc200x_ident_chip(mtd, 0);

	/* Find how many chips in each floor. */
	for (i = 1; i < 4; i++) {
		if (doc200x_ident_chip(mtd, i) != mfrid)
			break;
	}
	doc->chips_per_floor = i;
}

static int doc200x_wait(struct mtd_info *mtd, struct nand_chip *this, int state)
{
	struct doc_priv *doc = (void *)this->priv;

	int status;
	
	DoC_WaitReady(doc);
	this->cmdfunc(mtd, NAND_CMD_STATUS, -1, -1);
	DoC_WaitReady(doc);
	status = (int)this->read_byte(mtd);

	return status;
}

static void doc2001_write_byte(struct mtd_info *mtd, u_char datum)
{
	struct nand_chip *this = mtd->priv;
	struct doc_priv *doc = (void *)this->priv;
	unsigned long docptr = doc->virtadr;

	WriteDOC(datum, docptr, CDSNSlowIO);
	WriteDOC(datum, docptr, Mil_CDSN_IO);
	WriteDOC(datum, docptr, WritePipeTerm);
}

static u_char doc2001_read_byte(struct mtd_info *mtd)
{
	struct nand_chip *this = mtd->priv;
	struct doc_priv *doc = (void *)this->priv;
	unsigned long docptr = doc->virtadr;

	ReadDOC(docptr, CDSNSlowIO);
	/* 11.4.5 -- delay twice to allow extended length cycle */
	DoC_Delay(doc, 2);
	ReadDOC(docptr, ReadPipeInit);
	return ReadDOC(docptr, Mil_CDSN_IO);
}

static void doc2001_writebuf(struct mtd_info *mtd, 
			     const u_char *buf, int len)
{
	struct nand_chip *this = mtd->priv;
	struct doc_priv *doc = (void *)this->priv;
	unsigned long docptr = doc->virtadr;
	int i;

	for (i=0; i < len; i++)
		WriteDOC_(buf[i], docptr, DoC_Mil_CDSN_IO + i);
	/* Terminate write pipeline */
	WriteDOC(0x00, docptr, WritePipeTerm);
}

static void doc2001_readbuf(struct mtd_info *mtd, 
			    u_char *buf, int len)
{
	struct nand_chip *this = mtd->priv;
	struct doc_priv *doc = (void *)this->priv;
	unsigned long docptr = doc->virtadr;
	int i;

	/* Start read pipeline */
	ReadDOC(docptr, ReadPipeInit);

	for (i=0; i < len-1; i++)
		buf[i] = ReadDOC(docptr, Mil_CDSN_IO);

	/* Terminate read pipeline */
	buf[i] = ReadDOC(docptr, LastDataRead);
}
static int doc2001_verifybuf(struct mtd_info *mtd, 
			     const u_char *buf, int len)
{
	struct nand_chip *this = mtd->priv;
	struct doc_priv *doc = (void *)this->priv;
	unsigned long docptr = doc->virtadr;
	int i;

	/* Start read pipeline */
	ReadDOC(docptr, ReadPipeInit);

	for (i=0; i < len-1; i++)
		if (buf[i] != ReadDOC(docptr, Mil_CDSN_IO)) {
			ReadDOC(docptr, LastDataRead);
			return i;
		}
	if (buf[i] != ReadDOC(docptr, LastDataRead))
		return i;
	return 0;
}

static void doc200x_select_chip(struct mtd_info *mtd, int chip)
{
	struct nand_chip *this = mtd->priv;
	struct doc_priv *doc = (void *)this->priv;
	unsigned long docptr = doc->virtadr;
	int floor = 0;

	/* 11.4.4 -- deassert CE before changing chip */
	doc200x_hwcontrol(mtd, NAND_CTL_CLRNCE);

	if(debug)printk("select chip (%d)\n", chip);

	if (chip == -1)
		return;

	floor = chip / doc->chips_per_floor;
	chip -= (floor *  doc->chips_per_floor);

	WriteDOC(floor, docptr, FloorSelect);
	WriteDOC(chip, docptr, CDSNDeviceSelect);

	doc200x_hwcontrol(mtd, NAND_CTL_SETNCE);

	doc->curchip = chip;
	doc->curfloor = floor;
}

static void doc200x_hwcontrol(struct mtd_info *mtd, int cmd)
{
	struct nand_chip *this = mtd->priv;
	struct doc_priv *doc = (void *)this->priv;
	unsigned long docptr = doc->virtadr;

	switch(cmd) {
	case NAND_CTL_SETNCE:
		doc->CDSNControl |= CDSN_CTRL_CE;
		break;
	case NAND_CTL_CLRNCE:
		doc->CDSNControl &= ~CDSN_CTRL_CE;
		break;
	case NAND_CTL_SETCLE:
		doc->CDSNControl |= CDSN_CTRL_CLE;
		break;
	case NAND_CTL_CLRCLE:
		doc->CDSNControl &= ~CDSN_CTRL_CLE;
		break;
	case NAND_CTL_SETALE:
		doc->CDSNControl |= CDSN_CTRL_ALE;
		break;
	case NAND_CTL_CLRALE:
		doc->CDSNControl &= ~CDSN_CTRL_ALE;
		break;
	case NAND_CTL_SETWP:
		doc->CDSNControl |= CDSN_CTRL_WP;
		break;
	case NAND_CTL_CLRWP:
		doc->CDSNControl &= ~CDSN_CTRL_WP;
		break;
	}
	if (debug)printk("hwcontrol(%d): %02x\n", cmd, doc->CDSNControl);
	WriteDOC(doc->CDSNControl, docptr, CDSNControl);
	/* 11.4.3 -- 4 NOPs after CSDNControl write */
	DoC_Delay(doc, 4);
}

static int doc200x_dev_ready(struct mtd_info *mtd)
{
	struct nand_chip *this = mtd->priv;
	struct doc_priv *doc = (void *)this->priv;
	unsigned long docptr = doc->virtadr;

	/* 11.4.2 -- must NOP four times before checking FR/B# */
	DoC_Delay(doc, 4);
	if (!(ReadDOC(docptr, CDSNControl) & CDSN_CTRL_FR_B)) {
		if(debug)
			printk("not ready\n");
		return 0;
	}
	/* 11.4.2 -- Must NOP twice if it's ready */
	DoC_Delay(doc, 2);
	if (debug)printk("was ready\n");
	return 1; 
}	

static int doc200x_block_bad(struct mtd_info *mtd, unsigned long block)
{
	/* FIXME: Look it up in the BBT */
	return 0;
}

struct doc_priv mydoc = {
	.physadr = 0xd4000,
	.curfloor = -1,
	.curchip = -1,
};

u_char mydatabuf[528];

struct nand_chip mynand = {
	.priv = (void *)&mydoc,
	.select_chip = doc200x_select_chip,
	.hwcontrol = doc200x_hwcontrol,
	.dev_ready = doc200x_dev_ready,
	.waitfunc = doc200x_wait,
	.block_bad = doc200x_block_bad,
	.eccmode = NAND_ECC_SOFT,
	.data_buf = mydatabuf,
};

struct mtd_info mymtd = {
	.priv = (void *)&mynand,
	.owner = THIS_MODULE,
};

int __init init_nanddoc(void)
{
	mydoc.virtadr = (unsigned long)ioremap(mydoc.physadr, DOC_IOREMAP_LEN);
	int nrchips = 1;
	char *name;

	WriteDOC(DOC_MODE_CLR_ERR | DOC_MODE_MDWREN | DOC_MODE_RESET, 
		 mydoc.virtadr, DOCControl);
	WriteDOC(DOC_MODE_CLR_ERR | DOC_MODE_MDWREN | DOC_MODE_RESET, 
		 mydoc.virtadr, DOCControl);

	WriteDOC(DOC_MODE_CLR_ERR | DOC_MODE_MDWREN | DOC_MODE_NORMAL, 
		 mydoc.virtadr, DOCControl);
	WriteDOC(DOC_MODE_CLR_ERR | DOC_MODE_MDWREN | DOC_MODE_NORMAL, 
		 mydoc.virtadr, DOCControl);

	mydoc.ChipID = ReadDOC(mydoc.virtadr, ChipID);

	switch(mydoc.ChipID) {
	case DOC_ChipID_DocMil:
		mynand.write_byte = doc2001_write_byte;
		mynand.read_byte = doc2001_read_byte;
		mynand.write_buf = doc2001_writebuf;
		mynand.read_buf = doc2001_readbuf;
		mynand.verify_buf = doc2001_verifybuf;

		ReadDOC(mydoc.virtadr, ChipID);
		ReadDOC(mydoc.virtadr, ChipID);
		if (ReadDOC(mydoc.virtadr, ChipID) != DOC_ChipID_DocMil) {
			/* It's not a Millennium; it's one of the newer
			   DiskOnChip 2000 units with a similar ASIC. 
			   Treat it like a Millennium, except that it
			   can have multiple chips. */
			doc2000_count_chips(&mymtd);
			nrchips = 4 * mydoc.chips_per_floor;
			name = "DiskOnChip 2000 (INFTL Model)";
		} else {
			/* Bog-standard Millennium */
			mydoc.chips_per_floor = 1;
			nrchips = 1;
			name = "DiskOnChip Millennium";
		}
		break;

	case DOC_ChipID_Doc2k:
		mynand.write_byte = doc2000_write_byte;
		mynand.read_byte = doc2000_read_byte;
		mynand.write_buf = doc2000_writebuf;
		mynand.read_buf = doc2000_readbuf;
		mynand.verify_buf = doc2000_verifybuf;

		doc2000_count_chips(&mymtd);
		nrchips = 4 * mydoc.chips_per_floor;
		name = "DiskOnChip 2000 (NFTL Model)";
		mydoc.CDSNControl |= CDSN_CTRL_FLASH_IO;

		break;

	default:
		return -EIO;
	}
	if (nand_scan(&mymtd, nrchips)) {
		iounmap((void *)mydoc.virtadr);
		return -EIO;
	}
	mymtd.name = name;
	add_mtd_device(&mymtd);

	return 0;
}

void __exit cleanup_nanddoc(void)
{
	del_mtd_device(&mymtd);
	iounmap((void *)mydoc.virtadr);
}
	
module_init(init_nanddoc);
module_exit(cleanup_nanddoc);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("David Woodhouse <dwmw2@infradead.org>");
MODULE_DESCRIPTION("M-Systems DiskOnChip 2000 and Millennium device driver\n");
