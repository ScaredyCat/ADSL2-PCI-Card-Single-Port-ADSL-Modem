/* $Id: isdnlog.c,v 1.68 2001/10/15 19:51:48 akool Exp $
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
 * $Log: isdnlog.c,v $
 * Revision 1.68  2001/10/15 19:51:48  akool
 * isdnlog-4.53
 *  - verified Leo's correction of Paul's byte-order independent Patch to the CDB
 *    (now it's Ok, Leo, and *many* thanks to Paul!)
 *  - "rate-de.dat" updated
 *  - added "-Q" option to isdnlog
 *
 * Revision 1.67  2001/08/18 12:01:25  paul
 * Close stdout and stderr if we're becoming a daemon.
 *
 * Revision 1.66  2000/09/05 08:05:02  paul
 * Now isdnlog doesn't use any more ISDN_XX defines to determine the way it works.
 * It now uses the value of "COUNTRYCODE = 999" to determine the country, and sets
 * a variable mycountrynum to that value. That is then used in the code to set the
 * way isdnlog works.
 * It works for me, please check it! No configure.in / doc changes yet until
 * it has been checked to work.
 * So finally a version of isdnlog that can be compiled and distributed
 * internationally.
 *
 * Revision 1.65  2000/07/19 19:41:32  akool
 * isdnlog-4.34
 *   - since around Linux-2.2.16 signals are *not* reset to their default
 *     behavior when raised :-( (bug or feature?).
 *   - isdnlog/rate-pl.dat ... changes from Karsten Voss <vossdoku@gmx.net>
 *   - populated "samples/isdn.conf.de" with all german Internet-by-Call numbers
 *
 * Revision 1.64  2000/07/18 22:26:05  akool
 * isdnlog-4.33
 *   - isdnlog/tools/rate.c ... Bug fixed
 *   - isdnlog/isdnlog/isdnlog.c ... check for callfmt
 *   - "rate-de.dat" corrected (duplicates removed)
 *
 * Revision 1.63  2000/06/29 17:38:27  akool
 *  - Ported "imontty", "isdnctrl", "isdnlog", "xmonisdn" and "hisaxctrl" to
 *    Linux-2.4 "devfs" ("/dev/isdnctrl" -> "/dev/isdn/isdnctrl")
 *
 * Revision 1.62  2000/06/20 17:09:59  akool
 * isdnlog-4.29
 *  - better ASN.1 display
 *  - many new rates
 *  - new Option "isdnlog -Q" dump's "/etc/isdn/isdn.conf" into a SQL database
 *
 * Revision 1.61  2000/04/02 17:35:07  akool
 * isdnlog-4.18
 *  - isdnlog/isdnlog/isdnlog.8.in  ... documented hup3
 *  - isdnlog/tools/dest.c ... _DEMD1 not recogniced as key
 *  - mySQL Server version 3.22.27 support
 *  - new rates
 *
 * Revision 1.60  2000/03/09 18:50:02  akool
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
 * Revision 1.59  2000/02/11 10:41:52  akool
 * isdnlog-4.10
 *  - Set CHARGEINT to 11 if < 11
 *  - new Option "-dx" controls ABC_LCR feature (see README for infos)
 *  - new rates
 *
 * Revision 1.58  2000/02/03 18:24:50  akool
 * isdnlog-4.08
 *   isdnlog/tools/rate.c ... LCR patch again
 *   isdnlog/tools/isdnrate.c ... LCR patch again
 *   isdnbill enhanced/fixed
 *   DTAG AktivPlus fixed
 *
 * Revision 1.57  2000/02/02 22:43:09  akool
 * isdnlog-4.07
 *  - many new rates per 1.2.2000
 *
 * Revision 1.56  1999/12/31 13:30:01  akool
 * isdnlog-4.00 (Millenium-Edition)
 *  - Oracle support added by Jan Bolt (Jan.Bolt@t-online.de)
 *
 * Revision 1.55  1999/11/08 21:09:39  akool
 * isdnlog-3.65
 *   - added "B:" Tag to "rate-xx.dat"
 *
 * Revision 1.54  1999/11/07 13:29:27  akool
 * isdnlog-3.64
 *  - new "Sonderrufnummern" handling
 *
 * Revision 1.53  1999/10/30 18:03:31  akool
 *  - fixed "-q" option
 *  - workaround for "Sonderrufnummern"
 *
 * Revision 1.52  1999/10/29 19:46:00  akool
 * isdnlog-3.60
 *  - sucessfully ported/tested to/with:
 *      - Linux-2.3.24 SMP
 *      - egcs-2.91.66
 *    using -DBIG_PHONE_NUMBERS
 *
 *  - finally added working support for HFC-card in "echo mode"
 *    try this:
 *      hisaxctrl bri 10 1
 *      hisaxctrl bri 12 1
 *      isdnlog -21 -1
 * -----------------^^ new option
 *
 * Revision 1.51  1999/10/25 18:33:15  akool
 * isdnlog-3.57
 *   WARNING: Experimental version!
 *   	   Please use isdnlog-3.56 for production systems!
 *
 * Revision 1.50  1999/09/26 10:55:20  akool
 * isdnlog-3.55
 *   - Patch from Oliver Lauer <Oliver.Lauer@coburg.baynet.de>
 *     added hup3 to option file
 *   - changed country-de.dat to ISO 3166 Countrycode / Airportcode
 *
 * Revision 1.49  1999/09/11 22:28:23  akool
 * isdnlog-3.50
 *   added 3. parameter to "-h" Option: Controls CHARGEHUP for providers like
 *   DTAG (T-Online) or AOL.
 *   Many thanks to Martin Lesser <m-lesser@lesser-com.de>
 *
 * Revision 1.48  1999/08/20 19:28:12  akool
 * isdnlog-3.45
 *  - removed about 1 Mb of (now unused) data files
 *  - replaced areacodes and "vorwahl.dat" support by zone databases
 *  - fixed "Sonderrufnummern"
 *  - rate-de.dat :: V:1.10-Germany [20-Aug-1999 21:23:27]
 *
 * Revision 1.47  1999/06/28 19:16:03  akool
 * isdnlog Version 3.38
 *   - new utility "isdnrate" started
 *
 * Revision 1.46  1999/06/16 23:37:31  akool
 * fixed zone-processing
 *
 * Revision 1.45  1999/06/15 20:04:03  akool
 * isdnlog Version 3.33
 *   - big step in using the new zone files
 *   - *This*is*not*a*production*ready*isdnlog*!!
 *   - Maybe the last release before the I4L meeting in Nuernberg
 *
 * Revision 1.44  1999/06/13 14:07:44  akool
 * isdnlog Version 3.32
 *
 *  - new option "-U1" (or "ignoreCOLP=1") to ignore CLIP/COLP Frames
 *  - TEI management decoded
 *
 * Revision 1.43  1999/05/22 10:18:28  akool
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
 * Revision 1.42  1999/05/13 11:39:18  akool
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
 * Revision 1.41  1999/05/04 19:32:37  akool
 * isdnlog Version 3.24
 *
 *  - fully removed "sondernummern.c"
 *  - removed "gcc -Wall" warnings in ASN.1 Parser
 *  - many new entries for "rate-de.dat"
 *  - better "isdnconf" utility
 *
 * Revision 1.40  1999/04/10 16:35:27  akool
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
 * Revision 1.39  1999/03/25 19:39:51  akool
 * - isdnlog Version 3.11
 * - make isdnlog compile with egcs 1.1.7 (Bug report from Christophe Zwecker <doc@zwecker.com>)
 *
 * Revision 1.38  1999/03/24 19:37:46  akool
 * - isdnlog Version 3.10
 * - moved "sondernnummern.c" from isdnlog/ to tools/
 * - "holiday.c" and "rate.c" integrated
 * - NetCologne rates from Oliver Flimm <flimm@ph-cip.uni-koeln.de>
 * - corrected UUnet and T-Online rates
 *
 * Revision 1.37  1999/03/20 14:33:02  akool
 * - isdnlog Version 3.08
 * - more tesion)) Tarife from Michael Graw <Michael.Graw@bartlmae.de>
 * - use "bunzip -f" from Franz Elsner <Elsner@zrz.TU-Berlin.DE>
 * - show another "cheapest" hint if provider is overloaded ("OVERLOAD")
 * - "make install" now makes the required entry
 *     [GLOBAL]
 *     AREADIFF = /usr/lib/isdn/vorwahl.dat
 * - README: Syntax description of the new "rate-at.dat"
 * - better integration of "sondernummern.c" from mario.joussen@post.rwth-aachen.de
 * - server.c: buffer overrun fix from Michael.Weber@Post.RWTH-Aachen.DE (Michael Weber)
 *
 * Revision 1.36  1999/03/14 14:26:28  akool
 * - isdnlog Version 3.05
 * - new Option "-u1" (or "ignoreRR=1")
 * - added version information to "sonderrufnummern.dat"
 * - added debug messages if sonderrufnummern.dat or tarif.dat could not be opened
 * - sonderrufnummern.dat V 1.01 - new 01805 rates
 *
 * Revision 1.35  1999/02/28 19:32:38  akool
 * Fixed a typo in isdnconf.c from Andreas Jaeger <aj@arthur.rhein-neckar.de>
 * CHARGEMAX fix from Oliver Lauer <Oliver.Lauer@coburg.baynet.de>
 * isdnrep fix from reinhard.karcher@dpk.berlin.fido.de (Reinhard Karcher)
 * "takt_at.c" fixes from Ulrich Leodolter <u.leodolter@xpoint.at>
 * sondernummern.c from Mario Joussen <mario.joussen@post.rwth-aachen.de>
 * Reenable usage of the ZONE entry from Schlottmann-Goedde@t-online.de
 * Fixed a typo in callerid.conf.5
 *
 * Revision 1.34  1999/01/24 19:01:31  akool
 *  - second version of the new chargeint database
 *  - isdnrep reanimated
 *
 * Revision 1.33  1999/01/10 15:23:13  akool
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
 * Revision 1.32  1998/12/31 09:58:50  paul
 * converted termio calls to termios
 *
 * Revision 1.31  1998/12/09 20:39:28  akool
 *  - new option "-0x:y" for leading zero stripping on internal S0-Bus
 *  - new option "-o" to suppress causes of other ISDN-Equipment
 *  - more support for the internal S0-bus
 *  - Patches from Jochen Erwied <mack@Joker.E.Ruhr.DE>, fixes TelDaFax Tarif
 *  - workaround from Sebastian Kanthak <sebastian.kanthak@muehlheim.de>
 *  - new CHARGEINT chapter in the README from
 *    "Georg v.Zezschwitz" <gvz@popocate.hamburg.pop.de>
 *
 * Revision 1.30  1998/11/24 20:51:31  akool
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
 * Revision 1.29  1998/11/17 00:37:39  akool
 *  - fix new Option "-i" (Internal-S0-Bus)
 *  - more Providers (Nikoma, First Telecom, Mox)
 *  - isdnrep-Bugfix from reinhard.karcher@dpk.berlin.fido.de (Reinhard Karcher)
 *  - Configure.help completed
 *
 * Revision 1.28  1998/11/07 17:12:56  akool
 * Final cleanup. This _is_ isdnlog-3.00
 *
 * Revision 1.27  1998/11/01 08:49:43  akool
 *  - fixed "configure.in" problem with NATION_*
 *  - DESTDIR fixes (many thanks to Michael Reinelt <reinelt@eunet.at>)
 *  - isdnrep: Outgoing calls ordered by Zone/Provider/MSN corrected
 *  - new Switch "-i" -> running on internal S0-Bus
 *  - more providers
 *  - "sonderrufnummern.dat" extended (Frag Fred, Telegate ...)
 *  - added AVM-B1 to the documentation
 *  - removed the word "Teles" from the whole documentation ;-)
 *
 * Revision 1.26  1998/10/26 20:21:14  paul
 * thinko in check for symlink in /tmp
 *
 * Revision 1.25  1998/10/22 14:10:52  paul
 * Check that /tmp/isdnctrl0 is not a symbolic link, which is a potential
 * security threat (it can point to /etc/passwd or so!)
 *
 * Revision 1.24  1998/10/18 20:13:33  luethje
 * isdnlog: Added the switch -K
 *
 * Revision 1.23  1998/10/06 12:50:57  paul
 * As the exec is done within the signal handler, SIGHUP was blocked after the
 * first time. Now SIGHUP is unblocked so that you can send SIGHUP more than once.
 *
 * Revision 1.22  1998/09/26 18:29:07  akool
 *  - quick and dirty Call-History in "-m" Mode (press "h" for more info) added
 *    - eat's one more socket, Stefan: sockets[3] now is STDIN, FIRST_DESCR=4 !!
 *  - Support for tesion)) Baden-Wuerttemberg Tarif
 *  - more Providers
 *  - Patches from Wilfried Teiken <wteiken@terminus.cl-ki.uni-osnabrueck.de>
 *    - better zone-info support in "tools/isdnconf.c"
 *    - buffer-overrun in "isdntools.c" fixed
 *  - big Austrian Patch from Michael Reinelt <reinelt@eunet.at>
 *    - added $(DESTDIR) in any "Makefile.in"
 *    - new Configure-Switches "ISDN_AT" and "ISDN_DE"
 *      - splitted "takt.c" and "tools.c" into
 *          "takt_at.c" / "takt_de.c" ...
 *          "tools_at.c" / "takt_de.c" ...
 *    - new feature
 *        CALLFILE = /var/log/caller.log
 *        CALLFMT  = %b %e %T %N7 %N3 %N4 %N5 %N6
 *      in "isdn.conf"
 *  - ATTENTION:
 *      1. "isdnrep" dies with an seg-fault, if not HTML-Mode (Stefan?)
 *      2. "isdnlog/Makefile.in" now has hardcoded "ISDN_DE" in "DEFS"
 *      	should be fixed soon
 *
 * Revision 1.21  1998/06/21 11:52:46  akool
 * First step to let isdnlog generate his own AOCD messages
 *
 * Revision 1.20  1998/06/07 21:08:31  akool
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
 * Revision 1.19  1998/05/19 15:55:51  paul
 * Moved config stuff for City Weekend from isdnlog.c to tools/isdnconf.c, so
 * that isdnrep also understands a "cityweekend=y" line in isdn.conf.
 *
 * Revision 1.18  1998/05/19 15:47:03  paul
 * If logfile name is specified with leading '+', the logfile is not truncated
 * when isdnlog starts; instead, new messages are appended.
 *
 * Revision 1.17  1998/03/29 23:18:07  luethje
 * mySQL-Patch of Sascha Matzke
 *
 * Revision 1.16  1998/03/08 12:13:38  luethje
 * Patches by Paul Slootman
 *
 * Revision 1.15  1998/03/08 11:42:50  luethje
 * I4L-Meeting Wuerzburg final Edition, golden code - Service Pack number One
 *
 * Revision 1.14  1998/02/08 09:36:51  calle
 * fixed problems with FD_ISSET and glibc, if descriptor is not open.
 *
 * Revision 1.13  1997/06/22 23:03:23  luethje
 * In subsection FLAGS it will be checked if the section name FLAG is korrect
 * isdnlog recognize calls abroad
 * bugfix for program starts
 *
 * Revision 1.12  1997/05/25 19:40:58  luethje
 * isdnlog:  close all files and open again after kill -HUP
 * isdnrep:  support vbox version 2.0
 * isdnconf: changes by Roderich Schupp <roderich@syntec.m.EUnet.de>
 * conffile: ignore spaces at the end of a line
 *
 * Revision 1.11  1997/05/09 23:30:47  luethje
 * isdnlog: new switch -O
 * isdnrep: new format %S
 * bugfix in handle_runfiles()
 *
 * Revision 1.10  1997/05/06 22:13:26  luethje
 * bugfixes in HTML-Code of the isdnrep
 *
 * Revision 1.9  1997/05/04 20:19:47  luethje
 * README completed
 * isdnrep finished
 * interval-bug fixed
 *
 * Revision 1.8  1997/04/17 20:09:32  luethje
 * patch of Ingo Schneider
 *
 * Revision 1.7  1997/04/08 21:56:48  luethje
 * Create the file isdn.conf
 * some bug fixes for pid and lock file
 * make the prefix of the code in `isdn.conf' variable
 *
 * Revision 1.6  1997/04/08 00:02:14  luethje
 * Bugfix: isdnlog is running again ;-)
 * isdnlog creates now a file like /var/lock/LCK..isdnctrl0
 * README completed
 * Added some values (countrycode, areacode, lock dir and lock file) to
 * the global menu
 *
 */

