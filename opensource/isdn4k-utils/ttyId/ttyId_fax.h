/* $Id: ttyId_fax.h,v 1.1 2000/08/30 18:27:01 armin Exp $
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
 * $Log: ttyId_fax.h,v $
 * Revision 1.1  2000/08/30 18:27:01  armin
 * Okay, here is the first try for an user-land
 * ttyI daemon. Compilable but not useable.
 *
 *
 */



#define __u8 u_char

#define FAXIDLEN 21

#define ISDN_FAX_PHASE_IDLE     0
#define ISDN_FAX_PHASE_A        1
#define ISDN_FAX_PHASE_B        2
#define ISDN_FAX_PHASE_C        3
#define ISDN_FAX_PHASE_D        4
#define ISDN_FAX_PHASE_E        5


typedef struct T30_s {
        /* session parameters */
        __u8 resolution         __attribute__ ((packed));
        __u8 rate               __attribute__ ((packed));
        __u8 width              __attribute__ ((packed));
        __u8 length             __attribute__ ((packed));
        __u8 compression        __attribute__ ((packed));
        __u8 ecm                __attribute__ ((packed));
        __u8 binary             __attribute__ ((packed));
        __u8 scantime           __attribute__ ((packed));
        __u8 id[FAXIDLEN]       __attribute__ ((packed));
        /* additional parameters */
        __u8 phase              __attribute__ ((packed));
        __u8 direction          __attribute__ ((packed));
        __u8 code               __attribute__ ((packed));
        __u8 badlin             __attribute__ ((packed));
        __u8 badmul             __attribute__ ((packed));
        __u8 bor                __attribute__ ((packed));
        __u8 fet                __attribute__ ((packed));
        __u8 pollid[FAXIDLEN]   __attribute__ ((packed));
        __u8 cq                 __attribute__ ((packed));
        __u8 cr                 __attribute__ ((packed));
        __u8 ctcrty             __attribute__ ((packed));
        __u8 minsp              __attribute__ ((packed));
        __u8 phcto              __attribute__ ((packed));
        __u8 rel                __attribute__ ((packed));
        __u8 nbc                __attribute__ ((packed));
        /* remote station parameters */
        __u8 r_resolution       __attribute__ ((packed));
        __u8 r_rate             __attribute__ ((packed));
        __u8 r_width            __attribute__ ((packed));
        __u8 r_length           __attribute__ ((packed));
        __u8 r_compression      __attribute__ ((packed));
        __u8 r_ecm              __attribute__ ((packed));
        __u8 r_binary           __attribute__ ((packed));
        __u8 r_scantime         __attribute__ ((packed));
        __u8 r_id[FAXIDLEN]     __attribute__ ((packed));
        __u8 r_code             __attribute__ ((packed));
} T30_s;


