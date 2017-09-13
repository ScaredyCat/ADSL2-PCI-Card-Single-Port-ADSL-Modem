/* $Id: rate.h,v 1.21 2000/05/07 11:29:32 akool Exp $
 *
 * Tarifdatenbank
 *
 * Copyright 1995 .. 2000 by Andreas Kool (akool@isdn4linux.de)
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
 * $Log: rate.h,v $
 * Revision 1.21  2000/05/07 11:29:32  akool
 * isdnlog-4.21
 *  - isdnlog/tools/rate.{c,h} ...     new X:tag for exclusions
 *  - isdnlog/tools/telnum.c ... 	    new X:tag for exclusions
 *  - isdnlog/tools/rate-files.man ... -"-
 *  - isdnlog/tools/NEWS ... 	    -"-
 *  - isdnlog/README ... 		    -"-
 *  - new rates
 *
 * Revision 1.20  2000/01/16 12:36:59  akool
 * isdnlog-4.03
 *  - Patch from Gerrit Pape <pape@innominate.de>
 *    fixes html-output if "-t" option of isdnrep is omitted
 *  - Patch from Roland Rosenfeld <roland@spinnaker.de>
 *    fixes "%p" in ILABEL and OLABEL
 *
 * Revision 1.19  1999/12/31 13:57:20  akool
 * isdnlog-4.00 (Millenium-Edition)
 *  - Oracle support added by Jan Bolt (Jan.Bolt@t-online.de)
 *  - resolved *any* warnings against rate-de.dat
 *  - Many new rates
 *  - CREDITS file added
 *
 * Revision 1.18  1999/12/24 14:17:06  akool
 * isdnlog-3.81
 *  - isdnlog/tools/NEWS
 *  - isdnlog/tools/telrate/info.html.in  ... bugfix
 *  - isdnlog/tools/telrate/telrate.cgi.in ... new Service query
 *  - isdnlog/tools/telrate/Makefile.in ... moved tmp below telrate
 *  - isdnlog/samples/rate.conf.at ... fixed
 *  - isdnlog/tools/rate-at.c ... some changes
 *  - isdnlog/rate-at.dat ... ditto
 *  - isdnlog/tools/Makefile ... added path to pp_rate
 *  - isdnlog/tools/rate.{c,h}  ... getServiceNames, Date-Range in T:-Tag
 *  - isdnlog/tools/isdnrate.c ... fixed sorting of services, -X52 rets service names
 *  - isdnlog/tools/rate-files.man ... Date-Range in T:-Tag, moved from doc
 *  - isdnlog/tools/isdnrate.man ... moved from doc
 *  - doc/Makefile.in ... moved man's from doc to tools
 *  - isdnlog/Makefile.in ... man's, install isdn.conf.5
 *  - isdnlog/configure{,.in} ... sed, awk for man's
 *  - isdnlog/tools/zone/Makefile.in ... dataclean
 *  - isdnlog/tools/dest/Makefile.in ... dataclean
 *  - isdnlog/isdnlog/isdnlog.8.in ... upd
 *  - isdnlog/isdnlog/isdn.conf.5.in ... upd
 *
 * Revision 1.17  1999/12/01 21:47:25  akool
 * isdnlog-3.72
 *   - new rates for 01051
 *   - next version of isdnbill
 *
 *   - isdnlog/tools/telnum.c ... cleanup
 *   - isdnlog/tools/isdnrate.c ... -s Service
 *   - isdnlog/tools/rate.{c,h} ... -s
 *   - isdnlog/tools/NEWS ... -s
 *   - doc/isdnrate.man .. updated -o, -s
 *   - doc/rate-files.man ... updated
 *   - isdnlog/tools/dest/README.makedest ... updt.
 *   - isdnlog/isdnlog/isdnlog.8.in .. updt.
 *
 *   Telrate
 *   - isdnlog/tools/telrate/README-telrate
 *   - isdnlog/tools/telrate/config.in 	NEW
 *   - isdnlog/tools/telrate/configure 	NEW
 *   - isdnlog/tools/telrate/Makefile.in 	NEW
 *   - isdnlog/tools/telrate/index.html.in 	was index.html
 *   - isdnlog/tools/telrate/info.html.in 	was info.html
 *   - isdnlog/tools/telrate/telrate.cgi.in 	was telrate.cgi
 *   - isdnlog/tools/telrate/leo.sample 	NEW sample config
 *   - isdnlog/tools/telrate/alex.sample 	NEW sample config
 *
 * Revision 1.16  1999/11/25 22:58:40  akool
 * isdnlog-3.68
 *  - new utility "isdnbill" added
 *  - patch from Jochen Erwied (j.erwied@gmx.de)
 *  - new rates
 *  - small fixes
 *
 * Revision 1.15  1999/11/08 21:09:41  akool
 * isdnlog-3.65
 *   - added "B:" Tag to "rate-xx.dat"
 *
 * Revision 1.14  1999/11/07 13:29:29  akool
 * isdnlog-3.64
 *  - new "Sonderrufnummern" handling
 *
 * Revision 1.13  1999/09/26 10:55:20  akool
 * isdnlog-3.55
 *   - Patch from Oliver Lauer <Oliver.Lauer@coburg.baynet.de>
 *     added hup3 to option file
 *   - changed country-de.dat to ISO 3166 Countrycode / Airportcode
 *
 * Revision 1.12  1999/09/09 11:21:06  akool
 * isdnlog-3.49
 *
 * Revision 1.11  1999/08/25 17:07:18  akool
 * isdnlog-3.46
 *
 * Revision 1.10  1999/06/28 19:16:51  akool
 * isdnlog Version 3.38
 *   - new utility "isdnrate" started
 *
 * Revision 1.9  1999/06/15 20:05:16  akool
 * isdnlog Version 3.33
 *   - big step in using the new zone files
 *   - *This*is*not*a*production*ready*isdnlog*!!
 *   - Maybe the last release before the I4L meeting in Nuernberg
 *
 * Revision 1.8  1999/05/22 10:19:30  akool
 * isdnlog Version 3.29
 *
 *  - processing of "sonderrufnummern" much more faster
 *  - detection for sonderrufnummern of other provider's implemented
 *    (like 01929:FreeNet)
 *  - Patch from Oliver Lauer <Oliver.Lauer@coburg.baynet.de>
 *  - Patch from Markus Schoepflin <schoepflin@ginit.de>
 *  - easter computing corrected
 *  - rate-de.dat 1.02-Germany [22-May-1999 11:37:33] (from rate-CVS)
 *  - countries-de.dat 1.02-Germany [22-May-1999 11:37:47] (from rate-CVS)
 *  - new option "-B" added (see README)
 *    (using "isdnlog -B16 ..." isdnlog now works in the Netherlands!)
 *
 * Revision 1.7  1999/05/13 11:40:07  akool
 * isdnlog Version 3.28
 *
 *  - "-u" Option corrected
 *  - "ausland.dat" removed
 *  - "countries-de.dat" fully integrated
 *      you should add the entry
 *      "COUNTRYFILE = /usr/lib/isdn/countries-de.dat"
 *      into section "[ISDNLOG]" of your config file!
 *  - rate-de.dat V:1.02-Germany [13-May-1999 12:26:24]
 *  - countries-de.dat V:1.02-Germany [13-May-1999 12:26:26]
 *
 * Revision 1.6  1999/05/09 18:24:26  akool
 * isdnlog Version 3.25
 *
 *  - README: isdnconf: new features explained
 *  - rate-de.dat: many new rates from the I4L-Tarifdatenbank-Crew
 *  - added the ability to directly enter a country-name into "rate-xx.dat"
 *
 * Revision 1.5  1999/04/14 13:17:26  akool
 * isdnlog Version 3.14
 *
 * - "make install" now install's "rate-xx.dat", "rate.conf" and "ausland.dat"
 * - "holiday-xx.dat" Version 1.1
 * - many rate fixes (Thanks again to Michael Reinelt <reinelt@eunet.at>)
 *
 * Revision 1.4  1999/04/10 16:36:42  akool
 * isdnlog Version 3.13
 *
 * WARNING: This is pre-ALPHA-dont-ever-use-Code!
 * 	 "tarif.dat" (aka "rate-xx.dat"): the next generation!
 *
 * You have to do the following to test this version:
 *   cp /usr/src/isdn4k-utils/isdnlog/holiday-de.dat /etc/isdn
 *   cp /usr/src/isdn4k-utils/isdnlog/rate-de.dat /usr/lib/isdn
 *   cp /usr/src/isdn4k-utils/isdnlog/samples/rate.conf.de /etc/isdn/rate.conf
 *
 * After that, add the following entries to your "/etc/isdn/isdn.conf" or
 * "/etc/isdn/callerid.conf" file:
 *
 * [ISDNLOG]
 * SPECIALNUMBERS = /usr/lib/isdn/sonderrufnummern.dat
 * HOLIDAYS       = /usr/lib/isdn/holiday-de.dat
 * RATEFILE       = /usr/lib/isdn/rate-de.dat
 * RATECONF       = /etc/isdn/rate.conf
 *
 * Please replace any "de" with your country code ("at", "ch", "nl")
 *
 * Good luck (Andreas Kool and Michael Reinelt)
 *
 * Revision 1.3  1999/03/24 19:39:03  akool
 * - isdnlog Version 3.10
 * - moved "sondernnummern.c" from isdnlog/ to tools/
 * - "holiday.c" and "rate.c" integrated
 * - NetCologne rates from Oliver Flimm <flimm@ph-cip.uni-koeln.de>
 * - corrected UUnet and T-Online rates
 *
 * Revision 1.2  1999/03/16 17:38:10  akool
 * - isdnlog Version 3.07
 * - Michael Reinelt's patch as of 16Mar99 06:58:58
 * - fix a fix from yesterday with sondernummern
 * - ignore "" COLP/CLIP messages
 * - dont show a LCR-Hint, if same price
 *
 * Revision 1.1  1999/03/14 12:16:43  akool
 * - isdnlog Version 3.04
 * - general cleanup
 * - new layout for "rate-xx.dat" and "holiday-xx.dat" files from
 *     Michael Reinelt <reinelt@eunet.at>
 *     unused by now - it's a work-in-progress !
 * - bugfix for Wolfgang Siefert <siefert@wiwi.uni-frankfurt.de>
 *     The Agfeo AS 40 (Software release 2.1b) uses AOC_AMOUNT, not AOC_UNITS
 * - bugfix for Ralf G. R. Bergs <rabe@RWTH-Aachen.DE> - 0800/xxx numbers
 *     are free of charge ;-)
 * - tarif.dat V 1.08 - new mobil-rates DTAG
 *
 */

