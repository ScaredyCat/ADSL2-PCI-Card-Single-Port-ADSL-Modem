/*
 *  linux/drivers/char/danube_asc.c
 *
 *  Driver for DANUBEASC serial ports
 *
 *  Copyright (C) 2004 Infineon IFAP DC COM CPE
 *
 *  Based on drivers/char/serial.c, by Linus Torvalds, Theodore Ts'o.
 *  Based on drivers/serial/serial_s3c2400.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  $Id: danube_asc.c,v 1.7 2006/03/27 10:02:20 pliu Exp $
 *
 * This is a generic driver for DANUBEASC-type serial ports.
 */
#include <linux/config.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/major.h>
#include <linux/string.h>
#include <linux/fcntl.h>
#include <linux/ptrace.h>
#include <linux/ioport.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/circ_buf.h>
#include <linux/serial.h>
#include <linux/console.h>
#include <linux/sysrq.h>

#include <asm/system.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/bitops.h>
#include <asm/danube/danube.h>
#include <asm/danube/irq.h>
#include <asm/danube/serial.h>

#if defined(CONFIG_SERIAL_DANUBEASC_CONSOLE) && defined(CONFIG_MAGIC_SYSRQ)
#define SUPPORT_SYSRQ
#endif

#include <linux/serial_core.h>

#define UART_NR		1

#define SERIAL_DANUBEASC_MAJOR	TTY_MAJOR
#define CALLOUT_DANUBEASC_MAJOR	TTYAUX_MAJOR
#define SERIAL_DANUBEASC_MINOR	64
#define SERIAL_DANUBEASC_NR	UART_NR
#define PORT_DANUBEASC  112

static struct tty_driver normal, callout;
static struct tty_struct *danubeasc_table[UART_NR];
static struct termios *danubeasc_termios[UART_NR], *danubeasc_termios_locked[UART_NR];
static struct uart_port danubeasc_ports[UART_NR];
static struct uart_info *danubeasc_info;
#ifdef CONFIG_SERIAL_DANUBEASC_CONSOLE /*SUPPORT_SYSRQ*/
static struct console danubeasc_console;
#endif
static unsigned int uartclk = 0;

#define SET_BIT(reg, mask)    *reg |= (mask)
#define CLEAR_BIT(reg, mask)  *reg &= (~mask) 
#define CLEAR_BITS(reg, mask) CLEAR_BIT(reg, mask)
#define SET_BITS(reg, mask)   SET_BIT(reg, mask)
#define SET_BITFIELD(reg, mask, off, val) \
				{*reg &= (~mask); *reg |= (val << off);}

extern void mask_and_ack_danube_irq(unsigned int irq_nr);

/* forward declaration to shut the compiler up */
static void danubeasc_rx_int(int, void *, struct pt_regs *);
static void danubeasc_tx_chars(struct uart_info *info);

/* fake flag to indicate CREAD was not set -> throw away all bytes */
#define UART_DUMMY_UER_RX 1

#define ENABLE_IRQ(n) enable_irq((n))
#define DISABLE_IRQ(n) disable_irq((n))
static int asc_tx_irq_on=0;

/* macro to set the bit corresponding to an interrupt number */
#define BIT_NO(irq) (1 << (irq - 64))

#define  SERIAL_DEBUG
#undef TX_POLL_MODE		/* POLL base driver */

#ifdef SERIAL_DEBUG
/* for debugging */
char serdebug[256];
char *sbptr = NULL;
#define SERDEBUG(f) \
if (sbptr == NULL) sbptr = serdebug; \
sbptr += sprintf(sbptr, f); \
if ((sbptr - serdebug) > 245) \
sbptr = serdebug;
#else
#define SERDEBUG(f)
#endif

extern unsigned int danube_get_fpi_hz(void);

static void danubeasc_stop_tx(struct uart_port *port, u_int from_tty)
{
SERDEBUG("-1")
  if (asc_tx_irq_on){
	  DISABLE_IRQ(DANUBEASC1_TIR);
    asc_tx_irq_on = 0;
  }
}

