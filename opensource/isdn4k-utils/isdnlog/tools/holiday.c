 /* $Id: holiday.c,v 1.18 2000/01/12 23:22:53 akool Exp $
 *
 * Feiertagsberechnung
 *
 * Copyright 1999 by Michael Reinelt (reinelt@eunet.at)
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
 * $Log: holiday.c,v $
 * Revision 1.18  2000/01/12 23:22:53  akool
 * - isdnlog/tools/holiday.c ... returns ERVERYDAY for '*'
 * - FAQ/configure{,.in} ...  test '==' => '='
 * - isdnlog/tools/dest/configure{,.in} ...  test '==' => '='
 * - isdnlog/tools/dest/Makefile.in ...  test '==' => '='
 * - isdnlog/tools/zone/configure{,.in} ...  test '==' => '='
 *
 * - isdnlog/tools/rate-at.c ... P:1069
 * - isdnlog/rate-at.dat ... P:1069
 * - isdnlog/country-de.dat ... _DEMF
 *
 * - many new rates
 * - more EURACOM sequences decoded
 *
 * Revision 1.17  1999/12/31 13:57:19  akool
 * isdnlog-4.00 (Millenium-Edition)
 *  - Oracle support added by Jan Bolt (Jan.Bolt@t-online.de)
 *  - resolved *any* warnings against rate-de.dat
 *  - Many new rates
 *  - CREDITS file added
 *
 * Revision 1.16  1999/09/09 11:21:05  akool
 * isdnlog-3.49
 *
 * Revision 1.15  1999/07/15 16:41:49  akool
 * small enhancement's and fixes
 *
 * Revision 1.14  1999/06/22 19:41:19  akool
 * zone-1.1 fixes
 *
 * Revision 1.13  1999/06/15 20:05:04  akool
 * isdnlog Version 3.33
 *   - big step in using the new zone files
 *   - *This*is*not*a*production*ready*isdnlog*!!
 *   - Maybe the last release before the I4L meeting in Nuernberg
 *
 * Revision 1.12  1999/05/22 10:19:21  akool
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
 * Revision 1.11  1999/05/09 18:24:18  akool
 * isdnlog Version 3.25
 *
 *  - README: isdnconf: new features explained
 *  - rate-de.dat: many new rates from the I4L-Tarifdatenbank-Crew
 *  - added the ability to directly enter a country-name into "rate-xx.dat"
 *
 * Revision 1.10  1999/04/29 19:03:37  akool
 * isdnlog Version 3.22
 *
 *  - T-Online corrected
 *  - more online rates for rate-at.dat (Thanks to Leopold Toetsch <lt@toetsch.at>)
 *
 * Revision 1.9  1999/04/26 22:12:14  akool
 * isdnlog Version 3.21
 *
 *  - CVS headers added to the asn* files
 *  - repaired the "4.CI" message directly on CONNECT
 *  - HANGUP message extended (CI's and EH's shown)
 *  - reactivated the OVERLOAD message
 *  - rate-at.dat extended
 *  - fixes from Michael Reinelt
 *
 * Revision 1.8  1999/04/20 20:31:58  akool
 * isdnlog Version 3.19
 *   patches from Michael Reinelt
 *
 * Revision 1.7  1999/04/19 19:25:07  akool
 * isdnlog Version 3.18
 *
 * - countries-at.dat added
 * - spelling corrections in "countries-de.dat" and "countries-us.dat"
 * - LCR-function of isdnconf now accepts a duration (isdnconf -c .,duration)
 * - "rate-at.dat" and "rate-de.dat" extended/fixed
 * - holiday.c and rate.c fixed (many thanks to reinelt@eunet.at)
 *
 * Revision 1.6  1999/04/16 14:39:58  akool
 * isdnlog Version 3.16
 *
 * - more syntax checks for "rate-xx.dat"
 * - isdnrep fixed
 *
 * Revision 1.5  1999/04/14 13:17:15  akool
 * isdnlog Version 3.14
 *
 * - "make install" now install's "rate-xx.dat", "rate.conf" and "ausland.dat"
 * - "holiday-xx.dat" Version 1.1
 * - many rate fixes (Thanks again to Michael Reinelt <reinelt@eunet.at>)
 *
 * Revision 1.4  1999/04/10 16:36:31  akool
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
 * Revision 1.3  1999/03/24 19:38:53  akool
 * - isdnlog Version 3.10
 * - moved "sondernnummern.c" from isdnlog/ to tools/
 * - "holiday.c" and "rate.c" integrated
 * - NetCologne rates from Oliver Flimm <flimm@ph-cip.uni-koeln.de>
 * - corrected UUnet and T-Online rates
 *
 * Revision 1.2  1999/03/16 17:38:03  akool
 * - isdnlog Version 3.07
 * - Michael Reinelt's patch as of 16Mar99 06:58:58
 * - fix a fix from yesterday with sondernummern
 * - ignore "" COLP/CLIP messages
 * - dont show a LCR-Hint, if same price
 *
 * Revision 1.1  1999/03/14 12:16:23  akool
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

/*
 * Schnittstelle:
 *
 * int initHoliday(char *path, char **msg)
 *   initialisiert die Feiertagsberechnung, liest die Feiertagsdatei
 *   und gibt die Anzahl der Feiertage zurück, im Fehlerfall -1
 *
 * void exitHoliday()
 *   deinitialisiert die Feiertagsberechnung
 *
 * int isDay(struct tm *tm, bitfield mask, char **name)
 *   prüft, ob <tm> ein Tag aus <mask> ist, und liefert
 *   eine Beschreibung in <*name>
 */

