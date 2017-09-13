/* $Id: isdnrate.c,v 1.38 2001/03/01 14:59:16 paul Exp $

 * ISDN accounting for isdn4linux. (rate evaluation)
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
 * $Log: isdnrate.c,v $
 * Revision 1.38  2001/03/01 14:59:16  paul
 * Various patches to fix errors when using the newest glibc,
 * replaced use of insecure tempnam() function
 * and to remove warnings etc.
 *
 * Revision 1.37  2000/07/17 16:34:23  akool
 * isdnlog-4.32
 *  - added new Prefixes 0160 (D1) and 0162 (D2) to "country-de.dat"
 *  - corrected all german mobil phone numbers (many thank's to
 *    Tobias Becker <i4l-projects@talypso.de> for the tool "fix_rates.pl")
 *  - isdnlog/tools/rate.c ... New R:-tag
 *  - isdnlog/tools/isdnrate.c ... print warnings from getRate if verbose
 *  - isdnlog/tools/rate-files.man ... New R:-tag
 *  - isdnlog/tools/NEWS ... New R:-tag
 *  - isdnlog/README ... New R:-tag
 *  - isdnlog/samples/rtest.dat ... example rate-file for testing R:
 *
 * Revision 1.36  2000/03/09 18:50:03  akool
 * isdnlog-4.16
 *  - isdnlog/samples/isdn.conf.no ... changed VBN
 *  - isdnlog/isdnlog/isdnlog.c .. ciInterval
 *  - isdnlog/isdnlog/processor.c .. ciInterval
 *  - isdnlog/tools/tools.h .. ciInterval, abclcr conf option
 *  - isdnlog/tools/isdnconf.c .. ciInterval, abclcr conf option
 *  - isdnlog/tools/isdnrate.c .. removed a warning
 *  - isdnlog/NEWS ... updated
 *  - isdnlog/README ... updated
 *  - isdnlog/isdnlog/isdnlog.8.in ... updated
 *  - isdnlog/isdnlog/isdnlog.5.in ... updated
 *  - isdnlog/samples/provider ... NEW
 *
 *    ==> Please run a make clean, and be sure to read isdnlog/NEWS for changes
 *    ==> and new features.
 *
 * Revision 1.35  2000/03/06 07:03:21  akool
 * isdnlog-4.15
 *   - isdnlog/tools/tools.h ... moved one_call, sum_calls to isdnrep.h
 *     ==> DO A 'make clean' PLEASE
 *   - isdnlog/tools/telnum.c ... fixed a small typo
 *   - isdnlog/isdnrep/rep_main.c ... incl. dest.h
 *   - isdnlog/isdnrep/isdnrep.c ... fixed %l, %L
 *   - isdnlog/isdnrep/isdnrep.h ... struct one_call, sum_calls are now here
 *
 *   Support for Norway added. Many thanks to Tore Ferner <torfer@pvv.org>
 *     - isdnlog/rate-no.dat  ... NEW
 *     - isdnlog/holiday-no.dat  ... NEW
 *     - isdnlog/samples/isdn.conf.no ... NEW
 *     - isdnlog/samples/rate.conf.no ... NEW
 *
 * Revision 1.34  2000/02/28 19:53:56  akool
 * isdnlog-4.14
 *   - Patch from Roland Rosenfeld <roland@spinnaker.de> fix for isdnrep
 *   - isdnlog/tools/rate.c ... epnum
 *   - isdnlog/tools/rate-at.c ... new rates
 *   - isdnlog/rate-at.dat
 *   - isdnlog/tools/rate-files.man ... %.3f
 *   - doc/Configure.help ... unknown cc
 *   - isdnlog/configure.in ... unknown cc
 *   - isdnlog/.Config.in ... unknown cc
 *   - isdnlog/Makefile.in ... unknown cc
 *   - isdnlog/tools/dest/Makefile.in ... LANG => DEST_LANG
 *   - isdnlog/samples/rate.conf.pl ... NEW
 *   - isdnlog/samples/isdn.conf.pl ... NEW
 *   - isdnlog/rate-pl.dat ... NEW
 *   - isdnlog/tools/isdnrate.c ... fixed -P pid_dir, restarts on HUP now
 *   - isdnlog/tools/isdnrate.man ... SIGHUP documented
 *
 * Revision 1.33  2000/02/12 16:40:24  akool
 * isdnlog-4.11
 *  - isdnlog/Makefile.in ... sep install-targets, installs samples, no isdnconf
 *  - isdnlog/samples/rate.conf.{lu,nl} ... NEW
 *  - isdnlog/samples/isdn.conf.lu ... chg provider
 *  - isdnlog/samples/stop ... chg \d,\d => \d.\d
 *  - isdnlog/samples/isdnlog.isdnctrl0.options ... NEW
 *  - isdnlog/samples/isdnlog.users ... NEW
 *  - isdnlog/country-de.dat ... _DEMF again
 *  - isdnlog/isdnlog/processor.c ... LCR
 *  - isdnlog/tools/isdnrate.c ... fmt of s
 *
 *    Old config is not installed anymore, to acomplish this do
 *
 *    make install-old-conf
 *    make install
 *
 *    A running isdnlog is now HUP-ed not KILL-ed
 *
 * Revision 1.32  2000/02/03 18:24:51  akool
 * isdnlog-4.08
 *   isdnlog/tools/rate.c ... LCR patch again
 *   isdnlog/tools/isdnrate.c ... LCR patch again
 *   isdnbill enhanced/fixed
 *   DTAG AktivPlus fixed
 *
 * Revision 1.31  1999/12/31 13:57:20  akool
 * isdnlog-4.00 (Millenium-Edition)
 *  - Oracle support added by Jan Bolt (Jan.Bolt@t-online.de)
 *  - resolved *any* warnings against rate-de.dat
 *  - Many new rates
 *  - CREDITS file added
 *
 * Revision 1.30  1999/12/24 14:17:05  akool
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
 * Revision 1.29  1999/12/17 22:51:55  akool
 * isdnlog-3.79
 *  - isdnlog/isdnrep/isdnrep.{c,h} ... error -handling, print_msg
 *  - isdnlog/isdnrep/rep_main.c
 *  - isdnlog/isdnrep/isdnrep.1.in
 *  - isdnlog/tools/rate.c  ... dupl entry in rate.conf
 *  - isdnlog/tools/NEWS
 *  - isdnlog/tools/isdnrate.c
 *  - isdnlog/tools/dest/configure{,.in}
 *  - isdnlog/tools/zone/configure{,.in}
 *
 * Revision 1.28  1999/12/16 23:08:28  akool
 * isdnlog-3.78
 *  - isdnlog/tools/isdnrate.c ... new sOcket option
 *  - isdnlog/tools/NEWS ... new sOcket option
 *  - doc/isdnrate.man ... ditto
 *  - isdnlog/tools/telrate/config.in ... ditto
 *  - isdnlog/tools/telrate/telrate.cgi.in ... ditto
 *  - isdnlog/tools/telrate/README-telrate ditto
 *
 * Revision 1.27  1999/12/02 19:28:02  akool
 * isdnlog-3.73
 *  - isdnlog/tools/telrate/telrate.cgi.in faster
 *  - doc/isdnrate.man ... -P
 *  - isdnlog/tools/isdnrate.c ... -P
 *  - isdnlog/tools/NEWS ... -P
 *  - isdnlog/tools/rate-at.c ... 194040
 *  - isdnlog/rate-at.dat
 *  - isdnlog/tools/rate.c ... SIGSEGV
 *
 * Revision 1.26  1999/12/01 21:47:25  akool
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
 *   - isdnlog/tools/telrate/config.in  NEW
 *   - isdnlog/tools/telrate/configure  NEW
 *   - isdnlog/tools/telrate/Makefile.in        NEW
 *   - isdnlog/tools/telrate/index.html.in      was index.html
 *   - isdnlog/tools/telrate/info.html.in       was info.html
 *   - isdnlog/tools/telrate/telrate.cgi.in     was telrate.cgi
 *   - isdnlog/tools/telrate/leo.sample         NEW sample config
 *   - isdnlog/tools/telrate/alex.sample        NEW sample config
 *
 * Revision 1.25  1999/11/08 21:09:41  akool
 * isdnlog-3.65
 *   - added "B:" Tag to "rate-xx.dat"
 *
 * Revision 1.24  1999/11/07 13:29:28  akool
 * isdnlog-3.64
 *  - new "Sonderrufnummern" handling
 *
 * Revision 1.22  1999/10/26 18:17:14  akool
 * isdnlog-3.62
 *  - many new rates
 *  - next try to fix "Sonderrufnummern"
 *
 * isdnlog-3.58
 *   - big cleanup ( > 1.3 Mb removed!)
 *   - v0.02 of destination support - better, but not perfect
 *     (does't work with gcc-2.7.2.3 yet - use egcs!)
 *
 * Revision 1.21  1999/10/25 18:30:03  akool
 * isdnlog-3.57
 *   WARNING: Experimental version!
 *         Please use isdnlog-3.56 for production systems!
 *
 * Revision 1.20  1999/09/19 14:16:27  akool
 * isdnlog-3.53
 *
 * Revision 1.19  1999/09/16 20:27:21  akool
 * isdnlog-3.52
 *
 * Revision 1.18  1999/09/13 09:09:44  akool
 * isdnlog-3.51
 *   - changed getProvider() to not return NULL on unknown providers
 *     many thanks to Matthias Eder <mateder@netway.at>
 *   - corrected zone-processing when doing a internal -> world call
 *
 * Revision 1.17  1999/09/09 11:21:05  akool
 * isdnlog-3.49
 *
 * Revision 1.16  1999/08/04 23:06:52  akool
 * Better codeall for .at
 *
 * Revision 1.15  1999/07/31 09:25:36  akool
 * getRate() speedup
 *
 * Revision 1.14  1999/07/26 16:28:41  akool
 * getRate() speedup from Leo
 *
 * Revision 1.13  1999/07/25 15:57:46  akool
 * isdnlog-3.43
 *   added "telnum" module
 *
 * Revision 1.12  1999/07/24 08:45:17  akool
 * isdnlog-3.42
 *   rate-de.dat 1.02-Germany [18-Jul-1999 10:44:21]
 *   better Support for Ackermann Euracom
 *   WEB-Interface for isdnrate
 *   many small fixes
 *
 * Revision 1.11  1999/07/15 16:42:04  akool
 * small enhancement's and fixes
 *
 * Revision 1.10  1999/07/07 19:44:07  akool
 * patches from Michael and Leo
 *
 * Revision 1.9  1999/07/04 20:47:05  akool
 * rate-de.dat V:1.02-Germany [04-Jul-1999 22:56:37]
 *
 * Revision 1.8  1999/07/03 10:24:14  akool
 * fixed Makefile
 *
 * Revision 1.7  1999/07/02 19:18:00  akool
 * rate-de.dat V:1.02-Germany [02-Jul-1999 21:27:20]
 *
 * Revision 1.6  1999/07/02 18:20:50  akool
 * rate-de.dat V:1.02-Germany [02-Jul-1999 20:29:21]
 * country-de.dat V:1.02-Germany [02-Jul-1999 19:13:54]
 *
 * Revision 1.5  1999/07/01 20:40:07  akool
 * isdnrate optimized
 *
 * Revision 1.4  1999/06/30 20:53:28  akool
 * added "-t" option to "isdnrate"
 *
 * Revision 1.3  1999/06/30 17:17:37  akool
 * isdnlog Version 3.39
 *
 * Revision 1.2  1999/06/29 20:11:25  akool
 * now compiles with ndbm
 * (many thanks to Nima <nima_ghasseminejad@public.uni-hamburg.de>)
 *
 * Revision 1.1  1999/06/28 19:16:33  akool
 * isdnlog Version 3.38
 *   - new utility "isdnrate" started
 *
 */

