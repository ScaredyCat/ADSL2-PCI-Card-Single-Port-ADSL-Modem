/* $Id: asn1_diversion.c,v 1.6 2000/06/20 17:09:59 akool Exp $
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
 * $Log: asn1_diversion.c,v $
 * Revision 1.6  2000/06/20 17:09:59  akool
 * isdnlog-4.29
 *  - better ASN.1 display
 *  - many new rates
 *  - new Option "isdnlog -Q" dump's "/etc/isdn/isdn.conf" into a SQL database
 *
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
 * Revision 1.3  1999/05/04 19:32:33  akool
 * isdnlog Version 3.24
 *
 *  - fully removed "sondernummern.c"
 *  - removed "gcc -Wall" warnings in ASN.1 Parser
 *  - many new entries for "rate-de.dat"
 *  - better "isdnconf" utility
 *
 * Revision 1.2  1999/04/26 22:11:56  akool
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
#include "asn1_address.h"
#include "asn1_basic_service.h"
#include "asn1_diversion.h"

extern void  showmsg(const char *fmt, ...);
extern char *ns(char *num);


// ======================================================================
// Diversion Supplementary Services ETS 300 207-1 Table 3

int
ParseARGActivationDiversion(struct Aoc *chanp, u_char *p, u_char *end, int dummy)
{
	char procedure[10];
	char basicService[30];
	char servedUserNr[30];
	char address[60];
	INIT;

	XSEQUENCE_1(ParseProcedure, ASN1_TAG_ENUM, ASN1_NOT_TAGGED, procedure);
	XSEQUENCE_1(ParseBasicService, ASN1_TAG_ENUM, ASN1_NOT_TAGGED, basicService);
	XSEQUENCE_1(ParseAddress, ASN1_TAG_SEQUENCE, ASN1_NOT_TAGGED, address);
	XSEQUENCE_1(ParseServedUserNr, ASN1_NOT_TAGGED, ASN1_NOT_TAGGED, servedUserNr);

	showmsg("Activation Diversion %s (%s), %s -> %s\n", procedure, basicService, ns(servedUserNr), ns(address));
	return p - beg;
}

int
ParseARGDeactivationDiversion(struct Aoc *chanp, u_char *p, u_char *end, int dummy)
{
	char procedure[10];
	char basicService[30];
	char servedUserNr[30];
	INIT;

	XSEQUENCE_1(ParseProcedure, ASN1_TAG_ENUM, ASN1_NOT_TAGGED, procedure);
	XSEQUENCE_1(ParseBasicService, ASN1_TAG_ENUM, ASN1_NOT_TAGGED, basicService);
	XSEQUENCE_1(ParseServedUserNr, ASN1_NOT_TAGGED, ASN1_NOT_TAGGED, servedUserNr);

	showmsg("Deactivation Diversion %s (%s), %s\n", procedure, basicService, servedUserNr);
	return p - beg;
}

int
ParseARGActivationStatusNotificationDiv(struct Aoc *chanp, u_char *p, u_char *end, int dummy)
{
	char procedure[10];
	char basicService[30];
	char servedUserNr[30];
	char address[60];
	INIT;

	XSEQUENCE_1(ParseProcedure, ASN1_TAG_ENUM, ASN1_NOT_TAGGED, procedure);
	XSEQUENCE_1(ParseBasicService, ASN1_TAG_ENUM, ASN1_NOT_TAGGED, basicService);
	XSEQUENCE_1(ParseAddress, ASN1_TAG_SEQUENCE, ASN1_NOT_TAGGED, address);
	XSEQUENCE_1(ParseServedUserNr, ASN1_NOT_TAGGED, ASN1_NOT_TAGGED, servedUserNr);

	showmsg("Notification: Activated Diversion %s (%s), %s -> %s\n", procedure, basicService, servedUserNr, address);
	return p - beg;
}

int
ParseARGDeactivationStatusNotificationDiv(struct Aoc *chanp, u_char *p, u_char *end, int dummy)
{
	char procedure[10];
	char basicService[30];
	char servedUserNr[30];
	INIT;

	XSEQUENCE_1(ParseProcedure, ASN1_TAG_ENUM, ASN1_NOT_TAGGED, procedure);
	XSEQUENCE_1(ParseBasicService, ASN1_TAG_ENUM, ASN1_NOT_TAGGED, basicService);
	XSEQUENCE_1(ParseServedUserNr, ASN1_NOT_TAGGED, ASN1_NOT_TAGGED, servedUserNr);

	showmsg("Notification: Deactivated Diversion %s (%s), %s\n", procedure, basicService, servedUserNr);
	return p - beg;
}

int
ParseARGInterrogationDiversion(struct Aoc *chanp, u_char *p, u_char *end, int dummy)
{
	char procedure[10];
	char basicService[30];
	char servedUserNr[30];
	INIT;

	XSEQUENCE_1(ParseProcedure, ASN1_TAG_ENUM, ASN1_NOT_TAGGED, procedure);
	XSEQUENCE_1(ParseBasicService, ASN1_TAG_ENUM, ASN1_NOT_TAGGED, basicService);
	XSEQUENCE_1(ParseServedUserNr, ASN1_NOT_TAGGED, ASN1_NOT_TAGGED, servedUserNr);

	showmsg("Interrogation Diversion %s (%s), %s\n", procedure, basicService, servedUserNr);
	return p - beg;
}

int
ParseRESInterrogationDiversion(struct Aoc *chanp, u_char *p, u_char *end, int dummy)
{
	showmsg("Interrogation Diversion Result\n");

	return ParseIntResultList(chanp, p,  end, dummy);
}

int
ParseARGInterrogateServedUserNumbers(struct Aoc *chanp, u_char *p, u_char *end, int dummy)
{
	showmsg("Interrogate Served User Numbers\n");
	return 0;
}

int
ParseRESInterrogateServedUserNumbers(struct Aoc *chanp, u_char *p, u_char *end, int dummy)
{
	int ret;
	char servedUserNumberList[200] = "";

	ret = ParseServedUserNumberList(chanp, p, end, servedUserNumberList);
	if (ret < 0)
		return ret;

	showmsg("Interrogate Served User Numbers: %s\n", servedUserNumberList);

	return ret;
}

int
ParseARGDiversionInformation(struct Aoc *chanp, u_char *p, u_char *end, int dummy)
{
	char diversionReason[20];
	char basicService[30];
	char servedUserSubaddress[30] = "";
	char callingAddress[80] = "-";
	char originalCalledNr[80] = "-";
	char lastDivertingNr[80] = "-";
	char lastDivertingReason[20] = "-";
	INIT;

	XSEQUENCE_1(ParseDiversionReason, ASN1_TAG_ENUM, ASN1_NOT_TAGGED, diversionReason);
	XSEQUENCE_1(ParseBasicService, ASN1_TAG_ENUM, ASN1_NOT_TAGGED, basicService);
	XSEQUENCE_OPT_1(ParsePartySubaddress, ASN1_TAG_SEQUENCE, ASN1_NOT_TAGGED, servedUserSubaddress);
	XSEQUENCE_OPT_1(ParsePresentedAddressScreened, ASN1_NOT_TAGGED, 0 | ASN1_TAG_EXPLICIT, callingAddress);
	XSEQUENCE_OPT_1(ParsePresentedNumberUnscreened, ASN1_NOT_TAGGED, 1 | ASN1_TAG_EXPLICIT, originalCalledNr);
	XSEQUENCE_OPT_1(ParsePresentedNumberUnscreened, ASN1_NOT_TAGGED, 2 | ASN1_TAG_EXPLICIT, lastDivertingNr);
	XSEQUENCE_OPT_1(ParseDiversionReason, ASN1_TAG_ENUM, 3 | ASN1_TAG_EXPLICIT, lastDivertingReason);
//	XSEQUENCE_OPT_1(ParseQ931InformationElement, ASN1_NOT_TAGGED, ASN1_NOT_TAGGED, userInfo);

	showmsg("Diversion Information %s(%s) %s\n", diversionReason, basicService, servedUserSubaddress);
	showmsg("  callingAddress %s originalCalled Nr %s\n", callingAddress, originalCalledNr);
	showmsg("  lastDivertingNr %s lastDiverting Reason %s\n", lastDivertingNr, lastDivertingReason);
	return p - beg;
}

int
ParseIntResultList(struct Aoc *chanp, u_char *p, u_char *end, int dummy)
{
	int i;
	INIT;

	for (i = 0; i < 29; i++) {
		XSEQUENCE_OPT(ParseIntResult, ASN1_TAG_SEQUENCE, ASN1_NOT_TAGGED);
	}

	return p - beg;
}

int
ParseIntResult(struct Aoc *chanp, u_char *p, u_char *end, int dummy)
{
	char procedure[10];
	char basicService[30];
	char servedUserNr[30];
	char address[60];
	INIT;

	XSEQUENCE_1(ParseServedUserNr, ASN1_NOT_TAGGED, ASN1_NOT_TAGGED, servedUserNr);
	XSEQUENCE_1(ParseBasicService, ASN1_TAG_ENUM, ASN1_NOT_TAGGED, basicService);
	XSEQUENCE_1(ParseProcedure, ASN1_TAG_ENUM, ASN1_NOT_TAGGED, procedure);
	XSEQUENCE_1(ParseAddress, ASN1_TAG_SEQUENCE, ASN1_NOT_TAGGED, address);

	showmsg("  %s (%s), %s -> %s\n", procedure, basicService, servedUserNr, address);
	return p - beg;
}

int
ParseServedUserNrAll(struct Aoc *chanp, u_char *p, u_char *end, char *servedUserNr)
{
	int ret;

	ret = ParseNull(chanp, p, end, 0);
	if (ret < 0)
		return ret;
	sprintf(servedUserNr, "(all)");

	return ret;
}

int
ParseServedUserNr(struct Aoc *chanp, u_char *p, u_char *end, char *servedUserNr)
{
	INIT;
	*servedUserNr = 0;

	XCHOICE_1(ParseServedUserNrAll, ASN1_TAG_NULL, ASN1_NOT_TAGGED, servedUserNr);
	XCHOICE_1(ParsePartyNumber, ASN1_NOT_TAGGED, ASN1_NOT_TAGGED, servedUserNr);
	XCHOICE_DEFAULT;
}

int
ParseProcedure(struct Aoc *chanp, u_char *p, u_char *end, char *str)
{
	int ret;
	int procedure;

	ret = ParseEnum(chanp, p, end, &procedure);
	if (ret < 0)
		return ret;

	switch (procedure) {
	case 0: sprintf(str, "CFU"); break;
	case 1: sprintf(str, "CFB"); break;
	case 2: sprintf(str, "CFNR"); break;
	default: sprintf(str, "(%d)", procedure); break;
	}

	return ret;
}

int
ParseServedUserNumberList(struct Aoc *chanp, u_char *p, u_char *end, char *str)
{
	char partyNumber[30];
	int i;
	INIT;

	for (i = 0; i < 9; i++) { // 99
		partyNumber[0] = 0;
		XSEQUENCE_OPT_1(ParsePartyNumber, ASN1_NOT_TAGGED, ASN1_NOT_TAGGED, partyNumber);
		if (partyNumber[0]) {
			str += sprintf(str, "%s ", partyNumber);
		}
	}

	return p - beg;
}

int
ParseDiversionReason(struct Aoc *chanp, u_char *p, u_char *end, char *str)
{
	int ret;
	int diversionReason;

	ret = ParseEnum(chanp, p, end, &diversionReason);
	if (ret < 0)
		return ret;

	switch (diversionReason) {
	case 0: sprintf(str, "unknown"); break;
	case 1: sprintf(str, "CFU"); break;
	case 2: sprintf(str, "CFB"); break;
	case 3: sprintf(str, "CFNR"); break;
	case 4: sprintf(str, "CD (Alerting)"); break;
	case 5: sprintf(str, "CD (Immediate)"); break;
	default: sprintf(str, "(%d)", diversionReason); break;
	}

	return ret;
}