#define _ISDNLOG_C_

#include <linux/limits.h>
#include <termios.h>

#include "isdnlog.h"
#include "dest.h"
#ifdef POSTGRES
#include "postgres.h"
#endif
#ifdef MYSQLDB
#include "mysqldb.h"
#endif
#ifdef ORACLE
#include "oracle.h"
#endif

#define FD_SET_MAX(desc, set, max) { if (desc > max) max=desc; FD_SET(desc,set); }

#ifdef Q931
#define Q931dmp q931dmp
#else
#define Q931dmp 0
#endif

/*****************************************************************************/

 /* Letzte Exit-Nummer: 47 */

/*****************************************************************************/

#define X_FD_ISSET(fd, mask)    ((fd) >= 0 && FD_ISSET(fd,mask))

static void loop(void);
static void init_variables(int argc, char* argv[]);
static int  set_options(int argc, char* argv[]);
static void hup_handler(int isig);
static void exit_on_signal(int Sign);
int print_in_modules(const char *fmt, ...);
static int read_param_file(char *FileName);

/*****************************************************************************/

static char     usage[]   = "%s: usage: %s [ -%s ] file\n";
#ifdef Q931
static char     options[] = "qav:sp:x:m:l:rt:c:C:w:SVTDPMh:nW:H:f:bL:NFA:2:O:Ki:R:0:ou:B:U:1d:I:Q:";
#else
static char     options[] =  "av:sp:x:m:l:rt:c:C:w:SVTDPMh:nW:H:f:bL:NFA:2:O:Ki:R:0:ou:B:U:1d:I:Q:";
#endif
static char     msg1[]    = "%s: Can't open %s (%s)\n";
static char    *ptty = NULL;
static section *opt_dat = NULL;
static char	**hup_argv;	/* args to restart with */

