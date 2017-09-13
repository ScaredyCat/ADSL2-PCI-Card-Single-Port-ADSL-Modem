/* $Id: tools.h,v 1.57 2000/09/05 08:05:03 paul Exp $
 *
 * ISDN accounting for isdn4linux.
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Log: tools.h,v $
 * Revision 1.57  2000/09/05 08:05:03  paul
 * Now isdnlog doesn't use any more ISDN_XX defines to determine the way it works.
 * It now uses the value of "COUNTRYCODE = 999" to determine the country, and sets
 * a variable mycountrynum to that value. That is then used in the code to set the
 * way isdnlog works.
 * It works for me, please check it! No configure.in / doc changes yet until
 * it has been checked to work.
 * So finally a version of isdnlog that can be compiled and distributed
 * internationally.
 *
 * Revision 1.56  2000/08/06 13:06:53  akool
 * isdnlog-4.38
 *  - isdnlog now uses ioctl(IIOCNETGPN) to associate phone numbers, interfaces
 *    and slots in "/dev/isdninfo".
 *    This requires a Linux-Kernel 2.2.12 or better.
 *    Support for older Kernel's are implemented.
 *    If IIOCNETGPN is available, the entries "INTERFACE = xxx" in
 *    "/etc/isdn/isdn.conf" are obsolete.
 *  - added 01013:Tele2 totally Freecall on 12. and 13. August 2000
 *  - resolved *any* warning's from "rate-de.dat" (once more ...)
 *  - Patch from oliver@escape.de (Oliver Wellnitz) against
 *  	 "Ziffernwahl verschluckt Nummern"
 *
 *    **Please "make clean" before using this version of isdnlog!!**
 *
 * Revision 1.55  2000/06/29 17:38:28  akool
 *  - Ported "imontty", "isdnctrl", "isdnlog", "xmonisdn" and "hisaxctrl" to
 *    Linux-2.4 "devfs" ("/dev/isdnctrl" -> "/dev/isdn/isdnctrl")
 *
 * Revision 1.54  2000/03/09 18:50:03  akool
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
 * Revision 1.53  2000/03/06 07:03:21  akool
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
 * Revision 1.52  2000/02/11 10:41:53  akool
 * isdnlog-4.10
 *  - Set CHARGEINT to 11 if < 11
 *  - new Option "-dx" controls ABC_LCR feature (see README for infos)
 *  - new rates
 *
 * Revision 1.51  1999/12/31 13:57:20  akool
 * isdnlog-4.00 (Millenium-Edition)
 *  - Oracle support added by Jan Bolt (Jan.Bolt@t-online.de)
 *  - resolved *any* warnings against rate-de.dat
 *  - Many new rates
 *  - CREDITS file added
 *
 * Revision 1.50  1999/11/02 21:01:58  akool
 * isdnlog-3.62
 *  - many new rates
 *  - next try to fix "Sonderrufnummern"
 *
 * Revision 1.49  1999/10/29 19:46:01  akool
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
 * Revision 1.48  1999/10/25 18:30:04  akool
 * isdnlog-3.57
 *   WARNING: Experimental version!
 *   	   Please use isdnlog-3.56 for production systems!
 *
 * Revision 1.47  1999/06/28 19:16:54  akool
 * isdnlog Version 3.38
 *   - new utility "isdnrate" started
 *
 * Revision 1.46  1999/06/16 23:38:09  akool
 * fixed zone-processing
 *
 * Revision 1.45  1999/06/15 20:05:22  akool
 * isdnlog Version 3.33
 *   - big step in using the new zone files
 *   - *This*is*not*a*production*ready*isdnlog*!!
 *   - Maybe the last release before the I4L meeting in Nuernberg
 *
 * Revision 1.44  1999/06/13 14:08:28  akool
 * isdnlog Version 3.32
 *
 *  - new option "-U1" (or "ignoreCOLP=1") to ignore CLIP/COLP Frames
 *  - TEI management decoded
 *
 * Revision 1.43  1999/06/03 18:51:25  akool
 * isdnlog Version 3.30
 *  - rate-de.dat V:1.02-Germany [03-Jun-1999 19:49:22]
 *  - small fixes
 *
 * Revision 1.42  1999/05/22 10:19:36  akool
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
 * Revision 1.41  1999/05/13 11:40:11  akool
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
 * Revision 1.40  1999/05/09 18:24:31  akool
 * isdnlog Version 3.25
 *
 *  - README: isdnconf: new features explained
 *  - rate-de.dat: many new rates from the I4L-Tarifdatenbank-Crew
 *  - added the ability to directly enter a country-name into "rate-xx.dat"
 *
 * Revision 1.39  1999/05/04 19:33:50  akool
 * isdnlog Version 3.24
 *
 *  - fully removed "sondernummern.c"
 *  - removed "gcc -Wall" warnings in ASN.1 Parser
 *  - many new entries for "rate-de.dat"
 *  - better "isdnconf" utility
 *
 * Revision 1.38  1999/04/30 19:08:27  akool
 * isdnlog Version 3.23
 *
 *  - changed LCR probing duration from 181 seconds to 153 seconds
 *  - "rate-de.dat" filled with May, 1. rates
 *
 * Revision 1.37  1999/04/16 14:40:07  akool
 * isdnlog Version 3.16
 *
 * - more syntax checks for "rate-xx.dat"
 * - isdnrep fixed
 *
 * Revision 1.36  1999/04/14 13:17:30  akool
 * isdnlog Version 3.14
 *
 * - "make install" now install's "rate-xx.dat", "rate.conf" and "ausland.dat"
 * - "holiday-xx.dat" Version 1.1
 * - many rate fixes (Thanks again to Michael Reinelt <reinelt@eunet.at>)
 *
 * Revision 1.35  1999/04/10 16:36:48  akool
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
 * Revision 1.34  1999/04/03 12:47:50  akool
 * - isdnlog Version 3.12
 * - "%B" tag in ILABEL/OLABEL corrected
 * - isdnlog now register's incoming calls when there are no free B-channels
 *   (idea from sergio@webmedia.es)
 * - better "samples/rate.conf.de" (suppress provider without true call-by-call)
 * - "tarif.dat" V:1.17 [03-Apr-99]
 * - Added EWE-Tel rates from Reiner Klaproth <rk1@msjohan.dd.sn.schule.de>
 * - isdnconf can now be used to generate a Least-cost-router table
 *   (try "isdnconf -c .")
 * - isdnlog now simulate a RELEASE COMPLETE if nothing happpens after a SETUP
 * - CHARGEMAX Patches from Oliver Lauer <Oliver.Lauer@coburg.baynet.de>
 *
 * Revision 1.33  1999/03/24 19:39:06  akool
 * - isdnlog Version 3.10
 * - moved "sondernnummern.c" from isdnlog/ to tools/
 * - "holiday.c" and "rate.c" integrated
 * - NetCologne rates from Oliver Flimm <flimm@ph-cip.uni-koeln.de>
 * - corrected UUnet and T-Online rates
 *
 * Revision 1.32  1999/03/20 16:55:27  akool
 * isdnlog 3.09 : support for all Internet-by-call numbers
 *
 * Revision 1.31  1999/03/20 14:34:17  akool
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
 * Revision 1.30  1999/03/15 21:28:54  akool
 * - isdnlog Version 3.06
 * - README: explain some terms about LCR, corrected "-c" Option of "isdnconf"
 * - isdnconf: added a small LCR-feature - simply try "isdnconf -c 069"
 * - isdnlog: dont change CHARGEINT, if rate is't known!
 * - sonderrufnummern 1.02 [15-Mar-99] :: added WorldCom
 * - tarif.dat 1.09 [15-Mar-99] :: added WorldCom
 * - isdnlog now correctly handles the new "Ortstarif-Zugang" of UUnet
 *
 * Revision 1.29  1999/03/14 14:27:37  akool
 * - isdnlog Version 3.05
 * - new Option "-u1" (or "ignoreRR=1")
 * - added version information to "sonderrufnummern.dat"
 * - added debug messages if sonderrufnummern.dat or tarif.dat could not be opened
 * - sonderrufnummern.dat V 1.01 - new 01805 rates
 *
 * Revision 1.28  1999/03/07 18:20:11  akool
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
 * Revision 1.27  1999/02/28 19:33:52  akool
 * Fixed a typo in isdnconf.c from Andreas Jaeger <aj@arthur.rhein-neckar.de>
 * CHARGEMAX fix from Oliver Lauer <Oliver.Lauer@coburg.baynet.de>
 * isdnrep fix from reinhard.karcher@dpk.berlin.fido.de (Reinhard Karcher)
 * "takt_at.c" fixes from Ulrich Leodolter <u.leodolter@xpoint.at>
 * sondernummern.c from Mario Joussen <mario.joussen@post.rwth-aachen.de>
 * Reenable usage of the ZONE entry from Schlottmann-Goedde@t-online.de
 * Fixed a typo in callerid.conf.5
 *
 * Revision 1.26  1999/01/24 19:02:51  akool
 *  - second version of the new chargeint database
 *  - isdnrep reanimated
 *
 * Revision 1.25  1999/01/10 15:24:36  akool
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
 * Revision 1.24  1998/12/09 20:40:27  akool
 *  - new option "-0x:y" for leading zero stripping on internal S0-Bus
 *  - new option "-o" to suppress causes of other ISDN-Equipment
 *  - more support for the internal S0-bus
 *  - Patches from Jochen Erwied <mack@Joker.E.Ruhr.DE>, fixes TelDaFax Tarif
 *  - workaround from Sebastian Kanthak <sebastian.kanthak@muehlheim.de>
 *  - new CHARGEINT chapter in the README from
 *    "Georg v.Zezschwitz" <gvz@popocate.hamburg.pop.de>
 *
 * Revision 1.23  1998/11/24 20:53:10  akool
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
 * Revision 1.22  1998/11/01 08:50:35  akool
 *  - fixed "configure.in" problem with NATION_*
 *  - DESTDIR fixes (many thanks to Michael Reinelt <reinelt@eunet.at>)
 *  - isdnrep: Outgoing calls ordered by Zone/Provider/MSN corrected
 *  - new Switch "-i" -> running on internal S0-Bus
 *  - more providers
 *  - "sonderrufnummern.dat" extended (Frag Fred, Telegate ...)
 *  - added AVM-B1 to the documentation
 *  - removed the word "Teles" from the whole documentation ;-)
 *
 * Revision 1.21  1998/10/18 20:13:44  luethje
 * isdnlog: Added the switch -K
 *
 * Revision 1.20  1998/09/26 18:30:18  akool
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
 * Revision 1.19  1998/06/21 11:53:27  akool
 * First step to let isdnlog generate his own AOCD messages
 *
 * Revision 1.18  1998/06/07 21:10:02  akool
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
 * Revision 1.17  1998/03/08 11:43:18  luethje
 * I4L-Meeting Wuerzburg final Edition, golden code - Service Pack number One
 *
 * Revision 1.16  1997/05/29 17:07:30  akool
 * 1TR6 fix
 * suppress some noisy messages (Bearer, Channel, Progress) - can be reenabled with log-level 0x1000
 * fix from Bodo Bellut (bodo@garfield.ping.de)
 * fix from Ingo Schneider (schneidi@informatik.tu-muenchen.de)
 * limited support for Info-Element 0x76 (Redirection number)
 *
 * Revision 1.15  1997/05/25 19:41:16  luethje
 * isdnlog:  close all files and open again after kill -HUP
 * isdnrep:  support vbox version 2.0
 * isdnconf: changes by Roderich Schupp <roderich@syntec.m.EUnet.de>
 * conffile: ignore spaces at the end of a line
 *
 * Revision 1.14  1997/05/15 22:21:49  luethje
 * New feature: isdnrep can transmit via HTTP fax files and vbox files.
 *
 * Revision 1.13  1997/05/09 23:31:00  luethje
 * isdnlog: new switch -O
 * isdnrep: new format %S
 * bugfix in handle_runfiles()
 *
 * Revision 1.12  1997/05/04 20:20:05  luethje
 * README completed
 * isdnrep finished
 * interval-bug fixed
 *
 * Revision 1.11  1997/04/20 22:52:36  luethje
 * isdnrep has new features:
 *   -variable format string
 *   -can create html output (option -w1 or ln -s isdnrep isdnrep.cgi)
 *    idea and design from Dirk Staneker (dirk.staneker@student.uni-tuebingen.de)
 * bugfix of processor.c from akool
 *
 * Revision 1.10  1997/04/15 22:37:13  luethje
 * allows the character `"' in the program argument like the shell.
 * some bugfixes.
 *
 * Revision 1.9  1997/04/03 22:40:21  luethje
 * some bugfixes.
 *
 * Revision 1.8  1997/03/31 22:43:18  luethje
 * Improved performance of the isdnrep, made some changes of README
 *
 * Revision 1.7  1997/03/29 09:24:34  akool
 * CLIP presentation enhanced, new ILABEL/OLABEL operators
 *
 * Revision 1.6  1997/03/23 23:12:05  luethje
 * improved performance
 *
 * Revision 1.5  1997/03/23 21:04:10  luethje
 * some bugfixes
 *
 * Revision 1.4  1997/03/20 22:42:41  akool
 * Some minor enhancements.
 *
 * Revision 1.3  1997/03/20 00:19:18  luethje
 * inserted the line #include <errno.h> in avmb1/avmcapictrl.c and imon/imon.c,
 * some bugfixes, new structure in isdnlog/isdnrep/isdnrep.c.
 *
 * Revision 1.2  1997/03/17 23:21:08  luethje
 * README completed, new funktion Compare_Sections() written, "GNU_SOURCE 1"
 * added to tools.h and a sample file added.
 *
 * Revision 1.1  1997/03/16 20:59:25  luethje
 * Added the source code isdnlog. isdnlog is not working yet.
 * A workaround for that problem:
 * copy lib/policy.h into the root directory of isdn4k-utils.
 *
 * Revision 2.6.36  1997/02/10  09:30:43  akool
 * MAXCARDS implemented
 *
 * Revision 2.6.25  1997/01/17  23:30:43  akool
 * City Weekend Tarif implemented (Thanks to Oliver Schoett <schoett@muc.de>)
 *
 * Revision 2.6.20  1997/01/05  20:05:43  akool
 * neue "AreaCode" Release implemented
 *
 * Revision 2.6.15  1997/01/02  19:51:43  akool
 * CHARGEMAX erweitert
 * CONNECTMAX implementiert
 *
 * Revision 2.40    1996/06/19  17:45:43  akool
 * double2byte(), time2str() added
 *
 * Revision 2.3.26  1996/05/05  12:07:43  akool
 * known.interface added
 *
 * Revision 2.3.23  1996/04/28  12:16:43  akool
 * confdir()
 *
 * Revision 2.2.5  1996/03/25  19:17:43  akool
 * 1TR6 causes implemented
 *
 * Revision 2.23  1996/03/24  12:11:43  akool
 * Explicit decl. of basename() - new "unistd.h" dont have one
 *
 * Revision 2.15  1996/02/21  20:14:43  akool
 *
 * Revision 2.12  1996/02/13  20:08:43  root
 * Nu geht's (oder?)
 *
 * Revision 2.12  1996/02/13  20:08:43  root
 * Nu geht's (oder?)
 *
 * Revision 1.2  1996/02/13  20:05:28  root
 * so nun gehts
 *
 * Revision 1.1  1996/02/13  14:28:14  root
 * Initial revision
 *
 * Revision 2.05  1995/02/11  17:10:16  akool
 *
 */

