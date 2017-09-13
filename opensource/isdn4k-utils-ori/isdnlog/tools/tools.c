/* $Id: tools.c,v 1.51 2001/12/30 17:17:41 akool Exp $
 *
 * ISDN accounting for isdn4linux. (Utilities)
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
 * $Log: tools.c,v $
 * Revision 1.51  2001/12/30 17:17:41  akool
 * isdnlog-4.55:
 *   Tatahhh: isdnlog speaks Euro :-)
 *
 * 	Many thanks to Bernhard Schmidt (berni@birkenwald.de)!
 *
 * 	** At least, you have to install "/usr/lib/isdn/rate-de.dat"
 *   ** and modify your "/etc/isdn/isdn.conf" or "/etc/isdn/callerid.conf"
 *   ** to read:
 *   **   [ISDNLOG]
 * 	**	   CURRENCY = 0.062,EUR
 *
 * I wish all of you a happy new year!
 *
 * Revision 1.50  2001/01/14 12:13:50  akool
 * isdnlog-4.49
 *  - added more Euracom decodings
 *
 *  - added new prefixes
 *      0151 - Germany cellphone D1
 *      0152 - Germany cellphone D2
 *      0163 - Germany cellphone Eplus
 *    to "country-de.dat"
 *
 *  - removed Freecall "0130" and "Germany cellphone C"
 *
 * Revision 1.49  2000/06/29 17:38:28  akool
 *  - Ported "imontty", "isdnctrl", "isdnlog", "xmonisdn" and "hisaxctrl" to
 *    Linux-2.4 "devfs" ("/dev/isdnctrl" -> "/dev/isdn/isdnctrl")
 *
 * Revision 1.48  2000/04/02 17:35:07  akool
 * isdnlog-4.18
 *  - isdnlog/isdnlog/isdnlog.8.in  ... documented hup3
 *  - isdnlog/tools/dest.c ... _DEMD1 not recogniced as key
 *  - mySQL Server version 3.22.27 support
 *  - new rates
 *
 * Revision 1.47  2000/03/19 20:26:57  akool
 * isdnlog-4.17
 *  - new rates
 *  - Provider 01080:Telegate aus "samples/rate.conf.de" entfernt, Dienst wurde
 *    eingestellt
 *  - isdnlog/tools/tools.c  ... fixed sarea ($17, $18)
 *
 * Revision 1.46  2000/01/16 12:36:59  akool
 * isdnlog-4.03
 *  - Patch from Gerrit Pape <pape@innominate.de>
 *    fixes html-output if "-t" option of isdnrep is omitted
 *  - Patch from Roland Rosenfeld <roland@spinnaker.de>
 *    fixes "%p" in ILABEL and OLABEL
 *
 * Revision 1.45  2000/01/12 23:22:54  akool
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
 * Revision 1.44  2000/01/01 15:05:24  akool
 * isdnlog-4.01
 *  - first Y2K-Bug fixed
 *
 * Revision 1.43  1999/12/31 13:57:20  akool
 * isdnlog-4.00 (Millenium-Edition)
 *  - Oracle support added by Jan Bolt (Jan.Bolt@t-online.de)
 *  - resolved *any* warnings against rate-de.dat
 *  - Many new rates
 *  - CREDITS file added
 *
 * Revision 1.42  1999/12/24 14:17:06  akool
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
 * Revision 1.41  1999/11/07 13:29:29  akool
 * isdnlog-3.64
 *  - new "Sonderrufnummern" handling
 *
 * Revision 1.40  1999/10/30 18:03:31  akool
 *  - fixed "-q" option
 *  - workaround for "Sonderrufnummern"
 *
 * Revision 1.39  1999/10/29 08:17:02  akool
 *  - new rates
 *
 * Revision 1.38  1999/10/28 18:36:49  akool
 * isdnlog-3.59
 *  - problems with gcc-2.7.2.3 fixed
 *  - *any* startup-warning solved/removed (only 4u, Karsten!)
 *  - many new rates
 *
 * Revision 1.37  1999/10/25 18:30:03  akool
 * isdnlog-3.57
 *   WARNING: Experimental version!
 *   	   Please use isdnlog-3.56 for production systems!
 *
 * Revision 1.36  1999/09/19 14:16:27  akool
 * isdnlog-3.53
 *
 * Revision 1.35  1999/09/13 09:09:44  akool
 * isdnlog-3.51
 *   - changed getProvider() to not return NULL on unknown providers
 *     many thanks to Matthias Eder <mateder@netway.at>
 *   - corrected zone-processing when doing a internal -> world call
 *
 * Revision 1.34  1999/08/29 10:29:15  akool
 * isdnlog-3.48
 *   cosmetics
 *
 * Revision 1.33  1999/08/20 19:29:12  akool
 * isdnlog-3.45
 *  - removed about 1 Mb of (now unused) data files
 *  - replaced areacodes and "vorwahl.dat" support by zone databases
 *  - fixed "Sonderrufnummern"
 *  - rate-de.dat :: V:1.10-Germany [20-Aug-1999 21:23:27]
 *
 * Revision 1.32  1999/07/24 08:45:26  akool
 * isdnlog-3.42
 *   rate-de.dat 1.02-Germany [18-Jul-1999 10:44:21]
 *   better Support for Ackermann Euracom
 *   WEB-Interface for isdnrate
 *   many small fixes
 *
 * Revision 1.31  1999/06/22 19:41:25  akool
 * zone-1.1 fixes
 *
 * Revision 1.30  1999/06/16 19:13:03  akool
 * isdnlog Version 3.34
 *   fixed some memory faults
 *
 * Revision 1.29  1999/06/15 20:05:20  akool
 * isdnlog Version 3.33
 *   - big step in using the new zone files
 *   - *This*is*not*a*production*ready*isdnlog*!!
 *   - Maybe the last release before the I4L meeting in Nuernberg
 *
 * Revision 1.28  1999/06/03 18:51:22  akool
 * isdnlog Version 3.30
 *  - rate-de.dat V:1.02-Germany [03-Jun-1999 19:49:22]
 *  - small fixes
 *
 * Revision 1.27  1999/05/22 10:19:33  akool
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
 * Revision 1.26  1999/05/09 18:24:28  akool
 * isdnlog Version 3.25
 *
 *  - README: isdnconf: new features explained
 *  - rate-de.dat: many new rates from the I4L-Tarifdatenbank-Crew
 *  - added the ability to directly enter a country-name into "rate-xx.dat"
 *
 * Revision 1.25  1999/05/04 19:33:47  akool
 * isdnlog Version 3.24
 *
 *  - fully removed "sondernummern.c"
 *  - removed "gcc -Wall" warnings in ASN.1 Parser
 *  - many new entries for "rate-de.dat"
 *  - better "isdnconf" utility
 *
 * Revision 1.24  1999/04/14 13:17:28  akool
 * isdnlog Version 3.14
 *
 * - "make install" now install's "rate-xx.dat", "rate.conf" and "ausland.dat"
 * - "holiday-xx.dat" Version 1.1
 * - many rate fixes (Thanks again to Michael Reinelt <reinelt@eunet.at>)
 *
 * Revision 1.23  1999/04/10 16:36:46  akool
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
 * Revision 1.22  1999/04/03 12:47:45  akool
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
 * Revision 1.21  1999/03/20 16:55:22  akool
 * isdnlog 3.09 : support for all Internet-by-call numbers
 *
 * Revision 1.20  1999/03/20 14:34:10  akool
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
 * Revision 1.19  1999/03/14 12:16:44  akool
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
 * Revision 1.18  1999/02/28 19:33:48  akool
 * Fixed a typo in isdnconf.c from Andreas Jaeger <aj@arthur.rhein-neckar.de>
 * CHARGEMAX fix from Oliver Lauer <Oliver.Lauer@coburg.baynet.de>
 * isdnrep fix from reinhard.karcher@dpk.berlin.fido.de (Reinhard Karcher)
 * "takt_at.c" fixes from Ulrich Leodolter <u.leodolter@xpoint.at>
 * sondernummern.c from Mario Joussen <mario.joussen@post.rwth-aachen.de>
 * Reenable usage of the ZONE entry from Schlottmann-Goedde@t-online.de
 * Fixed a typo in callerid.conf.5
 *
 * Revision 1.17  1999/01/10 15:24:31  akool
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
 * Revision 1.16  1998/12/09 20:40:19  akool
 *  - new option "-0x:y" for leading zero stripping on internal S0-Bus
 *  - new option "-o" to suppress causes of other ISDN-Equipment
 *  - more support for the internal S0-bus
 *  - Patches from Jochen Erwied <mack@Joker.E.Ruhr.DE>, fixes TelDaFax Tarif
 *  - workaround from Sebastian Kanthak <sebastian.kanthak@muehlheim.de>
 *  - new CHARGEINT chapter in the README from
 *    "Georg v.Zezschwitz" <gvz@popocate.hamburg.pop.de>
 *
 * Revision 1.15  1998/11/24 20:53:07  akool
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
 * Revision 1.14  1998/09/26 18:30:14  akool
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
 * Revision 1.13  1998/06/21 11:53:23  akool
 * First step to let isdnlog generate his own AOCD messages
 *
 * Revision 1.12  1998/06/07 21:09:57  akool
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
 * Revision 1.11  1998/05/06 14:43:27  paul
 * Assumption about country codes always being 2 digits long fixed for the
 * USA case (caused strncpy to be called with length -1; ouch).
 *
 * Revision 1.10  1998/04/09 19:15:45  akool
 *  - CityPlus Implementation from Oliver Lauer <Oliver.Lauer@coburg.baynet.de>
 *  - dont change huptimeout, if disabled (via isdnctrl huptimeout isdnX 0)
 *  - Support for more Providers (TelePassport, Tele 2, TelDaFax)
 *
 * Revision 1.9  1998/03/08 11:43:16  luethje
 * I4L-Meeting Wuerzburg final Edition, golden code - Service Pack number One
 *
 * Revision 1.8  1997/05/11 22:41:43  luethje
 * README completed
 * changed the E-mail address for the switch -V
 *
 * Revision 1.7  1997/04/16 22:23:04  luethje
 * some bugfixes, README completed
 *
 * Revision 1.6  1997/04/08 21:56:59  luethje
 * Create the file isdn.conf
 * some bug fixes for pid and lock file
 * make the prefix of the code in `isdn.conf' variable
 *
 * Revision 1.5  1997/04/06 21:17:46  luethje
 * Bugfix von Andreas Jaeger.
 *
 * Revision 1.4  1997/04/03 22:40:21  luethje
 * some bugfixes.
 *
 * Revision 1.3  1997/03/31 22:15:32  akool
 * added support for the new glibc 2.0.x (aka libc 6.0)
 * changed "HOWTO" to reflect the current stage of development
 *
 * Revision 1.2  1997/03/29 09:24:33  akool
 * CLIP presentation enhanced, new ILABEL/OLABEL operators
 *
 * Revision 1.1  1997/03/16 20:59:24  luethje
 * Added the source code isdnlog. isdnlog is not working yet.
 * A workaround for that problem:
 * copy lib/policy.h into the root directory of isdn4k-utils.
 *
 * Revision 2.6.26  1997/01/19  22:23:43  akool
 * Weitere well-known number's hinzugefuegt
 *
 * Revision 2.6.24  1997/01/15  19:13:43  akool
 * neue AreaCode Lib 0.99 integriert
 *
 * Revision 2.6.20  1997/01/05  20:06:43  akool
 * atom() erkennt nun "non isdnlog" "/tmp/isdnctrl0" Output's
 *
 * Revision 2.6.19  1997/01/05  19:39:43  akool
 * LIBAREA Support added
 *
 * Revision 2.40    1996/06/16  10:06:43  akool
 * double2byte(), time2str() added
 *
 * Revision 2.3.26  1996/05/05  12:09:16  akool
 * known.interface added
 *
 * Revision 2.3.15  1996/04/22  21:10:16  akool
 *
 * Revision 2.3.4  1996/04/05  11:12:16  sl
 * confdir()
 *
 * Revision 2.2.5  1996/03/25  19:41:16  akool
 * 1TR6 causes implemented
 *
 * Revision 2.23  1996/03/14  20:29:16  akool
 * Neue Routine i2a()
 *
 * Revision 2.17  1996/02/25  19:14:16  akool
 * Soft-Error in atom() abgefangen
 *
 * Revision 2.06  1996/02/07  18:49:16  akool
 * AVON-Handling implementiert
 *
 * Revision 2.01  1996/01/20  12:11:16  akool
 * Um Einlesen der neuen isdnlog.conf Felder erweitert
 * discardconfig() implementiert
 *
 * Revision 2.00  1996/01/10  20:11:16  akool
 *
 */

