/* $Id: emulator.c,v 1.1 2000/08/30 18:27:01 armin Exp $
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
 * $Log: emulator.c,v $
 * Revision 1.1  2000/08/30 18:27:01  armin
 * Okay, here is the first try for an user-land
 * ttyI daemon. Compilable but not useable.
 *
 *
 */

#include "ttyId.h"


#define cmdchar(c) ((c>=' ')&&(c<=0x7f))

#define PARSE_ERROR { tty_modem_result(RESULT_ERROR); return; }
#define PARSE_ERROR1 { tty_modem_result(RESULT_ERROR); return 1; }


void
tty_at_cout(char *msg)
{
        atemu *m = &info.emu;
        char *p;
        char c;
        char *sp = 0;

        if (!msg) {
		logit(LOG_ERR, "Null-Message in tty_at_cout");
                return;
        }

        for (p = msg; *p; p++) {
                switch (*p) {
                        case '\r':
                                c = m->mdmreg[REG_CR];
                                break;
                        case '\n':
                                c = m->mdmreg[REG_LF];
                                break;
                        case '\b':
                                c = m->mdmreg[REG_BS];
                                break;
                        default:
                                c = *p;
                }
                if (!writepty(&c, 1))
			logit(LOG_ERR, "could not write in tty_at_cout");
        }
}


/*
 * Return result of AT-emulator to tty-receive-buffer, depending on
 * modem-register 12, bit 0 and 1.
 * For CONNECT-messages also switch to online-mode.
 * For RING-message handle auto-ATA if register 0 != 0
 */

static void
tty_modem_result(int code)
{
        atemu *m = &info.emu;
        static char *msg[] =
        {"OK", "CONNECT", "RING", "NO CARRIER", "ERROR",
         "CONNECT 64000", "NO DIALTONE", "BUSY", "NO ANSWER",
         "RINGING", "NO MSN/EAZ", "VCON", "RUNG"};
        char s[ISDN_MSNLEN+10];

        switch (code) {
                case RESULT_RING:
                        m->mdmreg[REG_RINGCNT]++;
                        if (m->mdmreg[REG_RINGCNT] == m->mdmreg[REG_RINGATA])
                                /* Automatically accept incoming call */
                                tty_cmd_ATA();
                        break;
                case RESULT_NO_CARRIER:
			logit(LOG_DEBUG, "Res: NO CARRIER");
                        m->mdmreg[REG_RINGCNT] = 0;
                        info.ncarrier = 0;
                        if (info.vonline & 1) {
				logit(LOG_DEBUG, "res: send DLE_ETX");
                                /* voice-recording, add DLE-ETX */
                                tty_at_cout("\020\003");
                        }
                        if (info.vonline & 2) {
				logit(LOG_DEBUG, "res: send DLE_DC4");
                                /* voice-playing, add DLE-DC4 */
                                tty_at_cout("\020\024");
                        }
                        break;
                case RESULT_CONNECT:
                case RESULT_CONNECT64000:
			logit(LOG_DEBUG, "Res: CONNECT");
                        sprintf(info.last_cause, "0000");
                        if (!info.online)
                                info.online = 2;
                        break;
                case RESULT_VCON:
			logit(LOG_DEBUG, "res: send VCON");
                        sprintf(info.last_cause, "0000");
                        if (!info.online)
                                info.online = 1;
                        break;
        } /* switch(code) */

        if (m->mdmreg[REG_RESP] & BIT_RESP) {
                /* Show results */
                if (m->mdmreg[REG_RESPNUM] & BIT_RESPNUM) {
                        /* Show numeric results only */
                        sprintf(s, "\r\n%d\r\n", code);
                        tty_at_cout(s);
                } else {
                        if (code == RESULT_RING) {
                            /* return if "show RUNG" and ringcounter>1 */
                            if ((m->mdmreg[REG_RUNG] & BIT_RUNG) &&
                                    (m->mdmreg[REG_RINGCNT] > 1))
                                                return;
                            /* print CID, _before_ _every_ ring */
                            if (!(m->mdmreg[REG_CIDONCE] & BIT_CIDONCE)) {
                                    tty_at_cout("\r\nCALLER NUMBER: ");
                                    tty_at_cout(info.OAD);
                            }
                        }
                        tty_at_cout("\r\n");
                        tty_at_cout(msg[code]);
                        switch (code) {
                                case RESULT_CONNECT:
                                        switch (m->mdmreg[REG_L2PROT]) {
                                                case ISDN_PROTO_L2_MODEM:
                                                        tty_at_cout(" ");
                                                        tty_at_cout(m->connmsg);
                                                        break;
                                        }
                                        break;
                                case RESULT_RING:
                                        /* Append CPN, if enabled */
                                        if ((m->mdmreg[REG_CPN] & BIT_CPN)) {
                                                sprintf(s, "/%s", m->cpn);
                                                tty_at_cout(s);
                                        }
                                        /* Print CID only once, _after_ 1st RING */
                                        if ((m->mdmreg[REG_CIDONCE] & BIT_CIDONCE) &&
                                            (m->mdmreg[REG_RINGCNT] == 1)) {
                                                tty_at_cout("\r\n");
                                                tty_at_cout("CALLER NUMBER: ");
                                    		tty_at_cout(info.OAD);
                                        }
                                        break;
                                case RESULT_NO_CARRIER:
                                case RESULT_NO_DIALTONE:
                                case RESULT_BUSY:
                                case RESULT_NO_ANSWER:
                                        m->mdmreg[REG_RINGCNT] = 0;
                                        /* Append Cause-Message if enabled */
                                        if (m->mdmreg[REG_RESPXT] & BIT_RESPXT) {
                                                sprintf(s, "/%s", info.last_cause);
                                                tty_at_cout(s);
                                        }
                                        break;
                                case RESULT_CONNECT64000:
                                        /* Append Protocol to CONNECT message */
                                        switch (m->mdmreg[REG_L2PROT]) {
                                                case ISDN_PROTO_L2_X75I:
                                                case ISDN_PROTO_L2_X75UI:
                                                case ISDN_PROTO_L2_X75BUI:
                                                        tty_at_cout("/X.75");
                                                        break;
                                                case ISDN_PROTO_L2_HDLC:
                                                        tty_at_cout("/HDLC");
                                                        break;
                                                case ISDN_PROTO_L2_V11096:
                                                        tty_at_cout("/V110/9600");
                                                        break;
                                                case ISDN_PROTO_L2_V11019:
                                                        tty_at_cout("/V110/19200");
                                                        break;
                                                case ISDN_PROTO_L2_V11038:
                                                        tty_at_cout("/V110/38400");
                                                        break;
                                        }
                                        if (m->mdmreg[REG_T70] & BIT_T70) {
                                                tty_at_cout("/T.70");
                                                if (m->mdmreg[REG_T70] & BIT_T70_EXT)
                                                        tty_at_cout("+");
                                        }
                                        break;
                        }
                        tty_at_cout("\r\n");
                }
        }
}


