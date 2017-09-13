/* $Id: asn1.c,v 1.4 2000/01/20 07:30:09 kai Exp $
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
 * $Log: asn1.c,v $
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
 * Revision 1.2  1999/04/26 22:11:48  akool
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

#define ASN1_DEBUG

int ParseTag(u_char *p, u_char *end, int *tag)
{
	*tag = *p;
	return 1;
}

int ParseLen(u_char *p, u_char *end, int *len)
{
	int l, i;

	if (*p == 0x80) { // indefinite
		*len = -1;
		return 1;
	}
	if (!(*p & 0x80)) { // one byte
		*len = *p;
		return 1;
	}
	*len = 0;
	l = *p & ~0x80;
	p++;
	for (i = 0; i < l; i++) {
		*len = (*len << 8) + *p; 
		p++;
	}
	return l+1;
}

int
ParseASN1(u_char *p, u_char *end, int level)
{
	int tag, len;
	int ret;
	int j;
	u_char *tag_end, *beg;

	beg = p;

	CallASN1(ret, p, end, ParseTag(p, end, &tag));
	CallASN1(ret, p, end, ParseLen(p, end, &len));
	for (j = 0; j < level*5; j++) print_msg(PRT_DEBUG_DECODE, " ");
	print_msg(PRT_DEBUG_DECODE, "TAG 0x%02x LEN %3d\n", tag, len);
	
	if (tag & ASN1_TAG_CONSTRUCTED) {
		if (len == -1) { // indefinite
			while (*p) {
				CallASN1(ret, p, end, ParseASN1(p, end, level + 1));
			}
			p++;
			if (*p) 
				return -1;
			p++;
		} else {
			tag_end = p + len;
			while (p < tag_end) {
				CallASN1(ret, p, end, ParseASN1(p, end, level +1));
			}
		}
	} else {
		for (j = 0; j < level*5; j++) print_msg(PRT_DEBUG_DECODE, " ");
		while (len--) {
			print_msg(PRT_DEBUG_DECODE, "%02x ", *p);
			p++;
		}
		print_msg(PRT_DEBUG_DECODE, "\n");
	}
	for (j = 0; j < level*5; j++) print_msg(PRT_DEBUG_DECODE, " ");
	print_msg(PRT_DEBUG_DECODE, "END (%d)\n", p - beg - 2);
	return p - beg;
}

#if 0

#if 0
u_char data[] = {"\xA2\x03\x02\x01\xA3"};
#endif
#if 0 // ActNotDiv
u_char data[] = {"\xA1\x2C\x02\x01\x7E\x02\x01\x09\x30\x24\x0A"
		 "\x01\x02\x0A\x01\x03\x30\x0C\x80\x0A\x30\x31"
		 "\x33\x30\x31\x34\x34\x37\x37\x30\xA1\x0E\x0A"
		 "\x01\x02\x12\x09\x32\x31\x31\x33\x34\x31\x38\x33\x30"};
#endif
#if 0 // ActDiv
u_char data[] = {"\xA1\x24\x02\x01\xA1\x02\x01\x07\x30\x1C\x0A"
		 "\x01\x02\x0A\x01\x01\x30\x0C\x80\x0A\x30"
		 "\x31\x33\x30\x31\x34\x34\x37\x37\x30\x80"
		 "\x06\x33\x34\x31\x38\x33\x30"};
#endif
#if 0 // DeactNotDiv
u_char data[] = {"\xA1\x1E\x02\x01\x08\x02\x01\x0A\x30\x16\x0A"
		 "\x01\x02\x0A\x01\x03\xA1\x0E\x0A\x01\x02\x12"
		 "\x09\x32\x31\x31\x33\x34\x31\x38\x33\x30"};
#endif
#if 1 // DeactDiv
u_char data[] = {"\xA1\x16\x02\x01\xB1\x02\x01\x08\x30\x0E\x0A"
		 "\x01\x02\x0A\x01\x01\x80\x06\x33\x34\x31\x38\x33\x30"};
#endif
#if 0 // AOCE, 0 Einheiten
u_char data[] = {"\xA1\x15\x02\x02\x00\xDC\x02\x01\x24\x30\x0C"
		 "\x30\x0A\xA1\x05\x30\x03\x02\x01\x00\x82\x01\x00"};
#endif
#if 0 // AOCE, 1 Einheit
u_char data[] = {"\xA1\x15\x02\x02\x00\xBC\x02\x01\x24\x30\x0C\x30"
		 "\x0A\xA1\x05\x30\x03\x02\x01\x01\x82\x01\x00"};
#endif
#if 0 // AOCD currency
u_char data[] = {"\xA1\x1A\x02\x02\x1C\x65\x02\x01\x21\x30\x11\xA1\x0C\x81\x02\x44\x4D\xA2\x06\x81\x01\x18\x82\x01\x01\x82\x01\x00"};
#endif
u_char *end = data + 47;

#include "asn1_component.h"

void
main()
{
	struct Aoc chan;

#ifdef ASN1_DEBUG
	ParseASN1(data, end, 0);
#endif

	ParseComponent(&chan, data, end);
}

#endif