/****************************************************************************/

#ifndef _TOOLS_H_
#define _TOOLS_H_

/****************************************************************************/

#define _GNU_SOURCE 1

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <malloc.h>
#include <time.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <fcntl.h>
#include <ctype.h>
#include <limits.h>
#include <signal.h>
#include <math.h>
#include <syslog.h>
#include <sys/ioctl.h>
#ifdef linux
#include <sys/kd.h>
#include <linux/isdn.h>
#else
#include <libgen.h>
#endif
#ifdef DBMALLOC
#include "dbmalloc.h"
#endif

/****************************************************************************/

#include "policy.h"
#include "libisdn.h"
#include "holiday.h"
#include "country.h"
#include "rate.h"

/****************************************************************************/

#ifndef OLDCONFFILE
#	define OLDCONFFILE "isdnlog.conf"
#endif

#ifndef RELOADCMD
#	define RELOADCMD "reload"
#endif

#ifndef STOPCMD
#	define STOPCMD "stop"
#endif

#ifndef REBOOTCMD
#	define REBOOTCMD "/sbin/reboot"
#endif

#ifndef LOGFILE
#	define LOGFILE "/sbin/reboot"
#endif

/****************************************************************************/

#define LOG_VERSION "3.2"

/****************************************************************************/

#undef  min
#undef  max
#define max(a,b)        (((a) > (b)) ? (a) : (b))
#define min(a,b)        (((a) < (b)) ? (a) : (b))
#define	abs(x)		(((x) < 0) ? -(x) : (x))