#include "isdnlog.h"
#include "tools/zone.h"
#include <unistd.h>
#include "dest.h"

#define WIDTH   19
#define _MAXLAST 20		/* the real max */
#define MAXLAST	 ((best>=_MAXLAST||best<=2)?5:best)


static void print_header(void);

static char *myname, *myshortname;
static char options[] = "ab:d:f:h:l:op:st:v::x:CD::G:HLNP:O:S:TUVX::Z";
static char usage[] = "%s: usage: %s [ -%s ] Destination ...\n";

static int header = 0, best = MAXPROVIDER, table = 0, explain = 0;
int     verbose = 0;
static int usestat = 0;
static int duration = LCR_DURATION;
static time_t start;
static int day, month, year, hour, min, sec;
static char ignore[MAXPROVIDER];
static char *fromarea = 0;
static char wanted_day;
static int list = 0;
static char *comment;
static char **providers = 0;	/* incl these */
static int n_providers = 0;
static int business = 0;
static char **xproviders = 0;	/* excl these */
static int nx_providers = 0;
static int xbusiness = 0;
static int all = 0;
static int booked = 0;
static int service = 0;

#define SOCKNAME "/tmp/isdnrate"
static int is_daemon = 0;
static int is_client = 0;
static int we_are_daemon = 0;
static int takt = 99999;
static char sortby;
static int need_dest;
static int h_param = 0;
static int lcr = 0;
static TELNUM srcnum, destnum;
static char *pid_dir = 0;
static char *pid_file = 0;
static char *socket_file = 0;
static char	**hup_argv;	/* args to restart with */

typedef struct {
  int     prefix;
  double  rate;
  char   *name;
  char   *explain;
} SORT;

typedef struct {
  int     weight;
  int     index;
} SORT2;

static SORT sort[MAXPROVIDER];

#undef MAXPROVIDER
#define MAXPROVIDER getNProvider()

#undef BUFSIZ			/* sorry but 8192 is too much for me */
#define BUFSIZ 256

