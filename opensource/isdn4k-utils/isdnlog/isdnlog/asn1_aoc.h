/* $Id: asn1_aoc.h,v 1.1 2000/01/20 07:30:09 kai Exp $
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
 * $Log: asn1_aoc.h,v $
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
// AOC EN 300 182-1 V1.3.3

int ParseAOCDCurrency(struct Aoc *chanp, u_char *p, u_char *end, int dummy);
int ParseAOCDChargingUnit(struct Aoc *chanp,u_char *p, u_char *end, int dummy);
int ParseAOCECurrency(struct Aoc *chanp, u_char *p, u_char *end, int dummy);
int ParseAOCEChargingUnit(struct Aoc *chanp,u_char *p, u_char *end, int dummy);
int ParseAOCDCurrencyInfo(struct Aoc *chanp,u_char *p, u_char *end, int dummy);
int ParseAOCDChargingUnitInfo(struct Aoc *chanp,u_char *p, u_char *end, int dummy);
int ParseRecordedCurrency(struct Aoc *chanp,u_char *p, u_char *end, int dummy);
int ParseRecordedUnitsList(struct Aoc *chanp,u_char *p, u_char *end, int *recordedUnits);
int ParseTypeOfChargingInfo(struct Aoc *chanp,u_char *p, u_char *end, int *typeOfChargingInfo);
int ParseRecordedUnits(struct Aoc *chanp,u_char *p, u_char *end, int *recordedUnits);
int ParseAOCDBillingId(struct Aoc *chanp, u_char *p, u_char *end, int *billingId);
int ParseAOCECurrencyInfo(struct Aoc *chanp, u_char *p, u_char *end, int dummy);
int ParseAOCEChargingUnitInfo(struct Aoc *chanp,u_char *p, u_char *end, int dummy);
int ParseAOCEBillingId(struct Aoc *chanp,u_char *p, u_char *end, int *billingId);
int ParseCurrency(struct Aoc *chanp,u_char *p, u_char *end, char *currency);
int ParseAmount(struct Aoc *chanp,u_char *p, u_char *end, int dummy);
int ParseCurrencyAmount(struct Aoc *chanp,u_char *p, u_char *end, int *currencyAmount);
int ParseMultiplier(struct Aoc *chanp,u_char *p, u_char *end, int *multiplier);
int ParseTypeOfUnit(struct Aoc *chanp,u_char *p, u_char *end, int *typeOfUnit);
int ParseNumberOfUnits(struct Aoc *chanp,u_char *p, u_char *end, int *numberOfUnits);
int ParseChargingAssociation(struct Aoc *chanp,u_char *p, u_char *end, int dummy);
int ParseChargeIdentifier(struct Aoc *chanp,u_char *p, u_char *end, int dummy);