/****************************************************************************/

#define  _TOOLS_C_

/****************************************************************************/

#include "tools.h"
#include "telnum.h"

/****************************************************************************/

/*static char *cclass(register char *p, register int sub);*/

/****************************************************************************/

char Months[][4] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
       	    	     "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

/****************************************************************************/

static int  cnf;

/****************************************************************************/

void set_print_fct_for_tools(int (*new_print_msg)(const char *, ...))
{
	_print_msg = new_print_msg;
	set_print_fct_for_lib(_print_msg);
}

/****************************************************************************/

time_t atom(register char *p)
{
  register char     *p1 = p;
  auto 	   struct tm tm;


#ifdef DEBUG_1
  if (strlen(p) < 20) {
    _print_msg(PRT_DEBUG_GENERAL, " DEBUG> Huch? atom(``%s'')\n", p);
    return((time_t)0);
  } /* if */
#endif

  tm.tm_mon = 0;

  while ((tm.tm_mon < 12) && memcmp(p1, Months[tm.tm_mon], 3)) tm.tm_mon++;

  if (tm.tm_mon == 12)
    return((time_t)-1);

  p1 += 4;
  p = p1 + 2;

  *p = 0;

  day = tm.tm_mday = atoi(p1);

  p1 += 3;
  p = p1 + 2;
  *p = 0;
  tm.tm_hour = atoi(p1);

  p1 = ++p;
  p += 2;
  *p = 0;
  tm.tm_min = atoi(p1);

  p1 = ++p;
  p += 2;
  *p = 0;
  tm.tm_sec = atoi(p1);

  p1 = ++p;
  p += 4;
  *p = 0;

  tm.tm_year = atoi(p1) - 1900;

#ifdef DEBUG_1
  if (tm.tm_year < 1995) {
    _print_msg(PRT_DEBUG_GENERAL, " DEBUG> Huch? atom(): year=%d\n", tm.tm_year);
    return((time_t)0);
  } /* if */
#endif

  tm.tm_wday = tm.tm_yday;
  tm.tm_isdst = UNKNOWN;

  return(mktime(&tm));
} /* atom */

