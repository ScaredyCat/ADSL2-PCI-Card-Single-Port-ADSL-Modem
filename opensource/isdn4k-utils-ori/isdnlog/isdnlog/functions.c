/* $Id: functions.c,v 1.33 2002/01/26 20:43:31 akool Exp $
 *
 * ISDN accounting for isdn4linux. (log-module)
 *
 * Copyright 1995 .. 2000 by Andreas Kool (akool@isdn4linux.de)
 *                     and Stefan Luethje (luethje@sl-gw.lake.de)
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
 *
 * $Log: functions.c,v $
 * Revision 1.33  2002/01/26 20:43:31  akool
 * isdnlog-4.56:
 *  - dont set the Provider-field of the MySQL DB to "?*? ???" on incoming calls
 *
 *  - implemented
 *      0190029 Telebillig        (17,5 Cent/minute to any cellphone)
 * 		 0190031 Teledump
 * 		 0190035 TeleDiscount
 * 		 0190037 Fonfux            (1,5 Cent/minute german-call)
 * 		 0190087 Phonecraft
 *
 *    you have to change:
 *
 *    1. "/etc/isdn/rate.conf" - add the following:
 *
 *      P:229=0		#E Telebillig
 * 		 P:231=0		#E Teledump
 * 		 P:235=0		#E TeleDiscount
 * 		 P:237=0		#E Fonfux
 * 		 P:287=0		#E Phonecraft
 *
 *    2. "/etc/isdn/isdn.conf" (or "/etc/isdn/callerid.conf"):
 *
 * 	     VBN = 010
 *
 * 	   to
 *
 * 	     VBN = 010:01900
 *
 * Revision 1.32  2001/08/18 12:04:08  paul
 * Don't attempt to write to stderr if we're a daemon.
 *
 * Revision 1.31  2000/12/15 14:36:05  leo
 * modilp, ilp - B-chan usage in /proc/isdnlog
 * s. isdnlog/ilp/README for more information
 *
 * Revision 1.30  2000/06/20 17:09:59  akool
 * isdnlog-4.29
 *  - better ASN.1 display
 *  - many new rates
 *  - new Option "isdnlog -Q" dump's "/etc/isdn/isdn.conf" into a SQL database
 *
 * Revision 1.29  1999/12/31 13:30:01  akool
 * isdnlog-4.00 (Millenium-Edition)
 *  - Oracle support added by Jan Bolt (Jan.Bolt@t-online.de)
 *
 * Revision 1.28  1999/11/25 22:58:39  akool
 * isdnlog-3.68
 *  - new utility "isdnbill" added
 *  - patch from Jochen Erwied (j.erwied@gmx.de)
 *  - new rates
 *  - small fixes
 *
 * Revision 1.27  1999/11/16 18:09:39  akool
 * isdnlog-3.67
 *   isdnlog-3.66 writes wrong provider number into it's logfile isdn.log
 *   there is a patch and a repair program available at
 *   http://www.toetsch.at/linux/i4l/i4l-3_66.htm
 *
 * Revision 1.26  1999/11/12 20:50:49  akool
 * isdnlog-3.66
 *   - Patch from Jochen Erwied <mack@joker.e.ruhr.de>
 *       makes the "-O" and "-C" options usable at the same time
 *
 *   - Workaround from Karsten Keil <kkeil@suse.de>
 *       segfault in ASN.1 parser
 *
 *   - isdnlog/tools/rate.c ... ignores "empty" providers
 *   - isdnlog/tools/telnum.h ... fixed TN_MAX_PROVIDER_LEN
 *
 * Revision 1.25  1999/09/13 09:09:43  akool
 * isdnlog-3.51
 *   - changed getProvider() to not return NULL on unknown providers
 *     many thanks to Matthias Eder <mateder@netway.at>
 *   - corrected zone-processing when doing a internal -> world call
 *
 * Revision 1.24  1999/06/15 20:04:01  akool
 * isdnlog Version 3.33
 *   - big step in using the new zone files
 *   - *This*is*not*a*production*ready*isdnlog*!!
 *   - Maybe the last release before the I4L meeting in Nuernberg
 *
 * Revision 1.23  1999/06/03 18:50:27  akool
 * isdnlog Version 3.30
 *  - rate-de.dat V:1.02-Germany [03-Jun-1999 19:49:22]
 *  - small fixes
 *
 * Revision 1.22  1999/04/19 19:24:35  akool
 * isdnlog Version 3.18
 *
 * - countries-at.dat added
 * - spelling corrections in "countries-de.dat" and "countries-us.dat"
 * - LCR-function of isdnconf now accepts a duration (isdnconf -c .,duration)
 * - "rate-at.dat" and "rate-de.dat" extended/fixed
 * - holiday.c and rate.c fixed (many thanks to reinelt@eunet.at)
 *
 * Revision 1.21  1999/04/10 16:35:22  akool
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
 * Revision 1.20  1999/03/25 19:39:48  akool
 * - isdnlog Version 3.11
 * - make isdnlog compile with egcs 1.1.7 (Bug report from Christophe Zwecker <doc@zwecker.com>)
 *
 * Revision 1.19  1999/03/07 18:18:48  akool
 * - new 01805 tarif of DTAG
 * - new March 1999 tarife
 * - added new provider "01051 Telecom"
 * - fixed a buffer overrun from Michael Weber <Michael.Weber@Post.RWTH-Aachen.DE>
 * - fixed a bug using "sondernnummern.c"
 * - fixed chargeint change over the time
 * - "make install" now install's "sonderrufnummern.dat", "tarif.dat",
 *   "vorwahl.dat" and "tarif.conf"! Many thanks to
 *   Mario Joussen <mario.joussen@post.rwth-aachen.de>
 * - Euracom Frames would now be ignored
 * - fixed warnings in "sondernnummern.c"
 * - "10plus" messages no longer send to syslog
 *
 * Revision 1.18  1999/01/24 19:01:27  akool
 *  - second version of the new chargeint database
 *  - isdnrep reanimated
 *
 * Revision 1.17  1999/01/10 15:23:07  akool
 *  - "message = 0" bug fixed (many thanks to
 *    Sebastian Kanthak <sebastian.kanthak@muehlheim.de>)
 *  - CITYWEEKEND via config-file possible
 *  - fixes from Michael Reinelt <reinelt@eunet.at>
 *  - fix a typo in the README from Sascha Ziemann <szi@aibon.ping.de>
 *  - Charge for .at optimized by Michael Reinelt <reinelt@eunet.at>
 *  - first alpha-Version of the new chargeinfo-Database
 *    ATTENTION: This version requires the following manual steps:
 *      cp /usr/src/isdn4k-utils/isdnlog/tarif.dat /usr/lib/isdn
 *      cp /usr/src/isdn4k-utils/isdnlog/samples/tarif.conf /etc/isdn
 *
 * Revision 1.16  1998/12/09 20:39:24  akool
 *  - new option "-0x:y" for leading zero stripping on internal S0-Bus
 *  - new option "-o" to suppress causes of other ISDN-Equipment
 *  - more support for the internal S0-bus
 *  - Patches from Jochen Erwied <mack@Joker.E.Ruhr.DE>, fixes TelDaFax Tarif
 *  - workaround from Sebastian Kanthak <sebastian.kanthak@muehlheim.de>
 *  - new CHARGEINT chapter in the README from
 *    "Georg v.Zezschwitz" <gvz@popocate.hamburg.pop.de>
 *
 * Revision 1.15  1998/11/24 20:51:26  akool
 *  - changed my email-adress
 *  - new Option "-R" to supply the preselected provider (-R24 -> Telepassport)
 *  - made Provider-Prefix 6 digits long
 *  - full support for internal S0-bus implemented (-A, -i Options)
 *  - isdnlog now ignores unknown frames
 *  - added 36 allocated, but up to now unused "Auskunft" Numbers
 *  - added _all_ 122 Providers
 *  - Patch from Jochen Erwied <mack@Joker.E.Ruhr.DE> for Quante-TK-Anlagen
 *    (first dialed digit comes with SETUP-Frame)
 *
 * Revision 1.14  1998/09/09 12:49:31  paul
 * fixed crash when using mysql (call to Provider() was omitted)
 *
 * Revision 1.13  1998/06/21 11:52:43  akool
 * First step to let isdnlog generate his own AOCD messages
 *
 * Revision 1.12  1998/06/14 15:33:48  akool
 * AVM B1 support (Layer 3)
 * Telekom's new currency DEM 0,121 supported
 * Disable holiday rates #ifdef ISDN_NL
 * memory leak in "isdnrep" repaired
 *
 * Revision 1.11  1998/06/07 21:08:26  akool
 * - Accounting for the following new providers implemented:
 *     o.tel.o, Tele2, EWE TEL, Debitel, Mobilcom, Isis, NetCologne,
 *     TelePassport, Citykom Muenster, TelDaFax, Telekom, Hutchison Telekom,
 *     tesion)), HanseNet, KomTel, ACC, Talkline, Esprit, Interoute, Arcor,
 *     WESTCom, WorldCom, Viag Interkom
 *
 *     Code shamelessly stolen from G.Glendown's (garry@insider.regio.net)
 *     program http://www.insider.org/tarif/gebuehr.c
 *
 * - Telekom's 10plus implemented
 *
 * - Berechnung der Gebuehrenzone implementiert
 *   (CityCall, RegioCall, GermanCall, GlobalCall)
 *   The entry "ZONE" is not needed anymore in the config-files
 *
 *   you need the file
 *     http://swt.wi-inf.uni-essen.de/~omatthes/tgeb/vorwahl2.exe
 *   and the new entry
 *     [GLOBAL]
 *       AREADIFF = /usr/lib/isdn/vorwahl.dat
 *   for that feature.
 *
 *   Many thanks to Olaf Matthes (olaf.matthes@uni-essen.de) for the
 *   Data-File and Harald Milz for his first Perl-Implementation!
 *
 * - Accounting for all "Sonderrufnummern" (0010 .. 11834) implemented
 *
 *   You must install the file
 *     "isdn4k-utils/isdnlog/sonderrufnummern.dat.bz2"
 *   as "/usr/lib/isdn/sonderrufnummern.dat"
 *   for that feature.
 *
 * ATTENTION: This is *NO* production-code! Please test it carefully!
 *
 * Revision 1.10  1998/03/29 23:18:03  luethje
 * mySQL-Patch of Sascha Matzke
 *
 * Revision 1.9  1998/03/08 11:42:48  luethje
 * I4L-Meeting Wuerzburg final Edition, golden code - Service Pack number One
 *
 * Revision 1.8  1997/05/29 17:07:19  akool
 * 1TR6 fix
 * suppress some noisy messages (Bearer, Channel, Progress) - can be reenabled with log-level 0x1000
 * fix from Bodo Bellut (bodo@garfield.ping.de)
 * fix from Ingo Schneider (schneidi@informatik.tu-muenchen.de)
 * limited support for Info-Element 0x76 (Redirection number)
 *
 * Revision 1.7  1997/05/09 23:30:45  luethje
 * isdnlog: new switch -O
 * isdnrep: new format %S
 * bugfix in handle_runfiles()
 *
 * Revision 1.6  1997/04/08 00:02:12  luethje
 * Bugfix: isdnlog is running again ;-)
 * isdnlog creates now a file like /var/lock/LCK..isdnctrl0
 * README completed
 * Added some values (countrycode, areacode, lock dir and lock file) to
 * the global menu
 *
 */