static void danubeasc_start_tx(struct uart_port *port, u_int nonempty, u_int from_tty)
{
SERDEBUG("+1")
	if (danubeasc_info)
		danubeasc_tx_chars(danubeasc_info);
#ifndef TX_POLL_MODE
  if (asc_tx_irq_on==0){
	  ENABLE_IRQ(DANUBEASC1_TIR);
    asc_tx_irq_on ++;
  }
#endif

}

static void danubeasc_stop_rx(struct uart_port *port)
{
SERDEBUG("-2")
	/* clear the RX enable bit */
	*DANUBE_ASC1_WHBSTATE = ASCWHBSTATE_CLRREN;
}

static void danubeasc_enable_ms(struct uart_port *port)
{
	/* no modem signals */
	return;
}

static void
#ifdef SUPPORT_SYSRQ
danubeasc_rx_chars(struct uart_info *info, struct pt_regs *regs)
#else
danubeasc_rx_chars(struct uart_info *info)
#endif
{
	struct tty_struct *tty = info->tty;
	unsigned int ch = 0, rsr = 0, fifocnt;
	struct uart_port *port = info->port;

SERDEBUG("R ")
	fifocnt = *DANUBE_ASC1_FSTAT & ASCFSTAT_RXFFLMASK;
	while (fifocnt--)
	{
		ch = *DANUBE_ASC1_RBUF;
		rsr = (*DANUBE_ASC1_STATE & ASCSTATE_ANY) | UART_DUMMY_UER_RX;

		if (tty->flip.count >= TTY_FLIPBUF_SIZE) {
			tty->flip.tqueue.routine((void *)tty);
			if (tty->flip.count >= TTY_FLIPBUF_SIZE) {
				printk(KERN_WARNING "TTY_DONT_FLIP set\n");
				return;
			}
		}

		*tty->flip.char_buf_ptr = ch;
		*tty->flip.flag_buf_ptr = TTY_NORMAL;
		port->icount.rx++;

		/*
		 * Note that the error handling code is
		 * out of the main execution path
		 */
		if (rsr & ASCSTATE_ANY) {
			if (rsr & ASCSTATE_PE) {
SERDEBUG("P ")
				port->icount.parity++;
				SET_BIT(DANUBE_ASC1_WHBSTATE, ASCWHBSTATE_CLRPE);
			} else if (rsr & ASCSTATE_FE) {
SERDEBUG("F ")
				port->icount.frame++;
				SET_BIT(DANUBE_ASC1_WHBSTATE, ASCWHBSTATE_CLRFE);
			}
			if (rsr & ASCSTATE_ROE) {
SERDEBUG("O ")
				port->icount.overrun++;
				SET_BIT(DANUBE_ASC1_WHBSTATE, ASCWHBSTATE_CLRROE);
			}

			rsr &= port->read_status_mask;

			if (rsr & ASCSTATE_PE)
				*tty->flip.flag_buf_ptr = TTY_PARITY;
			else if (rsr & ASCSTATE_FE)
				*tty->flip.flag_buf_ptr = TTY_FRAME;
		}

#ifdef SUPPORT_SYSRQ
		if (uart_handle_sysrq_char(info, ch, regs))
			continue;
#endif

		if ((rsr & port->ignore_status_mask) == 0) {
			tty->flip.flag_buf_ptr++;
			tty->flip.char_buf_ptr++;
			tty->flip.count++;
		}
		if ((rsr & ASCSTATE_ROE) &&
		    tty->flip.count < TTY_FLIPBUF_SIZE) {
			/*
			 * Overrun is special, since it's reported
			 * immediately, and doesn't affect the current
			 * character
			 */
			*tty->flip.char_buf_ptr++ = 0;
			*tty->flip.flag_buf_ptr++ = TTY_OVERRUN;
			tty->flip.count++;
		}
	}
	if (ch != 0)
		tty_flip_buffer_push(tty);
	return;
}

