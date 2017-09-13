/* $Id: isdnbill.c,v 1.18 2002/03/01 19:33:52 akool Exp $
 *
 * ISDN accounting for isdn4linux. (Billing-module)
 *
 * Copyright 1995 .. 2002 by Andreas Kool (akool@isdn4linux.de)
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
 * Revision 1.1  1995/09/23  16:44:19  akool
 * Initial revision
 *
 */

#include "isdnlog.h"
#include "tools/zone.h"
#include <unistd.h>
#include <asm/param.h>
#include <math.h>
#include "dest.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#define DTAG	 	33
#define CALLING          0
#define CALLED           1

#define MAXMYMSN        20
#define MAXSI           10

#define FREECALL         0
#define ORTSZONE         1
#define CITYCALL         2
#define REGIOCALL        3
#define GERMANCALL       4
#define SONDERRUFNUMMERN 5
#define AUSLAND          6
#define ELSEWHERE        7
#define MAXZONE          8

#define SUBTOTAL         0
#define TOTAL            1

#define COUNTRYLEN      32
#define PROVLEN         20
#define MSNLEN           6

#undef  ME
#undef  OTHER

#define ME     (c.dialout ? CALLING : CALLED)
#define OTHER  (c.dialout ? CALLED : CALLING)

typedef unsigned char UC;

typedef struct {
  char   num[2][64];
  double duration;
  time_t connect;
  int    units;
  int    dialout;
  int    cause;
  int    ibytes;
  int    obytes;
  char   version[64];
  int    si1;
  int    si2;
  double currency_factor;
  char   currency[64];
  double pay;
  int    provider;

  double compute;
  int    computed;
  double aktiv;
  char   country[BUFSIZ];
  char   sprovider[BUFSIZ];
  char   error[BUFSIZ];
  int    zone;
  int    ihome;
  int    known[2];
} CALLER;

typedef struct {
  char    msn[10];
  int     ncalls;
  double  pay;
  double  duration;
  int     ibytes;
  int     obytes;
  double  compute;
  double  aktiv;
  char   *alias;
} MSNSUM;

typedef struct {
  int    ncalls;
  double pay;
  double duration;
  int    ibytes;
  int    obytes;
  int    failed;
  double compute;
  double aktiv;
} PROVSUM;

typedef struct {
  int    ncalls;
  double pay;
  double duration;
  int    ibytes;
  int    obytes;
  double compute;
  double aktiv;
} ZONESUM;

typedef struct {
  char   num[64];
  int    ncalls;
  double pay;
  double duration;
  int    ibytes;
  int    obytes;
  double compute;
  int    ihome;
  int    si1;
} PARTNER;

static char  options[]   = "nv:VioeaN:mftIE";
static char  usage[]     = "%s: usage: %s [ -%s ]\n";


static CALLER   c;
static MSNSUM   msnsum[2][MAXSI][MAXMYMSN];
static PROVSUM  provsum[2][MAXPROVIDER];
static ZONESUM  zonesum[2][MAXZONE];
static PARTNER *partner[2];
static PARTNER *unknown[2];
static TELNUM   number[2];
static char     rstr[BUFSIZ];
static char     home[BUFSIZ];
static int      nhome = 0;
static int      nunknown[2] = { 0, 0 };

int verbose = 0;

static int      onlynumbers = 0;      /* -n    -> _nicht_ anstelle Rufnummern Alias-Bezeichnungen anzeigen */
static int	onlytoday = 0;	      /* -t    -> nur die heutigen Verbindungen anzeigen */
static int      showincoming = 0;     /* -i    -> reinkommende Verbindungen anzeigen */
static int      showoutgoing = 0;     /* -o    -> rausgehende Verbindungen anzeigen */
static int      showerrors = 0;       /* -e    -> nichtzustandegekommene Verbindungen anzeigen */
static int	netto = 0;   	      /* -m    -> ohne MwSt anzeigen */
static int	force = 0;   	      /* -f    -> Verbindungsentgeld _immer_ neu berechnen */
static char     onlythis[32] = { 0 }; /* -Nnnn -> nur Verbindungen mit _dieser_ Rufnummer anzeigen */
       			       	      /* -vn   -> Verbose Level */
                                      /* -V    -> Version anzeigen */
                                      /* -a    -> alle Verbindungen anzeigen i.e. "-ioe"  */
static int	onlyInternal = 0;     /* -I    -> nur Verbindungen am Internen S0-Bus anzeigen */
static int	onlyExternal = 0;     /* -E    -> nur Verbindungen am Externen S0-Bus anzeigen */


int print_msg(int Level, const char *fmt, ...)
{
  auto va_list ap;
  auto char    String[BUFSIZ * 3];


  if ((Level > 1 && !verbose) || ((Level > 2) && (verbose < 2)))
    return(1);

  va_start(ap, fmt);
  (void)vsnprintf(String, BUFSIZ * 3, fmt, ap);
  va_end(ap);

  fprintf((Level == PRT_NORMAL) ? stdout : stderr, "%s", String);

  return(0);
} /* print_msg */


