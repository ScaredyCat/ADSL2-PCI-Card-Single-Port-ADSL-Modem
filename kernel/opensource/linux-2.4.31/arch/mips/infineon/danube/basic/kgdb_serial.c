/* KGDB support
 * integrated from U-boot cpu/mips/serial.c
 * Peng Liu (C) 2004 <liupeng@infineon.com>
 *
 * ########################################################################
 *
 *  This program is free software; you can distribute it and/or modify it
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
 *  KGDB support
 */
#include <linux/config.h>
#include <linux/init.h>
#include <asm/addrspace.h>
#include <asm/types.h>
#include <asm/danube/danube.h>
#include <asm/danube/serial.h>

#define ASC_FIFO_PRESENT

#define SET_BIT(reg, mask)                  reg |= (mask)
#define CLEAR_BIT(reg, mask)                reg &= (~mask)
#define CLEAR_BITS(reg, mask)               CLEAR_BIT(reg, mask)
#define SET_BITS(reg, mask)                 SET_BIT(reg, mask)
#define SET_BITFIELD(reg, mask, off, val)   {reg &= (~mask); reg |= (val << off);}

#define uint u32
#define ulong u32

#define CONFIG_BAUDRATE			115200

#define DANUBEASC_PMU_ENABLE(BIT) *((volatile ulong*)0xBF102000) |= (0x1 << BIT);

typedef  struct         /* danubeAsc_t */
{
    volatile unsigned long  asc_clc;                            /*0x0000*/
    volatile unsigned long  asc_pisel;                          /*0x0004*/
    volatile unsigned long  asc_rsvd1[2];   /* for mapping */   /*0x0008*/
    volatile unsigned long  asc_con;                            /*0x0010*/
    volatile unsigned long  asc_bg;                             /*0x0014*/
    volatile unsigned long  asc_fdv;                            /*0x0018*/
    volatile unsigned long  asc_pmw;        /* not used */      /*0x001C*/
    volatile unsigned long  asc_tbuf;                           /*0x0020*/
    volatile unsigned long  asc_rbuf;                           /*0x0024*/
    volatile unsigned long  asc_rsvd2[2];   /* for mapping */   /*0x0028*/
    volatile unsigned long  asc_abcon;                          /*0x0030*/
    volatile unsigned long  asc_abstat;     /* not used */      /*0x0034*/
    volatile unsigned long  asc_rsvd3[2];   /* for mapping */   /*0x0038*/
    volatile unsigned long  asc_rxfcon;                         /*0x0040*/
    volatile unsigned long  asc_txfcon;                         /*0x0044*/
    volatile unsigned long  asc_fstat;                          /*0x0048*/
    volatile unsigned long  asc_rsvd4;      /* for mapping */   /*0x004C*/
    volatile unsigned long  asc_whbcon;                         /*0x0050*/
    volatile unsigned long  asc_whbabcon;                       /*0x0054*/
    volatile unsigned long  asc_whbabstat;  /* not used */      /*0x0058*/

} danubeAsc_t;


static int serial_setopt (void);
void serial_setbrg (void);
/* pointer to ASC register base address */
static volatile danubeAsc_t *pAsc = (danubeAsc_t *)DANUBE_ASC1;

void serial_setbrg (void)
{
    ulong      uiReloadValue, fdv;
    ulong      f_ASC;

    f_ASC = danube_get_fpi_hz();

#ifndef DANUBEASC_USE_FDV
    fdv = 2;
    uiReloadValue = (f_ASC / (fdv * 16 * CONFIG_BAUDRATE)) - 1;
#else 
    fdv = DANUBEASC_FDV_HIGH_BAUDRATE;
    uiReloadValue = (f_ASC / (8192 * CONFIG_BAUDRATE / fdv)) - 1;
#endif /* DANUBEASC_USE_FDV */
    
    if ( (uiReloadValue < 0) || (uiReloadValue > 8191) )
    {
#ifndef DANUBEASC_USE_FDV
        fdv = 3;
        uiReloadValue = (f_ASC / (fdv * 16 * CONFIG_BAUDRATE)) - 1;
#else 
        fdv = DANUBEASC_FDV_LOW_BAUDRATE;
        uiReloadValue = ((f_ASC/100)*fdv / (8192 * (CONFIG_BAUDRATE/100) )) - 1;
#endif /* DANUBEASC_USE_FDV */
        
        if ( (uiReloadValue < 0) || (uiReloadValue > 8191) )
        {
            return;    /* can't impossibly generate that baud rate */
        }
    }

    /* Disable Baud Rate Generator; BG should only be written when R=0 */
    CLEAR_BIT(pAsc->asc_con, ASCCON_R);

#ifndef DANUBEASC_USE_FDV
    /*
     * Disable Fractional Divider (FDE)
     * Divide clock by reload-value + constant (BRS)
     */
    /* FDE = 0 */
    CLEAR_BIT(pAsc->asc_con, ASCCON_FDE);

    if ( fdv == 2 )
        CLEAR_BIT(pAsc->asc_con, ASCCON_BRS);   /* BRS = 0 */
    else
        SET_BIT(pAsc->asc_con, ASCCON_BRS); /* BRS = 1 */

#else /* DANUBEASC_USE_FDV */

    /* Enable Fractional Divider */
    SET_BIT(pAsc->asc_con, ASCCON_FDE); /* FDE = 1 */

    /* Set fractional divider value */
    pAsc->asc_fdv = fdv & ASCFDV_VALUE_MASK;

#endif /* DANUBEASC_USE_FDV */

    /* Set reload value in BG */
    pAsc->asc_bg = uiReloadValue;

    /* Enable Baud Rate Generator */
    SET_BIT(pAsc->asc_con, ASCCON_R);           /* R = 1 */
}

/*******************************************************************************
*
* serial_setopt - set the serial options
*
* Set the channel operating mode to that specified. Following options
* are supported: CREAD, CSIZE, PARENB, and PARODD.
*
* Note, this routine disables the transmitter.  The calling routine
* may have to re-enable it.
*
* RETURNS:
* Returns 0 to indicate success, otherwise -1 is returned
*/

static int serial_setopt (void)
{
	//TODO
	return 0;
}

void serial_putc (const char c)
{
	//TODO:
}

void serial_puts (const char *s)
{
    while (*s)
    {
	serial_putc (*s++);
    }
}

int serial_tstc (void)
{
	//TODO:
	int res = 1;
	return res;
}

int serial_getc (void)
{
    ulong symbol_mask;
    char c;

    while (!serial_tstc());

    symbol_mask =
	((ASC_OPTIONS & ASCOPT_CSIZE) == ASCOPT_CS7) ? (0x7f) : (0xff);
    
    c = (char)(pAsc->asc_rbuf & symbol_mask);

#ifndef ASC_FIFO_PRESENT
    *(volatile unsigned long*)(SFPI_INTCON_BASEADDR + FBS_ISR) = FBS_ISR_AR;
#endif

    return c;
}


//for KGDB
int putDebugChar(char c)
{
	serial_putc(c);
	return 1;
}

char getDebugChar(void)
{
	return serial_getc();
}

/******************************************************************************
*
* serial_init - initialize a DANUBEASC channel
*
* This routine initializes the number of data bits, parity
* and set the selected baud rate. Interrupts are disabled.
* Set the modem control signals if the option is selected.
*
* RETURNS: N/A
*/

int kgdb_serial_init (void)
{
	//TODO:
	return 0;
}