/****************************************************************************/

char *num2nam(char *num, int si)
{
  register int i, n;


  if (*num) {
    for (n = 0; n < 2; n++) {
      for (i = 0; i < knowns; i++) {
        if (((known[i]->si == si) || n) && (!num_match(known[i]->num, num))) {
          if (++retnum == MAXRET)
            retnum = 0;

          cnf = i;
          return(strcpy(retstr[retnum], known[i]->who));
        } /* if */
      } /* for */
    } /* for */
  } /* if */

  cnf = UNKNOWN;
  return("");
} /* num2nam */

/****************************************************************************/

#if defined __GLIBC__ && __GLIBC__ >= 2
char *double2str(double n, int l, int d, int flags)
{
  if (++retnum == MAXRET)
    retnum = 0;

  sprintf(retstr[retnum], "%*.*f", l, d, n);
  return(retstr[retnum]);
} /* double2str */
#else
char *double2str(double n, int l, int d, int flags)
{
  register char *p, *ps, *pd, *px;
  auto     int   decpt, sign, dec, dp;
  auto     char  buf[BUFSIZ];
  static   char  proto[] = "                   0,000000000";


  if (++retnum == MAXRET)
    retnum = 0;

  p = retstr[retnum] + l + 1;
  *p = 0;

  dec = d ? d : UNKNOWN;
  dp = l - dec;

  *buf = '0';
  memcpy(buf + 1, ecvt(n, DIGITS, &decpt, &sign), DIGITS);

  ps = buf;
  px = ps + decpt + d;

  if (px >= buf) {
    int rfound = 0;
    pd = px + 1;

    if (*pd > '4') {
      pd++;
      rfound++;
    } /* if */

    if (rfound) {
      while (pd > px)
	if (*pd >= '5') {
	  pd--;
	  while (*pd == '9')
	    *pd-- = '0';
	  *pd += 1;
	}
	else
	  pd--;
    } /* if */

    if (*buf == '1')
      decpt++;
    else
      ps++;

    if ((dp < 2 + sign) || ((decpt ? decpt : 1) + sign) >= dp) {
      memset(retstr[retnum] + 1, '*', *retstr[retnum] = l);
      return(retstr[retnum] + 1);
    } /* if */

  } /* if */

  memcpy(retstr[retnum] + 1, proto + 21 - l + dec, *retstr[retnum] = l);

  if (!((decpt < 0) && ((dec + decpt) <= 0))) {
    pd = retstr[retnum] + dp - decpt;

    if (sign) {
      if (decpt > 0)
	*(pd - 1) = '-';
      else
	*(retstr[retnum] + dp - 2) = '-';
    } /* if */

    while (decpt-- > 0)
      *pd++ = *ps++;

    pd++; /* skip comma */

    while (d-- > 0)
      *pd++ = *ps++;
  } /* if */

  retstr[retnum][l + 1] = 0;

  if (flags & DEB) {
    p = retstr[retnum] + 1;

    while (*p == ' ')
      p++;

    return(p);
  } /* if */

  return(retstr[retnum] + 1);

} /* double2str */
#endif

