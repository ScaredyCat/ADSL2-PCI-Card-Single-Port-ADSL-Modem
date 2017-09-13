/************************************************************************
 *
 * Copyright (c) 2006
 * Infineon Technologies AG
 * St. Martin Strasse 53; 81669 Muenchen; Germany
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 ************************************************************************/

/*
 *  linux/drivers/serial/ifx_asc.c
 *
 *  Driver for IFX_ASC serial ports
 *
 *  Initial Version
 *  Copyright (C) 2002 Gary Jennejohn (gj@denx.de)
 *
 *  Based on drivers/char/serial.c, by Linus Torvalds, Theodore Ts'o.
 *  Based on drivers/serial/serial_s3c2400.c
 *
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
#include <linux/proc_fs.h>

#include <asm/system.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/bitops.h>

#include <linux/serial_core.h>

#if defined(CONFIG_DANUBE)      /* FIXME:It's not good idea to use #if defined here */
#include <asm/danube/danube.h>
#include <asm/danube/irq.h>
#include <asm/danube/ifx_serial.h>
#ifdef CONFIG_USE_EMULATOR
#include <asm/danube/emulation.h>
#endif
extern unsigned int danube_get_fpi_hz(void);
#define ifx_get_fpi_hz            danube_get_fpi_hz
#define IFX_ASC_CLEAR_INT(int)    *DANUBE_ICU_IM0_ISR = BIT_NO(int);
#define SERIAL_IFX_ASC_PORT_COUNT 2

#define IFX_ASC0_PORT_BASE     DANUBE_ASC0
#define IFX_ASC0_MODULE        "asc0"
#define IFX_ASC0_TIR           INT_NUM_IM0_IRL0 /* TX interrupt */
#define IFX_ASC0_TBIR          INT_NUM_IM0_IRL1 /* TX buffer interrupt */
#define IFX_ASC0_RIR           INT_NUM_IM0_IRL2 /* RX interrupt */
#define IFX_ASC0_EIR           INT_NUM_IM0_IRL3 /* ERROR interrupt */

#define IFX_ASC1_PORT_BASE     DANUBE_ASC1
#define IFX_ASC1_MODULE        "asc1"
#define IFX_ASC1_TIR           INT_NUM_IM0_IRL7  /* TX interrupt */
#define IFX_ASC1_TBIR          INT_NUM_IM0_IRL8  /* TX buffer interrupt */
#define IFX_ASC1_RIR           INT_NUM_IM0_IRL9  /* RX interrupt */
#define IFX_ASC1_EIR           INT_NUM_IM0_IRL10 /* ERROR interrupt */

#if defined(CONFIG_IFX_ASC_CONSOLE_ASC0)
#define IFX_ASC_CONSOLE_INDEX  0
#define IFX_ASC_PORT_BASE      IFX_ASC0_PORT_BASE
#define IFX_ASC_MODULE         IFX_ASC0_MODULE
#define IFX_ASC_TIR            IFX_ASC0_TIR
#define IFX_ASC_TBIR           IFX_ASC0_TBIR
#define IFX_ASC_RIR            IFX_ASC0_RIR
#define IFX_ASC_EIR            IFX_ASC0_EIR
#elif defined(CONFIG_IFX_ASC_CONSOLE_ASC1)
#define IFX_ASC_CONSOLE_INDEX  1
#define IFX_ASC_PORT_BASE      IFX_ASC1_PORT_BASE
#define IFX_ASC_MODULE         IFX_ASC1_MODULE
#define IFX_ASC_TIR            IFX_ASC1_TIR
#define IFX_ASC_TBIR           IFX_ASC1_TBIR
#define IFX_ASC_RIR            IFX_ASC1_RIR
#define IFX_ASC_EIR            IFX_ASC1_EIR
#else
#error No console port selected (ASC0 or ASC1)!
#endif

/*TODO: define all !*/
#ifdef CONFIG_USE_EMULATOR
#undef IFX_ASC0_INCLUDED
#define IFX_ASC1_INCLUDED
#else
#define IFX_ASC0_INCLUDED
#define IFX_ASC1_INCLUDED
#endif
/*TODO: fix baudrate values for 115200 */
#ifdef CONFIG_USE_EMULATOR
#ifdef CONFIG_USE_VENUS
/*Venus: FPI 625K, 9600*/
#define ASC_FDV 0x1f7
#define ASC_BG  3 
#else
/*IKOS: FPI 25K, 300*/
#define ASC_FDV 0x127
#define ASC_BG  2
#endif /*CONFIG_USE_VENUS*/
#else
typedef struct{
  u16 fdv; /* 0~511 fractional divider value*/
  u16 reload; /* 13 bit reload value*/
} ifx_asc_baud_reg_t;
static ifx_asc_baud_reg_t g_danube_asc_baud[4][2] = 
{
     {{436,76},{419,36}},   /* 1152000 @ 166.67M and half*/
      {{453,63},{453,31}},   /* 1152000 @ 133.3M  and half*/
      {{501,58},{510,29}},   /* 1152000 @ 111.11M and half*/
      {{419.36},{453,19}}    /* 1152000 @ 83.33M  and half*/
};
#endif /*CONFIG_USE_EMULATOR*/
static ifx_asc_port_priv_t ifx_asc_port_priv[SERIAL_IFX_ASC_PORT_COUNT] = { 
    {(ifx_asc_reg_t*) IFX_ASC0_PORT_BASE, 32, IFX_ASC0_TIR, IFX_ASC0_TBIR, IFX_ASC0_RIR, IFX_ASC0_EIR},
    {(ifx_asc_reg_t*) IFX_ASC1_PORT_BASE, 8, IFX_ASC1_TIR, IFX_ASC1_TBIR, IFX_ASC1_RIR, IFX_ASC1_EIR}
};