#define _FUNCTIONS_C_

#include "isdnlog.h"
#ifdef POSTGRES
#include "postgres.h"
#endif
#ifdef MYSQLDB
#include "mysqldb.h"
#endif
#ifdef ORACLE
#include "oracle.h"
#endif

/*****************************************************************************/

static void saveCharge()
{
  register int   i;
  auto 	   FILE *f;
  auto 	   char  fn[BUFSIZ], fno[BUFSIZ];


  sprintf(fn, "%s/%s", confdir(), CHARGEFILE);
  sprintf(fno, "%s.old", fn);
  (void)rename(fn, fno);

  if ((f = fopen(fn, "w")) != (FILE *)NULL) {
    for (i = 0; i < knowns; i++)
      if (known[i]->day > -1)
        fprintf(f, "%s %d %g %g %d %g %g %g %g\n", known[i]->who,
          known[i]->day, known[i]->charge, known[i]->scharge,
          known[i]->month, known[i]->online, known[i]->sonline,
          known[i]->bytes, known[i]->sbytes);

    fclose(f);
  } /* if */
} /* saveCharge */

/*****************************************************************************/
extern void procinfo(int channel, CALL * cp, int state);

void _Exit_isdnlog(char *File, int Line, int RetCode) /* WARNING: RetCode==-9 does _not_ call exit()! */
{
#ifdef Q931
  if (!q931dmp)
#endif

#ifdef DEBUG
    print_msg(PRT_NORMAL, "exit now %d in module `%s' at line %d!\n", RetCode, File, Line);
#else
    print_msg(PRT_NORMAL, "exit now %d\n", RetCode);
#endif

  if (socket_size(sockets) >= 2) {
    close(sockets[ISDNCTRL].descriptor);
    close(sockets[ISDNINFO].descriptor);

    if (xinfo && sockets[IN_PORT].descriptor != -2)
      close(sockets[IN_PORT].descriptor);
  } /* if */

  procinfo(1, NULL, -1);	/* close proc */
  closelog();

	if (fout)
		fclose(fout);

#ifdef POSTGRES
  dbClose();
#endif
#ifdef MYSQLDB
  mysql_dbClose();
#endif
#ifdef ORACLE
  oracle_dbClose();
#endif

  if (!replay) {
    saveCharge();
    handle_runfiles(NULL,NULL,STOP_PROG);
  } /* if */

  if (RetCode != -9)
    exit(RetCode);
} /* Exit */