/****************************************************************************/

char *double2byte(double bytes)
{
  static   char   mode[4] = " KMG";
  register int    m = 0;


  if (++retnum == MAXRET)
    retnum = 0;

  while (bytes > 999.9) {
    bytes /= 1024.0;
    m++;
  } /* while */

  sprintf(retstr[retnum], "%s%cb", double2str(bytes, 5, 1, 0), mode[m]);

  return(retstr[retnum]);
} /* double2byte */

/****************************************************************************/

char *time2str(time_t sec)
{
  static   char   mode[3] = "smh";
  register int    m = 0;
  auto     double s = (double)sec;


  if (++retnum == MAXRET)
    retnum = 0;

  while (s > 59.9) {
    s /= 60.0;
    m++;
  } /* while */

  sprintf(retstr[retnum], "%s%c", double2str(s, 4, 1, 0), mode[m]);

  return(retstr[retnum]);
} /* time2str */

/****************************************************************************/

char *double2clock(double n)
{
  auto int x, h, m, s;


  if (++retnum == MAXRET)
    retnum = 0;


  if (n <= 0.0)
    sprintf(retstr[retnum], "        ");
  else {
#if 0
    x = floor(n);
#else
    x = (int)n;
#endif

    h = (int)(x / 60 / 60);
    x %= 60 * 60;
    m = (int)(x / 60);
    s = (int)(x % 60);

#if 0
    sprintf(retstr[retnum], "%2d:%02d:%02d.%02d", h, m, s,
                                                  (int)((n - x) * 100));
#else
    sprintf(retstr[retnum], "%2d:%02d:%02d", h, m, s);
#endif
  } /* else */

  return(retstr[retnum]);
} /* double2clock */