static int      sqldump = 0;

/*****************************************************************************/

static void exit_on_signal(int Sign)
{
  signal(Sign, SIG_DFL);

  print_msg(PRT_NORMAL, "Got signal %d\n", Sign);

  Exit(7);
} /* exit_on_signal */

/*****************************************************************************/

static void hup_handler(int isig)
{
  print_msg(PRT_INFO, "restarting %s\n", myname);
  Exit(-9);
  execv(myname, hup_argv);
  print_msg(PRT_ERR,"Cannot restart %s: %s!\n", myname, strerror(errno));
} /* hup_handler */

/*****************************************************************************/

static void loop(void)
{
  auto fd_set readmask;
  auto fd_set exceptmask;
  auto int    maxdesc = 0;
  auto int    Cnt;
  auto int    len;
  auto int    queuenumber;
  auto int    NewSocket;
  auto int    NewClient = 0;
  auto struct sockaddr incoming;
  auto int    Interval;


  if (xinfo) {
    first_descr++;

    if ((NewSocket = listening(port)) >= 0) {
      if (add_socket(&sockets, NewSocket))
	Exit(11);
    }
    else
      Exit(17);
  }
  else
    if (add_socket(&sockets, -1))
      Exit(11);

  while (1) {
    if (replay) {

      if (trace)
        dotrace();

      if (!morectrl(0))
        break;
    }
    else {
      FD_ZERO(&readmask);
      FD_ZERO(&exceptmask);

      queuenumber = socket_size(sockets);

      if (NewClient) {
        /* Damit sich der neue Client anmelden kann, ohne
           das was anderes dazwischen funkt ... */
        if (sockets[NewClient].descriptor >= 0)
	        FD_SET_MAX(sockets[NewClient].descriptor, &readmask, maxdesc);

      	NewClient = 0;
      }
      else {
        for (Cnt = 0; Cnt < queuenumber; Cnt++)
          if (sockets[Cnt].descriptor >= 0)
            FD_SET_MAX(sockets[Cnt].descriptor, &readmask, maxdesc);

        for (Cnt = first_descr; Cnt < queuenumber; Cnt++)
          if (sockets[Cnt].descriptor >= 0)
            FD_SET_MAX(sockets[Cnt].descriptor, &exceptmask, maxdesc);
      } /* else */

      if (newcps && ((ifo[0].u & ISDN_USAGE_MASK) + (ifo[1].u & ISDN_USAGE_MASK)))
      	Interval = wakeup; /* AK:06-Jun-96 */
      else
      	Interval = 0;

      while ((Cnt = select(maxdesc+1, &readmask, NULL, &exceptmask, Get_Interval(Interval))) < 0 && (errno == EINTR));

      if ((Cnt < 0) && (errno != EINTR)) { /* kill -HUP ausgeschlossen */
        print_msg(PRT_DEBUG_CS, "Error select: %s\n", strerror(errno));
        Exit(12);
      } /* if */

      processflow();

      if (!Cnt) /* Abbruch durch Timeout -> Programm starten */
        Start_Interval();

      now();

      /* processRate(-1); */
      processcint();

      for (Cnt = first_descr; Cnt < socket_size(sockets); Cnt++) {
        if (X_FD_ISSET(sockets[Cnt].descriptor, &exceptmask)) {
          if (sockets[Cnt].fp == NULL) {
            disconnect_client(Cnt);
            break;
          }
          else {
            int        event    = sockets[Cnt].call_event;
            int        cur_call = sockets[Cnt].chan;
            info_args *infoarg  = sockets[Cnt].info_arg;

            Ring(NULL, NULL, Cnt, 0);

            if (infoarg->flag & RING_LOOP && call[cur_call].cur_event == event)
              Start_Process(cur_call, infoarg, event);

            break;
          } /* else */
        }
        else if (X_FD_ISSET(sockets[Cnt].descriptor, &readmask)) {
          if (sockets[Cnt].fp == NULL) {
            eval_message(Cnt);
            /* Arbeite immer nur ein Client ab, du weisst nicht, ob der
               naechste noch lebt */
            break;
          }
          else
            Print_Cmd_Output(Cnt);
        } /* else */
      } /* for */

      if (xinfo && X_FD_ISSET(sockets[IN_PORT].descriptor, &readmask)) {
        len = sizeof(incoming);

        if ((NewSocket = accept(sockets[IN_PORT].descriptor, &incoming, &len)) == -1)
          print_msg(PRT_DEBUG_CS, "Error accept: %s\n", strerror(errno));
        else {
          if (add_socket(&sockets, NewSocket))
            Exit(13);

      	  queuenumber = socket_size(sockets);

          sockets[queuenumber - 1].f_hostname = GetHostByAddr(&incoming);
          sockets[queuenumber - 1].waitstatus = WF_ACC;

          NewClient = queuenumber - 1;
        } /* else */
      }
      else if (X_FD_ISSET(sockets[ISDNINFO].descriptor, &readmask))
        moreinfo();
      else if (X_FD_ISSET(sockets[ISDNCTRL].descriptor, &readmask))
        (void)morectrl(0);
      else if (X_FD_ISSET(sockets[ISDNCTRL2].descriptor, &readmask))
        (void)morectrl(1);
      else if (X_FD_ISSET(sockets[STDIN].descriptor, &readmask))
        (void)morekbd();

    } /* else */
  } /* while */
} /* loop */