/*****************************************************************************/

int Change_Channel(int old_channel, int new_channel)
{
  change_channel(old_channel,new_channel);
  Change_Channel_Ring(old_channel,new_channel);

  return 0;
}

/*****************************************************************************/

void now(void)
{
  auto struct tms tms;


  time(&cur_time);
  tto = tt;
  tt = times(&tms);
  set_time_str();
} /* now */

/*****************************************************************************/

void set_time_str(void)
{
  auto struct tm *tm_time = localtime(&cur_time);


  tm_time->tm_isdst = 0;

  strftime(stl, 64, "%a %b %d %X %Y", tm_time);
  strftime(st, 64, "%a %b %d %X", tm_time);
  strftime(idate, 256, "%a%b%d%T", tm_time);

  fullhour = (!tm_time->tm_min && (tm_time->tm_sec <= (wakeup + 2)));

  day = tm_time->tm_mday;
  month = tm_time->tm_mon + 1;
  hour = tm_time->tm_hour;
} /* set_time_str */

/*****************************************************************************/

void logger(int chan)
{
  auto     int    tries, fd;
  register char  *p;
  auto 	   char   s[BUFSIZ];
#ifdef POSTGRES
  auto     DbStrIn db_set;
#endif
#ifdef MYSQLDB
  auto     mysql_DbStrIn mysql_db_set;
#endif
#ifdef ORACLE
  auto     oracle_DbStrIn oracle_db_set;
#endif


  strcpy(s, ctime(&call[chan].connect));

  if ((p = strchr(s, '\n')))
    *p = 0;


  /*
     .aoce :: -1 -> keine aOC Meldung von VSt, muss berechnet werden
     	       0 -> free of Charge (0130/...)
              >0 -> #of Units
  */

	tries = 0;

	if (access(logfile,W_OK) && errno == ENOENT)
	{
		if ((flog = fopen(logfile, "w")) == NULL)
		{
			tries = -1;
			print_msg(PRT_ERR,"Can not write file `%s' (%s)!\n", logfile, strerror(errno));
		}
		else
			fclose(flog);
	}

	if (tries != -1)
	{
		while (((fd = open(logfile, O_WRONLY | O_APPEND | O_EXCL)) == -1) && (tries < 1000))
			tries++;

		if ((tries >= 1000) || ((flog = fdopen(fd, "a")) == (FILE *)NULL))
			print_msg(PRT_ERR, "Can not open file `%s': %s!\n", logfile, strerror(errno));
		else
		{
                        /* Tarif leider nicht bekannt. Daher besser auf
                           "kostenlos", als auf "DM 1,00 geschenkt" stellen!
                        */

                        if (call[chan].pay == -1.00) {
                          if (call[chan].aocpay > 0.0) /* besser als nix ... */
                            call[chan].pay = call[chan].aocpay;
                          else
                            call[chan].pay = 0.0;
                        } /* if */

			fprintf(flog, "%s|%-16s|%-16s|%5d|%10d|%10d|%5d|%c|%3d|%10ld|%10ld|%s|%d|%d|%g|%s|%g|%3d|%3d|\n",
			              s + 4, call[chan].num[CALLING], call[chan].num[CALLED],
			              (int)(call[chan].disconnect - call[chan].connect),
			              (int)call[chan].duration, (int)call[chan].connect,
			              call[chan].aoce, call[chan].dialin ? 'I' : 'O',
			              call[chan].cause, call[chan].ibytes, call[chan].obytes,
			              LOG_VERSION, call[chan].si1, call[chan].si11,
			              currency_factor, currency, call[chan].pay,
				      			prefix2pnum(call[chan].provider),
			              call[chan].zone);

			fclose(flog);
		}
	}


#ifdef POSTGRES
  db_set.connect = call[chan].connect;
  strcpy(db_set.calling, call[chan].num[CALLING]);
  strcpy(db_set.called, call[chan].num[CALLED]);
  db_set.duration = (int)(call[chan].disconnect - call[chan].connect);
  db_set.hduration = (int)call[chan].duration;
  db_set.aoce = call[chan].aoce;
  db_set.dialin = call[chan].dialin ? 'I' : 'O';
  db_set.cause = call[chan].cause;
  db_set.ibytes = call[chan].ibytes;
  db_set.obytes = call[chan].obytes;
  db_set.version = atoi(LOG_VERSION);
  db_set.si1 = call[chan].si1;
  db_set.si11 = call[chan].si11;
  db_set.currency_factor = currency_factor;
  strcpy(db_set.currency, currency);
  db_set.pay = call[chan].pay;
  dbAdd(&db_set);
#endif
#ifdef MYSQLDB
  mysql_db_set.connect = call[chan].connect;
  strcpy(mysql_db_set.calling, call[chan].num[CALLING]);
  strcpy(mysql_db_set.called, call[chan].num[CALLED]);
  mysql_db_set.duration = (int)(call[chan].disconnect - call[chan].connect);
  mysql_db_set.hduration = (int)call[chan].duration;
  mysql_db_set.aoce = call[chan].aoce;
  mysql_db_set.dialin = call[chan].dialin ? 'I' : 'O';
  mysql_db_set.cause = call[chan].cause;
  mysql_db_set.ibytes = call[chan].ibytes;
  mysql_db_set.obytes = call[chan].obytes;
  mysql_db_set.version = atoi(LOG_VERSION);
  mysql_db_set.si1 = call[chan].si1;
  mysql_db_set.si11 = call[chan].si11;
  mysql_db_set.currency_factor = currency_factor;
  strcpy(mysql_db_set.currency, currency);
  mysql_db_set.pay = call[chan].pay;
  strcpy(mysql_db_set.provider, call[chan].dialin ? "" : getProvider(call[chan].provider));
  mysql_dbAdd(&mysql_db_set);
#endif
#ifdef ORACLE
  oracle_db_set.connect = call[chan].connect;
  strcpy(oracle_db_set.calling, call[chan].num[CALLING]);
  strcpy(oracle_db_set.called, call[chan].num[CALLED]);
  oracle_db_set.duration = (int)(call[chan].disconnect - call[chan].connect);
  oracle_db_set.hduration = (int)call[chan].duration;
  oracle_db_set.aoce = call[chan].aoce;
  strcpy(oracle_db_set.dialin, call[chan].dialin ? "I" : "O");
  oracle_db_set.cause = call[chan].cause;
  oracle_db_set.ibytes = call[chan].ibytes;
  oracle_db_set.obytes = call[chan].obytes;
  strncpy(oracle_db_set.version, LOG_VERSION, sizeof(oracle_db_set.version));
  oracle_db_set.version[sizeof(oracle_db_set.version)-1] = '\0';
  oracle_db_set.si1 = call[chan].si1;
  oracle_db_set.si11 = call[chan].si11;
  oracle_db_set.currency_factor = currency_factor;
  strncpy(oracle_db_set.currency, currency, sizeof(oracle_db_set.currency));
  oracle_db_set.currency[sizeof(oracle_db_set.currency)-1] = '\0';
  oracle_db_set.pay = call[chan].pay;
  oracle_db_set.provider = prefix2pnum(call[chan].provider);
  strncpy(oracle_db_set.provider_name, getProvider(call[chan].provider),
  	sizeof(oracle_db_set.provider_name));
  oracle_db_set.provider_name[sizeof(oracle_db_set.provider_name)-1] = '\0';
  oracle_db_set.zone = call[chan].zone;
  oracle_dbAdd(&oracle_db_set);
#endif
} /* logger */