int     print_msg(int Level, const char *fmt,...)
{
  auto va_list ap;
  auto char String[BUFSIZ * 3];

  if ((Level > PRT_ERR && !verbose) || (Level > PRT_WARN && verbose < 2))
    return (1);

  va_start(ap, fmt);
  (void) vsnprintf(String, BUFSIZ * 3, fmt, ap);
  va_end(ap);

  fprintf(stderr, "%s", String);

  return (0);
}				/* print_msg */


static void init()
{
  auto char *version, **message;

  if (readconfig(myshortname) < 0)
    exit(1);

  if (verbose)
    message = &version;
  else
    message = NULL;

  initHoliday(holifile, message);

  if (verbose && *version)
    print_msg(PRT_V, "%s\n", version);
  initDest(destfile, message);
  if (verbose && *version)
    print_msg(PRT_V, "%s\n", version);

  initRate(rateconf, ratefile, zonefile, message);

  if (verbose && *version)
    print_msg(PRT_V, "%s\n", version);

}				/* init */

#if 0
static void deinit(void)
{
  exitRate();
  exitDest();
  exitHoliday();
}
#endif

/* calc a day/time W | E | H */
static void get_day(char d)
{
  struct tm *tm;
  bitfield mask;
  int     what = 0;

  tm = localtime(&start);	/* now */
  switch (d) {
  case 'W':			/* we need a normal weekday, so we take
				   today and inc. day if today is
				   holiday */
    what = WORKDAY;
    if (!h_param)
      hour = 10;
    break;
  case 'N':
    what = WORKDAY;
    if (!h_param)
      hour = 23;
    break;
  case 'E':
    what = SUNDAY;
    if (!h_param)
      hour = 10;
    break;
  }
  mask = 1 << what;
  while (isDay(tm, mask, 0) != what) {
    tm->tm_mday++;
  }
  min = sec = 0;
  day = tm->tm_mday;
  month = tm->tm_mon + 1;
  year = tm->tm_year + 1900;
}

static void post_init()
{

  clearNum(&srcnum);
  if (fromarea) {
    Strncpy(srcnum.area, fromarea, TN_MAX_AREA_LEN);
    free(fromarea);
    fromarea = 0;
  }
  initNum(&srcnum);

  if (wanted_day)
    get_day(wanted_day);
}				/* post_init */

static int opts(int argc, char *argv[])
{
  register int c;
  register char *p;
  int     x;

  h_param = 0;

  optind = 0;			/* make it repeatable */
  need_dest = 1;
  while ((c = getopt(argc, argv, options)) != EOF) {
    switch (c) {
    case 'a':			/* all rates old/newer */
      all++;
      break;
    case 'b':
      best = strtol(optarg, NIL, 0);
      break;

/*    case 'c': country */
      break;

    case 'd':
      for (p = optarg; *p && isspace(*p); p++) ;
      if (isdigit(*p)) {
	wanted_day = '\0';
	day = atoi(optarg);
	if ((p = strchr(optarg, '.'))) {
	  month = atoi(p + 1);
	  if ((p = strchr(p + 1, '.'))) {
	    year = atoi(p + 1);
	    if (year < 50)
	      year += 2000;
	    else if (year < 100)
	      year += 1900;
	  }
	}
      }				/* isdigit */
      else {
	wanted_day = *p;
      }
      break;

    case 'f':
      if (optarg) {		/* from */
	for (p = optarg; (isspace(*p) || *p == '0') && *p; p++) ;
	fromarea = strdup(p);
      }
      break;

    case 'h':
      hour = atoi(optarg);
      h_param++;
      sec = 0;
      if ((p = strchr(optarg + 1, ':'))) {
	min = atoi(p + 1);
	if ((p = strchr(p + 1, ':')))
	  sec = atoi(p + 1);
      }
      else
	min = 0;
      break;

    case 'l':
      duration = strtol(optarg, NIL, 0);	/* l wie lt */
      break;

    case 'o':			/* booked only */
      booked++;
      break;

    case 's':			/* service */
      service++;
      break;

    case 't':
      x = strtol(optarg, NIL, 0);
      if (x > 0)
	takt = x;
      break;

    case 'x':			/* eXclude Poviders */
      {
	char   *arg = strdup(optarg);

	p = strtok(arg, ",");
	while (p) {
	  if (*p == 'B') {	/* Business Provider */
	    xbusiness = 1;
	    p = strtok(0, ",");
	    continue;
	  }
	  xproviders = realloc(xproviders, (nx_providers + 1) * sizeof(char *));

	  xproviders[nx_providers] = strdup(p);
	  p = strtok(0, ",");
	  nx_providers++;
	}
	free(arg);
      }
      break;

    case 'p':			/* Providers ... */
      {
	char   *arg = strdup(optarg);

	p = strtok(arg, ",");
	while (p) {
	  if (*p == 'B') {	/* Business Provider */
	    business = 1;
	    p = strtok(0, ",");
	    continue;
	  }
	  providers = realloc(providers, (n_providers + 1) * sizeof(char *));

	  providers[n_providers] = strdup(p);
	  p = strtok(0, ",");
	  n_providers++;
	}
	free(arg);
      }
      break;

    case 'v':
      verbose++;
      if (optarg && (x = atoi(optarg)))
	verbose = x;
      break;
/* Uppercase options are for output format */

    case 'C':
      is_client = 1;
      break;

    case 'D':
      is_daemon = 1;
      need_dest = 0;
      if (optarg) {
	x = atoi(optarg);
	is_daemon = x;
	if (we_are_daemon && x == 3) {
	  kill(getppid(), SIGTERM);
	  exit(EXIT_SUCCESS);
	}
      }
      break;

    case 'G':
      explain = atoi(optarg);
      break;
    case 'H':
      header++;
      break;
    case 'L':
      list++;
      explain = 9;
      break;
    case 'N':
      explain = 55;
      break;
    case 'P':
      pid_dir = strdup(optarg);
      break;
    case 'O':
      free(socket_file);
      socket_file = strdup(optarg);
      break;
    case 'S':
      sortby = *optarg;
      break;
    case 'T':
      table++;
      break;
/* Fixme: check/warn illegal kombinations of options */
    case 'U':
      usestat++;
      break;

    case 'V':
      print_version(myshortname);
      exit(0);

    case 'X':
      if (explain == 0) {
	explain++;
	if (optarg && isdigit(*optarg) && (x = atoi(optarg))) {
	  explain = x;
	  if (x == 50 || x == 51 || x == 52)
	    need_dest = 0;
	}
	else if (optarg) {
	  comment = strdup(optarg);
	  explain = 8;
	  need_dest = 0;
	}
	break;
      }
      /* fallthrough */
    case '?':
      print_msg(PRT_A, usage, myshortname, myshortname, options);
      break;

    case 'Z':
      lcr++;
      break;
    }				/* switch */
  }				/* while */
  if (is_client && is_daemon != 3 && is_daemon) {
    is_daemon = 0;
    print_msg(PRT_V, "Conflicting options, -D disabled\n");
  }
  if (list && table) {
    table = 0;
    print_msg(PRT_V, "Conflicting options, -T disabled\n");
  }
  if (usestat && !table)
    print_msg(PRT_V, "Conflicting options, -U ignored\n");
  if ((list || table) && explain >= 10) {
    list = table = 0;
    print_msg(PRT_V, "Conflicting options, -T, -L disabled\n");
  }
  if (best < 1) {
    best = MAXPROVIDER;
    print_msg(PRT_V, "Illegal options, -b ignored\n");
  }
  if ((explain == 50 || explain == 51 || explain == 52) && header) {
    print_msg(PRT_V, "Conflicting options, -H ignored\n");
  }
  if (explain >= 10 && service) {
    print_msg(PRT_V, "Conflicting options, -s ignored\n");
    service = 0;
  }
  if (argc > optind)
    return (optind);
  else
    return (0);
}				/* opts */