static void
tty_report(void)
{
        atemu *m = &info.emu;
        char s[80];

        tty_at_cout("\r\nStatistics of last connection:\r\n\r\n");
        sprintf(s, "    Remote Number:    %s\r\n", info.last_num);
        tty_at_cout(s);
        sprintf(s, "    Direction:        %s\r\n", info.last_dir ? "outgoing" : "incoming");
        tty_at_cout(s);
        tty_at_cout("    Layer-2 Protocol: ");
        switch (info.last_l2) {
                case ISDN_PROTO_L2_X75I:
                        tty_at_cout("X.75i");
                        break;
                case ISDN_PROTO_L2_X75UI:
                        tty_at_cout("X.75ui");
                        break;
                case ISDN_PROTO_L2_X75BUI:
                        tty_at_cout("X.75bui");
                        break;
                case ISDN_PROTO_L2_HDLC:
                        tty_at_cout("HDLC");
                        break;
                case ISDN_PROTO_L2_V11096:
                        tty_at_cout("V.110 9600 Baud");
                        break;
                case ISDN_PROTO_L2_V11019:
                        tty_at_cout("V.110 19200 Baud");
                        break;
                case ISDN_PROTO_L2_V11038:
                        tty_at_cout("V.110 38400 Baud");
                        break;
                case ISDN_PROTO_L2_TRANS:
                        tty_at_cout("transparent");
                        break;
                case ISDN_PROTO_L2_MODEM:
                        tty_at_cout("modem");
                        break;
                case ISDN_PROTO_L2_FAX:
                        tty_at_cout("fax");
                        break;
                default:
                        tty_at_cout("unknown");
                        break;
        }
        if (m->mdmreg[REG_T70] & BIT_T70) {
                tty_at_cout("/T.70");
                if (m->mdmreg[REG_T70] & BIT_T70_EXT)
                        tty_at_cout("+");
        }
        tty_at_cout("\r\n");
        tty_at_cout("    Service:          ");
        switch (info.last_si) {
                case 1:
                        tty_at_cout("audio\r\n");
                        break;
                case 5:
                        tty_at_cout("btx\r\n");
                        break;
                case 7:
                        tty_at_cout("data\r\n");
                        break;
                default:
                        sprintf(s, "%d\r\n", info.last_si);
                        tty_at_cout(s);
                        break;
        }
        sprintf(s, "    Hangup location:  %s\r\n", info.last_lhup ? "local" : "remote");
        tty_at_cout(s);
        sprintf(s, "    Last cause:       %s\r\n", info.last_cause);
        tty_at_cout(s);
}

