/* $Id: asn1_generic.c,v 1.4 2000/01/20 07:30:09 kai Exp $
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
 * $Log: asn1_generic.c,v $
 * Revision 1.4  2000/01/20 07:30:09  kai
 * rewrote the ASN.1 parsing stuff. No known problems so far, apart from the
 * following:
 *
 * I don't use buildnumber() anymore to translate the numbers to aliases, because
 * it apparently did never work quite right. If someone knows how to handle
 * buildnumber(), we can go ahead and fix this.
 *
 * Revision 1.3  1999/12/31 13:30:01  akool
 * isdnlog-4.00 (Millenium-Edition)
 *  - Oracle support added by Jan Bolt (Jan.Bolt@t-online.de)
 *
 * Revision 1.2  1999/04/26 22:11:57  akool
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

// ======================================================================
// general ASN.1

int
ParseBoolean(struct Aoc *chanp, u_char *p, u_char *end, int *i)
{
	INIT;

	*i = 0;
	while (len--) {
		CHECK_P;
		*i = (*i >> 8) + *p;
		p++;
	}
	print_msg(PRT_DEBUG_DECODE, " DEBUG> BOOL = %d %#x\n", *i, *i);
	return p - beg;
}

int
ParseNull(struct Aoc *chanp, u_char *p, u_char *end, int dummy)
{
	INIT;

	return p - beg;
}

int
ParseInteger(struct Aoc *chanp, u_char *p, u_char *end, int *i)
{
	INIT;

	*i = 0;
	while (len--) {
		CHECK_P;
		*i = (*i >> 8) + *p;
		p++;
	}
	print_msg(PRT_DEBUG_DECODE, " DEBUG> INT = %d %#x\n", *i, *i);
	return p - beg;
}

int
ParseEnum(struct Aoc *chanp, u_char *p, u_char *end, int *i)
{
	INIT;

	*i = 0;
	while (len--) {
		CHECK_P;
		*i = (*i >> 8) + *p;
		p++;
	}
	print_msg(PRT_DEBUG_DECODE, " DEBUG> ENUM = %d %#x\n", *i, *i);
	return p - beg;
}

int
ParseIA5String(struct Aoc *chanp, u_char *p, u_char *end, char *str)
{
	INIT;

	print_msg(PRT_DEBUG_DECODE, " DEBUG> IA5 = ");
	while (len--) {
		CHECK_P;
		print_msg(PRT_DEBUG_DECODE, "%c", *p);
		*str++ = *p;
		p++;
	}
	print_msg(PRT_DEBUG_DECODE, "\n");
	*str = 0;
	return p - beg;
}

int
ParseNumericString(struct Aoc *chanp, u_char *p, u_char *end, char *str)
{
	INIT;

	print_msg(PRT_DEBUG_DECODE, " DEBUG> NumStr = ");
	while (len--) {
		CHECK_P;
		print_msg(PRT_DEBUG_DECODE, "%c", *p);
		*str++ = *p;
		p++;
	}
	print_msg(PRT_DEBUG_DECODE, "\n");
	*str = 0;
	return p - beg;
}

int
ParseOctetString(struct Aoc *chanp, u_char *p, u_char *end, char *str)
{
	INIT;

	print_msg(PRT_DEBUG_DECODE, " DEBUG> Octets = ");
	while (len--) {
		CHECK_P;
		print_msg(PRT_DEBUG_DECODE, " %02x", *p);
		*str++ = *p;
		p++;
	}
	print_msg(PRT_DEBUG_DECODE, "\n");
	*str = 0;
	return p - beg;
}