int print_in_modules(const char *fmt, ...)
{
  auto va_list ap;
  auto char    String[LONG_STRING_SIZE];


  va_start(ap, fmt);
  (void)vsnprintf(String, LONG_STRING_SIZE, fmt, ap);
  va_end(ap);

  return print_msg(PRT_ERR, "%s", String);
} /* print_in_modules */


static void when(char *s, int *day, int *month)
{
  auto struct tm *tm = localtime(&c.connect);


  *day = tm->tm_mday;
  *month = tm->tm_mon + 1;

  sprintf(s, "%02d.%02d.%04d %02d:%02d:%02d",
    *day, *month, tm->tm_year + 1900, tm->tm_hour, tm->tm_min, tm->tm_sec);
} /* when */


static void deb(char *s)
{
  register char *p = strchr(s, 0);


  while (*--p == ' ')
    *p = 0;
} /* deb */


char *timestr(double n)
{
  auto int x, h, m, s;
#if 0
  auto int d = 0;
#endif


  if (n <= 0.0)
    sprintf(rstr, "         ");
  else {
    x = (int)n;

    h = (int)(x / 60 / 60);
    x %= 60 * 60;
    m = (int)(x / 60);
    s = (int)(x % 60);

#if 0
    if (h > 99) {
      while (h > 99) {
        h -= 24;
        d++;
      } /* while */

      sprintf(rstr, "%d day(s), %2d:%02d:%02d", d, h, m, s);
    }
    else
#endif
      sprintf(rstr, "%3d:%02d:%02d", h, m, s);
  } /* else */

  return(rstr);
} /* timestr */


static void strich(char c, int len)
{
  auto char s[BUFSIZ];


  memset(s, c, len);
  s[len] = 0;

  printf(s);
  printf("\n");
} /* strich */