#ifdef TX_POLL_MODE
static void danubeasc_print_one_char(char c)
{
	u32 fifocnt=0;
	do
	{
		fifocnt = (*DANUBE_ASC1_FSTAT & ASCFSTAT_TXFFLMASK)
				>> ASCFSTAT_TXFFLOFF;
	} while (fifocnt == DANUBEASC_TXFIFO_FULL);
	if (c == '\n')
	{
		*DANUBE_ASC1_TBUF = '\r';
		do
		{
			fifocnt = (*DANUBE_ASC1_FSTAT &
			ASCFSTAT_TXFFLMASK) >> ASCFSTAT_TXFFLOFF;
		} while (fifocnt == DANUBEASC_TXFIFO_FULL);
	}
	*DANUBE_ASC1_TBUF = c;

}
#endif


static void danubeasc_tx_chars(struct uart_info *info)
{
	struct uart_port *port = info->port;

	
	//printk("head=%d tail=%d stopped=%d hw_stopped=%d\n", info->xmit.head, info->xmit.tail, info->tty->stopped,info->tty->hw_stopped);
	if (info->xmit.head == info->xmit.tail
	    || info->tty->stopped
	    || info->tty->hw_stopped) {
SERDEBUG("TX ")
#ifndef	TX_POLL_MODE
		danubeasc_stop_tx(port, 0);
#endif
		return;
	}

#ifdef	TX_POLL_MODE

	while(1){

#else
	while (((*DANUBE_ASC1_FSTAT & ASCFSTAT_TXFFLMASK)
			>> ASCFSTAT_TXFFLOFF) != DANUBEASC_TXFIFO_FULL)
	{
#endif
		if (port->x_char) {
#ifdef TX_POLL_MODE
			danubeasc_print_one_char(port->x_char);			
#else
			*DANUBE_ASC1_TBUF = port->x_char;
#endif
			port->icount.tx++;
			port->x_char = 0;
			continue;
		}
#ifdef TX_POLL_MODE
		danubeasc_print_one_char(info->xmit.buf[info->xmit.tail]);
#else
		*DANUBE_ASC1_TBUF = info->xmit.buf[info->xmit.tail];
#endif
		info->xmit.tail = (info->xmit.tail + 1) & (UART_XMIT_SIZE - 1);
		port->icount.tx++;
		if (info->xmit.head == info->xmit.tail)
			break;
	}

	if (CIRC_CNT(info->xmit.head, info->xmit.tail, UART_XMIT_SIZE) <
			WAKEUP_CHARS)
		uart_event(info, EVT_WRITE_WAKEUP);

	if (info->xmit.head == info->xmit.tail)
	{
		SERDEBUG("ZZ")
#ifndef	TX_POLL_MODE		
		danubeasc_stop_tx(info->port, 0);
#endif
	}
}

static void danubeasc_tx_int(int irq, void *dev_id, struct pt_regs *regs)
{
	struct uart_info *info = dev_id;
#ifndef TX_POLL_MODE
	*(DANUBE_ASC1_IRNCR) = ASC_IRNCR_TIR;
#endif
SERDEBUG("IT ")
	/* this interrupt tells us that the TXB is empty - fill it */
	danubeasc_tx_chars(info);
	/*clear any pending interrupts */
#ifndef TX_POLL_MODE	
	mask_and_ack_danube_irq(DANUBEASC1_TIR);
#endif
}

static void danubeasc_er_int(int irq, void *dev_id, struct pt_regs *regs)
{
SERDEBUG("ER ")
	/* XXX */
	/* clear any pending interrupts */
	SET_BIT(DANUBE_ASC1_WHBSTATE, ASCWHBSTATE_CLRPE);
	SET_BIT(DANUBE_ASC1_WHBSTATE, ASCWHBSTATE_CLRFE);
	SET_BIT(DANUBE_ASC1_WHBSTATE, ASCWHBSTATE_CLRROE);
	return;
}

static void danubeasc_rx_int(int irq, void *dev_id, struct pt_regs *regs)
{
	struct uart_info *info = dev_id;
	*(DANUBE_ASC1_IRNCR) = ASC_IRNCR_RIR;	
SERDEBUG("IRX ")
#ifdef SUPPORT_SYSRQ
	danubeasc_rx_chars(info, regs);
#else
	danubeasc_rx_chars(info);
#endif
	/*clear any pending interrupts */
	mask_and_ack_danube_irq(DANUBEASC1_RIR);
}

static u_int danubeasc_tx_empty(struct uart_port *port)
{
	int status;

	/*
	 * FSTAT tells exactly how many bytes are in the FIFO.
	 * The question is whether we really need to wait for all
	 * 16 bytes to be transmitted before reporting that the
	 * transmitter is empty.
	 */
	status = *DANUBE_ASC1_FSTAT & ASCFSTAT_TXFFLMASK;
	return status ? 0 : TIOCSER_TEMT;
}

static u_int danubeasc_get_mctrl(struct uart_port *port)
{
	/* no modem control signals - the readme says to pretend all are set */
	return TIOCM_CTS|TIOCM_CAR|TIOCM_DSR;
}

static void danubeasc_set_mctrl(struct uart_port *port, u_int mctrl)
{
	/* no modem control - just return */
	return;
}

static void danubeasc_break_ctl(struct uart_port *port, int break_state)
{
	/* no way to send a break */
	return;
}
static void danubeasc1_hw_init()
{
  u32 con;
	/* this setup was probably already done in ROM/u-boot */
	/* ASC and GPIO Port 1 bits 3 and 4 share the same pins
	 * TODO: GPIO pins
	 */
	/* set up the CLC */
	CLEAR_BIT(DANUBE_ASC1_CLC, DANUBE_ASC1_CLC_DISS);
	SET_BITFIELD(DANUBE_ASC1_CLC, ASCCLC_RMCMASK, ASCCLC_RMCOFFSET, 1);
	/* asynchronous mode */
	con = ASCCON_M_8ASYNC;
	/* set error signals  - framing and overrun */
	con |= ASCCON_FEN;
	con |= ASCCON_TOEN;
	con |= ASCCON_ROEN;
	/* choose the line - there's only one */
	*DANUBE_ASC1_PISEL = 0;
	*DANUBE_ASC1_TXFCON = (((DANUBEASC_TXFIFO_FL<<ASCTXFCON_TXFITLOFF)&ASCTXFCON_TXFITLMASK) | ASCTXFCON_TXFEN |ASCTXFCON_TXFFLU);
	*DANUBE_ASC1_RXFCON = (((DANUBEASC_RXFIFO_FL<<ASCRXFCON_RXFITLOFF)&ASCRXFCON_RXFITLMASK) | ASCRXFCON_RXFEN |ASCRXFCON_RXFFLU);
	wmb();
	SET_BIT(DANUBE_ASC1_CON,con);
}
static int danubeasc_startup(struct uart_port *port, struct uart_info *info)
{
	int retval;
	u_int con = 0;
	unsigned long flags;
	
//TODO: FIXME
SERDEBUG("SUP ");
	/* this assumes: CON.BRS = CON.FDE = 0 */
	if (uartclk == 0)
		uartclk = danube_get_fpi_hz();

	danubeasc_ports[0].uartclk = uartclk;
	danubeasc_info = info;

  danubeasc1_hw_init();
	/* block the IRQs */
	local_irq_save(flags);

	/* Allocate the IRQs */

	retval = request_irq(DANUBEASC1_RIR, danubeasc_rx_int, 0, "asc_rx", info);
	if (retval)
		return retval;

#ifndef	TX_POLL_MODE
	retval = request_irq(DANUBEASC1_TIR, danubeasc_tx_int, 0, "asc_tx", info);
	if (retval)
	{
		free_irq(DANUBEASC1_RIR, info);
		return retval;
	}
	DISABLE_IRQ(DANUBEASC1_TIR);
  asc_tx_irq_on = 0;
#endif
	retval = request_irq(DANUBEASC1_EIR, danubeasc_er_int, 0, "asc_er", info);
	if (retval)
	{
		free_irq(DANUBEASC1_RIR, info);
#ifndef	TX_POLL_MODE
		free_irq(DANUBEASC1_TIR, info);
#endif
		return retval;
	}
  /* enable interrupt mask in peripheral*/
  /* TODO: */
  *DANUBE_ASC1_IRNREN= 7;
	/* unblock the IRQs */
	local_irq_restore(flags);
	return 0;
}

static void danubeasc_shutdown(struct uart_port *port, struct uart_info *info)
{
	/*
	 * Free the interrupts
	 */
	free_irq(DANUBEASC1_RIR, info);
#ifndef	TX_POLL_MODE
	free_irq(DANUBEASC1_TIR, info);
#endif
	free_irq(DANUBEASC1_EIR, info);
	/*
	 * disable the baudrate generator to disable the ASC
	 */
	// *DANUBE_ASC1_CON = 0;

	/* flush and then disable the fifos */
	SET_BIT(DANUBE_ASC1_RXFCON, ASCRXFCON_RXFFLU);
	CLEAR_BIT(DANUBE_ASC1_RXFCON, ASCRXFCON_RXFEN);
	SET_BIT(DANUBE_ASC1_TXFCON, ASCTXFCON_TXFFLU);
	CLEAR_BIT(DANUBE_ASC1_TXFCON, ASCTXFCON_TXFEN);
}

static void danubeasc_change_speed(struct uart_port *port, u_int cflag, u_int iflag, u_int quot)
{
	u_int con = 0;
	unsigned long flags;

	/* byte size and parity */
	switch (cflag & CSIZE) {
	/* 7 bits are always with parity */
	case CS7: con = ASCCON_M_7ASYNC; break;
	/* the ASC only suports 7 and 8 bits */
	case CS5:
	case CS6:
	default:
		/*
		if (cflag & PARENB)
			con = ASCCON_M_8ASYNCPAR;
		else
		*/
		con = ASCCON_M_8ASYNC;
		break;
	}
	if (cflag & CSTOPB)
		con |= ASCCON_STP;
	if (cflag & PARENB) {
		if (!(cflag & PARODD))
			con &= ~ASCCON_ODD;
		else
			con |= ASCCON_ODD;
	}

	port->read_status_mask = ASCSTATE_ROE;
	if (iflag & INPCK)
		port->read_status_mask |= ASCSTATE_FE | ASCSTATE_PE;
	/* the ASC can't really detect or generate a BREAK */
#if 0
	if (iflag & (BRKINT | PARMRK))
		port->read_status_mask |= UERSTAT_BREAK;
#endif
	/*
	 * Characters to ignore
	 */
	port->ignore_status_mask = 0;
	if (iflag & IGNPAR)
		port->ignore_status_mask |= ASCSTATE_FE | ASCSTATE_PE;
#if 0
	/* always ignore breaks - the ASC can't handle them XXXX */
	port->ignore_status_mask |= UERSTAT_BREAK;
#endif
	if (iflag & IGNBRK) {
		/*port->ignore_status_mask |= UERSTAT_BREAK;*/
		/*
		 * If we're ignoring parity and break indicators,
		 * ignore overruns too (for real raw support).
		 */
		if (iflag & IGNPAR)
			port->ignore_status_mask |= ASCSTATE_ROE;
	}

	/*
	 * Ignore all characters if CREAD is not set.
	 */
	if ((cflag & CREAD) == 0)
		port->ignore_status_mask |= UART_DUMMY_UER_RX;

	/* set error signals  - framing, parity  and overrun */
	con |= ASCCON_FEN;
	con |= ASCCON_TOEN;
	con |= ASCCON_ROEN;
	/*TODO: enable the baudrate */

	/* block the IRQs */
	local_irq_save(flags);
	/* set up CON */
	SET_BIT(DANUBE_ASC1_CON,con);

	/* Set baud rate - take a divider of 2 into account */
  /* quot is (f / (16 * Baudrate)) see uart_calculate_quot */
	quot = quot/2 - 1;
	/* the next 3 probably already happened when we set CON above */
#if 0
  printk("quot=%d\n",quot);
	/* disable the baudrate generator */
	CLEAR_BIT(DANUBE_ASC1_CON, ASCCON_R);
	/* Enable Fractional Divider */
	SET_BIT(DANUBE_ASC1_CON, ASCCON_FDE);
	/* Set fractional divider value */
	*DANUBE_ASC1_FDV = 503;
	/* Set reload value in BG */
	*DANUBE_ASC1_BG = 3;
#else
	/* disable the baudrate generator */
	CLEAR_BIT(DANUBE_ASC1_CON, ASCCON_R);
	/* make sure the fractional divider is off */
	CLEAR_BIT(DANUBE_ASC1_CON, ASCCON_FDE);
	/* set up to use divisor of 2 */
	CLEAR_BIT(DANUBE_ASC1_CON, ASCCON_BRS);
	/* now we can write the new baudrate into the register */
	*DANUBE_ASC1_BG = quot;
#endif
	/* turn the baudrate generator back on */
	SET_BIT(DANUBE_ASC1_CON, ASCCON_R);

  /* enable rx */
	*DANUBE_ASC1_WHBSTATE = ASCWHBSTATE_SETREN;
	/* unblock the IRQs */
	local_irq_restore(flags);
}

static const char *danubeasc_type(struct uart_port *port)
{
	return port->type == PORT_DANUBEASC ? "DANUBEASC" : NULL;
}

/*
 * Release the memory region(s) being used by 'port'
 */
static void danubeasc_release_port(struct uart_port *port)
{
	return;
}

/*
 * Request the memory region(s) being used by 'port'
 */
static int danubeasc_request_port(struct uart_port *port)
{
	return 0;
}

/*
 * Configure/autoconfigure the port.
 */
static void danubeasc_config_port(struct uart_port *port, int flags)
{
	if (flags & UART_CONFIG_TYPE) {
		port->type = PORT_DANUBEASC;
		danubeasc_request_port(port);
	}
}

/*
 * verify the new serial_struct (for TIOCSSERIAL).
 */
static int danubeasc_verify_port(struct uart_port *port, struct serial_struct *ser)
{
	int ret = 0;
	if (ser->type != PORT_UNKNOWN && ser->type != PORT_DANUBEASC)
		ret = -EINVAL;
	if (ser->irq < 0 || ser->irq >= NR_IRQS)
		ret = -EINVAL;
	if (ser->baud_base < 9600)
		ret = -EINVAL;
	return ret;
}

static struct uart_ops danubeasc_pops = {
	tx_empty:	danubeasc_tx_empty,
	set_mctrl:	danubeasc_set_mctrl,
	get_mctrl:	danubeasc_get_mctrl,
	stop_tx:	danubeasc_stop_tx,
	start_tx:	danubeasc_start_tx,
	stop_rx:	danubeasc_stop_rx,
	enable_ms:	danubeasc_enable_ms,
	break_ctl:	danubeasc_break_ctl,
	startup:	danubeasc_startup,
	shutdown:	danubeasc_shutdown,
	change_speed:	danubeasc_change_speed,
	type:		danubeasc_type,
	release_port:	danubeasc_release_port,
	request_port:	danubeasc_request_port,
	config_port:	danubeasc_config_port,
	verify_port:	danubeasc_verify_port,
};

static struct uart_port danubeasc_ports[UART_NR] = {
	{
		membase:	(void *)DANUBE_ASC1,
		mapbase:	DANUBE_ASC1,
		iotype:		SERIAL_IO_MEM,
		irq:		DANUBEASC1_RIR, /* RIR */
		uartclk:	0, /* filled in dynamically */
		fifosize:	16,
		unused:		{ DANUBEASC1_TIR, DANUBEASC1_EIR}, /* xmit/error/xmit-buffer-empty IRQ */
		type:		PORT_DANUBEASC,
		ops:		&danubeasc_pops,
		flags:		ASYNC_BOOT_AUTOCONF,
	},
};

#ifdef CONFIG_SERIAL_DANUBEASC_CONSOLE
#ifdef used_and_not_const_char_pointer
static int danubeasc_console_read(struct uart_port *port, char *s, u_int count)
{
	unsigned int status;
	int fifocnt;
	int c;
	unsigned long flags;
#if DEBUG
	printk("danubeasc_console_read() called\n");
#endif

	c = 0;
	/* block the IRQ */
SERDEBUG("-8")
	local_irq_save(flags);

	fifocnt = *DANUBE_ASC1_FSTAT & ASCFSTAT_RXFFLMASK;
	while (c < count && fifocnt) {
		*s++ = DANUBE_ASC1_RBUF;
		c++;
		fifocnt--;
	}
	/* clear any pending interrupts */
	//*DANUBE_ICU_IM2_ISR = BIT_NO(DANUBEASC1_RIR);
	/* unblock the IRQ */
	local_irq_restore(flags);
	/* return the count */
	return c;
}
#endif

static void danubeasc_console_write(struct console *co, const char *s, u_int count)
{
	int i, fifocnt;
	unsigned long flags;
#ifdef EARLY_PRINTK_HACK
	extern int do_early_printk;

	if (do_early_printk == 1)
		do_early_printk = 0;
	else {
		printk("danubeasc_console_write");
	}
#endif
	/* block the IRQ */
	local_irq_save(flags);
//SERDEBUG("-9")
	/*
	 *	Now, do each character
	 */
	for (i = 0; i < count;)
	{
		/* wait until the FIFO is not full */
		do
		{
			fifocnt = (*DANUBE_ASC1_FSTAT & ASCFSTAT_TXFFLMASK)
					>> ASCFSTAT_TXFFLOFF;
		} while (fifocnt == DANUBEASC_TXFIFO_FULL);
#if 1
		if (s[i] == '\0')
		{
			break;
		}
#endif
		if (s[i] == '\n')
		{
			*DANUBE_ASC1_TBUF = '\r';
			do
			{
				fifocnt = (*DANUBE_ASC1_FSTAT &
				ASCFSTAT_TXFFLMASK) >> ASCFSTAT_TXFFLOFF;
			} while (fifocnt == DANUBEASC_TXFIFO_FULL);
		}
		*DANUBE_ASC1_TBUF = s[i];
		i++;
	} /* for */

//SERDEBUG("+9")
	/* clear any pending interrupts */
	//*DANUBE_ICU_IM2_ISR = BIT_NO(DANUBEASC1_TIR);
	/* restore the IRQ */
	local_irq_restore(flags);
}

static kdev_t danubeasc_console_device(struct console *co)
{
	return mk_kdev(SERIAL_DANUBEASC_MAJOR,
		SERIAL_DANUBEASC_MINOR + co->index);
}

static void __init
danubeasc_console_get_options(struct uart_port *port, int *baud, int *parity, int *bits)
{
	u_int lcr_h;

SERDEBUG("CGO ");
	lcr_h = *DANUBE_ASC1_CON;
	/* do this only if the ASC is turned on */
	if (lcr_h & ASCCON_R) {
		u_int quot, div, fdiv, frac;

		*parity = 'n';
		if ((lcr_h & ASCCON_MODEMASK) == ASCCON_M_7ASYNC ||
		            (lcr_h & ASCCON_MODEMASK) == ASCCON_M_8ASYNC) {
			if (lcr_h & ASCCON_ODD)
				*parity = 'o';
			else
				*parity = 'e';
		}

		if ((lcr_h & ASCCON_MODEMASK) == ASCCON_M_7ASYNC)
			*bits = 7;
		else
			*bits = 8;

		quot = *DANUBE_ASC1_BG + 1;
		
		/* this gets hairy if the fractional divider is used */
		if (lcr_h & ASCCON_FDE)
		{
			div = 1;
			fdiv = *DANUBE_ASC1_FDV;
			if (fdiv == 0)
				fdiv = 512;
			frac = 512;
		}
		else
		{
			div = lcr_h & ASCCON_BRS ? 3 : 2;
			fdiv = frac = 1;
		}
		/*
		 * This doesn't work exactly because we use integer
		 * math to calculate baud which results in rounding
		 * errors when we try to go from quot -> baud !!
		 * Try to make this work for both the fractional divider
		 * and the simple divider. Also try to avoid rounding
		 * errors using integer math.
		 */
		
		*baud = frac * (port->uartclk / (div * 512 * 16 * quot));
		if (*baud > 1100 && *baud < 2400)
			*baud = 1200;
		if (*baud > 2300 && *baud < 4800)
			*baud = 2400;
		if (*baud > 4700 && *baud < 9600)
			*baud = 4800;
		if (*baud > 9500 && *baud < 19200)
			*baud = 9600;
		if (*baud > 19000 && *baud < 38400)
			*baud = 19200;
		if (*baud > 38400 && *baud < 57600)
			*baud = 38400;
		if (*baud > 57600 && *baud < 115200)
			*baud = 57600;
		if (*baud > 115200 && *baud < 230400)
			*baud = 115200;
	}
}

static int __init danubeasc_console_setup(struct console *co, char *options)
{
	struct uart_port *port;
	int baud;
	int bits = 8;
	int parity = 'n';
	int flow = 'n';

#ifdef SERIAL_DEBUG
if (sbptr == NULL) sbptr = serdebug;
#endif
SERDEBUG("COS ");
	/* this assumes: CON.BRS = CON.FDE = 0 */
	if (uartclk == 0)
		uartclk = danube_get_fpi_hz();

	danubeasc_ports[0].uartclk = uartclk;
	danubeasc_ports[0].type = PORT_DANUBEASC;

  danubeasc1_hw_init();
	/*
	 * Check whether an invalid uart number has been specified, and
	 * if so, search for the first available port that does have
	 * console support.
	 */
	port = uart_get_console(danubeasc_ports, UART_NR, co);

	if (options){
		uart_parse_options(options, &baud, &parity, &bits, &flow);
	} else {
		danubeasc_console_get_options(port, &baud, &parity, &bits);
	}

	return uart_set_options(port, co, baud, parity, bits, flow);
}

static struct console danubeasc_console = {
	name:		"ttyS",
	write:		danubeasc_console_write,
#ifdef used_and_not_const_char_pointer
	read:		danubeasc_console_read,
#endif
	device:		danubeasc_console_device,
	setup:		danubeasc_console_setup,
	flags:		CON_PRINTBUFFER,
	index:		0,
};

void __init danubeasc_console_init(void)
{
SERDEBUG("COI ");
	register_console(&danubeasc_console);
}

#define DANUBEASC_CONSOLE	&danubeasc_console
#else
#define DANUBEASC_CONSOLE	NULL
#endif

static struct uart_driver danubeasc_reg = {
	owner:			NULL,
	normal_major:		SERIAL_DANUBEASC_MAJOR,
#ifdef CONFIG_DEVFS_FS
	normal_name:		"ttyS%d",
	callout_name:		"cua%d",
#else
	normal_name:		"ttyS",
	callout_name:		"cua",
#endif
	normal_driver:		&normal,
	callout_major:		CALLOUT_DANUBEASC_MAJOR,
	callout_driver:		&callout,
	table:			danubeasc_table,
	termios:		danubeasc_termios,
	termios_locked:		danubeasc_termios_locked,
	minor:			SERIAL_DANUBEASC_MINOR,
	nr:			UART_NR,
	port:			danubeasc_ports,
	cons:			DANUBEASC_CONSOLE,
};

static int __init danubeasc_init(void)
{
	int ret;

	ret = uart_register_driver(&danubeasc_reg);
#if 0
	if (ret == 0)
		uart_add_one_port(&danubeasc_reg, &danubeasc_ports[0]);
#endif
	return ret;
}

static void __exit danubeasc_exit(void)
{
#if 0
	uart_remove_one_port(&danubeasc_reg, &danubeasc_ports[0]);
#endif
	uart_unregister_driver(&danubeasc_reg);
}

module_init(danubeasc_init);
module_exit(danubeasc_exit);

EXPORT_NO_SYMBOLS;

MODULE_AUTHOR("IFAP DC COM SD");
MODULE_DESCRIPTION("MIPS DANUBEASC serial port driver");
MODULE_LICENSE("GPL");