#undef X_FD_ISSET

/*****************************************************************************/

int print_in_modules(const char *fmt, ...)
{
  auto va_list ap;
  auto char    String[LONG_STRING_SIZE];


  va_start(ap, fmt);
  (void)vsnprintf(String, LONG_STRING_SIZE, fmt, ap);
  va_end(ap);

  return print_msg(PRT_ERR, "%s", String);
} /* print_in_modules */

/*****************************************************************************/

static void init_variables(int argc, char* argv[])
{
  flog     = NULL;   /* /var/adm/isdn.log          */
  fcons    = NULL;   /* /dev/ttyX      (or stderr) */
  fprot    = NULL;   /* /tmp/isdnctrl0 	 	   */
  isdnctrl = NULL;
  fout     = NULL;

  first_descr = FIRST_DESCR;
  message = PRT_NORMAL | PRT_WARN | PRT_ERR | PRT_INFO;
  syslogmessage = PRT_NOTHING;
  xinfo = 0;
  sound = 0;
  trace = 0;
  isdaemon = 0;
  imon = 0;
  port = 0;
  wakeup = 1;
  fullhour = 0;
  tty_dv = 0;
  net_dv = 0;
  inf_dv = 0;
  settime = 0;
  replay = 0;
  replaydev = 0;
  verbose = 0;
  synclog = 0;
  any = 1;
  stdoutput = 0;
  allflags = 0;
  newcps = 0;
  chans = 2;
  hupctrl = hup1 = hup2 = 0;
  trim = trimi = trimo = 0;
  bilingual = 0;
  other = 0;
  mcalls = MAX_CALLS_IN_QUEUE;
  xlog = MAX_PRINTS_IN_QUEUE;
  outfile = NULL;
  readkeyboard = 0;

  sockets = NULL;
  known = NULL;

  opt_dat = NULL;
  newline = 1;
  width = 0;
  watchdog = 0;
  use_new_config = 1;

  ignoreRR = ignoreCOLP = 0;

#ifdef Q931
  q931dmp = 0;
#endif
#if 0 /* Fixme: remove */
  CityWeekend = 0;
#endif

  sprintf(mlabel, "%%s%s  %%s%%s", "%e.%b %T %I");
  amtsholung = NULL;
  dual = 0;
  hfcdual = 0;
  hup3 = 240;
  abclcr = 0;
  ciInterval = 0;
  ehInterval = 0;

  myname = argv[0];
  myshortname = basename(myname);
} /* init_variables */

/*****************************************************************************/

#ifdef Q931
static void traceoptions()
{
  q931dmp++;
//  use_new_config = 0;
  replay++;
  port = 20012;
} /* traceoptions */
#endif

/*****************************************************************************/

