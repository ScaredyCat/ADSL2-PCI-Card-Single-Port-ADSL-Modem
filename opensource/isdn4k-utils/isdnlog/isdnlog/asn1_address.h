/* $Id: asn1_address.h,v 1.1 2000/01/20 07:30:09 kai Exp $
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
 * $Log: asn1_address.h,v $
 * Revision 1.1  2000/01/20 07:30:09  kai
 * rewrote the ASN.1 parsing stuff. No known problems so far, apart from the
 * following:
 *
 * I don't use buildnumber() anymore to translate the numbers to aliases, because
 * it apparently did never work quite right. If someone knows how to handle
 * buildnumber(), we can go ahead and fix this.
 *
 *
 *
 */

// ======================================================================
// Address Types EN 300 196-1 D.3

int ParsePresentedAddressScreened(struct Aoc *chanp, u_char *p, u_char *end, char *str);
int ParsePresentedNumberScreened(struct Aoc *chanp, u_char *p, u_char *end, char *str);
int ParsePresentedNumberUnscreened(struct Aoc *chanp, u_char *p, u_char *end, char *str);
int ParseAddressScreened(struct Aoc *chanp, u_char *p, u_char *end, char *str);
int ParseNumberScreened(struct Aoc *chanp, u_char *p, u_char *end, char *str);
int ParseAddress(struct Aoc *chanp, u_char *p, u_char *end, char *str);
int ParsePartyNumber(struct Aoc *chanp, u_char *p, u_char *end, char *str);
int ParsePublicPartyNumber(struct Aoc *chanp, u_char *p, u_char *end, char *str);
int ParsePrivatePartyNumber(struct Aoc *chanp, u_char *p, u_char *end, char *str);
int ParsePublicTypeOfNumber(struct Aoc *chanp, u_char *p, u_char *end, int *publicTypeOfNumber);
int ParsePrivateTypeOfNumber(struct Aoc *chanp, u_char *p, u_char *end, int dummy);
int ParsePartySubaddress(struct Aoc *chanp, u_char *p, u_char *end, char *str);
int ParseUserSpecifiedSubaddress(struct Aoc *chanp, u_char *p, u_char *end, char *str);
int ParseNSAPSubaddress(struct Aoc *chanp, u_char *p, u_char *end, char *str);
int ParseSubaddressInformation(struct Aoc *chanp, u_char *p, u_char *end, char *str);
int ParseScreeningIndicator(struct Aoc *chanp, u_char *p, u_char *end, char *str);
int ParseNumberDigits(struct Aoc *chanp, u_char *p, u_char *end, char *str);