static int (*compare_func) (const void *s1, const void *s2);
static int compare(const void *s1, const void *s2)
{
  return (((SORT *) s1)->rate > ((SORT *) s2)->rate);
}				/* compare */
static int compare_vbn(const void *s1, const void *s2)
{
  return (((SORT *) s1)->prefix > ((SORT *) s2)->prefix);
}
static int compare_name(const void *s1, const void *s2)
{
  return strcasecmp(((SORT *) s1)->name, ((SORT *) s2)->name);
}

static int compare2(const void *s1, const void *s2)
{
  return (((SORT2 *) s1)->weight < ((SORT2 *) s2)->weight);
}				/* compare2 */

char   *short_explainRate(RATE * Rate)
{

  static char buffer[BUFSIZ];
  char   *p = buffer;

  if (service && Rate->dst[1] && *Rate->dst[1])
    p += sprintf(p, "%s, ", Rate->dst[1]);

  if (Rate->Zone && *Rate->Zone)
    p += sprintf(p, "%s", Rate->Zone);
  else
    p += sprintf(p, "Zone %d", Rate->zone);

  if (!Rate->domestic && Rate->Country && *Rate->Country)
    p += sprintf(p, " (%s)", Rate->Country);

  if (Rate->Day && *Rate->Day)
    p += sprintf(p, ", %s", Rate->Day);

  if (Rate->Hour && *Rate->Hour)
    p += sprintf(p, ", %s", Rate->Hour);

  return buffer;
}

static char *printrate(RATE * Rate)
{
  static char message[BUFSIZ];

  if (Rate->Basic > 0)
    sprintf(message, "%s + %s/%.1fs = %s + %s/Min (%s)",
	    printRate(Rate->Basic),
	    printRate(Rate->Price),
	    Rate->Duration,
	    printRate(Rate->Basic),
	    printRate(60 * Rate->Price / Rate->Duration),
	    short_explainRate(Rate));
  else
    sprintf(message, "%s/%.1fs = %s/Min (%s)",
	    printRate(Rate->Price),
	    Rate->Duration,
	    printRate(60 * Rate->Price / Rate->Duration),
	    short_explainRate(Rate));

  return (message);
}				/* printrate */


static void buildtime()
{
  auto struct tm tm;

  tm.tm_sec = sec;
  tm.tm_min = min;
  tm.tm_hour = hour;
  tm.tm_mday = day;
  tm.tm_mon = month - 1;
  tm.tm_year = year - 1900;	/* ja, ja, ich wei� ;-) */
  tm.tm_isdst = -1;

  start = mktime(&tm);
}				/* buildtime */


static void splittime()
{
  auto struct tm *tm;

  tm = localtime(&start);
  sec = tm->tm_sec;
  min = tm->tm_min;
  hour = tm->tm_hour;
  day = tm->tm_mday;
  month = tm->tm_mon + 1;
  year = tm->tm_year + 1900;
}				/* splittime */

static char *Provider(int prefix)
{
  register char *p, *p1;
  register int l;
  static char s[BUFSIZ];
  char    prov[TN_MAX_PROVIDER_LEN];

  if (prefix == UNKNOWN)
    return ("?");

  p = getProvider(prefix);

  l = max(WIDTH, strlen(p)) - strlen(p);

  p1 = prefix2provider_variant(prefix, prov);

  l += (8 - strlen(p1));

  sprintf(s, "%s:%s%*s", p1, p, l, "");

  return (s);
}				/* Provider */

static char *takt_str(RATE * Rate)
{
  static char s[BUFSIZ];

  if (floor(Rate->Rhythm[0]) == Rate->Rhythm[0])
    sprintf(s, "%.0f/", Rate->Rhythm[0]);
  else
    sprintf(s, "%.3f/", Rate->Rhythm[0]);
  if (floor(Rate->Rhythm[1]) == Rate->Rhythm[1])
    sprintf(s + strlen(s), "%.0f", Rate->Rhythm[1]);
  else
    sprintf(s + strlen(s), "%.3f", Rate->Rhythm[1]);
  return s;
}

static inline char *P_EMPTY(char *s)
{
  char   *p = s;

  return p ? p : "";
}