int set_options(int argc, char* argv[])
{
  register int   c;
  register char *p;
  auto     int   defaultmsg = PRT_NORMAL | PRT_WARN | PRT_ERR | PRT_INFO;
  auto     int   newmessage = 0;


  if (!message)
    message = defaultmsg;

#ifdef Q931
  if (!strcmp(myshortname, "trace"))
    traceoptions();
#endif

  while ((c = getopt(argc, argv, options)) != EOF)
    switch (c) {
      case 'V' : print_version(myshortname);
      	       	 exit(0);
		 break;

      case 'v' : verbose = strtol(optarg, NIL, 0);
      	       	 break;

      case 's' : synclog++;
      	       	 break;

      case 'p' : port = strtol(optarg, NIL, 0);
      	       	 break;

      case 'm' : newmessage = strtol(optarg, NIL, 0);

      	       	 if (!newmessage) {
                   newmessage = defaultmsg;
                   printf("%s: WARNING: \"-m\" Option now requires numeric Argument\n", myshortname);
                 } /* if */
      	       	 break;

      case 'l' : syslogmessage = strtol(optarg, NIL, 0);

               	 if (!syslogmessage) {
                   syslogmessage = defaultmsg;
                   printf("%s: WARNING: \"-l\" Option requires numeric Argument\n", myshortname);
                 } /* if */
                 break;

      case 'x' : xinfo = strtol(optarg, NIL, 0);

               	 if (!xinfo)
                   xinfo = defaultmsg;
      	       	 break;

      case 'r' : replay++;
      	       	 break;

      case 't' : settime = strtol(optarg, NIL, 0); /* 1=once, 2=ever */
      	       	 break;

      case 'C' : ptty = strdup(optarg);
                 break;

      case 'M' : imon++;
      	       	 break;

      case 'S' : sound++;
      	       	 break;

      case 'w' : wakeup = strtol(optarg, NIL, 0);
      	       	 break;

      case 'D' : isdaemon++;
      	       	 break;

      case 'T' : trace++;
      	       	 break;

      case 'a' : any = 0;
      	       	 break;

      case 'P' : stdoutput = PRT_LOG;
      	       	 break;

      case 'h' : hupctrl++;

      	       	 if ((p = strchr(optarg, ':'))) {
                   *p = 0;

                   hup1 = atoi(optarg);
                   hup2 = atoi(p + 1);

                   if ((p = strchr(p + 1, ':')))
                     hup3 = atoi(p + 1);
      	       	 }
                 else
                   printf("%s: WARNING: \"-h\" Option requires 2 .. 3 arguments\n", myshortname);
      	       	 break;

      case 'b' : bilingual++;
      	       	 break;

      case 'c' : mcalls = strtol(optarg, NIL, 0);
      	       	 break;

      case 'L' : xlog = strtol(optarg, NIL, 0);
      	       	 break;

      case 'f' : read_param_file(optarg);
      	       	 break;

      case 'n' : newline = 0;
      	       	 break;

      case 'W' : width = strtol(optarg, NIL, 0);
      	       	 break;

      case 'H' : watchdog = strtol(optarg, NIL, 0);
      	       	 break;

      case 'N' : use_new_config = 0;
      	       	 break;

#ifdef Q931
      case 'q' : traceoptions();
      	       	 break;
#endif
#if 0 /* Fixme: remove */
      case 'F' : CityWeekend++;
      	       	 break;
#endif

      case 'A' : amtsholung = strdup(optarg);
      	       	 break;

      case '1' : hfcdual = 1;
      	       	 break;

      case '2' : dual = strtol(optarg, NIL, 0);
      	       	 break;

      case 'O' : outfile = strdup(optarg);
      	       	 break;

      case 'K' : readkeyboard++;
      	       	 break;

      case 'i' : interns0 = (int)strtol(optarg, NIL, 0);
      	       	 break;

      case 'R' : preselect = (int)strtol(optarg, NIL, 0);
      	       	 break;

      case '0' : trim++;

      	       	 if ((p = strchr(optarg, ':'))) {
                   *p = 0;
                   trimi = atoi(optarg);
                   trimo = atoi(p + 1);
      	       	 }
                 else
                   trimi = trimo = atoi(optarg);
      	       	 break;

      case 'o' : other++;
      	       	 break;

      case 'u' : ignoreRR = atoi(optarg);
      	       	 break;

      case 'U' : ignoreCOLP = atoi(optarg);
      	       	 break;

      case 'B' : free(vbn);
      	         vbn = strdup(optarg);
      	       	 break;

      case 'd' : abclcr = atoi(optarg);
      	       	 break;

      case 'I' :
      	       	 if ((p = strchr(optarg, ':'))) {
                   *p = 0;
                   ciInterval = atoi(optarg);
                   ehInterval = atoi(p + 1);
      	       	 }
                 else
                   ciInterval = ehInterval = atoi(optarg);
      	       	 break;

      case 'Q' : sqldump = atoi(optarg);
      	       	 break;

      case '?' : printf(usage, myshortname, myshortname, options);
	         exit(1);
    } /* switch */

  if (newmessage)
    message = newmessage;

  if (readkeyboard && isdaemon) {
    printf("%s","Can read from standard input daemonized!\n");
    exit(20);
  } /* if */

  if (trace && isdaemon) {
    printf("%s","Can not trace daemonized!\n");
    exit(20);
  } /* if */

  if (stdoutput && isdaemon) {
    printf("%s","Can not write to stdout as daemon!\n");
    exit(21);
  } /* if */

  if (isdaemon) {
    if (syslogmessage == -1)
      syslogmessage = defaultmsg;

    fflush(stdout); /* always advisable before a fork */
    fflush(stderr);
    switch (fork()) {
      case -1 : print_msg(PRT_ERR,"%s","Can not fork()!\n");
                Exit(18);
                break;
      case 0  : /* we're a daemon, so "close" stdout, stderr */
                /* stdin is "closed" elsewhere */
                freopen("/dev/null", "a+", stdout);
                freopen("/dev/null", "a+", stderr);
                break;

      default : _exit(0);
    }

    /* Wenn message nicht explixit gesetzt wurde, dann gibt es beim daemon auch
       kein Output auf der Console/ttyx                                   */

    if (!outfile && !newmessage && !ptty)
      message = 0;
  } /* if */

  allflags = syslogmessage | message | xinfo | stdoutput;

  return optind;
} /* set_options */

/*****************************************************************************/