#endif /* CONFIG_DANUBE */


#if defined(CONFIG_SERIAL_IFX_ASC_CONSOLE) && defined(CONFIG_MAGIC_SYSRQ)
#define SUPPORT_SYSRQ
#endif

#define SERIAL_IFX_ASC_MAJOR      TTY_MAJOR
#define CALLOUT_IFX_ASC_MAJOR     TTYAUX_MAJOR
#define SERIAL_IFX_ASC_MINOR      64
//#define UART_NR                   SERIAL_IFX_ASC_PORT_COUNT

static ifx_asc_reg_t* ifx_asc_reg[SERIAL_IFX_ASC_PORT_COUNT]; 
static struct uart_driver ifx_asc_drv;
static struct tty_driver normal, callout;
static struct tty_struct *ifx_asc_table[SERIAL_IFX_ASC_PORT_COUNT];
static struct termios *ifx_asc_termios[SERIAL_IFX_ASC_PORT_COUNT];
static struct termios *ifx_asc_termios_locked[SERIAL_IFX_ASC_PORT_COUNT];
static struct uart_port ifx_asc_ports[SERIAL_IFX_ASC_PORT_COUNT];
static struct uart_info *ifx_asc_info[SERIAL_IFX_ASC_PORT_COUNT];
static struct uart_port ifx_asc_ports[SERIAL_IFX_ASC_PORT_COUNT];
#ifdef CONFIG_SERIAL_IFX_ASC_CONSOLE /*SUPPORT_SYSRQ*/
static struct console ifx_asc_console;
#endif
static unsigned int uartclk = 0;
static int ifx_asc_structs_initialized=0;

int rx_bytes_count[SERIAL_IFX_ASC_PORT_COUNT]={};
int tx_bytes_count[SERIAL_IFX_ASC_PORT_COUNT]={};
int rx_parity_error_count[SERIAL_IFX_ASC_PORT_COUNT]={};
int rx_frame_error_count[SERIAL_IFX_ASC_PORT_COUNT]={};
int rx_overrun_error_count[SERIAL_IFX_ASC_PORT_COUNT]={};

#define SET_BIT(reg, mask)    (reg) |= (mask)
#define CLEAR_BIT(reg, mask)  (reg) &= (~(mask)) 
#define CLEAR_BITS(reg, mask) CLEAR_BIT(reg, mask)
#define SET_BITS(reg, mask)   SET_BIT(reg, mask)
#define SET_BITFIELD(reg, mask, off, val) \
                {*(reg) &= (~(mask)); *(reg) |= ((val) << (off));}

/* forward declaration to shut up the compiler */
static void ifx_asc_rx_int(int, void *, struct pt_regs *);
static void ifx_asc_tx_chars(struct uart_info *info);
static void ifx_asc_serial_out(struct uart_port *port, char ch);
static u_int ifx_asc_tx_empty(struct uart_port *port);

/* fake flag to indicate CREAD was not set -> throw away all bytes */
#define UART_DUMMY_UER_RX 1

#define ENABLE_IRQ(n) enable_irq((n))
#define DISABLE_IRQ(n) disable_irq((n))
static int asc_tx_irq_on[SERIAL_IFX_ASC_PORT_COUNT];

/* macro to set the bit corresponding to an interrupt number */
#define BIT_NO(irq) (1 << (irq))

#if (CONFIG_IFX_ASC_DEFAULT_BAUDRATE == 1200)
#define IFX_ASC_DEFAULT_BAUDRATE  B1200
#elif (CONFIG_IFX_ASC_DEFAULT_BAUDRATE == 2400)
#define IFX_ASC_DEFAULT_BAUDRATE  B2400
#elif (CONFIG_IFX_ASC_DEFAULT_BAUDRATE == 4800)
#define IFX_ASC_DEFAULT_BAUDRATE  B4800
#elif (CONFIG_IFX_ASC_DEFAULT_BAUDRATE == 9600)
#define IFX_ASC_DEFAULT_BAUDRATE  B9600
#elif (CONFIG_IFX_ASC_DEFAULT_BAUDRATE == 19200)
#define IFX_ASC_DEFAULT_BAUDRATE  B19200
#elif (CONFIG_IFX_ASC_DEFAULT_BAUDRATE == 38400)
#define IFX_ASC_DEFAULT_BAUDRATE  B38400
#elif (CONFIG_IFX_ASC_DEFAULT_BAUDRATE == 57600)
#define IFX_ASC_DEFAULT_BAUDRATE  B57600
#elif (CONFIG_IFX_ASC_DEFAULT_BAUDRATE == 115200)
#define IFX_ASC_DEFAULT_BAUDRATE  B115200
#else
#error CONFIG_IFX_ASC_DEFAULT_BAUDRATE not set to standard baudrate!
#endif

/**
 * Stop transmission.
 * This function stops transmission by disabling the Tx interrupt.
 *
 * \param port - not used
 * \param from_tty - not used
 */
static void ifx_asc_stop_tx(struct uart_port *port, u_int from_tty)
{
  if (asc_tx_irq_on[port->line]){
    DISABLE_IRQ(ifx_asc_port_priv[port->line].tir);
    asc_tx_irq_on[port->line] = 0;
  }
}