#define DEL ';'
static int compute(char *num)
{
  register int i, n = 0;
  register int low = 0, high = MAXPROVIDER - 1;
  auto RATE Rate;
  auto char s[BUFSIZ];
  struct tm *tm;
  char    prov[TN_MAX_PROVIDER_LEN];
  int     first = 1;
  static char BUSINESS[] = "Business";	/* in C:GT:Tag */

  if (destnum.nprovider != UNKNOWN) {
    low = high = destnum.nprovider;
  }				/* if */
  compare_func = compare;
  switch (sortby) {
  case 'v':
    compare_func = compare_vbn;
    break;
  case 'n':
    compare_func = compare_name;
    break;
  }

  buildtime();
  if (explain == 98 || explain == 97) {		/* Minutenpreis fuer
						   diese Woche */
    tm = localtime(&start);
    tm->tm_hour = 0;
    tm->tm_min = 1;
    tm->tm_sec = 0;
    start = mktime(tm);
    if (explain == 98) {
      while (tm->tm_wday) {	/* find last monday */
	start -= (60 * 60 * 24);
	tm = localtime(&start);
      }				/* while */
      start += (60 * 60 * 24);
    }
  }
  if (service) {
    num = getServiceNum(num);
    if (!num || !*num) {
      return 0;
    }
  }
  do {
    for (i = low; i <= high; i++) {
      int     found, p;
      char   *t;

      if (ignore[i])		/* Fixme: */
	continue;
      if (booked && !isProviderBooked(i))
	continue;
      if (!all && !isProviderValid(i, start))
	continue;
      t = getProvider(i);
      if (!t || t[strlen(t) - 1] == '?')	/* UNKNOWN Provider */
	continue;

      t = getComment(i, "GT");	/* get Geb. Text comment */
      if (business) {		/* only business wanted */
	if (t == 0)
	  continue;
	else if (strstr(t, BUSINESS) == 0)
	  continue;
      }
      if (xbusiness) {		/* no business wanted */
	if (t && strstr(t, BUSINESS) > 0)
	  continue;
      }
      found = 0;
      if (n_providers) {
	for (p = 0; p < n_providers; p++)
	  if (pnum2prefix_variant(providers[p], start) == i) {
	    found = 1;
	    break;
	  }
	if (!found)
	  continue;
      }
      if (nx_providers) {
	for (p = 0; p < nx_providers; p++)
	  if (pnum2prefix_variant(xproviders[p], start) == i) {
	    found = 1;
	    break;
	  }
	if (found)
	  continue;
      }
      clearRate(&Rate);
      Rate.src[0] = srcnum.country;
      Rate.src[1] = srcnum.area;
      Rate.src[2] = "";

      destnum.nprovider = i;
      Strncpy(destnum.provider, getProvider(i), TN_MAX_PROVIDER_LEN);
      if (normalizeNumber(num, &destnum, TN_ALL) == UNKNOWN) {
	continue;
      }

      Rate.dst[0] = destnum.country;
      Rate.dst[1] = destnum.area;
      Rate.dst[2] = destnum.msn;
      /* Rate.Service = "Internet by call"; */

      Rate.prefix = i;

      Rate.start = start;
      Rate.now = start + duration - 1;
      if (explain == 99) {
	int     j;
	double  oldCharge = -1.0;

	if (first && header)
	  print_header();
	first = 0;
	printf("@ %s\n", prefix2provider_variant(Rate.prefix, prov));
	Rate.now = start + 1;
	for (j = 1; j < duration; j++) {
	  if (!getRate(&Rate, NULL) && (Rate.Price != 99.99)) {
	    if (Rate.Charge != oldCharge || j == duration - 1) {
	      printf("%d %.4f\n", j, Rate.Charge);
	      oldCharge = Rate.Charge;
	    }
	  }
	  else
	    break;
	  Rate.now++;
	}
	if (Rate.Duration <= takt)
	  printf("@----- %s %s\n", currency, Rate.Provider);
      }
      if (explain == 98 || explain == 97) {	/* Minutenpreis fuer
						   diese Woche/Tag */
	int     j;

	if (first && header)
	  print_header();
	first = 0;
	printf("@ %s\n", prefix2provider_variant(Rate.prefix, prov));
	for (j = 0; j < (explain == 98 ? 7 * 24 : 24); j++) {
	  if (!getRate(&Rate, NULL) && (Rate.Price != 99.99)) {
	    printf("%d %.4f\n", j, Rate.Charge);
	  }
	  else
	    break;
	  Rate.now += 3600;
	  Rate.start += 3600;
	}
	if (Rate.Duration <= takt)
	  printf("@----- %s %s\n", currency, Rate.Provider);
      }
      else if (explain == 50 || explain == 51) {
	int     fi = 1;

	while (getZoneRate(&Rate, explain - 50, fi) == 0) {
	  double  cpm = Rate.Duration > 0 ? 60 * Rate.Price / Rate.Duration : 99.99;

	  if (Rate.Price == 0)
	    cpm = Rate.Basic;

	  fi = 0;
	  if (Rate.Price != 99.99)
	    printf("%s%c%s%c%s%c%.2f%c%.2f%c%s\n", prefix2provider(Rate.prefix, prov), DEL,
		   Rate.Provider, DEL, currency, DEL, Rate.Charge, DEL, cpm, DEL,
		   P_EMPTY(Rate.Country));
	  free(Rate.Country);
	}
      }
      else {
	/* kludge to suppress "impossible" Rates */
	char **message, *msg;
	if(verbose)
	  message = &msg;
	else
	  message = NULL;
	if (!getRate(&Rate, message) && (Rate.Price != 99.99)) {
	  if (!(Rate.Duration <= takt))
	    continue;
	  sort[n].prefix = Rate.prefix;
	  sort[n].rate = Rate.Charge;
	  sort[n].name = Rate.Provider;
	  switch (explain) {
	  case 1:
	    if (service)
	      sprintf(s, " (%s, %s)", P_EMPTY(Rate.dst[1]), Rate.Zone);
	    else
	      sprintf(s, " (%s)", Rate.Zone);
	    sort[n].explain = strdup(s);
	    break;
	  case 2:
	    sprintf(s, " (%s)", printrate(&Rate));
	    sort[n].explain = strdup(s);
	    break;
	  case 8:
	    sort[n].explain = strdup(P_EMPTY(getComment(i, comment)));
	    break;
	  case 9:		/* used by list */
	    {
	      double  cpm = Rate.Duration > 0 ? 60 * Rate.Price / Rate.Duration : 99.99;

	      if (Rate.Price == 0)
		cpm = Rate.Basic;

	      sprintf(s, "%s%c"
		      "%s%c%s%c%s%c%s%c"
		      "%s%c"
		      "%.3f%c%.4f%c%.4f%c%.2f%c%.3f%c"
		      "%s%c%.2f",
		      prefix2provider_variant(Rate.prefix, prov), DEL,
		      Rate.Provider, DEL, P_EMPTY(Rate.Zone), DEL, P_EMPTY(Rate.Day), DEL, P_EMPTY(Rate.Hour), DEL,
		      currency, DEL,	/* Fixme: global or per
					   Provider?? wg. EURO */
		      Rate.Charge, DEL, Rate.Basic, DEL, Rate.Price, DEL, Rate.Duration, DEL, cpm, DEL,
		      takt_str(&Rate), DEL, Rate.Sales);
	      sort[n].explain = strdup(s);
	    }
	    break;
	  default:
	    sort[n].explain = strdup("");
	    break;
	  }

	  n++;
	}			/* if */
	else if(verbose && *msg)
          print_msg(PRT_V, "%s\n", msg);

      }				/* else 99 */
    }				/* for i */
    if (service)
      num = getServiceNum(0);	/* get next service num */
    else
      num = 0;
  } while (num && *num);
  if (explain < 10) {
    qsort((void *) sort, n, sizeof(SORT), compare_func);
    if (lcr && n) {
      RATE    Cheap;
      int     res = getLeastCost(&Rate, &Cheap, booked, -1);

      if (res != UNKNOWN) {
	sort[n].prefix = Cheap.prefix;
	sort[n].rate = Cheap.Charge;
	sort[n].name = Cheap.Provider;
	sprintf(s, "(Cheapest: %s %s %s %s)", Cheap.dst[0], Cheap.dst[1], Cheap.dst[2], P_EMPTY(Cheap.Zone));
	sort[n].explain = strdup(s);
	n++;
      }				/* res */
    }				/* lcr */
  }				/* explain */
  return (n);
}				/* compute */

static void print_header(void)
{
  printf("Eine %d Sekunden lange Verbindung von %s nach %s kostet am %s\n",
  duration, formatNumber("%F", &srcnum), formatNumber("%F", &destnum),
	 ctime(&start));
}