#define UNKNOWN		-1
#define	UNDEFINED	-2

/****************************************************************************/

#define MAXDIG 128
#define GETARG(typ)     va_arg(ap, typ)

/****************************************************************************/

#define NIL  (char **)NULL

/****************************************************************************/

#define NUMSIZE    (ISDN_MSNLEN + 1)
#define	FNSIZE	      64
#define RETSIZE      128
#define MAXRET	       5
#define MAXPROVIDER 1000
#define MAXZONES     500
#define MAXCHAN        7
#define MAXCARDS       2

#define DIGITS 	     17
#define DEB           1

#define MAXUNKNOWN   50
#define MAXCONNECTS  50

/****************************************************************************/
#if 0 /* Fixme: remove */
#define SONDERNUMMER -2 /* FIXME: set by readconfig(), but unused by now */
#endif

#define INTERN	      0
#define FREECALL      0

#if 0 /* Fixme: remove */
#define	LOCALCALL     1
#define CITYCALL      2
#define REGIOCALL     3
#define GERMANCALL    4
#define C_MOBILBOX   10
#define C_NETZ       10
#define D1_NETZ      10
#define D2_NETZ      10
#define E_PLUS_NETZ  10
#define E2_NETZ      10
#define INTERNET    100
#define	AUKUNFT_IN   40
#define AUSKUNFT_AUS 41
#endif