static void total(int w)
{
  register int i, j, firsttime = 1;
  auto     int ncalls = 0, failed = 0;
  auto     double duration = 0.0, pay = 0.0, compute = 0.0, aktiv = 0.0;


  firsttime = 1;

  for (i = 0; i < nhome; i++) {
    for (j = 0; j < MAXSI; j++) {
      if (msnsum[w][j][i].ncalls) {

        if (firsttime) {
          firsttime = 0;

  	  printf("\n\nMSN                   calls  Duration         Charge       Computed\n");
  	  strich('-', 67);
        }

        printf("%6s,%d %-12s %5d %s  %s %9.4f",
          msnsum[w][j][i].msn,
          j,
          msnsum[w][j][i].alias ? msnsum[w][j][i].alias : "",
          msnsum[w][j][i].ncalls,
          timestr(msnsum[w][j][i].duration),
          c.currency,
          msnsum[w][j][i].pay);

        if (msnsum[w][j][i].pay == msnsum[w][j][i].compute)
          printf("\n");
        else
          printf("  %s %9.4f\n", c.currency, msnsum[w][j][i].compute);

        ncalls   += msnsum[w][j][i].ncalls;
        duration += msnsum[w][j][i].duration;
        pay      += msnsum[w][j][i].pay;
        compute  += msnsum[w][j][i].compute;
      } /* if */

      if (w == SUBTOTAL) {
        strcpy(msnsum[TOTAL][j][i].msn, msnsum[SUBTOTAL][j][i].msn);
        msnsum[TOTAL][j][i].alias = msnsum[SUBTOTAL][j][i].alias;
        msnsum[TOTAL][j][i].ncalls   += msnsum[SUBTOTAL][j][i].ncalls;
        msnsum[TOTAL][j][i].duration += msnsum[SUBTOTAL][j][i].duration;
        msnsum[TOTAL][j][i].pay      += msnsum[SUBTOTAL][j][i].pay;
        msnsum[TOTAL][j][i].compute  += msnsum[SUBTOTAL][j][i].compute;

        msnsum[SUBTOTAL][j][i].ncalls = 0;
        msnsum[SUBTOTAL][j][i].duration = 0;
        msnsum[SUBTOTAL][j][i].pay = 0;
        msnsum[SUBTOTAL][j][i].compute = 0;
      } /* if */
    } /* for */
  } /* for */

  if (!firsttime) {
  strich('=', 67);

  printf("                      %5d %s  %s %9.4f",
    ncalls,
    timestr(duration),
    c.currency,
    pay);

  if (pay == compute)
    printf("\n");
  else
    printf("  %s %9.4f\n", c.currency, compute);
  } /* if */

  ncalls = 0;
  failed = 0;
  duration = 0.0;
  pay = 0.0;
  compute = 0.0;

  firsttime = 1;


  for (i = 0; i < MAXPROVIDER; i++) {
    if (provsum[w][i].ncalls) {
      if (firsttime) {
        firsttime = 0;

  	printf("\n\nProvider                        calls  Duration         Charge       Computed failures  avail\n");
  	strich('-', 93);
      } /* if */

      if (i < 100)
        printf("  010%02d", i);
      else if (i < 200)
        printf("0100%03d", i - 100);
      else
        printf("01900%02d", i - 200);

      printf(":%-24s", getProvider(pnum2prefix(i, c.connect)));

      printf("%5d %s  %s %9.4f  %s %9.4f %8d %5.1f%%\n",
        provsum[w][i].ncalls,
        timestr(provsum[w][i].duration),
        c.currency,
        provsum[w][i].pay,
        c.currency,
        provsum[w][i].compute,
        provsum[w][i].failed,
        100.0 * (provsum[w][i].ncalls - provsum[w][i].failed) / provsum[w][i].ncalls);

      ncalls   += provsum[w][i].ncalls;
      duration += provsum[w][i].duration;
      pay      += provsum[w][i].pay;
      compute  += provsum[w][i].compute;
      failed   += provsum[w][i].failed;

      if (w == SUBTOTAL) {
        provsum[TOTAL][i].ncalls   += provsum[SUBTOTAL][i].ncalls;
        provsum[TOTAL][i].duration += provsum[SUBTOTAL][i].duration;
        provsum[TOTAL][i].pay      += provsum[SUBTOTAL][i].pay;
        provsum[TOTAL][i].failed   += provsum[SUBTOTAL][i].failed;
        provsum[TOTAL][i].compute  += provsum[SUBTOTAL][i].compute;

        provsum[SUBTOTAL][i].ncalls = 0;
        provsum[SUBTOTAL][i].duration = 0;
        provsum[SUBTOTAL][i].pay = 0;
        provsum[SUBTOTAL][i].failed = 0;
        provsum[SUBTOTAL][i].compute = 0;
      } /* if */
    } /* if */
  } /* for */

  if (!firsttime) {
  strich('=', 93);

  printf("%*s%5d %s  %s %9.4f  %s %9.4f %8d %5.1f%%\n",
    32, "",
    ncalls,
    timestr(duration),
    c.currency,
    pay,
    c.currency,
    compute,
    failed,
    100.0 * (ncalls - failed) / ncalls);
  } /* if */


  ncalls = 0;
  duration = 0.0;
  pay = 0.0;
  compute = 0.0;
  aktiv = 0;

  firsttime = 1;


  for (i = 0; i < MAXZONE; i++) {
    if (zonesum[w][i].ncalls) {

      if (firsttime) {
        firsttime = 0;

  			printf("\n\nZone            calls  Duration         Charge       Computed");

  			if (0 /* preselect == DTAG */) {
    		  printf("     AktivPlus\n");
    			strich('-', 73);
  			}
  			else {
    		  printf("\n");
    			strich('-', 61);
  			} /* else */
      } /* if */

      switch (i) {
        case 0 : printf("FreeCall        "); break;
        case 1 : printf("Ortszone        "); break;
        case 2 : printf("CityCall (R20)  "); break;
        case 3 : printf("RegioCall (R50) "); break;
        case 4 : printf("GermanCall      "); break;
        case 5 : printf("Sonderrufnummern"); break;
        case 6 : printf("Ausland         "); break;
        case 7 : printf("elsewhere       "); break;
      } /* switch */

      printf("%5d %s  %s %9.4f  %s %9.4f",
        zonesum[w][i].ncalls,
        timestr(zonesum[w][i].duration),
        c.currency,
        zonesum[w][i].pay,
        c.currency,
        zonesum[w][i].compute);

      if (0 /* preselect == DTAG */)
        printf("  %s %9.4f\n", c.currency, zonesum[w][i].aktiv);
      else
        printf("\n");

      ncalls   += zonesum[w][i].ncalls;
      duration += zonesum[w][i].duration;
      pay      += zonesum[w][i].pay;
      compute  += zonesum[w][i].compute;
      aktiv    += zonesum[w][i].aktiv;
    } /* if */

    if (w == SUBTOTAL) {
      zonesum[TOTAL][i].ncalls   += zonesum[SUBTOTAL][i].ncalls;
      zonesum[TOTAL][i].duration += zonesum[SUBTOTAL][i].duration;
      zonesum[TOTAL][i].pay      += zonesum[SUBTOTAL][i].pay;
      zonesum[TOTAL][i].compute  += zonesum[SUBTOTAL][i].compute;
      zonesum[TOTAL][i].aktiv    += zonesum[SUBTOTAL][i].aktiv;

      zonesum[SUBTOTAL][i].ncalls = 0;
      zonesum[SUBTOTAL][i].duration = 0;
      zonesum[SUBTOTAL][i].pay = 0;
      zonesum[SUBTOTAL][i].compute = 0;
      zonesum[SUBTOTAL][i].aktiv = 0;
    } /* if */
  } /* for */

  if (!firsttime) {
  strich('=', (0 /* preselect == DTAG */) ? 73 : 61);

  printf("%*s%5d %s  %s %9.4f  %s %9.4f",
    16, "",
    ncalls,
    timestr(duration),
    c.currency,
    pay,
    c.currency,
    compute);

  if (0 /* preselect == DTAG */)
    printf("  %s %9.4f\n", c.currency, aktiv);
  else
    printf("\n");

  printf("\n\n");
  } /* if */
} /* total */