static void printList(int n)
{
  int     i;

  if (header)
    print_header();
  if (n > best)
    n = best;

  for (i = 0; i < n; i++)
    printf("%s\n", sort[i].explain);
}

static void result(int n)
{

  register int i;

  if (header)
    print_header();

  if (n > best)
    n = best;
  for (i = 0; i < n; i++)
    printf("%s  %s %8.3f  %s\n",
    Provider(sort[i].prefix), currency, sort[i].rate, sort[i].explain);
}				/* result */


static void purge(int n)
{
  register int i;

  for (i = 0; i < n; i++)
    if (sort[i].explain)
      free(sort[i].explain);
}				/* purge */

/*
   Werktag
   Wochenende
   Ortszone
   Regionalzone
   Fernzone
   Handy
   Internet
   0..23 Uhr
 */

#define STARTHOUR 8

static void printTable(char *num)
{
  register int n, d, i, h, lasthour;
  auto struct tm *tm;
  auto SORT last[_MAXLAST];
  auto int used[MAXPROVIDER];
  auto int hours[MAXPROVIDER];
  auto int weight[MAXPROVIDER];
  auto int useds = 0, maxhour;
  auto SORT2 wsort[MAXPROVIDER];
  static int firsttime = 1;
  int     first;
  int     prefix;

  memset(used, 0, sizeof(used));
  memset(hours, 0, sizeof(hours));
  memset(weight, 0, sizeof(weight));

  for (d = 0; d < 3; d++) { /* Werktag, Samstag, Sonntag */
    last[0].prefix = UNKNOWN;
    lasthour = UNKNOWN;

    buildtime();
    tm = localtime(&start);

    if (!d) {			/* first time */
      while (tm->tm_wday != 5) { /* find next Friday */
	start += (60 * 60 * 24);
	tm = localtime(&start);
      }	/* while */
    }
    else			/* erst Samstag, dann */
      start += (60 * 60 * 24);	/* Sonntag */

    splittime();
    buildtime();

    hour = STARTHOUR;
    min = 0;

    first = 1;

    while (1) {
      destnum.nprovider = UNKNOWN;
      if (provider2prefix(num, &prefix))	/* set provider if it is in number */
	normalizeNumber(num, &destnum, TN_PROVIDER);
      n = compute(num);

      if (header && first && d == 0 && firsttime)
	print_header();

      if (header && first) {
        switch (d) {
          case 0 : printf("\nWerktag:\n"); break;
          case 1 : printf("\nSamstag:\n"); break;
          case 2 : printf("\nSonntag:\n"); break;
        } /* switch */

        first = 0;
      } /* if */

      if (last[0].prefix == UNKNOWN) {
	for (i = 0; i < min(n, MAXLAST); i++) {
	  if (sort[i].prefix) {
	    last[i].prefix = sort[i].prefix;
	    last[i].rate = sort[i].rate;
	    last[i].explain = strdup(sort[i].explain);
	  } /* if */
	} /* for */
      } /* if */

      if (lasthour == UNKNOWN)
	lasthour = hour;

      if (sort[0].prefix != last[0].prefix) {
	for (i = 0; i < min(n, MAXLAST); i++) {

	  if (!i)
	    printf("    %02d:00 .. %02d:59 %s = %s %s%s\n",
		   lasthour, hour - 1, Provider(last[i].prefix),
		   currency,
		   double2str(last[i].rate, 5, 3, DEB),
		   last[i].explain);
	  else
	    printf("                   %s = %s %s%s\n",
		   Provider(last[i].prefix),
		   currency,
		   double2str(last[i].rate, 5, 3, DEB),
		   last[i].explain);
	} /* for */

	used[last[0].prefix]++;

	if (lasthour >= hour)
	  h = ((24 - lasthour) + hour);
	else
	  h = hour - lasthour;

	hours[last[0].prefix] += h;

	if ((lasthour > 8) && (lasthour < 21))
	  h *= 2;

	weight[last[0].prefix] += h * (d ? 1 : 5);

	for (i = 0; i < min(n, MAXLAST); i++) {
	  last[i].prefix = sort[i].prefix;
	  last[i].rate = sort[i].rate;
	  last[i].explain = strdup(sort[i].explain);
	} /* for */

	lasthour = hour;
      }	/* if */

      purge(n);

      hour++;

      if (hour == 24)
	hour = 0;
      else if (hour == STARTHOUR)
	break;
    } /* while */

    for (i = 0; i < min(n, MAXLAST); i++) {

      if (!i) {
	if ((lasthour == STARTHOUR) && (hour == STARTHOUR))
	  printf("    immer          %s = %s %s%s\n",
		 Provider(last[i].prefix),
		 currency,
		 double2str(last[i].rate, 5, 3, DEB),
		 last[i].explain);
	else
	  printf("    %02d:00 .. %02d:59 %s = %s %s%s\n",
		 lasthour, hour - 1, Provider(last[i].prefix),
		 currency,
		 double2str(last[i].rate, 5, 3, DEB),
		 last[i].explain);
      }
      else
	printf("                   %s = %s %s%s\n",
	       Provider(last[i].prefix),
	       currency,
	       double2str(last[i].rate, 5, 3, DEB),
	       last[i].explain);
    } /* for */

    used[last[0].prefix]++;

    if (lasthour >= hour)
      h = ((24 - lasthour) + hour);
    else
      h = hour - lasthour;

    hours[last[0].prefix] += h;

    if ((lasthour > 8) && (lasthour < 21))
      h *= 2;

    weight[last[0].prefix] += h * (d ? 1 : 5);

  } /* for */

  if (usestat) {
    printf("\nProvider(s) used:\n");

    maxhour = 9999999;
    useds = 0;

    for (i = 0; i < MAXPROVIDER; i++)
      if (used[i]) {
	printf("%s %d times, %d hours, weight = %d\n",
	       Provider(i), used[i], hours[i], weight[i]);

	wsort[useds].weight = weight[i];
	wsort[useds].index = i;

	useds++;

	if (hours[i] < maxhour)
	  maxhour = hours[i];
      } /* if */

    if ((best < MAXPROVIDER) && (best < useds)) {
      printf("Retrying with only %d provider(s), eliminating %d provider(s)\n", best, useds - best);

      qsort((void *) wsort, useds, sizeof(SORT2), compare2);

      for (i = 0; i < useds; i++) {
	printf("%s %d times, %d hours, weight = %d\n",
	       Provider(wsort[i].index), used[wsort[i].index], hours[wsort[i].index], weight[wsort[i].index]);

	if (i == best - 1)
	  printf("\n");

	if (i >= best - 1)
	  ignore[wsort[i].index]++;
      }	/* for */

      if (firsttime)
	printTable(num);

      firsttime = 0;

    } /* if */
  } /* if */
} /* printTable */


static void clean_up()
{
  if (providers)
    free(providers);
  providers = 0;
  fromarea = 0;
  table = list = header = explain = 0;
  usestat = 0;
  duration = LCR_DURATION;
  wanted_day = '\0';
  takt = 99999;
  if (comment)
    free(comment);
  comment = 0;
  sortby = 0;
  need_dest = 1;
}