/**
 * Start transmission.
 * This function starts transmission by enabling the Tx interrupt. If the info
 * structure has already been set, buffered chars will be sent out if necessary.
 *
 * \param port - not used
 * \param nonempty - not used
 * \param from_tty - not used
 */
static void ifx_asc_start_tx(struct uart_port *port, u_int nonempty, u_int from_tty)
{
  if (asc_tx_irq_on[port->line]==0){
    ENABLE_IRQ(ifx_asc_port_priv[port->line].tir);
    asc_tx_irq_on[port->line]++;
  }
    if (ifx_asc_info[port->line])
        ifx_asc_tx_chars(ifx_asc_info[port->line]);
}

/**
 * Stop reception.
 * This function stops the receiver.
 *
 * \param port - not used
 */
static void ifx_asc_stop_rx(struct uart_port *port)
{
    /* clear the RX enable bit */
    ifx_asc_reg[port->line]->asc_whbstate = ASCWHBSTATE_CLRREN;
}

/**
 * Enable modem signals.
 * This function should enable modem signals, but this is not supported.
 *
 * \param port - not used
 */
static void ifx_asc_enable_ms(struct uart_port *port)
{
    /* no modem signals */
    return;
}

/**
 * Receive chars.
 * This function reads received characters from the Rx fifo and stores them in 
 * the tty buffer.
 *
 * \param info - Info structure for this uart device.
 * \param regs - In case of system request support the current context registers
 *               are also given.
 */