static char *beautify(char *num)
{
  auto char   s[BUFSIZ], sx[BUFSIZ], sy[BUFSIZ];
  auto TELNUM number;
  static char res[BUFSIZ];


  if (*num)
    normalizeNumber(num, &number, TN_ALL);
  else {
    sprintf(res, "%*s", -COUNTRYLEN, "UNKNOWN");
    return(res);
  } /* else */

  if (*number.msn)
    sprintf(sx, "%s%s%s", number.area, (*number.area ? "/" : ""), number.msn);
  else
    strcpy(sx, number.area);

  if (*number.country && strcmp(number.country, mycountry))
    sprintf(s, "%s %s", number.country, sx);
  else
    sprintf(s, "%s%s", (*number.country ? "0" : ""), sx);

  *sy = 0;

  if (getSpecial(num))
    sprintf(sy, "%s", getSpecialName(num));
  else {
    if (*number.country && strcmp(number.country, mycountry))
      sprintf(sy, "%s", number.scountry);
    else if (*number.scountry && strcmp(number.country, mycountry))
      sprintf(sy, "%s", number.scountry);

    if (*number.sarea) {
      if (*sy)
        strcat(sy, ", ");

      strcat(sy, number.sarea);

    } /* if */

    if (!*sy)
      sprintf(sy, "???");
  } /* else */


  sprintf(sx, "%s, %s", s, sy);
  sprintf(res, "%*s", -COUNTRYLEN, sx);
  res[COUNTRYLEN] = 0; /* clipping */

  return(res);
} /* beautify */


static char *clip(char *s, int len)
{
  static char ret[BUFSIZ];


  strcpy(ret, s);
  ret[len] = 0;
  return(ret);
} /* clip */


static void showpartner()
{
  register int   i, k;
  register char *p, *p1;
  auto	   int	 firsttime[2][2];


  memset(firsttime, 0, sizeof(firsttime));

  for (k = 0; k < 2; k++) {
    if (((k == 0) && showincoming) || ((k == 1) && showoutgoing)) {

      for (i = 0; i < knowns; i++)
        if (partner[k][i].ncalls) {
          if (k) {

            if (!firsttime[k][0]) {
	      printf("\nAngerufene (dialout)      calls   Charge        Duration       Computed\n");
              strich('-', 71);
              firsttime[k][0] = 1;
            } /* if */

            printf("%-25s %5d   %s %9.4f %s   %s %9.4f\n",
              clip(onlynumbers ? partner[k][i].num : known[i]->who, 25),
              partner[k][i].ncalls,
              c.currency,
              partner[k][i].pay,
              timestr(partner[k][i].duration),
              c.currency,
              partner[k][i].compute);
          }
          else {

            if (!firsttime[k][0]) {
              printf("\nAnrufer (dialin)          calls  Duration\n");
              strich('-', 41);
              firsttime[k][0] = 1;
            } /* if */

            printf("%-25s %5d %s\n",
              clip(onlynumbers ? partner[k][i].num : known[i]->who, 25),
              partner[k][i].ncalls,
              timestr(partner[k][i].duration));
          } /* else */
        } /* if */

      printf("\n");

      for (i = 0; i < nunknown[k]; i++)
        if (unknown[k][i].ncalls) {
          p = beautify(unknown[k][i].num);

          if (k) {

            if (!firsttime[k][1]) {
	      printf("\nunbekannte Angerufene (dialout)     von          calls   Charge         Duration       Computed\n");
              strich('-', 95);
              firsttime[k][1] = 1;
            } /* if */

            p1 = msnsum[SUBTOTAL][unknown[k][i].si1][unknown[k][i].ihome].alias;

            printf("%-16s <- %-12s %5d   %s %9.4f  %s   %s %9.4f\n",
              p,
              clip(p1 ? p1 : "", 16),
              unknown[k][i].ncalls,
              c.currency,
              unknown[k][i].pay,
              timestr(unknown[k][i].duration),
              c.currency,
              unknown[k][i].compute);
          }
          else {

            if (!firsttime[k][1]) {
	      printf("\nunbekannte Anrufer (dialin)         an Rufnummer calls  Duration\n");
              strich('-', 64);
              firsttime[k][1] = 1;
            } /* if */

            printf("%-16s -> %-12s %5d %s\n",
              p,
              msnsum[SUBTOTAL][unknown[k][i].si1][unknown[k][i].ihome].alias,
              unknown[k][i].ncalls,
              timestr(unknown[k][i].duration));
          } /* else */
        } /* if */
      } /* if */
  } /* for */
} /* showpartner */


static char *numtonam(int n, int other)
{
  register UC *p;
  register int i, j = UNKNOWN;
  static   UC  hash[32 * 255] = { 0 };
  auto     UC  s[64];


  if (onlynumbers || !*c.num[n]) {
    c.known[n] = UNKNOWN;
    return(NULL);
  } /* if */

  if (other) {
    sprintf(s + 2, "%s,%d", c.num[n], c.si1);

    if ((p = strstr(hash, s + 2)) && (p[-2] == 0xff)) {
      i = p[-1];
      c.known[n] = i;

      return(known[i]->who);
    } /* if */
  } /* if */

  for (i = 0; i < knowns; i++) {
    if (!num_match(known[i]->num, c.num[n])) {
      j = i;

      if (known[i]->si == c.si1)
        break;
    } /* if */
  } /* for */

  if (other && (j != UNKNOWN) && (j < 255)) {
    s[0] = (UC)0xff;
    s[1] = (UC)j;
    strcat(hash, s);
  } /* if */

  c.known[n] = j;

  return((j == UNKNOWN) ? NULL : known[j]->who);
} /* numtonam */