static char * sub_sp(char *p)
{
  char *o = p;
  int allupper=1;
  for (; *p; p++)
    if(!isupper(*p) && *p != '_' && !isdigit(*p)) { /* e.g. _DEMD1 */
      allupper = 0;
      break;
  }
  if (!allupper)
    for (p = o; *p; p++)
      if(*p == '_')
        *p = ' ';
  return o;
}
static void doit(int i, int argc, char *argv[])
{
  int     n;
  int     prefix;
  char   *num;

  post_init();
  memset(ignore, 0, sizeof(ignore));
  if (!need_dest && i == 0) {
    i = 0;
    argc = 1;
    argv[0] = "2345";
  }
  while (i < argc) {
    num = sub_sp(argv[i]);
    if (explain == 55) {
      if (n_providers) {
	destnum.nprovider = pnum2prefix_variant(providers[0], 0);
	Strncpy(destnum.provider, getProvider(destnum.nprovider), TN_MAX_PROVIDER_LEN);
	normalizeNumber(num, &destnum, TN_NO_PROVIDER);
      }
      else
	normalizeNumber(num, &destnum, TN_ALL);
      printf("%s => %s \n", num, formatNumber("%l - %p", &destnum));
      i++;
      continue;
    }
    else if (explain == 52) {	// enum Servce names

      int     first = 1;
      char   *p;

      do {
	p = getServiceNames(first);
	if (p)
	  printf("%s\n", p);
	first = 0;
      }
      while (p);
      return;
    }
    destnum.nprovider = UNKNOWN;
    if (provider2prefix(num, &prefix))	/* set provider if it is in
					   number */
      normalizeNumber(num, &destnum, TN_PROVIDER);
    if (table)
      printTable(num);
    else {
      n = compute(num);
      if (list)
	printList(n);
      else if (explain < 10)
	result(n);
      purge(n);
    }
    i++;
  }				/* while */
  clean_up();
}

static void err(char *s)
{
  print_msg(PRT_A, "%s - '%s'\n", s, strerror(errno));
  if (is_daemon == 2)
    kill(getppid(), SIGTERM);
  exit(2);
}

static int handle_client(int fd)
{
  char    buffer[BUFSIZ];
  int     argc, n, i;
  char  **argv;
  char   *p;

  if ((n = read(fd, buffer, BUFSIZ)) < 0)
    err("Read");
  if (n) {
    argv = calloc(sizeof(char *), 20);

    buffer[n] = '\0';
    if (verbose)
      fprintf(stderr, "got '%s'\n", buffer);
    argc = 0;
    argv[argc++] = strdup(myname);
    p = strtok(buffer, "\t\n ");
    while (p) {
      argv[argc++] = strdup(p);
      p = strtok(0, "\t\n ");
      if (argc >= 20)
	break;
    }
    time(&start);		/* set time of call */
    splittime();		/* date time my be overridden by opts */
    we_are_daemon = 1;
    if ((i = opts(argc, argv)) || need_dest == 0) {
      if (shutdown(fd, 0) < 0)	/* no read any more */
	err("shutdown");
      if (dup2(fd, STDOUT_FILENO) < 0)	/* stdout to sock */
	err("dup");
      doit(i, argc, argv);
      fflush(stdout);
      fclose(stdout);
    }
    for (i = 0; i < argc; i++)
      free(argv[i]);
    free(argv);
  }
  return n == 0 ? -1 : 0;
}

void    catch_sig(int sig)
{
  print_msg(PRT_A, "Signal %d\n", sig);
  unlink(socket_file);
  if (pid_dir)
    unlink(pid_file);
  err("Sig");
}

static void del_sock(void)
{
  if (getppid() > 0) {
    unlink(socket_file);
    if (pid_dir)
      unlink(pid_file);
  }
}

static volatile sig_atomic_t stopped = 0;
static volatile sig_atomic_t reinit = 0;

static void catch_term(int sig)
{
  stopped = 1;
}

static void catch_hup(int sig)
{
  print_msg(PRT_A, "Signal %d restarting %s\n", sig, myname);
  del_sock();
  execvp(myname, hup_argv);
  print_msg(PRT_A, "- failed\n");
}

static void do_reinit(void)
{
  /* deinit(), init() doesn't */
  reinit=0;
}

/* thank's to Jochen Erwied for this: */
#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(expression) \
 (__extension__ \
 ({ long int __result; \
 do __result = (long int) (expression); \
 while (__result == -1L && errno == EINTR); \
 __result; })) \

#endif

static void setup_daemon()
{
  int     sock;
  struct sockaddr_un sa;
  struct sockaddr_in client;
  fd_set  active_fd_set, read_fd_set;
  char   *sock_name = socket_file;
  socklen_t  size;
  struct stat stat_buf;
  int     i;
  pid_t   pid;
  char    pidname[] = "isdnrate.pid";
  FILE   *fp;

  if (verbose)
    fprintf(stderr, "Setup sockets\n");
  signal(SIGTERM, catch_term);
  signal(SIGINT, catch_sig);
  signal(SIGHUP, catch_hup);

  if (is_daemon == 2) {		/* go background */

    fprintf(stderr, "Going background\n");
    verbose = 0;
    pid = fork();
    if (pid < 0)
      err("Going bg failed");
    else if (pid > 0) {
      exit(EXIT_SUCCESS);
    }
  }
  if ((sock = socket(PF_UNIX, SOCK_STREAM, 0)) < 0)
    err("Can't open socket");
  sa.sun_family = AF_UNIX;
  strcpy(sa.sun_path, sock_name);
  size = offsetof(struct sockaddr_un, sun_path) + strlen(sa.sun_path) + 1;

  if (bind(sock, (struct sockaddr *) &sa, size) < 0)
    err("Can't bind sock");

  stat(sock_name, &stat_buf);
  chmod(sock_name, stat_buf.st_mode | S_IWOTH | S_IWGRP);
/* should better have perm wwwrun.nogroup */

  if (listen(sock, SOMAXCONN) < 0)
    err("Can't listen");
  if (pid_dir) {
    pid_file = malloc(strlen(pid_dir) + strlen(pidname) + 2);
    strcpy(pid_file, pid_dir);
    if (pid_file[strlen(pid_file) - 1] != '/')
      strcat(pid_file, "/");
    strcat(pid_file, pidname);
    if ((fp = fopen(pid_file, "w")) == 0)
      fprintf(stderr, "Can't write %s\n", pid_file);
    else {
      fprintf(fp, "%d\n", getpid());
      fclose(fp);
    }
  }
  atexit(del_sock);
  FD_ZERO(&active_fd_set);
  FD_SET(sock, &active_fd_set);
  while (!stopped) {
    if (reinit)
      do_reinit();
    read_fd_set = active_fd_set;
    if (TEMP_FAILURE_RETRY(select(FD_SETSIZE, &read_fd_set, 0, 0, 0)) < 0)
      err("select");
    for (i = 0; i < FD_SETSIZE; i++)
      if (FD_ISSET(i, &read_fd_set)) {
	if (i == sock) {	/* request on orig */
	  int     new;

	  size = sizeof(client);
	  if ((new = accept(sock, (struct sockaddr *) &client, &size)) < 0)
	    err("accept");
	  if (verbose)
	    fprintf(stderr, "Accepted %d\n", new);
	  FD_SET(new, &active_fd_set);
	}
	else {			/* already connected */
	  pid_t   pid;
	  int     status;

	  if (verbose)
	    fprintf(stderr, "Handle client %d\n", i);
	  pid = fork();
	  if (pid == 0) {
	    handle_client(i);
	    _exit(EXIT_SUCCESS);
	  }
	  else if (pid < 0)
	    err("fork");
	  else {
	    if (waitpid(pid, &status, 0) != pid)
	      err("waitpid");
	    close(i);
	    FD_CLR(i, &active_fd_set);
	  }
	}			/* if i */
      }				/* if ISSET */
  }				/* while */
  del_sock();
  exit(EXIT_SUCCESS);
}

