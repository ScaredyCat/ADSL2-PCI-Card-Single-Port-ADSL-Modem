/* $Id: profile.c,v 1.1 2000/08/30 18:27:01 armin Exp $
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
 * $Log: profile.c,v $
 * Revision 1.1  2000/08/30 18:27:01  armin
 * Okay, here is the first try for an user-land
 * ttyI daemon. Compilable but not useable.
 *
 *
 */

#include "ttyId.h"

void
modem_reset_faxpar(void)
{
        T30_s *f = &info.fax;

        f->code = 0;
        f->phase = ISDN_FAX_PHASE_IDLE;
        f->direction = 0;
        f->resolution = 1;      /* fine */
        f->rate = 5;            /* 14400 bit/s */
        f->width = 0;
        f->length = 0;
        f->compression = 0;
        f->ecm = 0;
        f->binary = 0;
        f->scantime = 0;
        memset(&f->id[0], 32, FAXIDLEN - 1);
        f->id[FAXIDLEN - 1] = 0;
        f->badlin = 0;
        f->badmul = 0;
        f->bor = 0;
        f->nbc = 0;
        f->cq = 0;
        f->cr = 0;
        f->ctcrty = 0;
        f->minsp = 0;
        f->phcto = 30;
        f->rel = 0;
        memset(&f->pollid[0], 32, FAXIDLEN - 1);
        f->pollid[FAXIDLEN - 1] = 0;
}

void
modem_reset_vpar(atemu * m)
{
        m->vpar[0] = 2;         /* Voice-device            (2 = phone line) */
        m->vpar[1] = 0;         /* Silence detection level (0 = none      ) */
        m->vpar[2] = 70;        /* Silence interval        (7 sec.        ) */
        m->vpar[3] = 2;         /* Compression type        (1 = ADPCM-2   ) */
        m->vpar[4] = 0;         /* DTMF detection level    (0 = softcode  ) */
        m->vpar[5] = 8;         /* DTMF interval           (8 * 5 ms.     ) */
}

void
modem_reset_regs(int force)
{
        atemu *m = &info.emu;
        if ((m->mdmreg[REG_DTRR] & BIT_DTRR) || force) {
                memcpy(m->mdmreg, m->profile, ISDN_MODEM_NUMREG);
                memcpy(m->msn, m->pmsn, ISDN_MSNLEN);
                memcpy(m->lmsn, m->plmsn, ISDN_LMSNLEN);
        }
        modem_reset_vpar(m);
        modem_reset_faxpar();
        m->mdmcmdl = 0;
}

void
reset_profile(void)
{
	atemu *m = &info.emu;

        m->profile[0] = 0;
        m->profile[1] = 0;
        m->profile[2] = 43;
        m->profile[3] = 13;
        m->profile[4] = 10;
        m->profile[5] = 8;
        m->profile[6] = 3;
        m->profile[7] = 60;
        m->profile[8] = 2;
        m->profile[9] = 6;
        m->profile[10] = 7;
        m->profile[11] = 70;
        m->profile[12] = 0x45;
        m->profile[13] = 4;
        m->profile[14] = ISDN_PROTO_L2_X75I;
        m->profile[15] = ISDN_PROTO_L3_TRANS;
        m->profile[16] = 128;
        m->profile[17] = ISDN_MODEM_WINSIZE;
        m->profile[18] = 4;
        m->profile[19] = 0;
        m->profile[20] = 0;
        m->profile[23] = 0;
        m->pmsn[0] = '\0';
        m->plmsn[0] = '\0';
}

void
modem_init(void)
{
	sprintf(info.last_cause, "0000");
	sprintf(info.last_num, "none");
	info.last_dir = 0;
	info.last_lhup = 1;
	info.last_l2 = -1;
	info.last_si = 0;
	reset_profile();
	modem_reset_regs(1);
}