static void justify(char *fromnum, char *tonum, TELNUM number)
{
  register char *p1, *p2;
  auto     char  s[BUFSIZ], sx[BUFSIZ], sy[BUFSIZ];


  /* AK:16-Dec-99                                                     */
  /* Hier ist noch ein Bug in normalizeNumber(), Leo!                 */
  /* Bei Sonderrufnummern landet die komplette Nummer in number.area, */
  /* wobei die letzten 2 Digits abgeschnitten sind (fehlen) 	      */

  if (!*number.msn) {
    strcpy(number.msn, c.dialout ? tonum : fromnum);
    *number.area = 0;
  } /* if */

  p1 = numtonam(c.dialout ? CALLED : CALLING, 1);

  if (*number.msn)
    sprintf(sx, "%s%s%s", number.area, (*number.area ? "/" : ""), number.msn);
  else
    strcpy(sx, number.area);

  if (*number.country && strcmp(number.country, mycountry))
    sprintf(s, "%s %s", number.country, sx);
  else
    sprintf(s, "%s%s", (*number.country ? "0" : ""), sx);

  p2 = msnsum[SUBTOTAL][c.si1][c.ihome].alias;

  printf("%12s %s %-21s",
    p2 ? p2 : fromnum,
    (c.dialout ? "->" : "<-"),
    p1 ? clip(p1, 20) : s);

  *s = 0;

  if (*tonum && getSpecial(tonum))
    sprintf(s, "%s", getSpecialName(tonum));
  else {
    if (*number.country && strcmp(number.country, mycountry))
      sprintf(s, "%s", number.scountry);
    else if (*number.scountry && strcmp(number.country, mycountry))
      sprintf(s, "%s", number.scountry);

    if (*number.sarea) {
      if (*s)
        strcat(s, ", ");

      strcat(s, number.sarea);

      if (c.dialout) {
        sprintf(sy, ",%d", c.zone);
        strcat(s, sy);
      } /* if */
    } /* if */

    if (!*s && *tonum)
      sprintf(s, "???");
  } /* else */

  s[COUNTRYLEN] = 0; /* clipping */

  sprintf(c.country, "%-*s", COUNTRYLEN, s);
} /* justify */


static void findme()
{
  register int   i;
  register char *p;
  auto     char  s[BUFSIZ];


  sprintf(s, "%*s", -MSNLEN, number[ME].msn);
  s[MSNLEN] = 0;

  p = strstr(home, s);

  if (p == NULL) {
    strcat(home, s);
    strcat(home, "|");

    p = strstr(home, s);
    c.ihome = (int)(p - home) / 7;

    i = c.si1;

    for (c.si1 = MAXSI; c.si1 >= 0; c.si1--) {
      strcpy(msnsum[SUBTOTAL][c.si1][c.ihome].msn, number[ME].msn);
      msnsum[SUBTOTAL][c.si1][c.ihome].alias = numtonam(ME, 0);
    } /* for */

    c.si1 = i;

    nhome++;
  } /* if */

  c.ihome = (int)(p - home) / 7;
} /* findme */


static void findrate()
{
  auto RATE  Rate;
  auto char *version;


  clearRate(&Rate);
  Rate.start  = c.connect;
  Rate.now    = c.connect + c.duration;
  Rate.prefix = pnum2prefix(c.provider, c.connect);

  Rate.src[0] = number[CALLING].country;
  Rate.src[1] = number[CALLING].area;
  Rate.src[2] = number[CALLING].msn;

  Rate.dst[0] = number[CALLED].country;
  Rate.dst[1] = number[CALLED].area;
  Rate.dst[2] = number[CALLED].msn;

  if (getRate(&Rate, &version) != UNKNOWN) {
    c.zone = Rate.z;

    if (!c.zone && (*number[CALLED].country && strcmp(number[CALLED].country, mycountry)))
      c.zone = AUSLAND;
    else if (getSpecial(c.num[CALLED]))
      c.zone = SONDERRUFNUMMERN;
    else if (c.zone == UNKNOWN)
      c.zone = ELSEWHERE;

    c.compute = Rate.Charge;
  }
  else {
    c.zone = ELSEWHERE;
    c.compute = c.pay;
    sprintf(c.error, " ??? %s", version);
  } /* else */

  zonesum[SUBTOTAL][c.zone].ncalls++;
} /* findrate */