static int connect_2_daemon(int argc, char *argv[])
{
  int     sock;
  struct sockaddr_un sa;
  char   *sock_name = socket_file;
  size_t  size;
  int     i, c, len;
  char   *p, *q, buffer[BUFSIZ];

  if ((sock = socket(PF_UNIX, SOCK_STREAM, 0)) < 0)
    err("Can't open socket");
  sa.sun_family = AF_UNIX;
  strcpy(sa.sun_path, sock_name);
  size = offsetof(struct sockaddr_un, sun_path) + strlen(sa.sun_path) + 1;

  if (connect(sock, (struct sockaddr *) &sa, size) < 0)
    err("Can't connect sock");
  p = buffer;
  optind = 0;			/* reinit */
  len = 0;
  while ((c = getopt(argc, argv, options)) != EOF && len < BUFSIZ - 10) {
    switch (c) {
    case '?':
    case 'C':
      break;
    case 'D':
      if (optarg && atoi(optarg) == 3) ;	/* goon, kill a running
						   daemon */
      else
	break;
    default:
      *p++ = '-';
      *p++ = c;
      len += 3;
      if (optarg)
	for (q = optarg; *q && len < BUFSIZ - 10; len++)
	  *p++ = *q++;
      *p++ = ' ';
      break;
    }
  }
  for (i = optind; i < argc; i++) {
    for (q = argv[i]; *q && len < BUFSIZ - 10; len++)
      *p++ = *q++;
    *p++ = ' ';
    len++;
  }
  *--p = '\x0';
  i = write(sock, buffer, strlen(buffer) + 1);
  if (i < 0)
    err("Write sock");
  while ((i = read(sock, buffer, BUFSIZ)) > 0)
    write(1, buffer, i);
  close(sock);
  return EXIT_SUCCESS;
}


int     main(int argc, char *argv[], char *envp[])
{
  register int i;
  sigset_t        unblock_set;

  myname = argv[0];
  myshortname = basename(myname);

  time(&start);
  splittime();
  socket_file = strdup(SOCKNAME);
  /* borrowed from isdnlog.c, thanks */
  hup_argv = argv;
  sigemptyset(&unblock_set);
  sigaddset(&unblock_set, SIGHUP);
  sigprocmask(SIG_UNBLOCK, &unblock_set, NULL);

  if ((i = opts(argc, argv)) || need_dest == 0) {
    if (is_client)
      exit(connect_2_daemon(argc, argv));
    else
      init();
    if (is_daemon) {
      clean_up();
      setup_daemon();
    }
    else
      doit(i, argc, argv);
  }
  else {
    print_msg(PRT_A, usage, myshortname, myshortname, options);
    print_msg(PRT_A, "\n");
    print_msg(PRT_A, "\t-a \tall=show old and newer rates (default actual only)\n", MAXPROVIDER);
    print_msg(PRT_A, "\t-b best\tshow only the first <best> provider(s) (default %d)\n", MAXPROVIDER);
    print_msg(PRT_A, "\t-d d[.m[.y]] | {W|N|E}\tstart date of call (default now)\n");
    print_msg(PRT_A, "\t-f areacode\tyou are calling from <areacode>\n");
    print_msg(PRT_A, "\t-h h[:m[:s]]\tstart time of call (default now)\n");
    print_msg(PRT_A, "\t-l duration\tduration of call in seconds (default %d seconds)\n", LCR_DURATION);
    print_msg(PRT_A, "\t-o \t show only booked providers\n");
    print_msg(PRT_A, "\t-p prov|B[,prov...]\t show only these providers\n");
    print_msg(PRT_A, "\t-s \t consider 'Destination' as a service name\n");
    print_msg(PRT_A, "\t-t takt\t\tshow providers if chargeduration<=takt\n");
    print_msg(PRT_A, "\t-v [level]\tverbose\n");
    print_msg(PRT_A, "\t-x prov|B[,prov...]\t exclude these providers\n");

    print_msg(PRT_A, "\tOutput and run options\n");
    print_msg(PRT_A, "\t-C\trun as client, connecting to a running daemon\n");
    print_msg(PRT_A, "\t-CD3\tkill a running daemon\n");
    print_msg(PRT_A, "\t-D\trun as daemon\n");
    print_msg(PRT_A, "\t-D2\trun as daemon & go background\n");
    print_msg(PRT_A, "\t-G which\tshow raw data\n");
    print_msg(PRT_A, "\t-H\tshow a header\n");
    print_msg(PRT_A, "\t-L\tshow a detailed list\n");
    print_msg(PRT_A, "\t-O sOcket-path\twrite socket to this file (def:tmp/isdnrate)\n");
    print_msg(PRT_A, "\t-P pid-dir\twrite own PID to pid-dir/isdnrate.pid\n");
    print_msg(PRT_A, "\t-N\tparse the given telefon numbers\n");
    print_msg(PRT_A, "\t-S[v|n]\tsort by v=VBN, n=Name, default=Charge\n");
    print_msg(PRT_A, "\t-T\tshow a table of day/night week/weekend\n");
    print_msg(PRT_A, "\t-U\tshow usage stats for table\n");
    print_msg(PRT_A, "\t-V\tshow version info\n");
    print_msg(PRT_A, "\t-X\texplain each rate\n");
    print_msg(PRT_A, "\t-X2\texplain more\n");
    print_msg(PRT_A, "\t-X comment\tprint <comment> from C:tag\n");
    print_msg(PRT_A, "\t-Z show entry from getLeastCost\n");
    print_msg(PRT_A, "\n\te.g.\t%s -b5 -f30 -TH -t1 Zaire\n", myshortname);
  }				/* else */
/*  deinit(); Fixme: this SIGSEGVs in exitHoliday */
  return (0);
}				/* isdnrate */