static void
#ifdef SUPPORT_SYSRQ
ifx_asc_rx_chars(struct uart_info *info, struct pt_regs *regs)
#else
ifx_asc_rx_chars(struct uart_info *info)
#endif
{
    struct tty_struct *tty = info->tty;
    unsigned int ch = 0, rsr = 0, fifocnt;
    struct uart_port *port = info->port;

    fifocnt = ifx_asc_reg[port->line]->asc_fstat & ASCFSTAT_RXFFLMASK;
    while (fifocnt--)
    {
        ch = ifx_asc_reg[port->line]->asc_rbuf;
        rsr = (ifx_asc_reg[port->line]->asc_state & ASCSTATE_ANY) | UART_DUMMY_UER_RX;

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
        rx_bytes_count[port->line]++;
        
        /*
         * Note that the error handling code is
         * out of the main execution path
         */
        if (rsr & ASCSTATE_ANY) {
            if (rsr & ASCSTATE_PE) {
                port->icount.parity++;
                rx_parity_error_count[port->line]++;
                SET_BIT(ifx_asc_reg[port->line]->asc_whbstate, ASCWHBSTATE_CLRPE);
            } else if (rsr & ASCSTATE_FE) {
                port->icount.frame++;
                rx_frame_error_count[port->line]++;
                SET_BIT(ifx_asc_reg[port->line]->asc_whbstate, ASCWHBSTATE_CLRFE);
            }
            if (rsr & ASCSTATE_ROE) {
                port->icount.overrun++;
                rx_overrun_error_count[port->line]++;
                SET_BIT(ifx_asc_reg[port->line]->asc_whbstate, ASCWHBSTATE_CLRROE);
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

/**
 * Transmit chars.
 * This function transmits the characters currently stored in the tty tx buffer.
 * If the buffer is empty, the transmitter is stopped.
 *
 * \param info - Info structure for this uart device.
 */
static void ifx_asc_tx_chars(struct uart_info *info)
{
    struct uart_port *port = info->port;

    if (info->xmit.head == info->xmit.tail
        || info->tty->stopped
        || info->tty->hw_stopped) {
        ifx_asc_stop_tx(port, 0);
        return;
    }

    if (port->x_char) {
        ifx_asc_serial_out(port, port->x_char);
        port->icount.tx++;
        port->x_char = 0;
        tx_bytes_count[port->line]++;
        return;
    }
    ifx_asc_serial_out(port, info->xmit.buf[info->xmit.tail]);
    info->xmit.tail = (info->xmit.tail + 1) & (UART_XMIT_SIZE - 1);
    port->icount.tx++;
    tx_bytes_count[port->line]++;

    if (CIRC_CNT(info->xmit.head, info->xmit.tail, UART_XMIT_SIZE) <
            WAKEUP_CHARS)
        uart_event(info, EVT_WRITE_WAKEUP);
    if (info->xmit.head == info->xmit.tail)
    {
        ifx_asc_stop_tx(info->port, 0);
    }
}

static void ifx_asc_tx_int(int irq, void *dev_id, struct pt_regs *regs)
{
    struct uart_info *info = dev_id;
    
    /* this interrupt tells us that the TXB is empty - fill it */
    ifx_asc_tx_chars(info);
}

static void ifx_asc_er_int(int irq, void *dev_id, struct pt_regs *regs)
{
    struct uart_info *info = (struct uart_info*) dev_id;
    struct uart_port *port = info->port;
    
    SET_BIT(ifx_asc_reg[port->line]->asc_whbstate, ASCWHBSTATE_CLRPE);
    SET_BIT(ifx_asc_reg[port->line]->asc_whbstate, ASCWHBSTATE_CLRFE);
    SET_BIT(ifx_asc_reg[port->line]->asc_whbstate, ASCWHBSTATE_CLRROE);
    return;
}

static void ifx_asc_rx_int(int irq, void *dev_id, struct pt_regs *regs)
{
    struct uart_info *info = dev_id;

#ifdef SUPPORT_SYSRQ
    ifx_asc_rx_chars(info, regs);
#else
    ifx_asc_rx_chars(info);
#endif
}

static u_int ifx_asc_tx_empty(struct uart_port *port)
{
    int status;

    /*
     * FSTAT tells exactly how many bytes are in the FIFO.
     * The question is whether we really need to wait for all
     * 16 bytes to be transmitted before reporting that the
     * transmitter is empty.
     */
    status = ifx_asc_reg[port->line]->asc_fstat & ASCFSTAT_TXFFLMASK;
    return status ? 0 : TIOCSER_TEMT;
}

static u_int ifx_asc_get_mctrl(struct uart_port *port)
{
    /* no modem control signals - the readme says to pretend all are set */
    return TIOCM_CTS|TIOCM_CAR|TIOCM_DSR;
}

static void ifx_asc_set_mctrl(struct uart_port *port, u_int mctrl)
{
    /* no modem control - just return */
    return;
}

static void ifx_asc_break_ctl(struct uart_port *port, int break_state)
{
    /* no way to send a break */
    return;
}
static char rx_irq_name[10];
static char tx_irq_name[10];
static char err_irq_name[10];
static int ifx_asc_startup(struct uart_port *port, struct uart_info *info)
{
    int retval;
    unsigned long flags;

    /* this assumes: CON.BRS = CON.FDE = 0 */
    if (uartclk == 0)
        uartclk = ifx_get_fpi_hz()/2;

    ifx_asc_ports[port->line].uartclk = uartclk;
    ifx_asc_info[port->line] = info;

    /* block the IRQs */
    local_irq_save(flags);

    /* FixMe:
        Need to set the ASC registers here since we unset them in shutdown. Will
        cause problems with new kernels which do a shutdown/startup...
    */  
    
    /* Set fifo size to 1 and flush fifo */
    ifx_asc_reg[port->line]->asc_rxfcon=0x00000102;
    ifx_asc_reg[port->line]->asc_txfcon=0x00000102;

    /* enable the fifos */
    SET_BIT(ifx_asc_reg[port->line]->asc_rxfcon, ASCRXFCON_RXFEN);
    SET_BIT(ifx_asc_reg[port->line]->asc_txfcon, ASCTXFCON_TXFEN);
    
    /* enable receiver */
    SET_BIT(ifx_asc_reg[port->line]->asc_whbstate, ASCWHBSTATE_SETREN);

    /* enable ASC interrupts in module */
    ifx_asc_reg[port->line]->asc_irnen=0x00000007;
    
    /* unblock the IRQs */
    local_irq_restore(flags);
    
    /*
     * Allocate the IRQs
     */
    sprintf(rx_irq_name,"asc%d_rx",port->line); 
    retval = request_irq(ifx_asc_port_priv[port->line].rir, ifx_asc_rx_int, 0, rx_irq_name, info);
    if (retval)
        return retval;
    sprintf(tx_irq_name,"asc%d_tx",port->line); 
    retval = request_irq(ifx_asc_port_priv[port->line].tir, ifx_asc_tx_int, 0, tx_irq_name, info);
    if (retval)
    {
        free_irq(ifx_asc_port_priv[port->line].rir, info);
        return retval;
    }
	  DISABLE_IRQ(ifx_asc_port_priv[port->line].tir);
    asc_tx_irq_on[port->line] = 0;
    sprintf(err_irq_name,"asc%d_er",port->line); 
    retval = request_irq(ifx_asc_port_priv[port->line].eir, ifx_asc_er_int, 0, err_irq_name, info);
    if (retval)
    {
        free_irq(ifx_asc_port_priv[port->line].rir, info);
        free_irq(ifx_asc_port_priv[port->line].tir, info);
        return retval;
    }
    
#if 0
    enable_irq(ifx_asc_port_priv[port->line].rir);
    enable_irq(ifx_asc_port_priv[port->line].tir);
    enable_irq(ifx_asc_port_priv[port->line].eir);
#endif 

    return 0;
}

static void ifx_asc_shutdown(struct uart_port *port, struct uart_info *info)
{
    
    /* disable ASC interrupts in module */
    ifx_asc_reg[port->line]->asc_irnen=0x00000000;

    disable_irq(ifx_asc_port_priv[port->line].rir);
    disable_irq(ifx_asc_port_priv[port->line].tir);
    disable_irq(ifx_asc_port_priv[port->line].eir);
    
    /*
     * Free the interrupts
     */
    free_irq(ifx_asc_port_priv[port->line].rir, info);
    free_irq(ifx_asc_port_priv[port->line].tir, info);
    free_irq(ifx_asc_port_priv[port->line].eir, info);
    /*
     * disable the baudrate generator to disable the ASC
     */
//  ifx_asc_reg[port->line]->asc_mcon = 0; 

    /* flush and then disable the fifos */
    SET_BIT(ifx_asc_reg[port->line]->asc_rxfcon, ASCRXFCON_RXFFLU);
    CLEAR_BIT(ifx_asc_reg[port->line]->asc_rxfcon, ASCRXFCON_RXFEN);
    SET_BIT(ifx_asc_reg[port->line]->asc_txfcon, ASCTXFCON_TXFFLU);
    CLEAR_BIT(ifx_asc_reg[port->line]->asc_txfcon, ASCTXFCON_TXFEN);
}

static void ifx_asc_change_speed(struct uart_port *port, u_int cflag, u_int iflag, u_int quot)
{
    u32 con = 0;
    unsigned long flags;
    u32 temp_reg=0; /* used to read register data back and make sure it was written */
    int t=0;
    u32 fdv=0;
    u32 bg=0;


/* FIXME: Set correct baudrate */
    
    /* block the IRQs */
    save_and_cli(flags);

    /* Wait for TX FIFO to be empty */
    while ( ( ifx_asc_reg[port->line]->asc_fstat & ASCFSTAT_TXFFLMASK ) > 0 ) {
        asm("nop;nop;nop;nop;nop;");
    }
    temp_reg=ifx_asc_reg[port->line]->asc_txfcon;
    /* Wait for characters to be sent out */
    for (t=0;t<60000;t++) {
        asm("nop;nop;nop;nop;nop;");
    }

    /* byte size and parity */
    switch (cflag & CSIZE) {
    /* 7 bits */
    case CS7: con = ASCMCON_M_7ASYNC | ASCMCON_PEN ; break;
    /* the ASC only suports 7 and 8 bits */
    case CS5:
    case CS6:
    default:
        if (cflag & PARENB)
            con = ASCMCON_M_8ASYNC | ASCMCON_PEN;
        else
            con = ASCMCON_M_8ASYNC;
        break;
    }
    if (cflag & CSTOPB)
        con |= ASCMCON_STP;
    if (cflag & PARENB) {
        if (!(cflag & PARODD))
            con &= ~ASCMCON_ODD;
        else
            con |= ASCMCON_ODD;
    }

    port->read_status_mask = ASCMCON_ROEN;
    if (iflag & INPCK)
        port->read_status_mask |= ASCMCON_FEN | ASCMCON_PAL;
    /* the ASC can't really detect or generate a BREAK */

    /*
     * Characters to ignore
     */
    port->ignore_status_mask = 0;
    if (iflag & IGNPAR)
        port->ignore_status_mask |= ASCMCON_FEN | ASCMCON_PAL;
    if (iflag & IGNBRK) {
        /*port->ignore_status_mask |= UERSTAT_BREAK;*/
        /*
         * If we're ignoring parity and break indicators,
         * ignore overruns too (for real raw support).
         */
        if (iflag & IGNPAR)
            port->ignore_status_mask |= ASCMCON_ROEN;
    }

    /*
     * Ignore all characters if CREAD is not set.
     */
    if ((cflag & CREAD) == 0)
        port->ignore_status_mask |= UART_DUMMY_UER_RX;

    /* set error signals  - framing, parity  and overrun */
    con |= ASCMCON_FEN;
//    con |= ASCMCON_ROEN;
//    con |= ASCMCON_PAL;

    /* disable the baudrate generator */
    CLEAR_BIT(ifx_asc_reg[port->line]->asc_mcon, ASCMCON_R);
    temp_reg=ifx_asc_reg[port->line]->asc_mcon;
    
    /* set up CON */
    con |= 0x00000200; /* Set external receive clock bit */
    ifx_asc_reg[port->line]->asc_mcon = con;

#if 0
    /* This is the way if we do not use fractional divider */
    /* Set baud rate - take a divider of 2 into account */
    quot = quot/2 - 1;
    /* the next 3 probably already happened when we set CON above */
    /* make sure the fractional divider is off */
    CLEAR_BIT(ifx_asc_reg[port->line]->asc_mcon, ASCMCON_FDE);
    /* set up to use divisor of 2 */
    CLEAR_BIT(ifx_asc_reg[port->line]->asc_mcon, ASCMCON_BRS);
    /* now we can write the new baudrate into the register */
    ifx_asc_reg[port->line]->asc_bg = quot;
    temp_reg=ifx_asc_reg[port->line]->asc_bg;
#else
    /* quot is (f / (16 * Baudrate)) see uart_calculate_quot */
    fdv = ifx_asc_reg[port->line]->asc_fdv;
    if (fdv == 0) {
        bg = quot/2 - 1;
    }
    else {
        bg = (u32)((quot*fdv+256)/(512) -1);
    }
#if defined(CONFIG_USE_EMULATOR) && !defined(CONFIG_USE_VENUS)
    ifx_asc_reg[port->line]->asc_bg = ASC_BG;
#else
    ifx_asc_reg[port->line]->asc_bg = bg;
#endif
    temp_reg=ifx_asc_reg[port->line]->asc_bg;

#endif

    /* turn the baudrate generator back on */
    SET_BIT(ifx_asc_reg[port->line]->asc_mcon, ASCMCON_R);
    temp_reg=ifx_asc_reg[port->line]->asc_mcon;
    /* Enable receiver */
    SET_BIT(ifx_asc_reg[port->line]->asc_whbstate, ASCWHBSTATE_SETREN);
    temp_reg=ifx_asc_reg[port->line]->asc_whbstate;
    /* Wait for changes to take effect */
    for (t=0;t<3000;t++) {
        asm("nop;nop;nop;nop;nop;");
    }
    /* unblock the IRQs */
    restore_flags(flags);
}

static const char *ifx_asc_type(struct uart_port *port)
{
    return port->type == PORT_IFX_ASC ? "IFX_ASC" : NULL;
}

/*
 * Release the memory region(s) being used by 'port'
 */
static void ifx_asc_release_port(struct uart_port *port)
{
    return;
}

/*
 * Request the memory region(s) being used by 'port'
 */
static int ifx_asc_request_port(struct uart_port *port)
{
    return 0;
}

/*
 * Configure/autoconfigure the port.
 */
static void ifx_asc_config_port(struct uart_port *port, int flags)
{
    if (flags & UART_CONFIG_TYPE) {
        port->type = PORT_IFX_ASC;
        ifx_asc_request_port(port);
    }
}

/*
 * verify the new serial_struct (for TIOCSSERIAL).
 */
static int ifx_asc_verify_port(struct uart_port *port, struct serial_struct *ser)
{
    int ret = 0;
    if (ser->type != PORT_UNKNOWN && ser->type != PORT_IFX_ASC)
        ret = -EINVAL;
    if (ser->irq < 0 || ser->irq >= NR_IRQS)
        ret = -EINVAL;
    if (ser->baud_base < 9600)
        ret = -EINVAL;
    return ret;
}

static struct uart_ops ifx_asc_pops = {
    tx_empty: ifx_asc_tx_empty,
    set_mctrl: ifx_asc_set_mctrl,
    get_mctrl: ifx_asc_get_mctrl,
    stop_tx: ifx_asc_stop_tx,
    start_tx: ifx_asc_start_tx,
    stop_rx: ifx_asc_stop_rx,
    enable_ms: ifx_asc_enable_ms,
    break_ctl: ifx_asc_break_ctl,
    startup: ifx_asc_startup,
    shutdown: ifx_asc_shutdown,
    change_speed: ifx_asc_change_speed,
    type: ifx_asc_type,
    release_port: ifx_asc_release_port,
    request_port: ifx_asc_request_port,
    config_port: ifx_asc_config_port,
    verify_port: ifx_asc_verify_port,
};

#ifdef CONFIG_SERIAL_IFX_ASC_CONSOLE

#define IFX_ASC_CONSOLE &ifx_asc_console

#ifdef used_and_not_const_char_pointer

static int ifx_asc_console_read(struct uart_port *port, char *s, u_int count)
{
    unsigned int status;
    int fifocnt;
    int c;
    unsigned long flags;
#if DEBUG
    printk("ifx_asc_console_read() called\n");
#endif

    c = 0;
    /* block the IRQ */
    local_irq_save(flags);

    fifocnt = ifx_asc_reg[port->line]->asc_fstat & ASCFSTAT_RXFFLMASK;
    while (c < count && fifocnt) {
        *s++ = ifx_asc_reg[port->line]->asc_rbuf;
        c++;
        fifocnt--;
    }
    /* clear any pending interrupts */
    IFX_ASC_CLEAR_INT(ifx_asc_port_priv[port->line].rir);
    /* unblock the IRQ */
    local_irq_restore(flags);
    /* return the count */
    return c;
}
#endif /* used_and_not_const_char_pointer */

static void ifx_asc_serial_out(struct uart_port *port, char ch)
{
    int fifocnt;
    unsigned long flags;
    int index=port->line;
    
    save_and_cli(flags);
    
    do {
        fifocnt = (ifx_asc_reg[index]->asc_fstat & ASCFSTAT_TXFREEMASK)
                >> ASCFSTAT_TXFREEOFF;
    } while (fifocnt == 0);
    /* We have either portwidth = 8 or portwidth = 32 */
    if (ifx_asc_port_priv[index].portwidth == 8) {
        ifx_asc_reg[index]->asc_tbuf = ch;
    } 
    else {
        *(((char*)&ifx_asc_reg[index]->asc_tbuf) + 3) = ch;
    }
    asm("sync");
    restore_flags(flags);
}

static void ifx_asc_console_write(struct console *co, const char *s, u_int count)
{
    int i;
    char scr;
    unsigned long flags;
    struct uart_port *port;
    
#ifdef EARLY_PRINTK_HACK
    extern int do_early_printk;

    if (do_early_printk == 1)
        do_early_printk = 0;
#endif
    //port = uart_get_console(ifx_asc_ports, SERIAL_IFX_ASC_PORT_COUNT, co);
    port = & ifx_asc_ports[IFX_ASC_CONSOLE_INDEX];
    /* block the IRQ */
    local_irq_save(flags);
    /*
     *  Now, do each character
     */
    scr = '\0';
    for (i = 0; i < count;i++)
    {
        if (s[i] == '\n')
            ifx_asc_serial_out(port,'\r');
        ifx_asc_serial_out(port,s[i]);

    } /* for */

    /* clear any pending interrupts */
//  IFX_ASC_CLEAR_INT(ifx_asc_port_priv[port->line].tir);
    /* restore the IRQ */
    local_irq_restore(flags);
}

static kdev_t ifx_asc_console_device(struct console *co)
{
    return mk_kdev(SERIAL_IFX_ASC_MAJOR,
        SERIAL_IFX_ASC_MINOR + co->index);
}

static void __init
ifx_asc_console_get_options(struct uart_port *port, int *baud, int *parity, int *bits)
{
    u_int lcr_h;

    lcr_h = ifx_asc_reg[port->line]->asc_mcon;
    /* do this only if the ASC is turned on */
    if (lcr_h & ASCMCON_R) {
        u_int quot, div, fdiv, frac;

        *parity = 'n';
        if ((lcr_h & ASCMCON_MODEMASK) == (ASCMCON_M_7ASYNC | ASCMCON_PEN) ||
                    (lcr_h & ASCMCON_MODEMASK) == (ASCMCON_M_8ASYNC | ASCMCON_PEN)) {
            if (lcr_h & ASCMCON_ODD)
                *parity = 'o';
            else
                *parity = 'e';
        }

        if ((lcr_h & ASCMCON_MODEMASK) == (ASCMCON_M_7ASYNC | ASCMCON_PEN))
            *bits = 7;
        else
            *bits = 8;

        quot = ifx_asc_reg[port->line]->asc_bg + 1;
        /* this gets hairy if the fractional divider is used */
        if (lcr_h & ASCMCON_FDE)
        {
            div = 1;
            fdiv = ifx_asc_reg[port->line]->asc_fdv;
            if (fdiv == 0)
                fdiv = 512;
            frac = 512;
        }
        else
        {
            div = lcr_h & ASCMCON_BRS ? 3 : 2;
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

static int __init ifx_asc_console_setup(struct console *co, char *options)
{
    struct uart_port *port;
    int baud = CONFIG_IFX_ASC_DEFAULT_BAUDRATE;
    int bits = 8;
    int parity = 'n';
    int flow = 'n';
    int t=0;

    /* this assumes: CON.BRS = CON.FDE = 0 */
    if (uartclk == 0)
        uartclk = ifx_get_fpi_hz();
    for (t=0;t<SERIAL_IFX_ASC_PORT_COUNT;t++) {
        ifx_asc_ports[t].uartclk = uartclk;
        ifx_asc_ports[t].type = PORT_IFX_ASC;
    }
    
    /*
     * Check whether an invalid uart number has been specified, and
     * if so, search for the first available port that does have
     * console support.
     */
    port = uart_get_console(ifx_asc_ports, SERIAL_IFX_ASC_PORT_COUNT, co);

    if (options)
        uart_parse_options(options, &baud, &parity, &bits, &flow);
    else
        ifx_asc_console_get_options(port, &baud, &parity, &bits);

    return uart_set_options(port, co, baud, parity, bits, flow);
}
static void ifx_asc_init_hardware(void)
{
  u32 reg;
  u32 fdv,reload;
#ifdef CONFIG_USE_EMULATOR
  fdv = ASC_FDV;
  reload = ASC_BG;
#else
  u32 ddr_sel,fpi_sel;
  ddr_sel = (* DANUBE_CGU_SYS) & 0x3;
  fpi_sel = ((* DANUBE_CGU_SYS) & 0x40)?1:0;
  fdv= g_danube_asc_baud[ddr_sel][fpi_sel].fdv;
  reload=g_danube_asc_baud[ddr_sel][fpi_sel].reload;
#endif
//TODO:
#ifdef IFX_ASC0_INCLUDED 
  /*TODO: PMU */
  *DANUBE_PMU_PWDCR &= (~(1<<7));
  /* GPIO P0.9 RTS P0.10 CTS P0.11 RX P0.12 TX*/
  /* TODO UART_RI UART_DCD, P1.9 P1.11 */
  *DANUBE_GPIO_P0_DIR |= ((1<<9) | (1<<12));
  *DANUBE_GPIO_P0_DIR &= (~((1<<10) | (1<<11)));
  *DANUBE_GPIO_P0_ALTSEL0 |= (((1<<9)|(1<<12)|(1<<10)|(1<<11)));
  *DANUBE_GPIO_P0_ALTSEL1 &= (~((1<<9)|(1<<12)|(1<<10)|(1<<11)));
  *DANUBE_GPIO_P0_OD |= ((1<<9) | (1<<12));
  /* Set CLC register*/
  /*TODO: is this correct? */
#ifdef CONFIG_USE_EMULATOR
  ifx_asc_reg[0]->asc_clc = 0x00000108;
#else
  ifx_asc_reg[0]->asc_clc = 0x00000100;
#endif

  /* Initialy we are in async mode */
  ifx_asc_reg[0]->asc_mcon = ASCMCON_M_8ASYNC | 0x00c00000 ;

  /* Set TXFIFO's filling level and enable FIFO */
  ifx_asc_reg[0]->asc_txfcon |= ASCTXFCON_TXFEN | 0x00000001;

  /* Set RXFIFO's filling level and enable FIFO */
  ifx_asc_reg[0]->asc_rxfcon |= 0x00000001;

  /* enable error signals */
  ifx_asc_reg[0]->asc_mcon |= ASCMCON_FEN;

  /* Clear all error interrupts and disable receiver */
  ifx_asc_reg[0]->asc_whbstate=0x000000fd;

  ifx_asc_reg[0]->asc_eomcon = 0x00010300;
  /* set up to use divisor of 2 */
  ifx_asc_reg[0]->asc_mcon |= ASCMCON_FDE | ASCMCON_BRS;
  /* now we can write the new baudrate into the register */
  ifx_asc_reg[0]->asc_fdv = fdv;
  ifx_asc_reg[0]->asc_bg = reload;
#endif
//TODO:
#ifdef IFX_ASC1_INCLUDED 
  /* Set CLC register*/
  ifx_asc_reg[1]->asc_clc = 0x00000100;

  /* Initialy we are in async mode */
  ifx_asc_reg[1]->asc_mcon = ASCMCON_M_8ASYNC | 0x00c00000 ;

  /* Set TXFIFO's filling level and enable FIFO */
  ifx_asc_reg[1]->asc_txfcon |= ASCTXFCON_TXFEN | 0x00000001;

  /* Set RXFIFO's filling level and enable FIFO */
  ifx_asc_reg[1]->asc_rxfcon |= 0x00000001;

  /* enable error signals */
  ifx_asc_reg[1]->asc_mcon |= ASCMCON_FEN;

  /* Clear all error interrupts and disable receiver */
  ifx_asc_reg[1]->asc_whbstate=0x000000fd;

  /* set up to use divisor of 2 */
  ifx_asc_reg[1]->asc_mcon |= ASCMCON_FDE | ASCMCON_BRS;
  /* now we can write the new baudrate into the register */
  ifx_asc_reg[1]->asc_fdv = fdv;
  ifx_asc_reg[1]->asc_bg = reload;
  ifx_asc_reg[1]->asc_mcon |= ASCMCON_R;
  ifx_asc_reg[1]->asc_whbstate |= ASCWHBSTATE_SETREN;
#endif
}

static void ifx_asc_init_structs(void) 
{
    int t;
    
    for (t=0;t<SERIAL_IFX_ASC_PORT_COUNT;t++) {

        ifx_asc_reg[t]=ifx_asc_port_priv[t].base;
        
        ifx_asc_ports[t].membase=ifx_asc_port_priv[t].base;
        ifx_asc_ports[t].mapbase=(u_long) ifx_asc_port_priv[t].base;
        ifx_asc_ports[t].iotype=SERIAL_IO_MEM;
        ifx_asc_ports[t].irq=ifx_asc_port_priv[t].rir;
        ifx_asc_ports[t].uartclk=0; /* filled in dynamically */
        ifx_asc_ports[t].fifosize=16;
        ifx_asc_ports[t].unused[0]=ifx_asc_port_priv[t].tir;
        ifx_asc_ports[t].unused[1]=ifx_asc_port_priv[t].eir;
        ifx_asc_ports[t].unused[2]=ifx_asc_port_priv[t].tbir;
        ifx_asc_ports[t].type=PORT_IFX_ASC;
        ifx_asc_ports[t].ops=&ifx_asc_pops;
        ifx_asc_ports[t].flags=ASYNC_BOOT_AUTOCONF;
        ifx_asc_ports[t].line=t;
    }
    
    ifx_asc_drv.owner=NULL;
    ifx_asc_drv.normal_major=SERIAL_IFX_ASC_MAJOR;
#ifdef CONFIG_DEVFS_FS
    ifx_asc_drv.normal_name="ttyS%d";
    ifx_asc_drv.callout_name="cua%d";
#else
    ifx_asc_drv.normal_name="ttyS";
    ifx_asc_drv.callout_name="cua";
#endif
    ifx_asc_drv.normal_driver=&normal;
    ifx_asc_drv.callout_major=CALLOUT_IFX_ASC_MAJOR;
    ifx_asc_drv.callout_driver=&callout;
    ifx_asc_drv.table=ifx_asc_table;
    ifx_asc_drv.termios=ifx_asc_termios;
    ifx_asc_drv.termios_locked=ifx_asc_termios_locked;
    ifx_asc_drv.minor=SERIAL_IFX_ASC_MINOR;
    ifx_asc_drv.nr=SERIAL_IFX_ASC_PORT_COUNT;
    ifx_asc_drv.port=ifx_asc_ports;
    ifx_asc_drv.cons=IFX_ASC_CONSOLE;

    ifx_asc_structs_initialized=1;
}


static struct console ifx_asc_console = {
    name: "ttyS",
    write: ifx_asc_console_write,
#ifdef used_and_not_const_char_pointer
    read: ifx_asc_console_read,
#endif
    device: ifx_asc_console_device,
    setup: ifx_asc_console_setup,
    flags: CON_PRINTBUFFER,
    index: IFX_ASC_CONSOLE_INDEX,
};


void __init ifx_asc_console_init(void)
{
    if (ifx_asc_structs_initialized == 0) {
        ifx_asc_init_structs();
        ifx_asc_init_hardware();
    }
    register_console(&ifx_asc_console);
}

int ifx_asc_read_procmem(char *buf, char **start, off_t offset, int count, int *eof, void *data)
{
    long    len = 0;
    int t=0;

    len = sprintf(buf,"\nIFX ASC Debug\n");

    for (t=0;t<SERIAL_IFX_ASC_PORT_COUNT;t++) {

        len=len+sprintf(buf+len,"Port ASC%d\n",t);
        len=len+sprintf(buf+len,"rx_bytes_count=%d\n",rx_bytes_count[t]);
        len=len+sprintf(buf+len,"tx_bytes_count=%d\n",tx_bytes_count[t]);
        len=len+sprintf(buf+len,"rx_overrun_error_count=%d\n",rx_overrun_error_count[t]);
        len=len+sprintf(buf+len,"rx_parity_error_count=%d\n",rx_parity_error_count[t]);
        len=len+sprintf(buf+len,"rx_frame_error_count=%d\n",rx_frame_error_count[t]);
    
        len=len+sprintf(buf+len,"\n");
    }
    
    *eof = 1;   // No more data available
    return len;
}

#else
#define IFX_ASC_CONSOLE NULL
#endif /* CONFIG_SERIAL_IFX_ASC_CONSOLE */

static int __init ifx_asc_init(void)
{
    int ret=0;
    int t=0;

    if (ifx_asc_structs_initialized == 0) {
        ifx_asc_init_structs();
        ifx_asc_init_hardware();
    }
    
    ret = uart_register_driver(&ifx_asc_drv);
    

    /* Overwrite init_termios speed value - console will do some prints
       with UART default baudrate (38400) otherwise... */
    normal.init_termios.c_cflag = IFX_ASC_DEFAULT_BAUDRATE | CS8 | CREAD | HUPCL | CLOCAL;

    /* Create proc file */ 
    create_proc_read_entry("driver/ifx_asc", 0, NULL, ifx_asc_read_procmem, NULL);

    return ret;
}

static void __exit ifx_asc_exit(void)
{
    uart_unregister_driver(&ifx_asc_drv);
}

module_init(ifx_asc_init);
module_exit(ifx_asc_exit);

EXPORT_NO_SYMBOLS;

MODULE_AUTHOR("Andre Messerschmidt");
MODULE_DESCRIPTION("MIPS IFX_ASC serial port driver");
MODULE_LICENSE("GPL");