/*
 * Display a modem-register-value.
 */
static void
tty_show_profile(int ridx)
{
        char v[6];

        sprintf(v, "\r\n%d", info.emu.mdmreg[ridx]);
        tty_at_cout(v);
}

static int
tty_check_ats(int mreg, int mval, atemu * m)
{
        /* Some plausibility checks */
        switch (mreg) {
                case REG_L2PROT:
                        if (mval > ISDN_PROTO_L2_MAX)
                                return 1;
                        break;
#if 0 /* TODO */
                case REG_PSIZE:
                        if ((mval * 16) > 1024) /* TODO */
                                return 1;
                        if ((m->mdmreg[REG_SI1] & 1) && (mval > 2048)) /* TODO */
                                return 1;
                        info.xmit_size = mval * 16;
                        switch (m->mdmreg[REG_L2PROT]) {
                                case ISDN_PROTO_L2_V11096:
                                case ISDN_PROTO_L2_V11019:
                                case ISDN_PROTO_L2_V11038:
                                        info.xmit_size /= 10;
                        }
                        break;
#endif
                case REG_SI1I:
                case REG_PLAN:
                case REG_SCREEN:
                        /* readonly registers */
                        return 1;
        }
        return 0;
}

/*
 * Perform ATS command
 */
static int
tty_cmd_ATS(char **p)
{
        atemu *m = &info.emu;
        int bitpos;
        int mreg;
        int mval;
        int bval;

        mreg = getnum(p);
        if (mreg < 0 || mreg >= ISDN_MODEM_NUMREG)
                PARSE_ERROR1;
        switch (*p[0]) {
                case '=':
                        p[0]++;
                        mval = getnum(p);
                        if (mval < 0 || mval > 255)
                                PARSE_ERROR1;
                        if (tty_check_ats(mreg, mval, m))
                                PARSE_ERROR1;
                        m->mdmreg[mreg] = mval;
                        break;
                case '.':
                        /* Set/Clear a single bit */
                        p[0]++;
                        bitpos = getnum(p);
                        if ((bitpos < 0) || (bitpos > 7))
                                PARSE_ERROR1;
                        switch (*p[0]) {
                                case '=':
                                        p[0]++;
                                        bval = getnum(p);
                                        if (bval < 0 || bval > 1)
                                                PARSE_ERROR1;
                                        if (bval)
                                                mval = m->mdmreg[mreg] | (1 << bitpos);
                                        else
                                                mval = m->mdmreg[mreg] & ~(1 << bitpos);
                                        if (tty_check_ats(mreg, mval, m))
                                                PARSE_ERROR1;
                                        m->mdmreg[mreg] = mval;
                                        break;
                                case '?':
                                        p[0]++;
                                        tty_at_cout("\r\n");
                                        tty_at_cout((m->mdmreg[mreg] & (1 << bitpos)) ? "1" : "0");
                                        break;
                                default:
                                        PARSE_ERROR1;
                        }
                        break;
                case '?':
                        p[0]++;
                        tty_show_profile(mreg);
                        break;
                default:
                        PARSE_ERROR1;
                        break;
        }
        return 0;
}

/*
 * Perform ATA command
 */