#ifndef _RATE_H_
#define _RATE_H_

typedef struct {
  int        prefix;    /* Providerkennung */
  int        zone;      /* Zonennummer */
  char      *src[3];    /* eigene Telefonnummer [Land, Vorwahl, Nummer]*/
  char      *dst[3];    /* gerufene Nummer */
  time_t     start;     /* Verbindungsaufbau */
  time_t     now;       /* momentane Zeit */
  int        domestic;  /* Inlandsverbindung */
  int        _area;     /* interner(!) Länderindex */
  int        _zone;     /* interner(!) Zonenindex */
  char      *Provider;  /* Name des Providers */
  char      *Country;   /* Landesname (Ausland) */
  char      *Zone;      /* Name der Zone */
  char      *Service;   /* Name des Dienstes (S:-Tag) */
  char      *Flags;     /* Inhalt des F:-Tags */
  char      *Day;       /* Wochen- oder Feiertag */
  char      *Hour;      /* Bezeichnung des Tarifs */
  double     Basic;     /* Grundpreis einer Verbindung */
  double     Sales;     /* Mindestumsatz einer Verbindung */
  double     Price;     /* Preis eines Tarifimpulses */
  double     Duration;  /* Länge eines Tarifimpulses */
  int        Units;     /* verbrauchte Tarifimpulse */
  double     Charge;    /* gesamte Verbindungskosten */
  double     Rhythm[2]; /* Taktung */
  time_t     Time;      /* gesamte Verbindungszeit */
  time_t     Rest;      /* bezahlte, aber noch nicht verbrauchte Zeit */
  int        z;		/* Zone lt. Verzonungstabelle (nur 1:Ortszone; 2:City/Regio 20; 3=Regio 50; 4=Fern sinnvoll!) */
} RATE;

#define UNZONE -2

void  exitRate(void);
int   initRate(char *conf, char *dat, char *dom, char **msg);
char *getProvider(int prefix);
char *getProviderVBN(int prefix);
int   getSpecial(char *number);
char* getSpecialName(char *number);
char *getServiceNum(char *name);
char *getServiceNames(int first);
char *getComment(int prefix, char *key);
void  clearRate (RATE *Rate);
int   getRate(RATE *Rate, char **msg);
int   getLeastCost(RATE *Current, RATE *Cheapest, int booked, int skip);
int   getZoneRate(RATE* Rate, int domestic, int first);
char *explainRate (RATE *Rate);
char *printRate (double value);

int pnum2prefix(int pnum, time_t when);
int pnum2prefix_variant(char * pnum, time_t when);
int vbn2prefix(char *vbn, int *len);
inline int getNProvider( void );
/* char   *prefix2provider(int prefix, char *s) is defined in telnum.h */
char   *prefix2provider_variant(int prefix, char *s);
int isProviderValid(int prefix, time_t when);
inline int isProviderBooked( int i);
int getPrsel(char *telnum, int *provider, int *zone, int *area);


#endif
