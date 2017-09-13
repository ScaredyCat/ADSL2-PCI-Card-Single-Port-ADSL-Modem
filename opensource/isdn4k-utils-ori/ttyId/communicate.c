/* $Id: communicate.c,v 1.1 2000/08/30 18:27:01 armin Exp $
 *
 * ttyId - CAPI TTY AT-command emulator
 *
 * based on the AT-command emulator of the isdn4linux
 * kernel subsystem.
 *
 * Copyright 2000 by Armin Schindler (mac@melware.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Log: communicate.c,v $
 * Revision 1.1  2000/08/30 18:27:01  armin
 * Okay, here is the first try for an user-land
 * ttyI daemon. Compilable but not useable.
 *
 *
 */


#include "ttyId.h"


int
tty_write(unsigned char *buf, int len)
{
	int c;
	int total = 0;
/* TODO */
        while (1) {
/*
                c = MIN(count, info->xmit_size - info->xmit_count);
                if (info->isdn_driver >= 0)
                        c = MIN(c, dev->drv[info->isdn_driver]->maxbufsize);
                if (c <= 0)
                        break;
*/
#if 0
                if ((info->online > 1)
                    || (info->vonline & 3)
                        ) {
                        if (!info->vonline)
                                isdn_tty_check_esc(buf, m->mdmreg[REG_ESC], c,
                                                   &(m->pluscount),
                                                   &(m->lastplus),
                                                   from_user);
                        if (from_user)
                                copy_from_user(&(info->xmit_buf[info->xmit_count]), buf, c);
                        else
                                memcpy(&(info->xmit_buf[info->xmit_count]), buf, c);
                        if (info->vonline) {
                                int cc = isdn_tty_handleDLEdown(info, m, c);
                                if (info->vonline & 2) {
                                        if (!cc) {
                                                /* If DLE decoding results in zero-transmit, but
                                                 * c originally was non-zero, do a wakeup.
                                                 */
                                                if ((tty->flags & (1 << TTY_DO_WRITE_WAKEUP)) &&
                                                 tty->ldisc.write_wakeup)
                                                        (tty->ldisc.write_wakeup) (tty);
                                                wake_up_interruptible(&tty->write_wait);
                                                info->msr |= UART_MSR_CTS;
                                                info->lsr |= UART_LSR_TEMT;
                                        }
                                        info->xmit_count += cc;
                                }
                                if ((info->vonline & 3) == 1) {
                                        /* Do NOT handle Ctrl-Q or Ctrl-S
                                         * when in full-duplex audio mode.
                                         */
                                        if (isdn_tty_end_vrx(buf, c, from_user)) {
                                                info->vonline &= ~1;
#ifdef ISDN_DEBUG_MODEM_VOICE
                                                printk(KERN_DEBUG
                                                       "got !^Q/^S, send DLE-ETX,VCON on ttyI%d\n",
                                                       info->line);
#endif
                                                isdn_tty_at_cout("\020\003\r\nVCON\r\n", info);
                                        }
                                }
                        } else

#if 0 /* def ISDN_TTY_FCLASS1 */
                        if (TTY_IS_FCLASS1(info)) {
                                int cc = isdn_tty_handleDLEdown(info, m, c);

                                if (info->vonline & 4) { /* ETX seen */
                                        isdn_ctrl c;

                                        c.command = ISDN_CMD_FAXCMD;
                                        c.driver = info->isdn_driver;
                                        c.arg = info->isdn_channel;
                                        c.parm.aux.cmd = ISDN_FAX_CLASS1_CTRL;
                                        c.parm.aux.subcmd = ETX;
                                        isdn_command(&c);
                                }
                                info->vonline = 0;
                                printk(KERN_DEBUG "fax dle cc/c %d/%d\n", cc,c);
                                info->xmit_count += cc;
                        } else
#endif

                                info->xmit_count += c;
                } else {
#endif
                        info.msr |= UART_MSR_CTS;
                        info.lsr |= UART_LSR_TEMT;
                        if (info.dialing) {
                                info.dialing = 0;
				logit(LOG_DEBUG, "Mhup in tty write");
/*
                                isdn_tty_modem_result(RESULT_NO_CARRIER, info);
                                isdn_tty_modem_hup(info, 1);
*/
                        } else {
                                tty_edit_at(buf, len);
				return(len);
			}
#if 0
                }
                buf += c;
                count -= c;
                total += c;
#endif
        }
#if 0
        atomic_dec(&info->xmit_lock);
        if ((info->xmit_count) || (skb_queue_len(&info->xmit_queue))) {
                if (m->mdmreg[REG_DXMT] & BIT_DXMT) {
                        isdn_tty_senddown(info);
                        isdn_tty_tint(info);
                }
                isdn_timer_ctrl(ISDN_TIMER_MODEMXMIT, 1);
        }
        if (from_user)
                up(&info->write_sem);
#endif
        return total;
}