void
tty_cmd_ATA(void)
{
        atemu *m = &info.emu;
        int l2;

        if (info.msr & UART_MSR_RI) {
                /* Accept incoming call */
                info.last_dir = 0;
                strcpy(info.last_num, info.OAD);
                m->mdmreg[REG_RINGCNT] = 0;
                info.msr &= ~UART_MSR_RI;
                l2 = m->mdmreg[REG_L2PROT];
                /* If more than one bit set in reg18, autoselect Layer2 */
                if ((m->mdmreg[REG_SI1] & m->mdmreg[REG_SI1I]) != m->mdmreg[REG_SI1]) {
                        if (m->mdmreg[REG_SI1I] == 1) {
                                if ((l2 != ISDN_PROTO_L2_MODEM) && (l2 != ISDN_PROTO_L2_FAX))
                                        l2 = ISDN_PROTO_L2_TRANS;
                        } else
                                l2 = ISDN_PROTO_L2_X75I;
                }
                info.last_l2 = l2;
#if 0
                cmd.driver = info->isdn_driver;
                cmd.command = ISDN_CMD_SETL2;
                cmd.arg = info->isdn_channel + (l2 << 8);
                isdn_command(&cmd);
                cmd.driver = info->isdn_driver;
                cmd.command = ISDN_CMD_SETL3;
                cmd.arg = info->isdn_channel + (m->mdmreg[REG_L3PROT] << 8);
                if (l2 == ISDN_PROTO_L2_FAX) {
                        cmd.parm.fax = info->fax;
                        info->fax->direction = ISDN_TTY_FAX_CONN_IN;
                }
                isdn_command(&cmd);
                cmd.driver = info->isdn_driver;
                cmd.arg = info->isdn_channel;
                cmd.command = ISDN_CMD_ACCEPTD;
                isdn_command(&cmd);
#endif
                info.dialing = 16;
                info.emu.carrierwait = 0;
             /*    isdn_timer_ctrl(ISDN_TIMER_CARRIER, 1);  */
        } else
                tty_modem_result(RESULT_NO_ANSWER);
}

/*
 * tty_cmd_dial() performs dialing of a tty
 */
static void
tty_cmd_dial(char *n, atemu * m)
{
        int usg = 0; /* ISDN_USAGE_MODEM; */
        int si = 7;
        int l2 = m->mdmreg[REG_L2PROT];
        int i;
        int j;


        tty_modem_result(RESULT_NO_DIALTONE);
	return;

#if 0 /* TODO */
        for (j = 7; j >= 0; j--)
                if (m->mdmreg[REG_SI1] & (1 << j)) {
                        si = bit2si[j];
                        break;
                }
        usg = isdn_calc_usage(si, l2);
        if ((si == 1) &&
                (l2 != ISDN_PROTO_L2_MODEM)
                && (l2 != ISDN_PROTO_L2_FAX)
                ) {
                l2 = ISDN_PROTO_L2_TRANS;
                usg = ISDN_USAGE_VOICE;
        }
        m->mdmreg[REG_SI1I] = si2bit[si];
        i = isdn_get_free_channel(usg, l2, m->mdmreg[REG_L3PROT], -1, -1, m->msn);
        if (i < 0) {
                isdn_tty_modem_result(RESULT_NO_DIALTONE, info);
        } else {
                info->isdn_driver = dev->drvmap[i];
                info->isdn_channel = dev->chanmap[i];
                info->drv_index = i;
                dev->m_idx[i] = info->line;
                dev->usage[i] |= ISDN_USAGE_OUTGOING;
                info->last_dir = 1;
                strcpy(info->last_num, n);
                isdn_info_update();
                cmd.driver = info->isdn_driver;
                cmd.arg = info->isdn_channel;
                cmd.command = ISDN_CMD_CLREAZ;
                isdn_command(&cmd);
                strcpy(cmd.parm.num, isdn_map_eaz2msn(m->msn, info->isdn_driver));
                cmd.driver = info->isdn_driver;
                cmd.command = ISDN_CMD_SETEAZ;
                isdn_command(&cmd);
                cmd.driver = info->isdn_driver;
                cmd.command = ISDN_CMD_SETL2;
                info->last_l2 = l2;
                cmd.arg = info->isdn_channel + (l2 << 8);
                isdn_command(&cmd);
                cmd.driver = info->isdn_driver;
                cmd.command = ISDN_CMD_SETL3;
                cmd.arg = info->isdn_channel + (m->mdmreg[REG_L3PROT] << 8);
#ifdef CONFIG_ISDN_TTY_FAX
                if (l2 == ISDN_PROTO_L2_FAX) {
                        cmd.parm.fax = info->fax;
                        info->fax->direction = ISDN_TTY_FAX_CONN_OUT;
                }
#endif
                isdn_command(&cmd);
                cmd.driver = info->isdn_driver;
                cmd.arg = info->isdn_channel;
                sprintf(cmd.parm.setup.phone, "%s", n);
                sprintf(cmd.parm.setup.eazmsn, "%s",
                        isdn_map_eaz2msn(m->msn, info->isdn_driver));
                cmd.parm.setup.si1 = si;
                cmd.parm.setup.si2 = m->mdmreg[REG_SI2];
                cmd.command = ISDN_CMD_DIAL;
                info->dialing = 1;
                info->emu.carrierwait = 0;
                strcpy(dev->num[i], n);
                isdn_info_update();
                isdn_command(&cmd);
                isdn_timer_ctrl(ISDN_TIMER_CARRIER, 1);
        }
#endif
}