/* this is specific to Germany */
#define	DTAG	     33

#define	LCR_DURATION 153

/****************************************************************************/

#define CALLING       0
#define CALLED        1
#define DATETIME      2

/****************************************************************************/

#define DIALOUT	      0
#define DIALIN	      1

/****************************************************************************/

#define ALERTING               0x01
#define CALL_PROCEEDING	       0x02
#define SETUP                  0x05
#define SETUP_ACKNOWLEDGE      0x0d
#define SUSPEND                0x25
#define SUSPEND_ACKNOWLEDGE    0x2d
#define RESUME                 0x26
#define RESUME_ACKNOWLEDGE     0x2e
#define CONNECT                0x07
#define CONNECT_ACKNOWLEDGE    0x0f
#define FACILITY               0x62
#define NOTIFY                 0x6e
#define STATUS                 0x7d
#define MAKEL_ACKNOWLEDGE      0x28
#define MAKEL_RESUME_ACK       0x33
#define DISCONNECT             0x45
#define RELEASE                0x4d
#define RELEASE_COMPLETE       0x5a
#define INFORMATION	       0x7b
#define AOCD_1TR6              0x6d

/****************************************************************************/

#define AOC_UNITS	        0
#define AOC_AMOUNT              1

/****************************************************************************/

#define	RING_INCOMING    1
#define RING_OUTGOING    2
#define	RING_RING        4
#define RING_CONNECT     8
#define RING_BUSY       16
#define RING_AOCD       32
#define RING_ERROR      64
#define RING_HANGUP    128
#define	RING_KILL      256
#define RING_SPEAK     512
#define RING_PROVIDER 1024
#define RING_LOOP     2048
#define RING_UNIQUE   4096
#define RING_INTERVAL 8192