#define _HOLIDAY_C_

#ifdef STANDALONE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#ifndef __GLIBC__
extern const char *basename (const char *name);
#endif
#else
#include "isdnlog.h"
#include "tools.h"
#endif

#include "holiday.h"

#define SOMEDAY (1<<MONDAY | 1<<TUESDAY | 1<<WEDNESDAY | 1<<THURSDAY | 1<<FRIDAY | 1<<SATURDAY | 1<<SUNDAY)
#define LENGTH 120  /* max length of lines in data file */
#define COUNT(array) sizeof(array)/sizeof(array[0])

typedef unsigned int julian;

typedef struct {
  int day;
  int month;
  char *name;
} HOLIDATE;

static char *defaultWeekday[] = { "", /* not used */
				  "Everyday",
				  "Workday",
				  "Weekend",
				  "Monday",
				  "Tuesday",
				  "Wednesday",
				  "Thursday",
				  "Friday",
				  "Saturday",
				  "Sunday",
				  "Holiday" };

static int        line = 0;
static char      *Weekday[COUNT(defaultWeekday)] = { NULL, };
static int        nHoliday = 0;
static HOLIDATE  *Holiday = NULL;

static void warning (char *file, char *fmt, ...)
{
  va_list ap;
  char msg[BUFSIZ];

  va_start (ap, fmt);
  vsnprintf (msg, BUFSIZ, fmt, ap);
  va_end (ap);
#ifdef STANDALONE
  fprintf(stderr, "WARNING: %s line %3d: %s\n", basename(file), line, msg);
#else
  print_msg(PRT_NORMAL, "WARNING: %s line %3d: %s\n", basename(file), line, msg);
#endif
}

/* easter & julian calculations by Guenther Brunthaler (gbr001@yahoo.com) */

static julian date2julian(int y, int m, int d)
{
  if (m<3) {m+=9; y--;} else m-=3;
  return (146097*(y/100))/4+(1461*(y%100))/4+(153*m+2)/5+d;
}

#if 0 /* not used by now */
static void julian2date(julian jd, int *yp, int *mp, int *dp)
{
  julian j,c,y,m,d;

  j=jd*4-1;
  c=(j/146097)*100;
  d=(j%146097)/4;
  y=(4*d+3)/1461;
  d=(((4*d+3)%1461)+4)/4;
  m=(5*d-3)/153;
  d=(((5*d-3)%153)+5)/5;

  if (m>9) {m-=9; y++;} else m+=3;

  *yp=c+y;
  *mp=m;
  *dp=d;
}
#endif