static void
tty_off_hook(void)
{
	logit(LOG_DEBUG, "isdn_tty_off_hook");
}

/*
 * Perform ATH Hangup
 */
static void
tty_on_hook(void)
{
	logit(LOG_DEBUG, "isdn_tty_on_hook");
	/* TODO tty_modem_hup(1); */
}

/*
 * Parse AT&.. commands.
 */
static int
tty_cmd_ATand(char **p)
{
        atemu *m = &info.emu;
        int i;
        char rb[100];

#define MAXRB (sizeof(rb) - 1)

        switch (*p[0]) {
#if 0 /*TODO*/
                case 'B':
                        /* &B - Set Buffersize */
                        p[0]++;
                        i = getnum(p);
                        if ((i < 0) || (i > ISDN_SERIAL_XMIT_MAX))
                                PARSE_ERROR1;
                        if ((m->mdmreg[REG_SI1] & 1) && (i > 2048))
                                PARSE_ERROR1;
                        m->mdmreg[REG_PSIZE] = i / 16;
                        info.xmit_size = m->mdmreg[REG_PSIZE] * 16;
                        switch (m->mdmreg[REG_L2PROT]) {
                                case ISDN_PROTO_L2_V11096:
                                case ISDN_PROTO_L2_V11019:
                                case ISDN_PROTO_L2_V11038:
                                        info.xmit_size /= 10;
                        }
                        break;
#endif
                case 'C':
                        /* &C - DCD Status */
                        p[0]++;
                        switch (getnum(p)) {
                                case 0:
                                        m->mdmreg[REG_DCD] &= ~BIT_DCD;
                                        break;
                                case 1:
                                        m->mdmreg[REG_DCD] |= BIT_DCD;
                                        break;
                                default:
                                        PARSE_ERROR1
                        }
                        break;
                case 'D':
                        /* &D - Set DTR-Low-behavior */
                        p[0]++;
                        switch (getnum(p)) {
                                case 0:
                                        m->mdmreg[REG_DTRHUP] &= ~BIT_DTRHUP;
                                        m->mdmreg[REG_DTRR] &= ~BIT_DTRR;
                                        break;
                                case 2:
                                        m->mdmreg[REG_DTRHUP] |= BIT_DTRHUP;
                                        m->mdmreg[REG_DTRR] &= ~BIT_DTRR;
                                        break;
                                case 3:
                                        m->mdmreg[REG_DTRHUP] |= BIT_DTRHUP;
                                        m->mdmreg[REG_DTRR] |= BIT_DTRR;
                                        break;
                                default:
                                        PARSE_ERROR1
                        }
                        break;
                case 'E':
                        /* &E -Set EAZ/MSN */
                        p[0]++;
                        get_msnstr(m->msn, p);
                        break;
                case 'F':
                        /* &F -Set Factory-Defaults */
                        p[0]++;
                        if (info.msr & UART_MSR_DCD)
                                PARSE_ERROR1;
                        reset_profile();
                        modem_reset_regs(1);
                        break;
                case 'K':
                        /* only for be compilant with common scripts */
                        /* &K Flowcontrol - no function */
                        p[0]++;
                        getnum(p);
                        break;
                case 'L':
                        /* &L -Set Numbers to listen on */
                        p[0]++;
                        i = 0;
                        while (*p[0] && (strchr("0123456789,-*[]?;", *p[0])) &&
                               (i < ISDN_LMSNLEN))
                                m->lmsn[i++] = *p[0]++;
                        m->lmsn[i] = '\0';
                        break;
#if 0 /*TODO*/
                case 'R':
                        /* &R - Set V.110 bitrate adaption */
                        p[0]++;
                        i = getnum(p);
                        switch (i) {
                                case 0:
                                        /* Switch off V.110, back to X.75 */
                                        m->mdmreg[REG_L2PROT] = ISDN_PROTO_L2_X75I;
                                        m->mdmreg[REG_SI2] = 0;
                                        info.xmit_size = m->mdmreg[REG_PSIZE] * 16;
                                        break;
                                case 9600:
                                        m->mdmreg[REG_L2PROT] = ISDN_PROTO_L2_V11096;
                                        m->mdmreg[REG_SI2] = 197;
                                        info.xmit_size = m->mdmreg[REG_PSIZE] * 16 / 10;
                                        break;
                                case 19200:
                                        m->mdmreg[REG_L2PROT] = ISDN_PROTO_L2_V11019;
                                        m->mdmreg[REG_SI2] = 199;
                                        info.xmit_size = m->mdmreg[REG_PSIZE] * 16 / 10;
                                        break;
                                case 38400:
                                        m->mdmreg[REG_L2PROT] = ISDN_PROTO_L2_V11038;
                                        m->mdmreg[REG_SI2] = 198; /* no existing standard for this */
                                        info.xmit_size = m->mdmreg[REG_PSIZE] * 16 / 10;
                                        break;
                                default:
                                        PARSE_ERROR1;
                        }
                        /* Switch off T.70 */
                        m->mdmreg[REG_T70] &= ~(BIT_T70 | BIT_T70_EXT);
                        /* Set Service 7 */
                        m->mdmreg[REG_SI1] |= 4;
                        break;
#endif
                case 'S':
                        /* &S - Set Windowsize */
                        p[0]++;
                        i = getnum(p);
                        if ((i > 0) && (i < 9))
                                m->mdmreg[REG_WSIZE] = i;
                        else
                                PARSE_ERROR1;
                        break;
                case 'V':
                        /* &V - Show registers */
                        p[0]++;
                        tty_at_cout("\r\n");
                        for (i = 0; i < ISDN_MODEM_NUMREG; i++) {
                                sprintf(rb, "S%02d=%03d%s", i,
                                        m->mdmreg[i], ((i + 1) % 10) ? " " : "\r\n");
                                tty_at_cout(rb);
                        }
                        sprintf(rb, "\r\nEAZ/MSN: %.50s\r\n",
                                strlen(m->msn) ? m->msn : "None");
                        tty_at_cout(rb);
                        if (strlen(m->lmsn)) {
                                tty_at_cout("\r\nListen: ");
                                tty_at_cout(m->lmsn);
                                tty_at_cout("\r\n");
                        }
                        break;
                case 'W':
                        /* &W - Write Profile */
                        p[0]++;
                        switch (*p[0]) {
                                case '0':
                                        p[0]++;
                                        /* TODO modem_write_profile(m); */
                                        break;
                                default:
                                        PARSE_ERROR1;
                        }
                        break;
#if 0 /*TODO*/
                case 'X':
                        /* &X - Switch to BTX-Mode and T.70 */
                        p[0]++;
                        switch (getnum(p)) {
                                case 0:
                                        m->mdmreg[REG_T70] &= ~(BIT_T70 | BIT_T70_EXT);
                                        info.xmit_size = m->mdmreg[REG_PSIZE] * 16;
                                        break;
                                case 1:
                                        m->mdmreg[REG_T70] |= BIT_T70;
                                        m->mdmreg[REG_T70] &= ~BIT_T70_EXT;
                                        m->mdmreg[REG_L2PROT] = ISDN_PROTO_L2_X75I;
                                        info.xmit_size = 112;
                                        m->mdmreg[REG_SI1] = 4;
                                        m->mdmreg[REG_SI2] = 0;
                                        break;
                                case 2:
                                        m->mdmreg[REG_T70] |= (BIT_T70 | BIT_T70_EXT);
                                        m->mdmreg[REG_L2PROT] = ISDN_PROTO_L2_X75I;
                                        info.xmit_size = 112;
                                        m->mdmreg[REG_SI1] = 4;
                                        m->mdmreg[REG_SI2] = 0;
                                        break;
                                default:
                                        PARSE_ERROR1;
                        }
                        break;
#endif
                default:
                        PARSE_ERROR1;
        }
        return 0;
}