/****************************************************************************/

#define STATE_RING		  1 /* "Telefonklingeln" ... jemand ruft an, oder man selbst ruft raus */
#define STATE_CONNECT	  	  2 /* Verbindung */
#define STATE_HANGUP	  	  3 /* Verbindung beendet */
#define STATE_AOCD		100 /* Gebuehrenimpuls _waehrend_ der Verbindung */
#define	STATE_CAUSE		101 /* Aussergewoehnliche Cause-Meldungen von der VSt */
#define STATE_TIME		102 /* Uhrzeit-Meldung von der VSt */
#define STATE_BYTE		103 /* Durchsatz-Meldung von Frank (Byte/s/B-Kanal) */
#define	STATE_HUPTIMEOUT	104 /* Wechsel des hangup-Timer's  */

/****************************************************************************/

#define AOC_OTHER          -999999L

/****************************************************************************/

#define QCMASK  	       0377
#define QUOTE   	       0200
#define QMASK      (QCMASK &~QUOTE)
#define NOT                     '!'
#if 0 /* Fixme: remove */
#define	AVON	             "avon"
#endif

#define	BIGBUFSIZ              2048

/****************************************************************************/

#define VAR_START      "START"
#define VAR_MYMSNS     "MYMSNS"
#define VAR_MYCOUNTRY  "MYAREA"
#define VAR_MYAREA     "MYPREFIX"
#define VAR_CURRENCY   "CURRENCY"
#define VAR_ILABEL     "ILABEL"
#define VAR_OLABEL     "OLABEL"
#define VAR_CHARGEMAX  "CHARGEMAX"
#define VAR_CONNECTMAX "CONNECTMAX"
#define VAR_BYTEMAX    "BYTEMAX"

/****************************************************************************/

#define VERSION_UNKNOWN       0
#define	VERSION_EDSS1	      1
#define	VERSION_1TR6	      2

#define DEF_NUM_MSN 3

/****************************************************************************/

#define  OTHER (call[chan].dialin ? CALLING : CALLED)
#define  ME    (call[chan].dialin ? CALLED : CALLING)
#define	 CLIP  2
#define	 REDIR 3
#define  _OTHER(call) (call->dialin ? CALLING : CALLED)
#define  _ME(call)    (call->dialin ? CALLED : CALLING)

#define	 MAXMSNS  (REDIR + 1)

/****************************************************************************/

#define SHORT_STRING_SIZE      256
#define LONG_STRING_SIZE      1024
#define BUF_SIZE              4096

/****************************************************************************/

/* Keywords for parameter file */