/****************************************************************************/

char *vnum(int chan, int who)
{
  register int    l = strlen(call[chan].num[who]);
#if 0
  register char  *p1, *p2;
  auto	   int	  lx;
#endif
  auto	   int	  l1;
#if 0
  auto	   int 	  prefix = strlen(countryprefix);
  auto	   int 	  cc_len = 2;   /* country code length defaults to 2 */
#endif
  auto	   TELNUM number;
  auto	   char	  s[BUFSIZ];


  if (++retnum == MAXRET)
    retnum = 0;

  *call[chan].vorwahl[who] =
  *call[chan].rufnummer[who] =
  *call[chan].alias[who] =
  *call[chan].area[who] = 0;
  call[chan].confentry[who] = UNKNOWN;

  if (!l) {       /* keine Meldung von der Vst (Calling party number fehlt) */
    sprintf(retstr[retnum], "%c", C_UNKNOWN);
    return(retstr[retnum]);
  } /* if */

  if (*call[chan].num[who] == '#') { /* Euracom Befehl ... */
    auto char arg1[BUFSIZ], arg2[BUFSIZ], arg3[BUFSIZ];

    if (!memcmp(call[chan].num[who] + 1, "*421", 4)) {
      Strncpy(arg1, call[chan].num[who] + 5, 4 + 1);
      Strncpy(arg2, call[chan].num[who] + 9, 2 + 1);

      if (!strcmp(arg2, "00"))
        strcpy(arg3, "alle TN");
      else {
        strcpy(arg3, num2nam(arg2, 1));

        if (cnf == UNKNOWN)
          strcpy(arg3, arg2);
      } /* else */

      sprintf(retstr[retnum], "[TK:Morgen Terminruf um %c%c:%c%c Uhr an %s]",
        arg1[0], arg1[1], arg1[2], arg1[3], arg3);

      return(retstr[retnum]);
    }
    else if (!memcmp(call[chan].num[who] + 1, "*9999", 5)) {
      sprintf(retstr[retnum], "[TK:Reset]");
      return(retstr[retnum]);
    }
    else if (!memcmp(call[chan].num[who] + 1, "4", 1)) {
      sprintf(retstr[retnum], "[TK:Pickup]");
      return(retstr[retnum]);
    }
    else if (!memcmp(call[chan].num[who] + 1, "*481", 1)) {
      switch (call[chan].num[who][5]) {
        case '0' : sprintf(retstr[retnum], "[TK:LCR-Zeitprofil Automatik]"); break;
        case '1' : sprintf(retstr[retnum], "[TK:LCR-Zeitprofil Werktag]");   break;
        case '4' : sprintf(retstr[retnum], "[TK:LCR-Zeitprofil Feiertag]");  break;
        default	 : sprintf(retstr[retnum], "[TK:LCR-Zeitprofil ???]");       break;
      } /* switch */
      return(retstr[retnum]);
    }
    else if (!memcmp(call[chan].num[who] + 1, "*002", 5)) {
      register char *p = call[chan].num[who] + 5;

      sprintf(retstr[retnum], "[TK:Uhrzeit:%c%c:%c%c]", *p, *(p + 1), *(p + 2), *(p + 3));
      return(retstr[retnum]);
    } /* else */
  } /* if */

  strcpy(call[chan].alias[who], num2nam(call[chan].num[who], call[chan].si1));

  if (cnf > UNKNOWN) {                    /* Alias gefunden! */
    call[chan].confentry[who] = cnf;
    strcpy(retstr[retnum], call[chan].alias[who]);
  } /* if */

  if ((call[chan].sondernummer[who] != UNKNOWN) || call[chan].intern[who]) {
    strcpy(call[chan].rufnummer[who], call[chan].num[who]);

    if (cnf > UNKNOWN)
      strcpy(retstr[retnum], call[chan].alias[who]);
    else if (call[chan].sondernummer[who] != UNKNOWN) {
      if ((l1 = call[chan].sondernummer[who]) < l) {
        register char *p = call[chan].num[who] + l1;
        register char  c = *p;


        *call[chan].areacode[who] = *call[chan].area[who] = 0;

        *p = 0;

        sprintf(retstr[retnum], "%s - %c%s", call[chan].num[who], c, p + 1);
	strcpy(call[chan].vorwahl[who], call[chan].num[who]);
	strcpy(call[chan].rufnummer[who], p + 1);

        *p = c;
      }
      else
        sprintf(retstr[retnum], "%s", call[chan].num[who]);
    }
    else
      sprintf(retstr[retnum], "TN %s", call[chan].num[who]);

    return(retstr[retnum]);
  }
  else {
    if (!q931dmp) {
      normalizeNumber(call[chan].num[who], &number, TN_ALL);

      strcpy(call[chan].areacode[who], number.country);
      strcpy(call[chan].vorwahl[who], number.area);
      strcpy(call[chan].area[who], number.sarea);
      strcpy(call[chan].rufnummer[who], number.msn);

      strcpy(s, formatNumber("%F", &number));
    } /* if */

    if (cnf > UNKNOWN)
      strcpy(retstr[retnum], call[chan].alias[who]);
    else
      strcpy(retstr[retnum], s);

    return(retstr[retnum]);
  } /* else */
#if 0 /* -lt- dead code ??? Fixme: */
  if (l > 1) {
    if (call[chan].num[who][prefix] == '1')
      cc_len = 1; /* USA is only country with country code length 1 */
    /*
     * there should be code for country codes > 2 in length,
     * but that at least doesn't cause a possible strncpy(x, y, -1) call!
     */
    lx = cc_len + prefix;

    if (lx > 0)
      strncpy(call[chan].areacode[who], call[chan].num[who], lx);

    lx = l - cc_len - prefix;

    if (lx > 0)
      strncpy(call[chan].vorwahl[who], call[chan].num[who] + cc_len + prefix, lx);

    strcpy(call[chan].rufnummer[who], call[chan].num[who] + l);
  } /* if */

  if (cnf > UNKNOWN)
    strcpy(retstr[retnum], call[chan].alias[who]);
  else if (l > 1)
    sprintf(retstr[retnum], "%s %s/%s, %s",
      call[chan].areacode[who],
      call[chan].vorwahl[who],
      call[chan].rufnummer[who],
      call[chan].area[who]);
  else
    strcpy(retstr[retnum], call[chan].num[who]);

  return(retstr[retnum]);
#endif
} /* vnum */