int main(int argc, char *argv[], char *envp[])
{
  register char    *pl, *pr, *p, x;
#ifdef AK
  auto     FILE    *f = fopen("/www/log/isdn.log", "r");
#else
  auto     FILE    *f = fopen("/var/log/isdn.log", "r");
#endif
  auto     char     s[BUFSIZ], sx[BUFSIZ];
  auto     int      i, l, col, day, lday = UNKNOWN, month, lmonth = UNKNOWN;
  auto     double   dur;
  auto     char    *version;
  auto     char    *myname = basename(argv[0]);
  auto     int      opt, go, s0, indent;
  auto	   time_t   now;
  auto 	   struct   tm *tm;


  if (f != (FILE *)NULL) {

    while ((opt = getopt(argc, argv, options)) != EOF)
      switch (opt) {
        case 'n' : onlynumbers++;
                   break;

        case 'v' : verbose = atoi(optarg);
                   break;

        case 'V' : print_version(myname);
                   exit(0);

        case 'i' : showincoming++;
                   break;

        case 'o' : showoutgoing++;
                   break;

        case 'e' : showerrors++;
                   break;

        case 'a' : showincoming = showoutgoing = showerrors = 1;
                   break;

        case 'N' : strcpy(onlythis, optarg);
                   break;

        case 'm' : netto++;
             	   break;

        case 'f' : force++;
             	   break;

        case 't' : onlytoday++;
             	   break;

        case 'I' : onlyInternal++;
             	   break;

        case 'E' : onlyExternal++;
             	   break;

        case '?' : printf(usage, argv[0], argv[0], options);
                   return(1);
      } /* switch */

    if (!showincoming && !showoutgoing && !showerrors) {
      printf("This makes no sense! You must specify -i, -o or -e\n");
      printf("\t-a    -> alle Verbindungen anzeigen i.e. \"-ioe\"\n");
      printf("\t-e    -> nichtzustandegekommene Verbindungen anzeigen\n");
      printf("\t-f    -> Verbindungsentgeld _immer_ neu berechnen\n");
      printf("\t-i    -> reinkommende Verbindungen anzeigen\n");
      printf("\t-m    -> ohne MwSt anzeigen\n");
      printf("\t-n    -> _nicht_ anstelle Rufnummern Alias-Bezeichnungen anzeigen\n");
      printf("\t-o    -> rausgehende Verbindungen anzeigen\n");
      printf("\t-t    -> nur die heutigen Verbindungen anzeigen\n");
      printf("\t-vn   -> Verbose Level\n");
      printf("\t-Nnnn -> nur Verbindungen mit _dieser_ Rufnummer anzeigen\n");
      printf("\t-I    -> nur Verbindungen am Internen S0-Bus anzeigen\n");
      printf("\t-E    -> nur Verbindungen am Externen S0-Bus anzeigen\n");
      printf("\t-V    -> Version anzeigen\n");

      return(1);
    } /* if */

    *home = 0;

    interns0 = 3;

    set_print_fct_for_tools(print_in_modules);

    if (!readconfig(myname)) {

      initHoliday(holifile, &version);

      if (verbose)
        fprintf(stderr, "%s\n", version);

      initDest(destfile, &version);

      if (verbose)
        fprintf(stderr, "%s\n", version);

      initRate(rateconf, ratefile, zonefile, &version);

      if (verbose)
        fprintf(stderr, "%s\n", version);

      memset(&msnsum, 0, sizeof(msnsum));
      memset(&provsum, 0, sizeof(provsum));
      memset(&zonesum, 0, sizeof(zonesum));

      partner[0] = (PARTNER *)calloc(knowns, sizeof(PARTNER));
      partner[1] = (PARTNER *)calloc(knowns, sizeof(PARTNER));

      time(&now);
      tm = localtime(&now);
      tm->tm_sec = tm->tm_min = tm->tm_hour = 0;
      now = mktime(tm);

      while (fgets(s, BUFSIZ, f)) {
        pl = s;
        col = 0;

        memset(&c, 0, sizeof(c));

        while ((pr = strchr(pl, '|'))) {
          memcpy(sx, pl, (l = (pr - pl)));
          sx[l] = 0;
          pl = pr + 1;

          switch (col++) {
            case  0 :                               break;

            case  1 : deb(sx);
                      strcpy(c.num[CALLING], sx);
                      break;

            case  2 : deb(sx);
                      strcpy(c.num[CALLED], sx);
                      break;

            case  3 : dur = atoi(sx);               break;

            case  4 : c.duration = atol(sx) / HZ;
                      break;

            case  5 : c.connect = atol(sx);         break;
            case  6 : c.units = atoi(sx);           break;
            case  7 : c.dialout = *sx == 'O';       break;
            case  8 : c.cause = atoi(sx);           break;
            case  9 : c.ibytes = atoi(sx);          break;
            case 10 : c.obytes = atoi(sx);          break;
            case 11 : strcpy(c.version, sx);        break;
            case 12 : c.si1 = atoi(sx);             break;
            case 13 : c.si2 = atoi(sx);             break;
            case 14 : c.currency_factor = atof(sx); break;
            case 15 : strcpy(c.currency, sx);       break;
            case 16 : c.pay = atof(sx);             break;
            case 17 : c.provider = atoi(sx);        break;
            case 18 :                               break;
          } /* switch */

        } /* while */


        /* Repair wrong entries from older (or current?) isdnlog-versions ... */

        if (abs((int)dur - (int)c.duration) > 1) {
          if (verbose)
            fprintf(stderr, "REPAIR: Duration %f -> %f\n", c.duration, dur);

          c.duration = dur;
        } /* if */

        if (!memcmp(c.num[CALLED], "+4910", 5)) {
          p = c.num[CALLED] + 7;
          x = *p;
          *p = 0;

          c.provider = atoi(c.num[CALLED] + 5);

          *p = x;

          if (strlen(c.num[CALLED]) > 7)
            memmove(c.num[CALLED] + 3, c.num[CALLED] + 8, strlen(c.num[CALLED]) - 7);

          if (verbose)
            fprintf(stderr, "REPAIR: Provider=%d\n", c.provider);
        } /* if */

        if (!c.provider || (c.provider == UNKNOWN)) {
          if (verbose)
            fprintf(stderr, "REPAIR: Provider %d -> %d\n", c.provider, preselect);

          c.provider = preselect;
        } /* if */

        if (c.dialout && (strlen(c.num[CALLED]) > 3) && !getSpecial(c.num[CALLED])) {
          sprintf(s, "0%s", c.num[CALLED] + 3);

          if (getSpecial(s)) {
            if (verbose)
              fprintf(stderr, "REPAIR: Callee %s -> %s\n", c.num[CALLED], s);

            strcpy(c.num[CALLED], s);
          } /* if */
        } /* if */

        if (!c.dialout && (strlen(c.num[CALLING]) > 3) && !getSpecial(c.num[CALLING])) {
          sprintf(s, "0%s", c.num[CALLING] + 3);

          if (getSpecial(s)) {
            if (verbose)
              fprintf(stderr, "REPAIR: Caller %s -> %s\n", c.num[CALLING], s);

            strcpy(c.num[CALLING], s);
          } /* if */
        } /* if */


        go = 0;

        if (showoutgoing && c.dialout && c.duration)
          go++;

        if (showincoming && !c.dialout && c.duration)
          go++;

        if (showerrors && !c.duration)
          go++;

        if (*onlythis && strstr(c.num[CALLING], onlythis) == NULL &&
           	      	 strstr(c.num[CALLED], onlythis) == NULL)
          go = 0;

        if (onlytoday && c.connect < now)
          go = 0;

        s0 = 0; /* Externer S0 */

        if (c.dialout && (strlen(c.num[CALLING]) < interns0))
          s0 = 1; /* Interner S0-Bus */

        if (!c.dialout && (strlen(c.num[CALLED]) < interns0))
          s0 = 1; /* Interner S0-Bus */

        if (onlyInternal && !s0)
          go = 0;

        if (onlyExternal && s0)
          go = 0;


        if (go) {
          when(s, &day, &month);

          if (lmonth == UNKNOWN)
            lmonth = month;
          else if (month != lmonth) {
            total(SUBTOTAL);
            lmonth = month;
          } /* if */

          if (lday == UNKNOWN)
            lday = day;
          else if (day != lday) {
            printf("\n");
            lday = day;
          } /* else */

          printf("%s%s ", s, timestr(c.duration));

	  if (*c.num[CALLING])
            normalizeNumber(c.num[CALLING], &number[CALLING], TN_ALL);
          else {
	    memset(&number[CALLING], 0, sizeof(TELNUM));
            strcpy(number[CALLING].msn, "UNKNOWN");
          } /* else */

	  if (*c.num[CALLED])
            normalizeNumber(c.num[CALLED], &number[CALLED], TN_ALL);
          else {
	    memset(&number[CALLED], 0, sizeof(TELNUM));
            strcpy(number[CALLED].msn, "UNKNOWN");
          } /* else */

          findme();

          indent = 11 + strlen(c.currency);

          if (c.dialout) {

            findrate();

            msnsum[SUBTOTAL][c.si1][c.ihome].ncalls++;

            justify(number[CALLING].msn, c.num[CALLED], number[CALLED]);

            provsum[SUBTOTAL][c.provider].ncalls++;

            strcpy(s, getProvider(pnum2prefix(c.provider, c.connect)));
            s[PROVLEN] = 0;

            if (c.provider < 100)
              sprintf(c.sprovider, "  010%02d:%-*s", c.provider, PROVLEN, s);
            else if (c.provider < 200)
              sprintf(c.sprovider, "0100%03d:%-*s", c.provider - 100, PROVLEN, s);
            else
              sprintf(c.sprovider, "01900%02d:%-*s", c.provider - 200, PROVLEN, s);


            if (c.duration) {

#if 0 // Berechnung, um wieviel es mit AktivPlus der DTAG billiger waere -- stimmt irgendwie eh nicht mehr ...

              if ((preselect == DTAG) && ((c.zone == 1) || (c.zone == 2))) {
                auto struct tm *tm = localtime(&c.connect);
                auto int        takte;
                auto double     price;


                takte = (c.duration + 59) / 60;

                if ((tm->tm_wday > 0) && (tm->tm_wday < 5)) {   /* Wochentag */
                  if ((tm->tm_hour > 8) && (tm->tm_hour < 18))  /* Hauptzeit */
                    price = 0.06;
                  else
                    price = 0.03;
                }
                else                                            /* Wochenende */
                  price = 0.03;

                c.aktiv = takte * price;

                msnsum[SUBTOTAL][c.si1][c.ihome].aktiv += c.aktiv;
                provsum[SUBTOTAL][c.provider].aktiv += c.aktiv;
                zonesum[SUBTOTAL][c.zone].aktiv += c.aktiv;
              } /* if */
#endif

              if (c.pay < 0.0) { /* impossible! */
                c.pay = c.compute;
                c.computed++;
              } /* if */

              if (force || fabs(c.pay - c.compute) > 1.00) {
                c.pay = c.compute;
                c.computed++;
              } /* if */

              if (netto)
                c.pay = c.pay * 100.0 / 116.0;

              if (c.pay)
                printf("%s%9.4f%s ", c.currency, c.pay, c.computed ? "*" : " ");
              else
                printf("%*s", indent, "");

              printf("%s%s%s", c.country, c.sprovider, c.error);

#if 0
              if (c.aktiv)
                printf(" AktivPlus - %s%9.4f", c.currency, c.pay - c.aktiv);
#endif

              msnsum[SUBTOTAL][c.si1][c.ihome].pay += c.pay;
              msnsum[SUBTOTAL][c.si1][c.ihome].duration += c.duration;
              msnsum[SUBTOTAL][c.si1][c.ihome].compute += c.compute;
              msnsum[SUBTOTAL][c.si1][c.ihome].ibytes += c.ibytes;
              msnsum[SUBTOTAL][c.si1][c.ihome].obytes += c.obytes;

              provsum[SUBTOTAL][c.provider].pay += c.pay;
              provsum[SUBTOTAL][c.provider].duration += c.duration;
              provsum[SUBTOTAL][c.provider].compute += c.compute;
              provsum[SUBTOTAL][c.provider].ibytes += c.ibytes;
              provsum[SUBTOTAL][c.provider].obytes += c.obytes;

              zonesum[SUBTOTAL][c.zone].pay += c.pay;
              zonesum[SUBTOTAL][c.zone].duration += c.duration;
              zonesum[SUBTOTAL][c.zone].compute += c.compute;
              zonesum[SUBTOTAL][c.zone].ibytes += c.ibytes;
              zonesum[SUBTOTAL][c.zone].obytes += c.obytes;
            }
            else {
              printf("%*s%s%s", indent, "", c.country, c.sprovider);

              if ((c.cause != 0x1f) && /* Normal, unspecified */
                  (c.cause != 0x10))   /* Normal call clearing */
                printf(" %s", qmsg(TYPE_CAUSE, VERSION_EDSS1, c.cause));

              if ((c.cause == 0x22) || /* No circuit/channel available */
                  (c.cause == 0x2a) || /* Switching equipment congestion */
                  (c.cause == 0x2f))   /* Resource unavailable, unspecified */
                provsum[SUBTOTAL][c.provider].failed++;
            } /* else */
          }
          else { /* Dialin: */
            justify(number[CALLED].msn, c.num[CALLING], number[CALLING]);
            printf("%*s%s%s", indent, "", c.country, c.sprovider);
          } /* else */


          if (c.known[OTHER] == UNKNOWN) {
            l = UNKNOWN;

            for (i = 0; i < nunknown[c.dialout]; i++) {
              if (!strcmp(unknown[c.dialout][i].num, c.num[OTHER])) {
                l = i;
                break;
              } /* if */
            } /* for */

            if (l == UNKNOWN) {
              l = nunknown[c.dialout];

              nunknown[c.dialout]++;

              if (!l)
                unknown[c.dialout] = (PARTNER *)malloc(sizeof(PARTNER));
              else
                unknown[c.dialout] = (PARTNER *)realloc(unknown[c.dialout], sizeof(PARTNER) * nunknown[c.dialout]);

              memset(&unknown[c.dialout][l], 0, sizeof(PARTNER));
            } /* if */

            strcpy(unknown[c.dialout][l].num, c.num[OTHER]);

            unknown[c.dialout][l].ihome = c.ihome;
            unknown[c.dialout][l].ncalls++;
            unknown[c.dialout][l].pay += c.pay;
            unknown[c.dialout][l].duration += c.duration;
            unknown[c.dialout][l].compute += c.compute;
            unknown[c.dialout][l].ibytes += c.ibytes;
            unknown[c.dialout][l].obytes += c.obytes;
          }
          else {
            strcpy(partner[c.dialout][c.known[OTHER]].num, c.num[OTHER]);
            partner[c.dialout][c.known[OTHER]].ncalls++;
            partner[c.dialout][c.known[OTHER]].pay += c.pay;
            partner[c.dialout][c.known[OTHER]].duration += c.duration;
            partner[c.dialout][c.known[OTHER]].compute += c.compute;
            partner[c.dialout][c.known[OTHER]].ibytes += c.ibytes;
            partner[c.dialout][c.known[OTHER]].obytes += c.obytes;
          } /* else */

          printf("\n");

        } /* if */
      } /* while */

      fclose(f);
      total(SUBTOTAL);

      if (!onlytoday)
        total(TOTAL);

      showpartner();

    }
    else
      fprintf(stderr, "%s: Can't read configuration file(s)\n", myname);
  }
  else
    fprintf(stderr, "%s: Can't open \"isdn.log\" file\n", myname);

  return(0);
} /* isdnbill */