#define CONF_SEC_OPT     "OPTIONS"
#define CONF_ENT_DEV     "DEVICE"
#define CONF_ENT_LOG     "LOG"
#define CONF_ENT_FLUSH   "FLUSH"
#define CONF_ENT_PORT    "PORT"
#define CONF_ENT_STDOUT  "STDOUT"
#define CONF_ENT_SYSLOG  "SYSLOG"
#define CONF_ENT_XISDN   "XISDN"
#define CONF_ENT_TIME    "TIME"
#define CONF_ENT_CON     "CONSOLE"
#define CONF_ENT_START   "START"
#define CONF_ENT_THRU    "THRUPUT"
#define CONF_ENT_DAEMON  "DAEMON"
#define CONF_ENT_PIPE    "PIPE"
#define CONF_ENT_BI      "BILINGUAL"
#define CONF_ENT_MON     "MONITOR"
#define CONF_ENT_HANGUP  "HANGUP"
#define CONF_ENT_CALLS   "CALLS"
#define CONF_ENT_XLOG    "XLOG"
#define CONF_ENT_NL			 "NEWLINE"
#define CONF_ENT_WIDTH	 "WIDTH"
#define CONF_ENT_WD			 "WATCHDOG"
#define CONF_ENT_AMT		 "AMT"
#define CONF_ENT_DUAL		 "DUAL"
#define CONF_ENT_Q931		 "Q931DUMP"
#define CONF_ENT_OUTFILE "OUTFILE"
#define CONF_ENT_KEYBOARD "KEYBOARD"
#define CONF_ENT_INTERNS0 "INTERNS0"
#define CONF_ENT_PRESELECT "PRESELECTED"
#define	CONF_ENT_TRIM	   "TRIM"
#define	CONF_ENT_OTHER	   "OTHER"
#define CONF_ENT_IGNORERR  "IGNORERR"
#define CONF_ENT_IGNORECOLP "IGNORECOLP"
#define	CONF_ENT_VBN	   "VBN"
#define	CONF_ENT_VBNLEN	   "VBNLEN"
#define CONF_ENT_CIINTERVAL "CIINTERVAL"
#define CONF_ENT_ABCLCR	"ABCLCR"
#define CONF_ENT_PROVIDERCHANGE "PROVIDERCHANGE"
/****************************************************************************/

/* Keywords for isdn.conf */

#define CONF_SEC_ISDNLOG  "ISDNLOG"
#define CONF_ENT_CHARGE   "CHARGEMAX"
#define CONF_ENT_CONNECT  "CONNECTMAX"
#define CONF_ENT_BYTE     "BYTEMAX"
#define CONF_ENT_CURR     "CURRENCY"
#define CONF_ENT_ILABEL   "ILABEL"
#define CONF_ENT_OLABEL   "OLABEL"
#define CONF_ENT_RELOAD   "RELOADCMD"
#define CONF_ENT_STOP     "STOPCMD"
#define CONF_ENT_REBOOT   "REBOOTCMD"
#define CONF_ENT_LOGFILE  "LOGFILE"

#define CONF_SEC_START    "START"
#define CONF_SEC_FLAG     "FLAG"
#define CONF_ENT_FLAGS    "FLAGS"
#define CONF_ENT_PROG     "PROGRAM"
#define CONF_ENT_USER     "USER"
#define CONF_ENT_GROUP    "GROUP"
#define CONF_ENT_INTVAL   "INTERVAL"
#define CONF_ENT_TIME     "TIME"

#define CONF_ENT_REPFMT   "REPFMT"

#define CONF_ENT_CALLFILE "CALLFILE"
#define CONF_ENT_CALLFMT  "CALLFMT"

#define CONF_ENT_HOLIFILE    "HOLIDAYS"
#define CONF_ENT_DESTFILE    "DESTFILE"
#define CONF_ENT_COUNTRYFILE "COUNTRYFILE"
#define CONF_ENT_ZONEFILE    "ZONEFILE"
#define CONF_ENT_RATECONF    "RATECONF"
#define CONF_ENT_RATEFILE    "RATEFILE"
#define CONF_ENT_LCDFILE     "LCDFILE"

#define CONF_ENT_VBOXVER  "VBOXVERSION"
#define CONF_ENT_VBOXPATH "VBOXPATH"
#define CONF_ENT_VBOXCMD1 "VBOXCMD1"
#define CONF_ENT_VBOXCMD2 "VBOXCMD2"
#define CONF_ENT_MGTYVER  "MGETTYVERSION"
#define CONF_ENT_MGTYPATH "MGETTYPATH"
#define CONF_ENT_MGTYCMD  "MGETTYCMD"

/****************************************************************************/