/****************************************************************************/

char *i2a(int n, int l, int base)
{
  static   char  Digits[] = "0123456789abcdef";
  register char *p;
  register int	 dot = 0;


  if (++retnum == MAXRET)
    retnum = 0;

  p = retstr[retnum] + RETSIZE - 1;
  *p = 0;

  while (n || (l > 0)) {
    if (n) {
      *--p = Digits[n % base];
      n /= base;
    }
    else
      *--p = '0';

    l--;

    dot++;

    if (!(dot % 8))
      *--p = ' ';
    else if (!(dot % 4))
      *--p = '.';
  } /* while */

  return(((*p == ' ') || (*p == '.')) ? p + 1 : p);
} /* i2a */

/****************************************************************************/

static char *itoa(register unsigned int num, register char *p, register int radix, int dots)
{
  register int   i, j = 0;
  register char *q = p + MAXDIG;


  do {
    i = (int)(num % radix);
    i += '0';

    if (i > '9')
      i += 'A' - '0' - 10;

    *--q = i;

    if (dots)
      if (!(++j % 3))
	*--q = '.';

  } while ((num = num / radix));

  if (*q == '.')
    q++;

  i = p + MAXDIG - q;

  do
    *p++ = *q++;
  while (--i);

  return(p);
} /* itoa */

