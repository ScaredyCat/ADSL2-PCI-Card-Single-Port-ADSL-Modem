/* $Id: asn1_comp.c,v 1.5 2000/06/20 17:09:59 akool Exp $
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
 * $Log: asn1_comp.c,v $
 * Revision 1.5  2000/06/20 17:09:59  akool
 * isdnlog-4.29
 *  - better ASN.1 display
 *  - many new rates
 *  - new Option "isdnlog -Q" dump's "/etc/isdn/isdn.conf" into a SQL database
 *
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
 * Revision 1.2  1999/04/26 22:11:55  akool
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
#include "asn1_diversion.h"
#include "asn1_aoc.h"
#include "asn1_comp.h"

// ======================================================================
// Component EN 300 196-1 D.1

int
ParseInvokeId(struct Aoc *chanp, u_char *p, u_char *end, int *invokeId)
{
	return ParseInteger(chanp, p, end, invokeId);
}

int
ParseErrorValue(struct Aoc *chanp, u_char *p, u_char *end, int *errorValue)
{
	return ParseInteger(chanp, p, end, errorValue);
}

int
ParseOperationValue(struct Aoc *chanp, u_char *p, u_char *end, int *operationValue)
{
	return ParseInteger(chanp, p, end, operationValue);
}

int
ParseInvokeComponent(struct Aoc *chanp, u_char *p, u_char *end, int dummy)
{
	int invokeId, operationValue;
	INIT;

	XSEQUENCE_1(ParseInvokeId, ASN1_TAG_INTEGER, ASN1_NOT_TAGGED, &invokeId);
//	XSEQUENCE_OPT(ParseLinkedId, ASN1_TAG_INTEGER, 0);
	XSEQUENCE_1(ParseOperationValue, ASN1_TAG_INTEGER, ASN1_NOT_TAGGED, &operationValue);
	switch (operationValue) {
	case 7:	 XSEQUENCE(ParseARGActivationDiversion, ASN1_TAG_SEQUENCE, ASN1_NOT_TAGGED); break;
	case 8:	 XSEQUENCE(ParseARGDeactivationDiversion, ASN1_TAG_SEQUENCE, ASN1_NOT_TAGGED); break;
 	case 9:	 XSEQUENCE(ParseARGActivationStatusNotificationDiv, ASN1_TAG_SEQUENCE, ASN1_NOT_TAGGED); break;
 	case 10: XSEQUENCE(ParseARGDeactivationStatusNotificationDiv, ASN1_TAG_SEQUENCE, ASN1_NOT_TAGGED); break;
 	case 11: XSEQUENCE(ParseARGInterrogationDiversion, ASN1_TAG_SEQUENCE, ASN1_NOT_TAGGED); break;
 	case 12: XSEQUENCE(ParseARGDiversionInformation, ASN1_TAG_SEQUENCE, ASN1_NOT_TAGGED); break;
 	case 17: XSEQUENCE(ParseARGInterrogateServedUserNumbers, ASN1_TAG_SEQUENCE, ASN1_NOT_TAGGED); break;
//	case 30: XSEQUENCE(ParseChargingRequest, ASN1_TAG_SEQUENCE, ASN1_NOT_TAGGED); break;
//	case 31: XSEQUENCE(ParseAOCSCurrency, ASN1_TAG_SEQUENCE, ASN1_NOT_TAGGED); break;
//	case 32: XSEQUENCE(ParseAOCSSpecialArr, ASN1_TAG_SEQUENCE, ASN1_NOT_TAGGED); break;
	case 33: XSEQUENCE(ParseAOCDCurrency, ASN1_TAG_SEQUENCE, ASN1_NOT_TAGGED); break;
	case 34: XSEQUENCE(ParseAOCDChargingUnit, ASN1_TAG_SEQUENCE, ASN1_NOT_TAGGED); break;
	case 35: XSEQUENCE(ParseAOCECurrency, ASN1_TAG_SEQUENCE, ASN1_NOT_TAGGED); break;
	case 36: XSEQUENCE(ParseAOCEChargingUnit, ASN1_TAG_SEQUENCE, ASN1_NOT_TAGGED); break;
	default:
		return -1;
	}

	switch (operationValue) {
	case 33:
	case 34:
	case 35:
	case 36:
		chanp->type = operationValue;
		break;
	}

	return p - beg;
}

int
ParseReturnResultComponentSequence(struct Aoc *chanp, u_char *p, u_char *end, int dummy)
{
	int operationValue;
	INIT;

	XSEQUENCE_1(ParseOperationValue, ASN1_TAG_INTEGER, ASN1_NOT_TAGGED, &operationValue);
	switch (operationValue) {
 	case 11: XSEQUENCE(ParseRESInterrogationDiversion, ASN1_TAG_SET, ASN1_NOT_TAGGED); break;
 	case 17: XSEQUENCE(ParseRESInterrogateServedUserNumbers, ASN1_TAG_SET, ASN1_NOT_TAGGED); break;
	default: return -1;
	}

	return p - beg;
}

int
ParseReturnResultComponent(struct Aoc *chanp, u_char *p, u_char *end, int dummy)
{
	int invokeId;
	INIT;

	XSEQUENCE_1(ParseInvokeId, ASN1_TAG_INTEGER, ASN1_NOT_TAGGED, &invokeId);
	XSEQUENCE_OPT(ParseReturnResultComponentSequence, ASN1_TAG_SEQUENCE, ASN1_NOT_TAGGED);

	return p - beg;
}

int
ParseReturnErrorComponent(struct Aoc *chanp, u_char *p, u_char *end, int dummy)
{
	int invokeId;
	int errorValue;
	char error[80];
	INIT;

	XSEQUENCE_1(ParseInvokeId, ASN1_TAG_INTEGER, ASN1_NOT_TAGGED, &invokeId);
	XSEQUENCE_1(ParseErrorValue, ASN1_TAG_INTEGER, ASN1_NOT_TAGGED, &errorValue);

	switch (errorValue) {
	case 0: sprintf(error, "not subscribed"); break;
	case 3: sprintf(error, "not available"); break;
	case 4: sprintf(error, "not implemented"); break;
	case 6: sprintf(error, "invalid served user nr"); break;
	case 7: sprintf(error, "invalid call state"); break;
	case 8: sprintf(error, "basic service not provided"); break;
	case 9: sprintf(error, "not incoming call"); break;
	case 10: sprintf(error, "supplementary service interaction not allowed"); break;
	case 11: sprintf(error, "resource unavailable"); break;
	case 12: sprintf(error, "invalid diverted-to nr"); break;
	case 14: sprintf(error, "special service nr"); break;
	case 15: sprintf(error, "diversion to served user nr"); break;
	case 23: sprintf(error, "incoming call accepted"); break;
	case 24: sprintf(error, "number of diversions exceeded"); break;
	case 46: sprintf(error, "not activated"); break;
	case 48: sprintf(error, "request already accepted"); break;
	default: sprintf(error, "(%d)", errorValue); break;
	}
	showmsg("ReturnError: %s\n", error);

	return p - beg;
}

int
ParseComponent(struct Aoc *chanp, u_char *p, u_char *end)
{
        INIT;

	XCHOICE(ParseInvokeComponent, ASN1_TAG_SEQUENCE, 1);
	XCHOICE(ParseReturnResultComponent, ASN1_TAG_SEQUENCE, 2);
	XCHOICE(ParseReturnErrorComponent, ASN1_TAG_SEQUENCE, 3);
//	XCHOICE(ParseRejectComponent, ASN1_TAG_SEQUENCE, 4);
	XCHOICE_DEFAULT;
}