#define PRT_ERR                1
#define PRT_WARN               2
#define PRT_INFO               4
#define PRT_PROG_OUT           4
#define PRT_NORMAL             4
#define PRT_LOG                8

/****************************************************************************/

#define NO_MSN       -1

#define C_FLAG_DELIM '|'

/****************************************************************************/

#define C_UNKNOWN '?'
#define S_UNKNOWN "UNKNOWN"

/****************************************************************************/

#define S_QUOTES "\\$@;,#"

/****************************************************************************/

#define TYPE_STRING   0
#define TYPE_MESSAGE  1
#define TYPE_ELEMENT  2
#define TYPE_CAUSE    3
#define TYPE_SERVICE  4

/****************************************************************************/

typedef struct {
  int	  state;
  int     cref;
  int     tei;
  int	  dialin;
  int	  cause;
  int	  loc;
  int	  aoce;
  int	  traffic;
  int	  channel;
  int	  dialog;
  int     bearer;
  int	  si1;     /* Service Indicator entsprechend i4l convention */
  int	  si11;	   /* if (si1 == 1) :: 0 = Telefon analog / 1 = Telefon digital */
  char    onum[MAXMSNS][NUMSIZE];
  int	  screening;
  char    num[MAXMSNS][NUMSIZE];
  char    vnum[MAXMSNS][256];
  int	  provider;
  int	  sondernummer[MAXMSNS];
  int	  local[MAXMSNS];
  int	  intern[MAXMSNS];
  char    id[32];
  char	  usage[16];
  int	  confentry[MAXMSNS];
  time_t  time;
  time_t  connect;
  time_t  t_duration;
  time_t  disconnect;
  clock_t duration;
  int     cur_event;
  long	  ibytes;
  long	  obytes;
  long	  libytes;
  long	  lobytes;
  double  ibps;
  double  obps;
  char	  areacode[MAXMSNS][NUMSIZE];
  char	  vorwahl[MAXMSNS][NUMSIZE];
  char	  rufnummer[MAXMSNS][NUMSIZE];
  char	  alias[MAXMSNS][NUMSIZE];
  char	  area[MAXMSNS][128];
  char	  money[64];
  char	  currency[32];
  char    msg[BUFSIZ];
  int     stat;
  int	  version;
  int	  bchan;
  double  tick;
  int	  chargeint;
  int     huptimeout;
  char	  service[32];
  double  pay;
  double  aocpay;
  char	  digits[NUMSIZE];
  int	  oc3;
  int	  takteChargeInt;
  int 	  card;
  int	  knock;
  time_t  lastcint;
  time_t  lasteh;
  float	  cint;
  int     cinth;
  int	  ctakt;
  int	  zone; /* Fixme: zone is in Rate : _zone */
  int	  uid;
  int	  hint;
  int	  tz;
  int	  tarifknown;
  RATE    Rate;
  char	  interface[10];
  char    fnum[NUMSIZE];
} CALL;

/****************************************************************************/

typedef struct {
  int	  flag;
  char *time;
  char *infoarg;
  int   interval;
  char *user;
  char *group;
/*  char *service; */
} info_args;

/****************************************************************************/

typedef struct {
  char   *num;
  char	 *who;
  int	  zone;
  int	  flags;
  int     si;
  char	 *interface;
  info_args **infoargs;
  /* above from "isdnlog.conf" */
  int	  usage[2];
  double  dur[2];
  int     eh;
  double  pay;
  double  charge;
  double  rcharge;
  double  scharge;
  int	  day;
  int	  month;
  double  online;
  double  sonline;
  double  bytes;
  double  sbytes;
  double  ibytes[2];
  double  obytes[2];
} KNOWN;


/****************************************************************************/

typedef struct {
  unsigned long i;
  unsigned long o;
} IO;

/****************************************************************************/

typedef struct {
  char id[20];
  int  ch;
  int  dr;
  int  u;
  int  f;
  char n[20];
} IFO;

/****************************************************************************/

#ifdef _TOOLS_C_
#define _EXTERN
#else
#define _EXTERN extern

_EXTERN char     Months[][4];

#endif /* _TOOLS_C_ */