/****************************************************************************/

/*
static char *ltoa(register unsigned long num, register char *p, register int radix, int dots)
{
  register int   i, j = 0;
  register char *q = p + MAXDIG;


  do {
    i = (int)(num % radix);
    i += '0';

    if (i > '9')
      i += 'A' - '0' - 10;

    *--q = i;

    if (dots)
      if (!(++j % 3))
        *--q = '.';

  } while ((num = num / radix));

  if (*q == '.')
    q++;

  i = p + MAXDIG - q;

  do
    *p++ = *q++;
  while (--i);

  return(p);
}
*/

/****************************************************************************/

int iprintf(char *obuf, int chan, register char *fmt, ...)
{
  register char     *p, *s;
  register int       c, i, who;
  register short int width, ndigit;
  register int       ndfnd, ljust, zfill, lflag;
  register int	     unknown = !*call[chan].digits && !*call[chan].onum[OTHER];
  register char     *op = obuf;
  auto     char      buf[MAXDIG + 1]; /* +1 for sign */
  auto	   char	     sx[BUFSIZ];
  static   char      nul[] = "(null)";
  auto     va_list   ap;


  va_start(ap, fmt);

  for (;;) {
    c = *fmt++;

    if (!c) {
      va_end(ap);

      *op = 0;

      return((int)(op - obuf));
    } /* if */

    if (c != '%') {
      if (c == '\\') {
	c = *fmt++;
	switch (c) {
	case 't':
	  *op++ = '\t';
	  break;
	default:
	  *op++ = '\\';
	  *op++ = c;
	}
      } else {
	*op++ = c;
      }
      continue;
    } /* if */
    
    p = s = buf;

    ljust = 0;

    if (*fmt == '-') {
      fmt++;
      ljust++;
    } /* if */

    zfill = ' ';

    if (*fmt == '0') {
      fmt++;
      zfill = '0';
    } /* if */

    for (width = 0;;) {
      c = *fmt++;

      if (isdigit(c))
	c -= '0';
      else if (c == '*')
	c = GETARG(int);
      else
	break;

      width *= 10;
      width += c;
    } /* for */

    ndfnd = ndigit = 0;

    if (c == '.') {
      for (;;) {
	c = *fmt++;

	if (isdigit(c))
	  c -= '0';
	else if (c == '*')
	  c = GETARG(int);
	else
	  break;

	ndigit *= 10;
	ndigit += c;
	ndfnd++;
      } /* for */
    } /* if */

    lflag = 0;

    if (tolower(c) == 'l') {
      lflag++;

      if (*fmt)
	c = *fmt++;
    } /* if */

    who = OTHER;

    switch (c) {
      case 's' : zfill = ' ';

	         if ((s = GETARG(char *)) == NULL)
	           s = nul;

	         if (!ndigit)
	           ndigit = 32767;

	         for (p = s; *p && --ndigit >= 0; p++);

	         break;

      case 'k' : p = itoa(call[chan].card, p, 10, 0);
      	       	 break;

      case 't' : p = itoa(call[chan].tei, p, 10, 0);
      	       	 break;

      case 'C' : p = itoa(call[chan].cref, p, 10, 0);
      	       	 break;

      case 'B' : p = itoa(call[chan].channel, p, 10, 0);
      	       	 break;

      case 'A' : s = sx;
      	         if (*call[chan].onum[CLIP])
      	       	   sprintf(sx, " alias %s", call[chan].vnum[CLIP]);
      		 else
                   *sx = 0;
                 p = s + strlen(s);
                 break;

#if 0 /* DELETE_ME AK:18-Aug-99 */
      case 'z' : p = itoa(area_diff(NULL, call[chan].num[OTHER]), p, 10, 0);
      	       	 break;

      case 'Z' : s = sx;
      	         if (*call[chan].num[OTHER])
      	       	   sprintf(sx, " %s", area_diff_string(NULL, call[chan].num[OTHER]));
      	         else
                   *sx = 0;
                 p = s + strlen(s);
                 break;
#else
      case 'z' :
      case 'Z' : s = "";
      	         p = s + strlen(s);
                 break;
#endif

      case 'n' : who = ME;    goto go;
      case 'c' : who = CLIP;  goto go;
      case 'N' :
go:   	         if (!ndigit)
	           ndigit = 32767;

      		 if (*fmt) {
                   switch (*fmt++) {
                     case '0' : s = call[chan].onum[who];      break;
                     case '1' : s = call[chan].num[who];       break;
                     case '2' : s = call[chan].vnum[who];      break;
                     case '3' : s = call[chan].vorwahl[who];   break;
                     case '4' : s = call[chan].rufnummer[who]; break;
                     case '5' : s = call[chan].alias[who];     break;
                     case '6' : s = call[chan].area[who];      break;
                     case '7' : s = call[chan].areacode[who];  break;
                      default : s = nul; 		       break;
                   } /* switch */

                   p = s + strlen(s);
      		 } /* if */
                 break;

      case 'I' : switch (chan) {
     	       	   case 0 : s = "";   p = s;     break;
     		   case 1 : s = "  "; p = s + 2; break;
    		  default : s = "* "; p = s + 2; break;
     		 } /* switch */
		 break;

      case 'a' : s = idate; p = s + 3;
      	       	 break;

      case 'b' : s = idate + 3; p = s + 3;
      	       	 break;

      case 'e' : s = idate + 6; p = s + 2;
                 break;

      case 'T' : s = idate + 8; p = s + 8;
                 break;

      case ' ' :
      case '(' :
      case ')' :
      case '/' : sprintf(sx, "%c", unknown ? 0 : c);
      		 s = sx;
      	       	 p = s + strlen(s);
		 break;

      case 'p' : s = sx;
      	         if (call[chan].provider != UNKNOWN) {
		   sprintf(sx, "%s", getProviderVBN(call[chan].provider));
      	         }
      		 else
                   *sx = 0;
                 p = s + strlen(s);
                 break;

      case 'P' : s = sx;
      	         if (call[chan].provider != UNKNOWN)
      	       	   sprintf(sx, " via %s", getProvider(call[chan].provider));
      		 else
                   *sx = 0;
                 p = s + strlen(s);
                 break;

      case 'S' : p = itoa(call[chan].si1, p, 10, 0);
      	       	 break;

      default  : *p++ = c;
	         break;
    } /* switch */

    i = p - s;

    if ((width -= i) < 0)
      width = 0;

    if (!ljust)
      width = -width;

    if (width < 0) {
      if ((*s == '-') && (zfill == '0')) {
	*op++ = *s++;
	i--;
      } /* if */

      do
	*op++ = zfill;
      while (++width);
    } /* if */

    while (--i >= 0)
      *op++ = *s++;

    while (width) {
      *op++ = zfill;
      width--;
    } /* while */
  } /* for */

} /* iprintf */

/****************************************************************************/

int print_version(char *myname)
{
	_print_msg("%s Version %s\n", myname, VERSION);
	_print_msg("Copyright (C) 1995 .. 2002 by Andreas Kool (akool@isdn4linux.de)\n\n");
	_print_msg("The isdnlog project is the work of many people;\n");
	_print_msg("for at least a partial list see CREDITS.\n");
	_print_msg("%s comes with ABSOLUTELY NO WARRANTY; for details see COPYING.\n", myname);
	_print_msg("This is free software, and you are welcome to redistribute it\n");
	_print_msg("under certain conditions; see COPYING for details.\n");
	return 0;
}

/****************************************************************************/