static julian getEaster(int year)
{
  int g, c, x, z, d, e, n, m;

  g=year%19+1;
  c=year/100+1;
  x=3*c/4-12;
  z=(8*c+5)/25-5;
  d=5*year/4-x-10;
  e=(11*g+20+z-x)%30;
  if ((e==25 && g>11) || e==24) e++;
  n=44-e;
  if (n<21) n+=30;
  n+=7-(d+n)%7;
  if (n>31) {n-=31; m=4;} else m=3;
  return date2julian(year,m,n);
}

static char *strip (char *s)
{
  char *p;

  while (isblank(*s)) s++;
  for (p=s; *p; p++)
    if (*p=='#' || *p=='\n') {
      *p='\0';
      break;
    }
  for (p--; p>s && isblank(*p); p--)
    *p='\0';
  return s;
}

void exitHoliday(void)
{
  int i;

  for (i=0; i<COUNT(Weekday); i++) {
    if (Weekday[i]) {
      free (Weekday[i]);
      Weekday[i]=NULL;
    }
  }
  for (i=0; i<nHoliday; i++) {
    if (Holiday[i].name) free (Holiday[i].name);
  }
  if (Holiday) free (Holiday);
  nHoliday=0;
  Holiday=NULL;
}

int initHoliday(char *path, char **msg)
{
  FILE *stream;
  int   i,m,d;
  char *s, *date, *name;
  char  buffer[LENGTH];
  char  version[LENGTH]="<unknown>";
  static char message[LENGTH];

  if (msg)
    *(*msg=message)='\0';

  exitHoliday();

  for (i=0; i<COUNT(Weekday); i++)
    Weekday[i]=strdup(defaultWeekday[i]);

  if (!path || !*path) {
    if (msg) snprintf (message, LENGTH, "Warning: no holiday database specified!");
    return 0;
  }

  if ((stream=fopen(path,"r"))==NULL) {
    if (msg) snprintf (message, LENGTH, "Error: could not load holidays from %s: %s",
		       path, strerror(errno));
    return -1;
  }

  line=0;
  while ((s=fgets(buffer,LENGTH,stream))!=NULL) {
    line++;
    if (*(s=strip(s))=='\0')
      continue;
    if (s[1]!=':') {
      warning (path, "expected ':', got '%s'!", s+1);;
      continue;
    }
    switch (*s) {
    case 'W':
      s+=2; while (isblank(*s)) s++;
      if (isdigit(*s)) {
	d=strtol(s,&s,10);
	if (d<1 || d>7) {
	  warning(path, "invalid weekday %d", d);
	  continue;
	}
	d+=MONDAY-1;
      } else if (*s=='W') {
	d=WORKDAY;
	s++;
      } else if (*s=='E') {
	d=WEEKEND;
	s++;
      } else if (*s=='H') {
	d=HOLIDAY;
	s++;
      } else if (*s=='*') {
	d=EVERYDAY;
	s++;
      } else {
	warning(path, "invalid weekday %c", *s);
	continue;
      }
      if (!isblank(*s)) {
	warning ("expected whitespace, got '%s'!", s);;
	continue;
      }
      if (*(name=strip(s))=='\0') {
	warning(path, "empty weekday %d", d);
	continue;
      }
      if (Weekday[d]) free(Weekday[d]);
      Weekday[d]=strdup(name);
      break;

    case 'D':
      name=s+2;
      if ((date=strsep(&name," \t"))==NULL) {
	warning(path, "Syntax error");
	continue;
      }
      if (strncmp(date,"easter",6)==0) {
	m=-1;
	d=atoi(date+6);
      } else {
	d=atof(strsep(&date,"."));
	m=atof(strsep(&date,"."));
      }

      Holiday=(HOLIDATE*)realloc(Holiday,(nHoliday+1)*sizeof(HOLIDATE));
      Holiday[nHoliday].day=d;
      Holiday[nHoliday].month=m;
      Holiday[nHoliday].name=strdup(strip(name));
      nHoliday++;
      break;

    case 'V': /* V:xxx Version der Datenbank */
      s+=2; while(isblank(*s)) s++;
      strcpy(version, s);
      break;

    default:
      warning(path, "Unknown tag '%c'", *s);
    }
  }
  fclose(stream);

  if (msg) snprintf (message, LENGTH, "Holiday Version %s loaded [%d entries from %s]",
		     version, nHoliday, path);

  return nHoliday;
}

