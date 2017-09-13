/* $Id: peb3086.c,v 1.1.4.1  2007/1/4 03:18¤U¤È Exp $
 *
 * low level stuff for PEB3086 isdn module
 *
 * Author       Aceex corp.
 *              based on source code from Karsten Keil
 * Copyright    by Aceex corp.
 * 
 * This software may be used and distributed according to the terms
 * of the GNU General Public License, incorporated herein by reference.
 *
 */

#include <linux/config.h>
#include <linux/init.h>
#define __NO_VERSION__
#include "hisax.h"
#include "isac.h"
#include "hscx.h"
#include "isdnl1.h"
#include <asm/danube/danube.h>
#include <asm/danube/irq.h>
#include <asm/danube/ifx_ssc_defines.h>
#include <asm/danube/ifx_ssc.h>
#include <asm/danube/ifx_peripheral_definitions.h>
extern int ifx_ssc_open(struct inode *, struct file *);
extern int ifx_ssc_close(struct inode *, struct file *);
extern int ifx_ssc_cs_low(u32 pin);
extern int ifx_ssc_cs_high(u32 pin);
extern int ifx_ssc_txrx(char * tx_buf, u32 tx_len, char * rx_buf, u32 rx_len);
extern int ifx_ssc_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
extern const char *CardType[];
const char *peb3086_revision = "$Revision: 1.1.4.1 $";

/* Interface functions */

static u_char
ReadISAC(struct IsdnCardState *cs, u_char offset)
{
	u_char value=0;
	u_char *tx_buf=NULL, *rx_buf, *p;
	u32 flags;
	
	p=tx_buf;
	if(offset < 0x80)
		*p++ = 0x48;
	else
		*p++ = 0x4C;
	
	*p++ = offset|0x80;
	save_and_cli(flags);
	if(ifx_ssc_open((struct inode*)0, NULL)!=-EBUSY)
  {	
		ifx_ssc_cs_low(0x10);
		ifx_ssc_txrx(tx_buf, 2, rx_buf, 1);
		value = *rx_buf;	
		ifx_ssc_cs_high(0x10);
		ifx_ssc_close((struct inode*)0, NULL);
	}	
	restore_flags(flags);
	return (value);
	
}

static void
WriteISAC(struct IsdnCardState *cs, u_char offset, u_char value)
{
	u_char *tx_buf=NULL, *p;
	u32 flags;
	
	p=tx_buf;
	if(offset < 0x80)
		*p++ = 0x48;
	else
		*p++ = 0x4C;
	
	*p++ = offset&0x7f;
	*p++ = value;
	save_and_cli(flags);
	if(ifx_ssc_open((struct inode*)0, NULL)!=-EBUSY)
  {	
		ifx_ssc_cs_low(0x10);
		ifx_ssc_txrx(tx_buf, 3, NULL, 0); 
		ifx_ssc_cs_high(0x10);
		ifx_ssc_close((struct inode*)0, NULL);
	}	
	restore_flags(flags);
}

static void
ReadISACfifo(struct IsdnCardState *cs, u_char * data, int size)
{
	u_char *tx_buf=NULL, *p;
	u32 flags;
	
	p=tx_buf;
	*p++ = 0x43;
	*p++ = 0x80;
	save_and_cli(flags);
	if(ifx_ssc_open((struct inode*)0, NULL)!=-EBUSY)
  {	
		ifx_ssc_cs_low(0x10);
		ifx_ssc_txrx(tx_buf, 2, data, size);
		ifx_ssc_cs_high(0x10);
		ifx_ssc_close((struct inode*)0, NULL);
	}	
	restore_flags(flags);
}

static void
WriteISACfifo(struct IsdnCardState *cs, u_char * data, int size)
{
	u_char *tx_buf=NULL, *p;
	int i;
	u32 flags;
	
	p=tx_buf;
	*p++ = 0x43;
	*p++ = 0;
	*p++ = data;
	for(i = 0; i < size; i++)
	 *p++ = *data++;
	save_and_cli(flags);
	if(ifx_ssc_open((struct inode*)0, NULL)!=-EBUSY)
  {	 
		ifx_ssc_cs_low(0x10);
		ifx_ssc_txrx(tx_buf, size+2, NULL, 0);
		ifx_ssc_cs_high(0x10);
		ifx_ssc_close((struct inode*)0, NULL);
	}	
	restore_flags(flags);
}