/*
 * Parse and perform an AT-command-line.
 */
static void
tty_parse_at(void)
{
        atemu *m = &info.emu;
        char *p;
        char ds[80];

	logit(LOG_DEBUG, "AT: '%s'", m->mdmcmd);

        for (p = &m->mdmcmd[2]; *p;) {
                switch (*p) {
                        case ' ':
                                p++;
                                break;
                        case 'A':
                                /* A - Accept incoming call */
                                p++;
                                tty_cmd_ATA();
                                return;
                        case 'D':
                                /* D - Dial */
                                if (info.msr & UART_MSR_DCD)
                                        PARSE_ERROR;
                                if (info.msr & UART_MSR_RI) {
                                        tty_modem_result(RESULT_NO_CARRIER);
                                        return;
                                }
                                getdial(++p, ds, sizeof(ds));
                                p += strlen(p);
                                if (!strlen(m->msn))
                                        tty_modem_result(RESULT_NO_MSN_EAZ);
                                else if (strlen(ds))
                                        tty_cmd_dial(ds, m);
                                else
                                        PARSE_ERROR;
                                return;
                        case 'E':
                                /* E - Turn Echo on/off */
                                p++;
                                switch (getnum(&p)) {
                                        case 0:
                                                m->mdmreg[REG_ECHO] &= ~BIT_ECHO;
                                                break;
                                        case 1:
                                                m->mdmreg[REG_ECHO] |= BIT_ECHO;
                                                break;
                                        default:
                                                PARSE_ERROR;
                                }
                                break;
                        case 'H':
                                /* H - On/Off-hook */
                                p++;
                                switch (*p) {
                                        case '0':
                                                p++;
                                                tty_on_hook();
                                                break;
                                        case '1':
                                                p++;
                                                tty_off_hook();
                                                break;
                                        default:
                                                tty_on_hook();
                                                break;
                                }
                                break;
                        case 'I':
                                /* I - Information */
                                p++;
                                tty_at_cout("\r\nisdn4linux AT command emulator");
                                switch (*p) {
                                        case '0':
                                        case '1':
                                                p++;
                                                break;
                                        case '2':
                                                p++;
                                                tty_report();
                                                break;
                                        case '3':
                                                p++;
                                                sprintf(ds, "\r\n%d", info.emu.charge);
                                                tty_at_cout(ds);
                                                break;
                                        default:
                                }
                                break;
                        case 'L':
                        case 'M':
                                /* only for be compilant with common scripts */
                                /* no function */
                                p++;
                                getnum(&p);
                                break;
                        case 'O':
                                /* O - Go online */
                                p++;
                                if (info.msr & UART_MSR_DCD) {
                                        /* if B-Channel is up */
                                        tty_modem_result((m->mdmreg[REG_L2PROT] == ISDN_PROTO_L2_MODEM) ? RESULT_CONNECT:RESULT_CONNECT64000);
                                } else
                                        tty_modem_result(RESULT_NO_CARRIER);
                                return;
                        case 'Q':
                                /* Q - Turn Emulator messages on/off */
                                p++;
                                switch (getnum(&p)) {
                                        case 0:
                                                m->mdmreg[REG_RESP] |= BIT_RESP;
                                                break;
                                        case 1:
                                                m->mdmreg[REG_RESP] &= ~BIT_RESP;
                                                break;
                                        default:
                                                PARSE_ERROR;
                                }
                                break;
                        case 'S':
                                /* S - Set/Get Register */
                                p++;
                                if (tty_cmd_ATS(&p))
                                        return;
                                break;
                        case 'V':
                                /* V - Numeric or ASCII Emulator-messages */
                                p++;
                                switch (getnum(&p)) {
                                        case 0:
                                                m->mdmreg[REG_RESP] |= BIT_RESPNUM;
                                                break;
                                        case 1:
                                                m->mdmreg[REG_RESP] &= ~BIT_RESPNUM;
                                                break;
                                        default:
                                                PARSE_ERROR;
                                }
                                break;
                        case 'Z':
                                /* Z - Load Registers from Profile */
                                p++;
                                if (info.msr & UART_MSR_DCD) {
                                        info.online = 0;
                                        /* TODO isdn_tty_on_hook(info); */
                                }
                                modem_reset_regs(1);
                                break;
#if 0 /* TODO */
                        case '+':
                                p++;
                                switch (*p) {
                                        case 'F':
                                                p++;
                                                if (isdn_tty_cmd_PLUSF(&p, info))
                                                        return;
                                                break;
                                        case 'V':
                                                if ((!(m->mdmreg[REG_SI1] & 1)) ||
                                                        (m->mdmreg[REG_L2PROT] == ISDN_PROTO_L2_MODEM))
                                                        PARSE_ERROR;
                                                p++;
                                                if (isdn_tty_cmd_PLUSV(&p, info))
                                                        return;
                                                break;
                                        case 'S':       /* SUSPEND */
                                                p++;
                                                isdn_tty_get_msnstr(ds, &p);
                                                isdn_tty_suspend(ds, info, m);
                                                break;
                                        case 'R':       /* RESUME */
                                                p++;
                                                isdn_tty_get_msnstr(ds, &p);
                                                isdn_tty_resume(ds, info, m);
                                                break;
                                        case 'M':       /* MESSAGE */
                                                p++;
                                                isdn_tty_send_msg(info, m, p);
                                                break;
                                        default:
                                                PARSE_ERROR;
                                }
                                break;
#endif
                        case '&':
                                p++;
                                if (tty_cmd_ATand(&p))
                                        return;
                                break;
                        default:
                                PARSE_ERROR;
                }
        }
        if (!info.vonline)
                tty_modem_result(RESULT_OK);
}