static int isHoliday(struct tm *tm, char **name)
{
  int i;
  julian easter, day;

  day=date2julian(tm->tm_year+1900,tm->tm_mon+1,tm->tm_mday);
  easter=getEaster(tm->tm_year+1900);

  for (i=0; i<nHoliday; i++) {
    if ((Holiday[i].month==-1 && Holiday[i].day==day-easter) ||
	(Holiday[i].month==tm->tm_mon+1  && Holiday[i].day==tm->tm_mday)) {
      if(name) *name=Holiday[i].name;
      return 1;
    }
  }
  return 0;
}

static char *staticString (char *fmt, ...)
{
  va_list ap;
  char buffer[BUFSIZ];
  int i;

  static char **Table=NULL;
  static int    Size=0;

  va_start (ap, fmt);
  vsnprintf (buffer, BUFSIZ, fmt, ap);
  va_end (ap);

  for (i=0; i<Size; i++)
    if (strcmp (buffer, Table[i])==0)
      return Table[i];

  Size++;
  Table=realloc(Table, Size*sizeof(char*));
  Table[Size-1]=strdup(buffer);

  return Table[Size-1];
}

int isDay(struct tm *tm, bitfield mask, char **name)
{
  char *holiname;

  julian day=(date2julian(tm->tm_year+1900,tm->tm_mon+1,tm->tm_mday)-6)%7+MONDAY;

  if ((mask & (1<<HOLIDAY)) && isHoliday(tm, &holiname)) {
    if (name) *name=staticString("%s (%s)", Weekday[HOLIDAY], holiname);
    return HOLIDAY;
  }

  if ((mask & (1<<WEEKEND)) && (day==SATURDAY || day==SUNDAY)) {
    if (name) *name=staticString("%s (%s)", Weekday[WEEKEND], Weekday[day]);
    return WEEKEND;
  }

  if ((mask & (1<<WORKDAY)) && day!=SATURDAY && day!=SUNDAY && !isHoliday(tm, NULL)) {
    if (name) *name=staticString("%s (%s)", Weekday[WORKDAY], Weekday[day]);
    return WORKDAY;
  }

  if (mask & (1<<day)) {
    if (name) *name=staticString("%s", Weekday[day]);
    return day;
  }

  if (mask & (1<<EVERYDAY)) {
    if (name) *name=staticString("%s", Weekday[EVERYDAY]);
    return EVERYDAY;
  }

  return 0;
}

#ifdef HOLITEST
void main (int argc, char *argv[])
{
  int i, d;
  char *msg, *name;
  struct tm tm;


  initHoliday("../holiday-de.dat", &msg);
  printf ("%s\n", msg);

  for (i=1; i < argc; i++) {
    tm.tm_mday=atoi(strsep(argv+i,"."));
    tm.tm_mon=atoi(strsep(argv+i,"."))-1;
    tm.tm_year=atoi(strsep(argv+i,"."))-1900;

    d=isDay(&tm,1<<HOLIDAY,&name);
    printf ("%02d.%02d.%04d\t%2d = %s\n", tm.tm_mday,tm.tm_mon+1,tm.tm_year+1900,d,d?name:"no Holiday");
    d=isDay(&tm,1<<WEEKEND,&name);
    printf ("%02d.%02d.%04d\t%2d = %s\n", tm.tm_mday,tm.tm_mon+1,tm.tm_year+1900,d,d?name:"no Weekend");
    d=isDay(&tm,1<<WORKDAY,&name);
    printf ("%02d.%02d.%04d\t%2d = %s\n", tm.tm_mday,tm.tm_mon+1,tm.tm_year+1900,d,d?name:"no Workday");
    d=isDay(&tm,SOMEDAY,&name);
    printf ("%02d.%02d.%04d\t%2d = %s\n", tm.tm_mday,tm.tm_mon+1,tm.tm_year+1900,d,d?name:"no Day (Uh?)");
  }
}
#endif
