/* $Id: asn1_aoc.c,v 1.4 2000/01/20 07:30:09 kai Exp $
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
 * $Log: asn1_aoc.c,v $
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
 * Revision 1.2  1999/04/26 22:11:53  akool
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
#include "asn1_aoc.h"

// ======================================================================
// AOC EN 300 182-1 V1.3.3

// AOCDCurrency

int
ParseAOCDCurrency(struct Aoc *chanp, u_char *p, u_char *end, int dummy)
{
	INIT;

	XCHOICE(ParseNull, ASN1_TAG_NULL, ASN1_NOT_TAGGED); // chargeNotAvail
	XCHOICE(ParseAOCDCurrencyInfo, ASN1_TAG_SEQUENCE, ASN1_NOT_TAGGED);
	XCHOICE_DEFAULT;
}

// AOCDChargingUnit

int
ParseAOCDChargingUnit(struct Aoc *chanp, u_char *p, u_char *end, int dummy)
{
	INIT;

	XCHOICE(ParseNull, ASN1_TAG_NULL, ASN1_NOT_TAGGED); // chargeNotAvail
	XCHOICE(ParseAOCDChargingUnitInfo, ASN1_TAG_SEQUENCE, ASN1_NOT_TAGGED);
	XCHOICE_DEFAULT;
}

// AOCECurrency

int
ParseAOCECurrency(struct Aoc *chanp, u_char *p, u_char *end, int dummy)
{
	INIT;

	XCHOICE(ParseNull, ASN1_TAG_NULL, ASN1_NOT_TAGGED); // chargeNotAvail
	XCHOICE(ParseAOCECurrencyInfo, ASN1_TAG_SEQUENCE, ASN1_NOT_TAGGED);
	XCHOICE_DEFAULT;
}

// AOCEChargingUnit

int
ParseAOCEChargingUnit(struct Aoc *chanp, u_char *p, u_char *end, int dummy)
{
	INIT;

	XCHOICE(ParseNull, ASN1_TAG_NULL, ASN1_NOT_TAGGED); // chargeNotAvail
	XCHOICE(ParseAOCEChargingUnitInfo, ASN1_TAG_SEQUENCE, ASN1_NOT_TAGGED);
	XCHOICE_DEFAULT;
}

// AOCDCurrencyInfo

int
ParseAOCDSpecificCurrency(struct Aoc *chanp, u_char *p, u_char *end, int dummy)
{
	int typeOfChargingInfo;
	int billingId;
	INIT;

	XSEQUENCE(ParseRecordedCurrency, ASN1_TAG_SEQUENCE, 1);
	XSEQUENCE_1(ParseTypeOfChargingInfo, ASN1_TAG_ENUM, 2, &typeOfChargingInfo);
	XSEQUENCE_OPT_1(ParseAOCDBillingId, ASN1_TAG_ENUM, 3, &billingId);

	return p - beg;
}

int
ParseAOCDCurrencyInfo(struct Aoc *chanp, u_char *p, u_char *end, int dummy)
{
	INIT;

	XCHOICE(ParseAOCDSpecificCurrency, ASN1_TAG_SEQUENCE, ASN1_NOT_TAGGED);
	XCHOICE(ParseNull, ASN1_TAG_NULL, 1); // freeOfCharge
	XCHOICE_DEFAULT;
}

// AOCDChargingUnitInfo

int
ParseAOCDSpecificChargingUnits(struct Aoc *chanp, u_char *p, u_char *end, int dummy)
{
	int recordedUnits;
	int typeOfChargingInfo;
	int billingId;
	INIT;

	XSEQUENCE_1(ParseRecordedUnitsList, ASN1_TAG_SEQUENCE, 1, &recordedUnits);
	XSEQUENCE_1(ParseTypeOfChargingInfo, ASN1_TAG_ENUM, 2, &typeOfChargingInfo);
	XSEQUENCE_OPT_1(ParseAOCDBillingId, ASN1_TAG_ENUM, 3, &billingId);

	chanp->type_of_charging_info = typeOfChargingInfo;
	chanp->amount = recordedUnits;

	return p - beg;
}

int
ParseAOCDChargingUnitInfo(struct Aoc *chanp, u_char *p, u_char *end, int dummy)
{
	INIT;

	XCHOICE(ParseAOCDSpecificChargingUnits, ASN1_TAG_SEQUENCE, ASN1_NOT_TAGGED);
	XCHOICE(ParseNull, ASN1_TAG_NULL, 1); // freeOfCharge
	XCHOICE_DEFAULT;
}

// RecordedCurrency

int
ParseRecordedCurrency(struct Aoc *chanp, u_char *p, u_char *end, int dummy)
{
	INIT;

	XSEQUENCE_1(ParseCurrency, ASN1_TAG_IA5_STRING, 1, chanp->currency);
	XSEQUENCE(ParseAmount, ASN1_TAG_SEQUENCE, 2);

	return p - beg;
}

// RecordedUnitsList

int
ParseRecordedUnitsList(struct Aoc *chanp, u_char *p, u_char *end, int *recordedUnits)
{
	int i;
	INIT;

	XSEQUENCE_1(ParseRecordedUnits, ASN1_TAG_SEQUENCE, ASN1_NOT_TAGGED, recordedUnits);
	for (i = 0; i < 31; i++) 
		XSEQUENCE_OPT_1(ParseRecordedUnits, ASN1_TAG_SEQUENCE, ASN1_NOT_TAGGED, recordedUnits);

	return p - beg;
}

// TypeOfChargingInfo

int
ParseTypeOfChargingInfo(struct Aoc *chanp, u_char *p, u_char *end, int *typeOfChargingInfo)
{
	return ParseEnum(chanp, p, end, typeOfChargingInfo);
}

// RecordedUnits

int
ParseRecordedUnitsChoice(struct Aoc *chanp, u_char *p, u_char *end, int *recordedUnits)
{
	INIT;

	XCHOICE_1(ParseNumberOfUnits, ASN1_TAG_INTEGER, ASN1_NOT_TAGGED, recordedUnits);
	XCHOICE(ParseNull, ASN1_TAG_NULL, ASN1_NOT_TAGGED); // not available
	XCHOICE_DEFAULT;
}

int
ParseRecordedUnits(struct Aoc *chanp, u_char *p, u_char *end, int *recordedUnits)
{
	int typeOfUnit;
	INIT;

	XSEQUENCE_1(ParseRecordedUnitsChoice, ASN1_NOT_TAGGED, ASN1_NOT_TAGGED, recordedUnits);
	XSEQUENCE_OPT_1(ParseTypeOfUnit, ASN1_TAG_INTEGER, ASN1_NOT_TAGGED, &typeOfUnit);

	return p - beg;
}

// AOCDBillingId

int
ParseAOCDBillingId(struct Aoc *chanp, u_char *p, u_char *end, int *billingId)
{
	return ParseEnum(chanp, p, end, billingId);
}

// AOCECurrencyInfo

int
ParseAOCESpecificCurrency(struct Aoc *chanp, u_char *p, u_char *end, int dummy)
{
	int billingId;
	INIT;

	XSEQUENCE(ParseRecordedCurrency, ASN1_TAG_SEQUENCE, 1);
	XSEQUENCE_OPT_1(ParseAOCEBillingId, ASN1_TAG_ENUM, 2, &billingId);

	return p - beg;
}

int
ParseAOCECurrencyInfoChoice(struct Aoc *chanp, u_char *p, u_char *end, int dummy)
{
	INIT;

	XCHOICE(ParseAOCESpecificCurrency, ASN1_TAG_SEQUENCE, ASN1_NOT_TAGGED);
	XCHOICE(ParseNull, ASN1_TAG_NULL, 1); // freeOfCharge
	XCHOICE_DEFAULT;
}

int
ParseAOCECurrencyInfo(struct Aoc *chanp, u_char *p, u_char *end, int dummy)
{
	INIT;

	XSEQUENCE(ParseAOCECurrencyInfoChoice, ASN1_NOT_TAGGED, ASN1_NOT_TAGGED);
	XSEQUENCE_OPT(ParseChargingAssociation, ASN1_NOT_TAGGED, ASN1_NOT_TAGGED);
	XCHOICE_DEFAULT;
}

// AOCEChargingUnitInfo

int
ParseAOCESpecificChargingUnits(struct Aoc *chanp, u_char *p, u_char *end, int dummy)
{
	int recordedUnits;
	int billingId;
	INIT;

	XSEQUENCE_1(ParseRecordedUnitsList, ASN1_TAG_SEQUENCE, 1, &recordedUnits);
	XSEQUENCE_OPT_1(ParseAOCEBillingId, ASN1_TAG_ENUM, 2, &billingId);

	chanp->amount = recordedUnits;

	return p - beg;
}

int
ParseAOCEChargingUnitInfoChoice(struct Aoc *chanp, u_char *p, u_char *end, int dummy)
{
	INIT;

	XCHOICE(ParseAOCESpecificChargingUnits, ASN1_TAG_SEQUENCE, ASN1_NOT_TAGGED);
	XCHOICE(ParseNull, ASN1_TAG_NULL, 1); // freeOfCharge
	XCHOICE_DEFAULT;
}

int
ParseAOCEChargingUnitInfo(struct Aoc *chanp, u_char *p, u_char *end, int dummy)
{
	INIT;

	XSEQUENCE(ParseAOCEChargingUnitInfoChoice, ASN1_NOT_TAGGED, ASN1_NOT_TAGGED);
	XSEQUENCE_OPT(ParseChargingAssociation, ASN1_NOT_TAGGED, ASN1_NOT_TAGGED);

	return p - beg;
}

// AOCEBillingId

int
ParseAOCEBillingId(struct Aoc *chanp, u_char *p, u_char *end, int *billingId)
{
	return ParseEnum(chanp, p, end, billingId);
}

// Currency

int
ParseCurrency(struct Aoc *chanp, u_char *p, u_char *end, char *currency)
{
	return ParseIA5String(chanp, p, end, currency);
}

// Amount

int
ParseAmount(struct Aoc *chanp, u_char *p, u_char *end, int dummy)
{
	int multiplier;
	INIT;

	XSEQUENCE_1(ParseCurrencyAmount, ASN1_TAG_INTEGER, 1, &chanp->amount);
	XSEQUENCE_1(ParseMultiplier, ASN1_TAG_INTEGER, 2, &multiplier);

	chanp->multiplier = pow(10, multiplier-3);

	return p - beg;
}

// CurrencyAmount

int
ParseCurrencyAmount(struct Aoc *chanp, u_char *p, u_char *end, int *currencyAmount)
{
	return ParseInteger(chanp, p, end, currencyAmount);
}

// Multiplier

int
ParseMultiplier(struct Aoc *chanp, u_char *p, u_char *end, int *multiplier)
{
	return ParseEnum(chanp, p, end, multiplier);
}

// TypeOfUnit

int
ParseTypeOfUnit(struct Aoc *chanp, u_char *p, u_char *end, int *typeOfUnit)
{
	return ParseInteger(chanp, p, end, typeOfUnit);
}

// NumberOfUnits

int
ParseNumberOfUnits(struct Aoc *chanp, u_char *p, u_char *end, int *numberOfUnits)
{
	return ParseInteger(chanp, p, end, numberOfUnits);
}

// Charging Association

int
ParseChargingAssociation(struct Aoc *chanp, u_char *p, u_char *end, int dummy)
{
	char partyNumber[30];
	INIT;

	XCHOICE_1(ParsePartyNumber, ASN1_TAG_SEQUENCE, 0, partyNumber);
	XCHOICE(ParseChargeIdentifier, ASN1_TAG_INTEGER, ASN1_NOT_TAGGED);
	XCHOICE_DEFAULT;
}

// ChargeIdentifier

int
ParseChargeIdentifier(struct Aoc *chanp, u_char *p, u_char *end, int dummy)
{
	int chargeIdentifier;

	return ParseInteger(chanp, p, end, &chargeIdentifier);
}