/*
 * Perform line-editing of AT-commands
 *
 * Parameters:
 *   p        inputbuffer
 *   count    length of buffer
 */
int
tty_edit_at(const char *p, int count)
{
        atemu *m = &info.emu;
        int total = 0;
        u_char c;
        char eb[2];
        int cnt;

        for (cnt = count; cnt > 0; p++, cnt--) {
                c = *p;
                total++;
                if (c == m->mdmreg[REG_CR] || c == m->mdmreg[REG_LF]) {
                        /* Separator (CR or LF) */
                        m->mdmcmd[m->mdmcmdl] = 0;
                        if (m->mdmreg[REG_ECHO] & BIT_ECHO) {
                                eb[0] = c;
                                eb[1] = 0;
                                tty_at_cout(eb);
                        }
                        if ((m->mdmcmdl >= 2) && (!(strncmp(m->mdmcmd, "AT", 2))))
                                tty_parse_at();
                        m->mdmcmdl = 0;
                        continue;
                }
                if (c == m->mdmreg[REG_BS] && m->mdmreg[REG_BS] < 128) {
                        /* Backspace-Function */
                        if ((m->mdmcmdl > 2) || (!m->mdmcmdl)) {
                                if (m->mdmcmdl)
                                        m->mdmcmdl--;
                                if (m->mdmreg[REG_ECHO] & BIT_ECHO)
                                        tty_at_cout("\b");
                        }
                        continue;
                }
                if (cmdchar(c)) {
                        if (m->mdmreg[REG_ECHO] & BIT_ECHO) {
                                eb[0] = c;
                                eb[1] = 0;
                                tty_at_cout(eb);
                        }
                        if (m->mdmcmdl < 255) {
                                c = toupper(c);
                                switch (m->mdmcmdl) {
                                        case 1:
                                                if (c == 'T') {
                                                        m->mdmcmd[m->mdmcmdl] = c;
                                                        m->mdmcmd[++m->mdmcmdl] = 0;
                                                        break;
                                                } else
                                                        m->mdmcmdl = 0;
                                                /* Fall through, check for 'A' */
                                        case 0:
                                                if (c == 'A') {
                                                        m->mdmcmd[m->mdmcmdl] = c;
                                                        m->mdmcmd[++m->mdmcmdl] = 0;
                                                }
                                                break;
                                        default:
                                                m->mdmcmd[m->mdmcmdl] = c;
                                                m->mdmcmd[++m->mdmcmdl] = 0;
                                }
                        }
                }
        }
        return total;
}