/*****************************************************************************/

int print_msg(int Level, const char *fmt, ...)
{
  /* ACHTUNG IN DIESER FKT DARF KEIN print_msg() AUFGERUFEN WERDEN !!!!! */
  auto int     SLevel = PRT_NOTHING, l;
  auto char    String[LONG_STRING_SIZE], s[LONG_STRING_SIZE];
  auto va_list ap;


  va_start(ap, fmt);
  l = vsnprintf(String, LONG_STRING_SIZE, fmt, ap);
  va_end(ap);

  if (width) {
    memcpy(s, String, l + 1);

    if (l > width) {
      if (s[l - 1] < ' ') {
        s[width] = s[l - 1];
        s[width + 1] = 0;
      }
      else
        s[width] = 0;
    } /* if */
  } /* if */

  SLevel = IS_DEBUG(Level) ? LOG_DEBUG : LOG_INFO;

  if (Level & syslogmessage)
    syslog(SLevel, "%s", String);

  if (Level & stdoutput) {
    (void)fputs(width ? s : String, stdout);
    fflush(stdout);
  } /* if */

  if (Level & message)
  {
    /* no console, no outfile -> log to stderr */
    /* if a daemon, don't use stderr either! */
    if (!fout && !fcons && !isdaemon) {
      fputs(width ? s : String, stderr);
      fflush(stderr);
    }

    /* log to console */
    if (fcons != NULL) {
      fputs(width ? s : String, fcons);
      fflush(fcons);
    } /* else */

    /* log to file */
    if (fout != NULL)
    {
      fputs(width ? s : String, fout);
      fflush(fout);
    }
  }

  if (Level & xinfo)
    print_from_server(String);

  if (verbose && (Level & PRT_LOG)) {
    fprintf(fprot, "%s  %s", stl, String);

    if (synclog)
      fflush(fprot);
  } /* if */

  return(0);
} /* print_msg */