static int read_param_file(char *FileName)
{
  register char    *p;
  auto 	   section *SPtr;
  auto 	   entry   *Ptr;


	if (opt_dat != NULL)
	{
    print_msg(PRT_ERR, "Only one option file allowed (file %s)!\n", FileName);
		return -1;
	}

	if ((SPtr = opt_dat = read_file(NULL, FileName, 0)) == NULL)
		return -1;

	while (SPtr != NULL)
	{
		if (!strcmp(SPtr->name,CONF_SEC_OPT))
		{
			Ptr = SPtr->entries;

			while (Ptr != NULL)
			{
				if (!strcmp(Ptr->name,CONF_ENT_DEV))
					isdnctrl = Ptr->value;
				else
				if (!strcmp(Ptr->name,CONF_ENT_LOG))
					verbose = (int)strtol(Ptr->value, NIL, 0);
				else
				if (!strcmp(Ptr->name,CONF_ENT_FLUSH))
					synclog = toupper(*(Ptr->value)) == 'Y'?1:0;
				else
				if (!strcmp(Ptr->name,CONF_ENT_PORT))
					port = strtol(Ptr->value, NIL, 0);
				else
				if (!strcmp(Ptr->name,CONF_ENT_STDOUT))
					message = strtol(Ptr->value, NIL, 0);
				else
				if (!strcmp(Ptr->name,CONF_ENT_SYSLOG))
					syslogmessage = strtol(Ptr->value, NIL, 0);
				else
				if (!strcmp(Ptr->name,CONF_ENT_XISDN))
				{
					xinfo = strtol(Ptr->value, NIL, 0);
				}
				else
				if (!strcmp(Ptr->name,CONF_ENT_TIME))
					settime = (int)strtol(Ptr->value, NIL, 0);
				else
				if (!strcmp(Ptr->name,CONF_ENT_CON))
					ptty = Ptr->value;
				else
				if (!strcmp(Ptr->name,CONF_ENT_START))
					sound = toupper(*(Ptr->value)) == 'Y'?1:0;
				else
				if (!strcmp(Ptr->name,CONF_ENT_THRU))
					wakeup = strtol(Ptr->value, NIL, 0);
				else
				if (!strcmp(Ptr->name,CONF_ENT_DAEMON))
					isdaemon = toupper(*(Ptr->value)) == 'Y'?1:0;
				else
				if (!strcmp(Ptr->name,CONF_ENT_MON))
					imon = toupper(*(Ptr->value)) == 'Y'?1:0;
				else
				if (!strcmp(Ptr->name,CONF_ENT_PIPE))
					stdoutput = toupper(*(Ptr->value)) == 'Y'?PRT_LOG:0;
				else
				if (!strcmp(Ptr->name,CONF_ENT_MON))
					imon = toupper(*(Ptr->value)) == 'Y'?1:0;
				else
				if (!strcmp(Ptr->name,CONF_ENT_HANGUP)) {
				  hupctrl++;
      	       	 		  if ((p = strchr(Ptr->value, ':'))) {
                 		    *p = 0;
                 		    hup1 = atoi(Ptr->value);
                 		    hup2 = atoi(p + 1);

				    if ((p = strchr(p + 1, ':')))
				      hup3 = atoi(p + 1);
				  } /* if */
				  else
				    printf("%s: WARNING: \"-h\" Option requires 2 .. 3 arguments\n", myshortname);
				}
				else
				if (!strcmp(Ptr->name,CONF_ENT_PROVIDERCHANGE))
				  providerchange = strdup(Ptr->value);
				else
				if (!strcmp(Ptr->name,CONF_ENT_ABCLCR))
				  abclcr = atoi(Ptr->value);
				else
                                if (!strcmp(Ptr->name, CONF_ENT_CIINTERVAL)) {
                                  if ((p = strchr(Ptr->value, ':'))) {
                                    *p = 0;
                                    ciInterval = atoi(Ptr->value);
                                    ehInterval = atoi(p + 1);
                                  }
                                  else
                                    ciInterval = ehInterval = atoi(Ptr->value);
                                }
				else
                                if (!strcmp(Ptr->name, CONF_ENT_TRIM)) {
                                  trim++;
                                  if ((p = strchr(Ptr->value, ':'))) {
                                    *p = 0;
                                    trimi = atoi(Ptr->value);
                                    trimo = atoi(p + 1);
                                  }
                                  else
                                    trimi = trimo = atoi(Ptr->value);
                                }
                                else
				if (!strcmp(Ptr->name,CONF_ENT_BI))
					bilingual = toupper(*(Ptr->value)) == 'Y'?1:0;
				else
				if (!strcmp(Ptr->name,CONF_ENT_CALLS))
					mcalls = strtol(Ptr->value, NIL, 0);
				else
				if (!strcmp(Ptr->name,CONF_ENT_XLOG))
					xlog = strtol(Ptr->value, NIL, 0);
				else
				if (!strcmp(Ptr->name,CONF_ENT_NL))
					newline = toupper(*(Ptr->value)) == 'Y'?1:0;
				else
				if (!strcmp(Ptr->name,CONF_ENT_WIDTH))
					width = (int)strtol(Ptr->value, NIL, 0);
				else
				if (!strcmp(Ptr->name,CONF_ENT_DUAL))
					dual = (int)strtol(Ptr->value, NIL, 0);
				else
				if (!strcmp(Ptr->name,CONF_ENT_AMT))
                                       amtsholung = strdup(Ptr->value);
				else
#ifdef Q931
				if (!strcmp(Ptr->name,CONF_ENT_Q931))
				{
					if(toupper(*(Ptr->value)) == 'Y')
						traceoptions();
				}
				else
#endif
				if (!strcmp(Ptr->name,CONF_ENT_WD))
					watchdog = (int)strtol(Ptr->value, NIL, 0);
				else
				if (!strcmp(Ptr->name,CONF_ENT_OUTFILE))
					outfile = Ptr->value;
				else
				if (!strcmp(Ptr->name,CONF_ENT_KEYBOARD))
					readkeyboard = toupper(*(Ptr->value)) == 'Y'?1:0;
				else
				if (!strcmp(Ptr->name,CONF_ENT_INTERNS0))
					interns0 = (int)strtol(Ptr->value, NIL, 0);
				else
                                if (!strcmp(Ptr->name,CONF_ENT_PRESELECT))
				        preselect = (int)strtol(Ptr->value, NIL, 0);
                                else
                                if (!strcmp(Ptr->name,CONF_ENT_OTHER))
				        other = toupper(*(Ptr->value)) == 'Y'?1:0;
                                else
#if 0 /* Fixme: remove */
				if (!strcmp(Ptr->name,CONF_ENT_CW))
				  CityWeekend++;
                                else
#endif
                                if (!strcmp(Ptr->name,CONF_ENT_IGNORERR))
				        ignoreRR = (int)strtol(Ptr->value, NIL, 0);
                                else
                                if (!strcmp(Ptr->name,CONF_ENT_IGNORECOLP))
				        ignoreCOLP = (int)strtol(Ptr->value, NIL, 0);
                                else
                                if (!strcmp(Ptr->name,CONF_ENT_VBN)) {
                                        free(vbn);
				        vbn = strdup(Ptr->value);
                                }
                                else
                                if (!strcmp(Ptr->name,CONF_ENT_VBNLEN)) {
                                        free(vbnlen);
				        vbnlen = strdup(Ptr->value);
                                }
                                else
					print_msg(PRT_ERR,"Error: Invalid entry `%s'!\n",Ptr->name);

				Ptr = Ptr->next;
			}
		}
		else
			print_msg(PRT_ERR,"Error: Invalid section `%s'!\n",SPtr->name);

		SPtr = SPtr->next;
	}

	return 0;
}

/*****************************************************************************/

static void restoreCharge()
{
  register int   i, j, n = 0, nx = max(knowns, 1);
  auto 	   FILE *f;
  auto 	   char  fn[BUFSIZ];

  typedef struct {
    char    who[16];
    int	    day;
    double  charge;
    double  scharge;
    int	    month;
    double  online;
    double  sonline;
    double  bytes;
    double  sbytes;
  } CHARGE;

  auto     CHARGE *charge;



  if ((charge = (CHARGE *)malloc(sizeof(CHARGE) * nx)) != (CHARGE *)NULL) {

    sprintf(fn, "%s/%s", confdir(), CHARGEFILE);

    if ((f = fopen(fn, "r")) != (FILE *)NULL) {

      while (fscanf(f, "%s %d %lg %lg %d %lg %lg %lg %lg\n",
        charge[n].who, &charge[n].day, &charge[n].charge, &charge[n].scharge,
        &charge[n].month, &charge[n].online, &charge[n].sonline,
        &charge[n].bytes, &charge[n].sbytes) == 9) {

        n++;

        if (n == nx) {
          nx++;

  	  if ((charge = (CHARGE *)realloc((void *)charge, sizeof(CHARGE) * nx)) == (CHARGE *)NULL) {
      	    fclose(f);
            return;
          } /* if */
        } /* if */

      } /* while */

      if (n) {
        for (i = 0; i < knowns; i++)
          for (j = 0; j < n; j++)
            if (!strcmp(known[i]->who, charge[j].who)) {
      	      known[i]->day     = charge[j].day;
      	      known[i]->charge  = charge[j].charge;
      	      known[i]->scharge = charge[j].scharge;
      	      known[i]->month	= charge[j].month;
      	      known[i]->online	= charge[j].online;
      	      known[i]->sonline	= charge[j].sonline;
      	      known[i]->bytes	= charge[j].bytes;
      	      known[i]->sbytes	= charge[j].sbytes;

              break;
            } /* if */
      } /* if */

      fclose(f);
    } /* if */

    free(charge);
  } /* if */
} /* restoreCharge */

