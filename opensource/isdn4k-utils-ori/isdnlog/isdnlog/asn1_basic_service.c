/* $Id: asn1_basic_service.c,v 1.5 2000/01/20 07:30:09 kai Exp $
 *
 * ISDN accounting for isdn4linux. (ASN.1 parser)
 *
 * Copyright 1995 .. 2000 by Andreas Kool (akool@isdn4linux.de)
 *
 * ASN.1 parser written by Kai Germaschewski <kai@thphy.uni-duesseldorf.de>
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
 * $Log: asn1_basic_service.c,v $
 * Revision 1.5  2000/01/20 07:30:09  kai
 * rewrote the ASN.1 parsing stuff. No known problems so far, apart from the
 * following:
 *
 * I don't use buildnumber() anymore to translate the numbers to aliases, because
 * it apparently did never work quite right. If someone knows how to handle
 * buildnumber(), we can go ahead and fix this.
 *
 * Revision 1.4  1999/12/31 13:30:01  akool
 * isdnlog-4.00 (Millenium-Edition)
 *  - Oracle support added by Jan Bolt (Jan.Bolt@t-online.de)
 *
 * Revision 1.3  1999/10/30 13:42:36  akool
 * isdnlog-3.60
 *   - many new rates
 *   - compiler warnings resolved
 *   - fixed "Sonderrufnummer" Handling
 *
 * Revision 1.2  1999/04/26 22:11:54  akool
 * isdnlog Version 3.21
 *
 *  - CVS headers added to the asn* files
 *  - repaired the "4.CI" message directly on CONNECT
 *  - HANGUP message extended (CI's and EH's shown)
 *  - reactivated the OVERLOAD message
 *  - rate-at.dat extended
 *  - fixes from Michael Reinelt
 *
 *
 * Revision 0.1  1999/04/25 20:00.00  akool
 * Initial revision
 *
 */

#include "asn1.h"
#include "asn1_generic.h"
#include "asn1_basic_service.h"

// ======================================================================
// Basic Service Elements EN 300 196-1 D.6

int ParseBasicService(struct Aoc *chanp, u_char *p, u_char *end, char *str)
{
       int basicService;
       int ret;

       ret = ParseEnum(chanp, p, end, &basicService);
       if (ret < 0)
	       return ret;

       switch (basicService) {
       case 0: sprintf(str, "all services"); break;
       case 1: sprintf(str, "speech"); break;
       case 2: sprintf(str, "unrestricted digital"); break;
       case 3: sprintf(str, "audio 3.1 kHz"); break;
       case 4: sprintf(str, "unrestricted digital+"); break;
       case 5: sprintf(str, "multirate"); break;
       case 32: sprintf(str, "telephony 3.1 kHz"); break;
       case 33: sprintf(str, "teletex"); break;
       case 34: sprintf(str, "telefax G4"); break;
       case 35: sprintf(str, "videotex"); break;
       case 36: sprintf(str, "video telephony"); break;
       case 37: sprintf(str, "telefax G2/G3"); break;
       case 38: sprintf(str, "telephony 7 kHz"); break;
       case 39: sprintf(str, "eurofile"); break;
       case 40: sprintf(str, "file transfer"); break;
       case 41: sprintf(str, "video conference"); break;
       case 42: sprintf(str, "audio graphic conference"); break;
       default: sprintf(str, "(%d)", basicService); break;
       }

       return ret;
}