/*****************************************************************************/

void info(int chan, int reason, int state, char *msg)
{
  register int  i;
  auto   char   s[BUFSIZ], *left = "", *right = "\n";
  static int    lstate = 0, lchan = -1;


#if 0
  if (!newline) {

    if (state == STATE_BYTE) {
      right = "";

      if (lstate == STATE_BYTE)
        left = "\r";
      else
        left = "";
    }
    else {
      right = "\n";

      if (lstate == STATE_BYTE)
        left = "\n";
      else
        left = "";
    } /* else */

    if ((lchan != chan) && (lstate == STATE_BYTE))
      left = "\r\n";

    lstate = state;
    lchan = chan;
  } /* if */
#else
  if (!newline) {
    left = "";
    right = "\r";

    if ((lchan != chan) || (lstate != state) || (state != STATE_BYTE))
      left = "\n";

    lstate = state;
    lchan = chan;
  }
  else {
    left = "";
    right = "\n";
  } /* else */
#endif

  if (allflags & PRT_DEBUG_GENERAL)
    if (allflags & PRT_DEBUG_INFO)
      print_msg(PRT_DEBUG_INFO, "%d INFO> ", chan);

  (void)iprintf(s, chan, call[chan].dialin ? ilabel : olabel, left, msg, right);

  print_msg(PRT_DEBUG_INFO, "%s", s);

  print_msg(reason, "%s", s);

  if (xinfo) {
    if ((i = (sizeof(call[chan].msg) -1)) < strlen(msg)) /* clipping ... */
      msg[i] = 0;

    strcpy(call[chan].msg, msg);
    call[chan].stat = state;

    message_from_server(&(call[chan]), chan);

    print_msg(PRT_DEBUG_CS, "SOCKET> %s: MSG_CALL_INFO chan=%d\n", st + 4, chan);
  } /* if */
} /* info */