_EXTERN KNOWN    start_procs;
_EXTERN KNOWN  **known;
_EXTERN int      mymsns;
_EXTERN int      knowns;
_EXTERN int	currency_mode;
_EXTERN double   currency_factor;
_EXTERN double   chargemax;
_EXTERN double   connectmax;
_EXTERN double   bytemax;
_EXTERN int   	connectmaxmode;
_EXTERN int   	bytemaxmode;
_EXTERN char    *currency;
_EXTERN int	 hour;
_EXTERN int      day;
_EXTERN int      month;
_EXTERN int      retnum;
_EXTERN int      ln;
_EXTERN char     retstr[MAXRET + 1][RETSIZE];
_EXTERN time_t   cur_time;
_EXTERN section *conf_dat;
_EXTERN char     ilabel[256];
_EXTERN char    	olabel[256];
_EXTERN char    	idate[256];
_EXTERN CALL    	call[MAXCHAN];
#ifdef Q931
_EXTERN int     	q931dmp;
#else
#define q931dmp 0
#endif
#if 0 /* Fixme: remove */
_EXTERN int     	CityWeekend;
#endif
_EXTERN	int	 preselect;
_EXTERN int	dual;
_EXTERN int	hfcdual;
_EXTERN int	abclcr;
_EXTERN char  * providerchange;
_EXTERN int	ciInterval;
_EXTERN int	ehInterval;
_EXTERN char    mlabel[BUFSIZ];
_EXTERN char    *amtsholung;
_EXTERN int	ignoreRR;
_EXTERN int	ignoreCOLP;
_EXTERN int 	interns0;
_EXTERN	char    *vbn;
_EXTERN	char    *vbnlen;
_EXTERN char	*mynum;
_EXTERN int	myicountry;
_EXTERN int	conf_country;	/* replaces the ISDN_xx defines */
#undef _EXTERN

/****************************************************************************/

extern int   optind, errno;
extern char *optarg;

/****************************************************************************/

#ifdef _TOOLS_C_
#define _EXTERN

_EXTERN char* reloadcmd = RELOADCMD;
_EXTERN char* stopcmd   = STOPCMD;
_EXTERN char* rebootcmd = REBOOTCMD;
_EXTERN char* logfile   = LOGFILE;
_EXTERN char* callfile  = NULL;
_EXTERN char* callfmt   = NULL;
_EXTERN char* holifile  = NULL;
_EXTERN char* destfile  = NULL;
_EXTERN char* countryfile = NULL;
_EXTERN char* zonefile  = NULL;
_EXTERN char* rateconf  = NULL;
_EXTERN char* ratefile  = NULL;
_EXTERN char* lcdfile   = NULL;
_EXTERN int  (*_print_msg)(const char *, ...) = printf;
_EXTERN int   use_new_config = 1;
_EXTERN char ***lineformats = NULL;
_EXTERN char *vboxpath      = NULL;
_EXTERN char *vboxcommand1  = NULL;
_EXTERN char *vboxcommand2  = NULL;
_EXTERN char *mgettypath    = NULL;
_EXTERN char *mgettycommand = NULL;

#else
#define _EXTERN extern

_EXTERN char* reloadcmd;
_EXTERN char* stopcmd;
_EXTERN char* rebootcmd;
_EXTERN char* logfile;
_EXTERN char* callfile;
_EXTERN char* callfmt;
_EXTERN char* holifile;
_EXTERN char* destfile;
_EXTERN char* countryfile;
_EXTERN char* zonefile;
_EXTERN char* rateconf;
_EXTERN char* ratefile;
_EXTERN char* lcdfile;
_EXTERN int  (*_print_msg)(const char *, ...);
_EXTERN int   use_new_config;
_EXTERN char ***lineformats;
_EXTERN char *vboxpath;
_EXTERN char *vboxcommand1;
_EXTERN char *vboxcommand2;
_EXTERN char *mgettypath;
_EXTERN char *mgettycommand;

#endif

_EXTERN void set_print_fct_for_tools(int (*new_print_msg)(const char *, ...));
_EXTERN int    print_version(char *myname);
_EXTERN time_t atom(register char *p);
_EXTERN char  *num2nam(char *num, int si);
_EXTERN char  *double2str(double n, int l, int d, int flags);
_EXTERN char  *double2byte(double bytes);
_EXTERN char  *time2str(time_t sec);
_EXTERN char  *double2clock(double n);
_EXTERN char  *vnum(int chan, int who);
_EXTERN char  *i2a(int n, int l, int base);
_EXTERN int    iprintf(char *obuf, int chan, register char *fmt, ...);
_EXTERN char  *qmsg(int type, int version, int val);
_EXTERN char  *Myname;
_EXTERN	char  *zonen[MAXZONES];
#undef _EXTERN

/****************************************************************************/

#ifdef _ISDNCONF_C_
#define _EXTERN
#else
#define _EXTERN extern
#endif

_EXTERN int    readconfig(char *myname);
_EXTERN void   setDefaults(void);
_EXTERN void   discardconfig(void);
_EXTERN section *read_isdnconf(section **_conf_dat);

#undef _EXTERN

/****************************************************************************/

#endif /* _TOOLS_H_ */