/*****************************************************************************/

void raw_mode(int state)
{
  static struct termios newterminfo, oldterminfo;


  if (state) {
    tcgetattr(fileno(stdin), &oldterminfo);
    newterminfo = oldterminfo;

    newterminfo.c_iflag &= ~(INLCR | ICRNL | IUCLC | ISTRIP);
    newterminfo.c_lflag &= ~(ICANON | ECHO);
    newterminfo.c_cc[VMIN] = 1;
    newterminfo.c_cc[VTIME] = 1;

    tcsetattr(fileno(stdin), TCSAFLUSH, &newterminfo);
  }
  else
    tcsetattr(fileno(stdin), TCSANOW, &oldterminfo);
} /* raw_mode */

static int checkconfig(void) {
  if (callfile && (!callfmt || !*callfmt)) {
    print_msg(PRT_ERR, "No CALLFMT given.");
    return -1;
  }
  /* there shold propably be more checks here */

  return 0;
}
/*****************************************************************************/

int main(int argc, char *argv[], char *envp[])
{
  register char  *p;
  register int    i, res = 0;
  auto     int    lastarg;
  auto     char   rlogfile[PATH_MAX], s[BUFSIZ];
  auto	   char	  *version;
  auto     char **devices = NULL;
  sigset_t        unblock_set;
#ifdef TESTCENTER
  extern   void   test_center(char*);
#endif


  if (getuid() != 0) {
    fprintf(stderr,"Can only run as root!\n");
    exit(35);
  } /* if */

  set_print_fct_for_lib(print_in_modules);
  hup_argv = argv;
  init_variables(argc, argv);

  lastarg = set_options(argc,argv);

#ifdef TESTCENTER
    test_center(argv[lastarg]);
#endif

	if (outfile != NULL)
	{
		if (!message)
		{
 	   print_msg(PRT_ERR,"Can not set outfile `%s', when -m is not set!\n",outfile);
 	   Exit(44);
		}
		else
		{
			char *openmode;
			if (*outfile == '+')
			{
				outfile++;
				openmode = "a";
			}
			else
			{
				openmode = "w";
			}
			if ((fout = fopen(outfile, openmode)) == NULL)
			{
 	 			print_msg(PRT_ERR,"Can not open file `%s': %s!\n",outfile, strerror(errno));
 	  		Exit(45);
			}
		}
	}

  if (lastarg < argc)
    isdnctrl = argv[lastarg];

  if (isdnctrl == NULL) {
    print_msg(PRT_ERR,"There is no valid input device!\n");
    Exit(31);
  }
  else {
    p = strrchr(isdnctrl, C_SLASH);

    if (add_socket(&sockets, -1) ||  /* reserviert fuer isdnctrl */
        add_socket(&sockets, -1) ||  /* reserviert fuer isdnctrl2 */
        add_socket(&sockets, -1) ||  /* reserviert fuer isdninfo */
        add_socket(&sockets, -1) )   /* reserviert fuer stdin */
      Exit(19);

    if (replay) {
      verbose = 0;
      synclog = 0;
      settime = 0;
      xinfo = 0;
      *isdnctrl2 = 0;
    }
    else {
      if (strcmp(isdnctrl, "-") && dual && !hfcdual) {
        strcpy(isdnctrl2, isdnctrl);
      	p = strrchr(isdnctrl2, 0) - 1;

        if (!isdigit(*p)) /* "/dev/isdnctrl" */
          strcat(++p, "0");

        if (isdigit(*(p - 1)))
          p--;

        i = atoi(p);

        sprintf(p, "%d", i + 2);
      }
      else
        *isdnctrl2 = 0;

    } /* else */


    openlog(myshortname, LOG_NDELAY, LOG_DAEMON);

    if (xinfo && read_user_access())
      Exit(22);

    signal(SIGPIPE, SIG_IGN);
    signal(SIGHUP,  hup_handler);
    signal(SIGINT,  exit_on_signal);
    signal(SIGTERM, exit_on_signal);
    signal(SIGALRM, exit_on_signal);
    signal(SIGQUIT, exit_on_signal);
    signal(SIGILL,  exit_on_signal);

    if (!(allflags & PRT_DEBUG_GENERAL))
      signal(SIGSEGV, exit_on_signal);

    /*
     * If hup_handler() already did an execve(), then SIGHUP is still
     * blocked, and so you can only send a SIGHUP once. Here we unblock
     * SIGHUP so that this feature can be used more than once.
     */
    sigemptyset(&unblock_set);
    sigaddset(&unblock_set, SIGHUP);
    sigprocmask(SIG_UNBLOCK, &unblock_set, NULL);

    sockets[ISDNCTRL].descriptor = !strcmp(isdnctrl, "-") ? fileno(stdin) : open(isdnctrl, O_RDONLY | O_NONBLOCK);
    if (*isdnctrl2)
      sockets[ISDNCTRL2].descriptor = open(isdnctrl2, O_RDONLY | O_NONBLOCK);

    if ((sockets[ISDNCTRL].descriptor >= 0) && (!*isdnctrl2 || (sockets[ISDNCTRL2].descriptor >= 0))) {

      if (ptty != NULL)
        fcons = fopen(ptty, "a");

      if ((ptty == NULL) || (fcons != (FILE *)NULL)) {
        if (verbose) {
          struct stat st;
          if ((p = strrchr(isdnctrl, '/')))
            p++;
          else
            p = argv[lastarg];

          sprintf(tmpout, "%s/%s", TMPDIR, p);
          /*
           * If tmpout is a symlink, refuse to write to it (security hole).
           * E.g. someone can create a link /tmp/isdnctrl0 -> /etc/passwd.
           */
          if (!lstat(tmpout, &st) && S_ISLNK(st.st_mode)) {
            print_msg(PRT_ERR, "File \"%s\" is a symlink, not writing to it!\n", tmpout);
            verbose = 0;
          }
        } /* if */

        if (!verbose || ((fprot = fopen(tmpout, "a")) != (FILE *)NULL)) {

          for (i = 0; i < MAXCHAN; i++)
	    clearchan(i, 1);

#ifdef Q931AK
          if (q931dmp) {
  	    mymsns         = 3;
  	    mycountry      = "+49";
  	    mycountrynum   = 49;
  	    myarea   	   = "6171";
            currency   	   = NULL;
            dual	   = 1;
  	    chargemax  	   = 0.0;
  	    connectmax 	   = 0.0;
  	    bytemax        = 0.0;
  	    connectmaxmode = 0;
  	    bytemaxmode    = 0;
  	    knowns     	   = retnum = 0;
  	    known      	   = (KNOWN **)NULL;
  	    start_procs.infoargs = NULL;
  	    start_procs.flags    = 0;

  	    preselect = 33;        /* Telekomik */
  	    vbn = "010"; 	   /* Germany */
	    vbnlen = "2:3";

	    setDefaults();
          }
          else
#endif
          {
      	    if (!Q931dmp)
    	      print_msg(PRT_NORMAL, "%s Version %s starting\n", myshortname, VERSION);

            if (readconfig(myshortname) < 0)
              Exit(30);
	    if (checkconfig() < 0)
	      Exit(30);

            restoreCharge();
          } /* if */

    	  if (replay) {
	    sprintf(rlogfile, "%s.rep", logfile);
	    logfile = rlogfile;
	  }
	  else {
            append_element(&devices,isdnctrl);

            switch (i = handle_runfiles(myshortname,devices,START_PROG)) {
              case  0 : break;

              case -1 : print_msg(PRT_ERR,"Can not open pid/lock file: %s!\n", strerror(errno));
            	          Exit(36);

              default : Exit(37);
            } /* switch */
          } /* if */


	  if (!replay) {
	    sockets[ISDNINFO].descriptor = open("/dev/isdn/isdninfo", O_RDONLY | O_NONBLOCK);
	    if (sockets[ISDNINFO].descriptor<0)
	      sockets[ISDNINFO].descriptor = open("/dev/isdninfo", O_RDONLY | O_NONBLOCK);
	  }

          if (replay || (sockets[ISDNINFO].descriptor >= 0)) {

            if (readkeyboard) {
	      raw_mode(1);
	      sockets[STDIN].descriptor = dup(fileno(stdin));
            } /* if */

            now();

#ifdef POSTGRES
            dbOpen();
#endif
#ifdef MYSQLDB
	    mysql_dbOpen();
#endif
#ifdef ORACLE
	    oracle_dbOpen();
#endif

	    sprintf(s, "%s%s", mycountry, myarea);
            mynum = strdup(s);
            myicountry = atoi(mycountry + strlen(countryprefix));

      	    if (!Q931dmp) {
	    initHoliday(holifile, &version);

	    if (*version)
	      print_msg(PRT_NORMAL, "%s\n", version);

	    initDest(destfile, &version);

	    if (*version)
	      print_msg(PRT_NORMAL, "%s\n", version);

	    initRate(rateconf, ratefile, zonefile, &version);

	    if (*version)
	      print_msg(PRT_NORMAL, "%s\n", version);
	    } /* if */

	    if (sqldump) {
	      auto     FILE *fo = fopen(((sqldump == 2) ? "/tmp/isdnconf.csv" : "/tmp/isdn.conf.sql"), "w");
              register int   i;
	      register char *p1, *p2;


              if (fo != (FILE *)NULL) {
                if (sqldump == 1) {
                  fprintf(fo, "USE isdn;\n");
                  fprintf(fo, "DROP TABLE IF EXISTS conf;\n");
                  fprintf(fo, "CREATE TABLE conf (\n");
                  fprintf(fo, "   MSN char(32) NOT NULL,\n");
                  fprintf(fo, "   SI tinyint(1) DEFAULT '1' NOT NULL,\n");
                  fprintf(fo, "   ALIAS char(64) NOT NULL,\n");
                  fprintf(fo, "   KEY MSN (MSN)\n");
                  fprintf(fo, ");\n");
                }
                else
		  fprintf(fo, "\"Vorname\",\"Nachname\",\"Firma\",\"Straﬂe gesch‰ftlich\",\"Ort gesch‰ftlich\",\"Postleitzahl gesch‰ftlich\",\"Fax gesch‰ftlich\",\"Telefon gesch‰ftlich\",\"Mobiltelefon\",\"E-Mail-Adresse\",\"E-Mail: Angezeigter Name\",\"Geburtstag\",\"Webseite\"\n");

      	        for (i = 0; i < knowns; i++) {
                  p1 = known[i]->num;
                  while ((p2 = strchr(p1, ','))) {
                    *p2 = 0;

                    if (sqldump == 1)
              	      fprintf(fo, "INSERT INTO conf VALUES('%s',%d,'%s');\n",
  		        p1, known[i]->si, known[i]->who);
                    else
		      fprintf(fo, "\"\",\"%s\",\"\",\"\",\"\",\"\",\"\",\"%s\",\"\",\"\",\"\",\"\",\"\"\n",
		        known[i]->who, p1);

              	    *p2 = ',';
              	    p1 = p2 + 1;

                    while (*p1 == ' ')
                      p1++;
                  } /* while */

                  if (sqldump == 1)
                    fprintf(fo, "INSERT INTO conf VALUES('%s',%d,'%s');\n",
                      p1, known[i]->si, known[i]->who);
                    else
		      fprintf(fo, "\"\",\"%s\",\"\",\"\",\"\",\"\",\"\",\"%s\",\"\",\"\",\"\",\"\",\"\"\n",
		        known[i]->who, p1);
		} /* for */
              } /* if */

              fclose(fo);
              exit(0);
      	    } /* if */

            loop();

            if (sockets[ISDNINFO].descriptor >= 0)
              close(sockets[ISDNINFO].descriptor);
	  }
          else {
            print_msg(PRT_ERR, msg1, myshortname, "/dev/isdninfo", strerror(errno));
            res = 7;
          } /* else */

          if (verbose)
            fclose(fprot);
        }
        else {
          print_msg(PRT_ERR, msg1, myshortname, tmpout, strerror(errno));
          res = 5;
        } /* else */

        if (ptty != NULL)
          fclose(fcons);
      }
      else {
        print_msg(PRT_ERR, msg1, myshortname, ptty, strerror(errno));
        res = 3;
      } /* else */

      if (strcmp(isdnctrl, "-"))
        close(sockets[ISDNCTRL].descriptor);
      if (*isdnctrl2)
        close(sockets[ISDNCTRL2].descriptor);

      if (readkeyboard) {
        raw_mode(0);
      	close(sockets[STDIN].descriptor);
      } /* if */
    }
    else {
      print_msg(PRT_ERR, msg1, myshortname, isdnctrl, strerror(errno));
      res = 2;
    } /* else */

    if (!Q931dmp)
      print_msg(PRT_NORMAL, "%s Version %s exiting\n", myshortname, VERSION);

  } /* else */

  Exit(res);
  return(res);
} /* main */
/* vim:set ts=2: */