/*****************************************************************************/

void showmsg(const char *fmt, ...)
{
  auto char    s[BUFSIZ], s1[BUFSIZ];
  auto va_list ap;


  va_start(ap, fmt);
  (void)vsnprintf(s, BUFSIZ, fmt, ap);
  va_end(ap);

  (void)iprintf(s1, -1, mlabel, "", s, "");
  print_msg(PRT_SHOWNUMBERS, "%s", s1);
} /* showmsg */

/*****************************************************************************/
int ringer(int chan, int event)
{
  register int i, j, c;
  int   ProcessStarted = 0;
  int   old_c = -1;
  info_args *infoarg = NULL;

  print_msg(PRT_DEBUG_EXEC, "Got ring event %d on channel %d, number of sockets: %d\n", event, chan, socket_size(sockets));

  /* chan; Der jeweilige B-Kanal (0 oder 1)

     event:
       RING_RING    = Nummern wurden uebertragen
       RING_CONNECT = Hoerer abgenommen
       RING_HANGUP  = Hoerer aufgelegt
       RING_AOCD    = Gebuehrenimpuls (wird noch nicht verwendet)
  */

  if (event != RING_HANGUP)
  {
    if (event == RING_RING || event == RING_CONNECT)
      call[chan].cur_event = event;
  }
  else
    call[chan].cur_event = 0;

  for (i = CALLING; i <= CALLED; i++) {   /* fuer beide beteilige Telefonnummern */
    c = call[chan].confentry[i];

    if (c != -1 && c != old_c) { /* wenn Eintrag in isdnlog.conf gefunden */
    	old_c = c;

      for (j = 0;known[c]->infoargs != NULL && (infoarg = known[c]->infoargs[j]) != NULL;j++)
        ProcessStarted += Start_Ring(chan,infoarg,event,0);
    }
  }

  if (ProcessStarted == 0)
  {
    for (j = 0;start_procs.infoargs != NULL && (infoarg = start_procs.infoargs[j]) != NULL;j++)
      ProcessStarted += Start_Ring(chan,infoarg,event,0);
  }

  return ProcessStarted;
} /* ringer */
/* vim:set ts=2: */