static void
peb3086_interrupt(int intno, void *dev_id, struct pt_regs *regs)
{
#define MAXCOUNT 5
	struct IsdnCardState *cs = dev_id;
	u_char valisac;
	int count = 0;

	if (!cs) {
		printk(KERN_WARNING "PEB3086: Spurious interrupt!\n");
		return;
	}
	do {

		valisac = ReadISAC(cs, ISAC_ISTAD);
		if (valisac)
			isac_interrupt(cs, valisac);
		count++;
	} while ( valisac && (count < MAXCOUNT));


	WriteISAC(cs, ISAC_MASKD, 0xFF);
	WriteISAC(cs, ISAC_MASKD, 0x0);
}



static int
reset_peb3086(struct IsdnCardState *cs)
{
			/* P1.4 set to ISDN_RESET altsel0=0, altsel1=0 dir=1 */
			*(DANUBE_GPIO_P1_DIR) = ((*DANUBE_GPIO_P1_DIR)|(0x10));
			*(DANUBE_GPIO_P1_ALTSEL0) = ((*DANUBE_GPIO_P1_ALTSEL0)&(~0x10));
			*(DANUBE_GPIO_P1_ALTSEL1) = ((*DANUBE_GPIO_P1_ALTSEL1)&(~0x10));
			*DANUBE_GPIO_P1_OUT = ((*DANUBE_GPIO_P1_OUT)&(~0x10));
			HZDELAY(10);
			*DANUBE_GPIO_P1_OUT = ((*DANUBE_GPIO_P1_OUT)|(0x10));
	return (0);
}

static int
PEB3086_card_msg(struct IsdnCardState *cs, int mt, void *arg)
{
	switch (mt) {
		case CARD_RESET:
			reset_peb3086(cs);
			return (0);
		case CARD_INIT:
			initisac(cs);
			return (0);
		case CARD_TEST:
			return (0);
	}
	return (0);
}


static int __init
init_peb3086(struct IsdnCard *card, struct IsdnCardState *cs)
{
	printk(KERN_INFO "PEB3086: Module control by SPI \n");

	cs->hw.peb3086.spi_cs = IFX_SSC_DEF_GPO_CS;
	
	    /*GPIO1 ISDN_INT used as EXINT*/
    *DANUBE_GPIO_P0_DIR     &= ~(1<<1);
    *DANUBE_GPIO_P0_ALTSEL0 |=  (1<<1);
    *DANUBE_GPIO_P0_ALTSEL1 &= ~(1<<1);
    *DANUBE_GPIO_P0_OD      |=  (1<<1);

    /*set EXTIN1 to falling edge and enable */
    (*DANUBE_ICU_EIU_EXIN_C) |= 0x20;
    (*DANUBE_ICU_EIU_INEN)   |= 0x02;
    if (request_irq (INT_NUM_IM3_IRL31, peb3086_interrupt,
                     SA_INTERRUPT, "ISDN", (void *)cs) != 0)
    {
    	printk(KERN_WARNING  "PEB3086: request interrupt failed\n");
    }
    
	return (0);
}

int __init
setup_peb3086(struct IsdnCard *card)
{
	struct IsdnCardState *cs = card->cs;
	char tmp[64];
	u_char val;

	strcpy(tmp, peb3086_revision);
	printk(KERN_INFO "PEB3086: Driver Revision %s\n", HiSax_getrev(tmp));

	if (cs->typ != ISDN_CTYPE_PEB3086)
		return (0);

	if (init_peb3086(card, cs))
		return (0);

	if (reset_peb3086(cs)) 
		return (0);
		
	cs->readisac = &ReadISAC;
	cs->writeisac = &WriteISAC;
	cs->readisacfifo = &ReadISACfifo;
	cs->writeisacfifo = &WriteISACfifo;
/*	
	cs->BC_Read_Reg = &ReadHSCX;
	cs->BC_Write_Reg = &WriteHSCX;
	cs->BC_Send_Data = &hscx_fill_fifo;
*/	
	cs->cardmsg = &PEB3086_card_msg;
	cs->irq_func = &peb3086_interrupt;
	ISACVersion(cs, "PEB3086:");

	return (1);
}
