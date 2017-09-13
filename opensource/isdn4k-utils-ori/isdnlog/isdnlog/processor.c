/* $Id: processor.c,v 1.123 2002/03/11 16:18:43 paul Exp $
 *
 * ISDN accounting for isdn4linux. (log-module)
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
 * $Log: processor.c,v $
 * Revision 1.123  2002/03/11 16:18:43  paul
 * DM -> EUR; and only test for IIOCNETGPN on i386 systems
 *
 * Revision 1.122  2002/01/26 20:43:31  akool
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
 * Revision 1.121  2001/03/13 14:39:30  leo
 * added IIOCNETGPN support for 2.0 kernels
 * s. isdnlog/kernel_2_0/README for more information (isdnlog 4.51)
 *
 * Revision 1.120  2000/12/21 09:56:47  leo
 * modilp, ilp - show duration, bugfix
 * s. isdnlog/ilp/README for more information isdnlog 4.48
 *
 * Revision 1.119  2000/12/15 14:36:05  leo
 * modilp, ilp - B-chan usage in /proc/isdnlog
 * s. isdnlog/ilp/README for more information
 *
 * Revision 1.118  2000/12/13 14:43:16  paul
 * Translated progress messages;
 * german language version still available with #define LANG_DE
 *
 * Revision 1.117  2000/12/07 12:48:00  paul
 * Add support for both 2.2 and 2.4 kernels so that recompile is not necessary
 * (seems to work in debian version already).
 *
 * Revision 1.116  2000/09/05 10:53:20  paul
 * 1.15 was 1.12 with my patches! So changes from 1.13 and 1.14 were lost.
 * Now put back.
 *
 * Revision 1.115  2000/09/05 08:05:02  paul
 * Now isdnlog doesn't use any more ISDN_XX defines to determine the way it works.
 * It now uses the value of "COUNTRYCODE = 999" to determine the country, and sets
 * a variable mycountrynum to that value. That is then used in the code to set the
 * way isdnlog works.
 * It works for me, please check it! No configure.in / doc changes yet until
 * it has been checked to work.
 * So finally a version of isdnlog that can be compiled and distributed
 * internationally.
 *
 * Revision 1.114  2000/08/27 15:18:20  akool
 * isdnlog-4.41
 *  - fix a fix within Change_Channel()
 *
 *  - isdnlog/tools/dest/CDB_File_Dump.pm ... fixed bug with duplicates like _DEMD2
 *
 *    After installing this, please rebuild dest.cdb by:
 *    $ cd isdnlog/tools/dest
 *    $ rm dest.cdb
 *    $ make alldata
 *    $ su -c "cp ./dest.cdb /usr/lib/isdn"
 *
 *  - isdnlog/isdnlog/processor.c ... fixed warning
 *
 * Revision 1.114  2000/08/27 15:18:20  akool
 * isdnlog-4.41
 *  - fix a fix within Change_Channel()
 *
 *  - isdnlog/tools/dest/CDB_File_Dump.pm ... fixed bug with duplicates like _DEMD2
 *
 *    After installing this, please rebuild dest.cdb by:
 *    $ cd isdnlog/tools/dest
 *    $ rm dest.cdb
 *    $ make alldata
 *    $ su -c "cp ./dest.cdb /usr/lib/isdn"
 *
 *  - isdnlog/isdnlog/processor.c ... fixed warning
 *
 * Revision 1.113  2000/08/17 21:34:43  akool
 * isdnlog-4.40
 *  - README: explain possibility to open the "outfile=" in Append-Mode with "+"
 *  - Fixed 2 typos in isdnlog/tools/zone/de - many thanks to
 *      Tobias Becker <tobias@talypso.de>
 *  - detect interface (via IIOCNETGPN) _before_ setting CHARGEINT/HUPTIMEOUT
 *  - isdnlog/isdnlog/processor.c ... fixed wrong init of IIOCNETGPNavailable
 *  - isdnlog/isdnrep/isdnrep.c ... new option -S summary
 *  - isdnlog/isdnrep/rep_main.c
 *  - isdnlog/isdnrep/isdnrep.1.in
 *  - isdnlog/tools/NEWS
 *  - isdnlog/tools/cdb/debian ... (NEW dir) copyright and such from orig
 *  - new "rate-de.dat" from sourceforge (hi and welcome: Who is "roro"?)
 *
 * Revision 1.112  2000/08/14 18:41:43  akool
 * isdnlog-4.39
 *  - fixed 2 segfaults in processor.c
 *  - replaced non-GPL "cdb" with "freecdb_0.61.tar.gz"
 *
 * Revision 1.111  2000/08/06 13:06:53  akool
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
 *	 "Ziffernwahl verschluckt Nummern"
 *
 *    **Please "make clean" before using this version of isdnlog!!**
 *
 * Revision 1.110  2000/08/01 20:31:30  akool
 * isdnlog-4.37
 * - removed "09978 Schoenthal Oberpfalz" from "zone-de.dtag.cdb". Entry was
 *   totally buggy.
 *
 * - isdnlog/isdnlog/processor.c ... added err msg for failing IIOCGETCPS
 *
 * - isdnlog/tools/cdb       ... (NEW DIR) cdb Constant Data Base
 * - isdnlog/Makefile.in     ... cdb Constant Data Base
 * - isdnlog/configure{,.in}
 * - isdnlog/policy.h.in
 * - isdnlog/FAQ                 sic!
 * - isdnlog/NEWS
 * - isdnlog/README
 * - isdnlog/tools/NEWS
 * - isdnlog/tools/dest.c
 * - isdnlog/tools/isdnrate.man
 * - isdnlog/tools/zone/Makefile.in
 * - isdnlog/tools/zone/configure{,.in}
 * - isdnlog/tools/zone/config.h.in
 * - isdnlog/tools/zone/common.h
 * - isdnlog/tools/dest/Makefile.in
 * - isdnlog/tools/dest/configure{,.in}
 * - isdnlog/tools/dest/makedest
 * - isdnlog/tools/dest/CDB_File_Dump.{pm,3pm} ... (NEW) writes cdb dump files
 * - isdnlog/tools/dest/mcdb ... (NEW) convert testdest dumps to cdb dumps
 *
 * - isdnlog/tools/Makefile ... clean:-target fixed
 * - isdnlog/tools/telnum{.c,.h} ... TELNUM.vbn was not always initialized
 * - isdnlog/tools/rate.c ... fixed bug with R:tag and isdnlog not always
 *                            calculating correct rates (isdnrate worked)
 *
 *  s. isdnlog/tools/NEWS on details for using cdb. and
 *     isdnlog/README 20.a Datenbanken for a note about databases (in German).
 *
 *  As this is the first version with cdb and a major patch there could be
 *  still some problems. If you find something let me know. <lt@toetsch.at>
 *
 * Revision 1.109  2000/07/07 19:38:30  akool
 * isdnlog-4.30
 *  - isdnlog/tools/rate-at.c ... 1001 onlinetarif
 *  - isdnlog/rate-at.dat ... 1001 onlinetarif
 *  - isdnlog $ILABEL / $OLABEL may now contain "\t" (Tab)
 *  - isdnlog/isdnlog/processor.c ... clearchan .pay = -1
 *  - added
 *     - freenet PowerTarif
 *     - DTAG flatrate
 *       if you really want to use a flatrate please start isdnlog with the
 *       Option "-h86399 -I86399" to hangup after 23 hour's 59 seconds ;-)
 *
 *     - new Provider 01094:Startec, 010012:11883 Telecom, 010021:FITphone
 *
 * Revision 1.108  2000/06/29 17:38:27  akool
 *  - Ported "imontty", "isdnctrl", "isdnlog", "xmonisdn" and "hisaxctrl" to
 *    Linux-2.4 "devfs" ("/dev/isdnctrl" -> "/dev/isdn/isdnctrl")
 *
 * Revision 1.107  2000/06/20 17:09:59  akool
 * isdnlog-4.29
 *  - better ASN.1 display
 *  - many new rates
 *  - new Option "isdnlog -Q" dump's "/etc/isdn/isdn.conf" into a SQL database
 *
 * Revision 1.106  2000/06/02 12:14:27  akool
 * isdnlog-4.28
 *  - isdnlog/tools/rate.c ... patch by Hans Klein, unknown provider
 *  - fixed RR on HFC-cards
 *
 * Revision 1.105  2000/04/25 20:12:20  akool
 * isdnlog-4.19
 *   isdnlog/isdnlog/processor.c ... abclcr (-d0) turn off
 *   isdnlog/tools/dest.c ... isKey
 *
 * Revision 1.104  2000/03/09 18:50:02  akool
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
 * Revision 1.103  2000/02/22 20:04:10  akool
 * isdnlog-4.13
 *  - isdnlog/tools/rate-at.c ... chg. 1003
 *  - isdnlog/tools/country.c ... no dupl. area warning
 *  - isdnlog/rate-at.dat ... chg. 1003
 *  - isdnlog/tools/dest/pp_rate ... added 'q'
 *  - isdnlog/country-de.dat ... splitted _INM*
 *
 *  - isdnlog/tools/rate.c ... getSpecial, vbn2prefix fixed, include
 *  - isdnlog/tools/dest/pp_rate ... include
 *  - isdnlog/tools/rate-files.man ... include
 *
 *  - new rates, Services (S:, N:) reenabled
 *
 * Revision 1.102  2000/02/20 19:03:07  akool
 * isdnlog-4.12
 *  - ABC_LCR enhanced
 *  - country-de.dat more aliases
 *  - new rates
 *  - isdnlog/Makefile.in ... defined NATION
 *  - isdnlog/isdnlog/processor.c ... msn patch for NL
 *  - isdnlog/tools/isdnconf.c ... default config
 *
 * Revision 1.101  2000/02/12 16:40:22  akool
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
 * Revision 1.100  2000/02/11 15:16:33  akool
 * zred.dtag.bz2 added binary
 * 01040:GTS Weekend 0,039/Min
 *
 * Revision 1.99  2000/02/11 10:41:52  akool
 * isdnlog-4.10
 *  - Set CHARGEINT to 11 if < 11
 *  - new Option "-dx" controls ABC_LCR feature (see README for infos)
 *  - new rates
 *
 * Revision 1.98  2000/01/24 23:06:20  akool
 * isdnlog-4.05
 *  - ABC_LCR tested and fixed. It's really working now, Detlef!
 *  - Patch from Hans Klein <hansi.klein@net-con.net>
 *    German-"Verzonungstabelle" fixed
 *  - new "zone-de-dtag.gdbm" generated
 *
 * Revision 1.97  2000/01/23 22:31:13  akool
 * isdnlog-4.04
 *  - Support for Luxemburg added:
 *   - isdnlog/country-de.dat ... no +352 1 luxemburg city
 *   - isdnlog/rate-lu.dat ... initial LU version NEW
 *   - isdnlog/holiday-lu.dat ... NEW - FIXME
 *   - isdnlog/.Config.in  ... LU support
 *   - isdnlog/configure.in ... LU support
 *   - isdnlog/samples/isdn.conf.lu ... LU support NEW
 *
 *  - German zone-table enhanced
 *   - isdnlog/tools/zone/de/01033/mk ...fixed, with verify now
 *   - isdnlog/tools/zone/redzone ... fixed
 *   - isdnlog/tools/zone/de/01033/mzoneall ... fixed, faster
 *   - isdnlog/tools/zone/mkzonedb.c .... data Version 1.21
 *
 *  - Patch from Philipp Matthias Hahn <pmhahn@titan.lahn.de>
 *   - PostgreSQL SEGV solved
 *
 *  - Patch from Armin Schindler <mac@melware.de>
 *   - Eicon-Driver Support for isdnlog
 *
 * Revision 1.96  2000/01/20 07:30:09  kai
 * rewrote the ASN.1 parsing stuff. No known problems so far, apart from the
 * following:
 *
 * I don't use buildnumber() anymore to translate the numbers to aliases, because
 * it apparently did never work quite right. If someone knows how to handle
 * buildnumber(), we can go ahead and fix this.
 *
 * Revision 1.95  2000/01/12 23:22:52  akool
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
 * Revision 1.94  2000/01/01 15:05:23  akool
 * isdnlog-4.01
 *  - first Y2K-Bug fixed
 *
 * Revision 1.93  1999/12/31 13:30:02  akool
 * isdnlog-4.00 (Millenium-Edition)
 *  - Oracle support added by Jan Bolt (Jan.Bolt@t-online.de)
 *
 * Revision 1.92  1999/12/12 14:35:53  akool
 * isdnlog-3.75
 *  - ABC_LCR support (untested)
 *
 * Revision 1.91  1999/11/12 20:50:49  akool
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
 * Revision 1.90  1999/11/08 21:09:39  akool
 * isdnlog-3.65
 *   - added "B:" Tag to "rate-xx.dat"
 *
 * Revision 1.89  1999/11/07 13:29:27  akool
 * isdnlog-3.64
 *  - new "Sonderrufnummern" handling
 *
 * Revision 1.88  1999/11/05 20:22:01  akool
 * isdnlog-3.63
 *  - many new rates
 *  - cosmetics
 *
 * Revision 1.87  1999/10/30 14:38:47  akool
 * isdnlog-3.61
 *
 * Revision 1.86  1999/10/30 13:42:36  akool
 * isdnlog-3.60
 *   - many new rates
 *   - compiler warnings resolved
 *   - fixed "Sonderrufnummer" Handling
 *
 * Revision 1.85  1999/10/29 19:46:00  akool
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
 * Revision 1.84  1999/10/26 18:17:13  akool
 * isdnlog-3.58
 *   - big cleanup ( > 1.3 Mb removed!)
 *   - v0.02 of destination support - better, but not perfect
 *     (does't work with gcc-2.7.2.3 yet - use egcs!)
 *
 * Revision 1.83  1999/09/13 09:09:43  akool
 * isdnlog-3.51
 *   - changed getProvider() to not return NULL on unknown providers
 *     many thanks to Matthias Eder <mateder@netway.at>
 *   - corrected zone-processing when doing a internal -> world call
 *
 * Revision 1.82  1999/09/11 22:28:24  akool
 * isdnlog-3.50
 *   added 3. parameter to "-h" Option: Controls CHARGEHUP for providers like
 *   DTAG (T-Online) or AOL.
 *   Many thanks to Martin Lesser <m-lesser@lesser-com.de>
 *
 * Revision 1.81  1999/08/21 12:59:51  akool
 * small fixes
 *
 * Revision 1.80  1999/08/20 19:28:18  akool
 * isdnlog-3.45
 *  - removed about 1 Mb of (now unused) data files
 *  - replaced areacodes and "vorwahl.dat" support by zone databases
 *  - fixed "Sonderrufnummern"
 *  - rate-de.dat :: V:1.10-Germany [20-Aug-1999 21:23:27]
 *
 * Revision 1.79  1999/07/25 15:57:21  akool
 * isdnlog-3.43
 *   added "telnum" module
 *
 * Revision 1.78  1999/07/24 08:44:19  akool
 * isdnlog-3.42
 *   rate-de.dat 1.02-Germany [18-Jul-1999 10:44:21]
 *   better Support for Ackermann Euracom
 *   WEB-Interface for isdnrate
 *   many small fixes
 *
 * Revision 1.77  1999/07/15 16:41:32  akool
 * small enhancement's and fixes
 *
 * Revision 1.76  1999/07/11 15:30:55  akool
 * Patch from Karsten (thanks a lot!)
 *
 * Revision 1.75  1999/07/01 20:39:52  akool
 * isdnrate optimized
 *
 * Revision 1.74  1999/06/30 17:17:19  akool
 * isdnlog Version 3.39
 *
 * Revision 1.73  1999/06/29 20:11:10  akool
 * now compiles with ndbm
 * (many thanks to Nima <nima_ghasseminejad@public.uni-hamburg.de>)
 *
 * Revision 1.72  1999/06/28 19:16:10  akool
 * isdnlog Version 3.38
 *   - new utility "isdnrate" started
 *
 * Revision 1.71  1999/06/26 12:25:29  akool
 * isdnlog Version 3.37
 *   fixed some warnings
 *
 * Revision 1.70  1999/06/22 19:40:46  akool
 * zone-1.1 fixes
 *
 * Revision 1.69  1999/06/21 19:33:53  akool
 * isdnlog Version 3.35
 *   zone data for .nl (many thanks to Paul!)
 *
 *   WARNING: This version of isdnlog dont even compile! *EXPERIMENTAL*!!
 *
 * Revision 1.68  1999/06/16 23:37:35  akool
 * fixed zone-processing
 *
 * Revision 1.67  1999/06/15 20:04:09  akool
 * isdnlog Version 3.33
 *   - big step in using the new zone files
 *   - *This*is*not*a*production*ready*isdnlog*!!
 *   - Maybe the last release before the I4L meeting in Nuernberg
 *
 * Revision 1.66  1999/06/13 14:07:50  akool
 * isdnlog Version 3.32
 *
 *  - new option "-U1" (or "ignoreCOLP=1") to ignore CLIP/COLP Frames
 *  - TEI management decoded
 *
 * Revision 1.65  1999/06/09 19:58:26  akool
 * isdnlog Version 3.31
 *  - Release 0.91 of zone-Database (aka "Verzonungstabelle")
 *  - "rate-de.dat" V:1.02-Germany [09-Jun-1999 21:45:26]
 *
 * Revision 1.64  1999/06/03 18:50:33  akool
 * isdnlog Version 3.30
 *  - rate-de.dat V:1.02-Germany [03-Jun-1999 19:49:22]
 *  - small fixes
 *
 * Revision 1.63  1999/05/22 10:18:34  akool
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
 * Revision 1.62  1999/05/13 11:39:24  akool
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
 * Revision 1.61  1999/05/10 20:37:27  akool
 * isdnlog Version 3.26
 *
 *  - fixed the "0800" -> free of charge problem
 *  - *many* additions to "ausland.dat"
 *  - first relase of "rate-de.dat" from the CVS-Server of the I4L-Tarif-Crew
 *
 * Revision 1.60  1999/05/04 19:32:45  akool
 * isdnlog Version 3.24
 *
 *  - fully removed "sondernummern.c"
 *  - removed "gcc -Wall" warnings in ASN.1 Parser
 *  - many new entries for "rate-de.dat"
 *  - better "isdnconf" utility
 *
 * Revision 1.59  1999/04/30 19:07:56  akool
 * isdnlog Version 3.23
 *
 *  - changed LCR probing duration from 181 seconds to 153 seconds
 *  - "rate-de.dat" filled with May, 1. rates
 *
 * Revision 1.58  1999/04/29 19:03:24  akool
 * isdnlog Version 3.22
 *
 *  - T-Online corrected
 *  - more online rates for rate-at.dat (Thanks to Leopold Toetsch <lt@toetsch.at>)
 *
 * Revision 1.57  1999/04/26 22:12:00  akool
 * isdnlog Version 3.21
 *
 *  - CVS headers added to the asn* files
 *  - repaired the "4.CI" message directly on CONNECT
 *  - HANGUP message extended (CI's and EH's shown)
 *  - reactivated the OVERLOAD message
 *  - rate-at.dat extended
 *  - fixes from Michael Reinelt
 *
 * Revision 1.56  1999/04/25 17:34:45  akool
 * isdnlog Version 3.20
 *
 *  - added ASN.1 Parser from Kai Germaschewski <kai@thphy.uni-duesseldorf.de>
 *    isdnlog now fully support all fac- and cf-messages!
 *
 *  - some additions to the "rate-de.dat"
 *
 * Revision 1.55  1999/04/19 19:24:45  akool
 * isdnlog Version 3.18
 *
 * - countries-at.dat added
 * - spelling corrections in "countries-de.dat" and "countries-us.dat"
 * - LCR-function of isdnconf now accepts a duration (isdnconf -c .,duration)
 * - "rate-at.dat" and "rate-de.dat" extended/fixed
 * - holiday.c and rate.c fixed (many thanks to reinelt@eunet.at)
 *
 * Revision 1.54  1999/04/17 14:11:08  akool
 * isdnlog Version 3.17
 *
 * - LCR functions of "isdnconf" fixed
 * - HINT's fixed
 * - rate-de.dat: replaced "1-5" with "W" and "6-7" with "E"
 *
 * Revision 1.53  1999/04/15 19:14:38  akool
 * isdnlog Version 3.15
 *
 * - reenable the least-cost-router functions of "isdnconf"
 *   try "isdnconf -c <areacode>" or even "isdnconf -c ."
 * - README: "rate-xx.dat" documented
 * - small fixes in processor.c and rate.c
 * - "rate-de.dat" optimized
 * - splitted countries.dat into countries-de.dat and countries-us.dat
 *
 * Revision 1.52  1999/04/14 13:16:27  akool
 * isdnlog Version 3.14
 *
 * - "make install" now install's "rate-xx.dat", "rate.conf" and "ausland.dat"
 * - "holiday-xx.dat" Version 1.1
 * - many rate fixes (Thanks again to Michael Reinelt <reinelt@eunet.at>)
 *
 * Revision 1.51  1999/04/10 17:19:51  akool
 * fix a typo
 *
 * Revision 1.50  1999/04/10 16:35:35  akool
 * isdnlog Version 3.13
 *
 * WARNING: This is pre-ALPHA-dont-ever-use-Code!
 *	 "tarif.dat" (aka "rate-xx.dat"): the next generation!
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
 * Revision 1.49  1999/04/03 12:47:03  akool
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
 * Revision 1.48  1999/03/25 19:40:01  akool
 * - isdnlog Version 3.11
 * - make isdnlog compile with egcs 1.1.7 (Bug report from Christophe Zwecker <doc@zwecker.com>)
 *
 * Revision 1.47  1999/03/24 19:37:55  akool
 * - isdnlog Version 3.10
 * - moved "sondernnummern.c" from isdnlog/ to tools/
 * - "holiday.c" and "rate.c" integrated
 * - NetCologne rates from Oliver Flimm <flimm@ph-cip.uni-koeln.de>
 * - corrected UUnet and T-Online rates
 *
 * Revision 1.46  1999/03/20 16:54:45  akool
 * isdnlog 3.09 : support for all Internet-by-call numbers
 *
 * Revision 1.45  1999/03/20 14:33:07  akool
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
 * Revision 1.44  1999/03/16 17:37:18  akool
 * - isdnlog Version 3.07
 * - Michael Reinelt's patch as of 16Mar99 06:58:58
 * - fix a fix from yesterday with sondernummern
 * - ignore "" COLP/CLIP messages
 * - dont show a LCR-Hint, if same price
 *
 * Revision 1.43  1999/03/15 21:27:58  akool
 * - isdnlog Version 3.06
 * - README: explain some terms about LCR, corrected "-c" Option of "isdnconf"
 * - isdnconf: added a small LCR-feature - simply try "isdnconf -c 069"
 * - isdnlog: dont change CHARGEINT, if rate is't known!
 * - sonderrufnummern 1.02 [15-Mar-99] :: added WorldCom
 * - tarif.dat 1.09 [15-Mar-99] :: added WorldCom
 * - isdnlog now correctly handles the new "Ortstarif-Zugang" of UUnet
 *
 * Revision 1.42  1999/03/14 18:47:44  akool
 * damn CLIP :-( Internal call's are free of charge!!
 *
 * Revision 1.41  1999/03/14 14:26:38  akool
 * - isdnlog Version 3.05
 * - new Option "-u1" (or "ignoreRR=1")
 * - added version information to "sonderrufnummern.dat"
 * - added debug messages if sonderrufnummern.dat or tarif.dat could not be opened
 * - sonderrufnummern.dat V 1.01 - new 01805 rates
 *
 * Revision 1.40  1999/03/14 12:16:08  akool
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
 * Revision 1.39  1999/03/07 18:18:55  akool
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
 * Revision 1.38  1999/02/28 19:32:42  akool
 * Fixed a typo in isdnconf.c from Andreas Jaeger <aj@arthur.rhein-neckar.de>
 * CHARGEMAX fix from Oliver Lauer <Oliver.Lauer@coburg.baynet.de>
 * isdnrep fix from reinhard.karcher@dpk.berlin.fido.de (Reinhard Karcher)
 * "takt_at.c" fixes from Ulrich Leodolter <u.leodolter@xpoint.at>
 * sondernummern.c from Mario Joussen <mario.joussen@post.rwth-aachen.de>
 * Reenable usage of the ZONE entry from Schlottmann-Goedde@t-online.de
 * Fixed a typo in callerid.conf.5
 *
 * Revision 1.37  1999/01/24 19:01:40  akool
 *  - second version of the new chargeint database
 *  - isdnrep reanimated
 *
 * Revision 1.36  1999/01/10 15:23:23  akool
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
 * Revision 1.35  1998/12/09 20:39:36  akool
 *  - new option "-0x:y" for leading zero stripping on internal S0-Bus
 *  - new option "-o" to suppress causes of other ISDN-Equipment
 *  - more support for the internal S0-bus
 *  - Patches from Jochen Erwied <mack@Joker.E.Ruhr.DE>, fixes TelDaFax Tarif
 *  - workaround from Sebastian Kanthak <sebastian.kanthak@muehlheim.de>
 *  - new CHARGEINT chapter in the README from
 *    "Georg v.Zezschwitz" <gvz@popocate.hamburg.pop.de>
 *
 * Revision 1.34  1998/11/24 20:51:45  akool
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
 * Revision 1.33  1998/11/07 17:13:01  akool
 * Final cleanup. This _is_ isdnlog-3.00
 *
 * Revision 1.32  1998/11/06 23:43:52  akool
 * for Paul
 *
 * Revision 1.31  1998/11/06 14:28:31  calle
 * AVM-B1 d-channel trace level 2 (newer firmware) now running with isdnlog.
 *
 * Revision 1.30  1998/11/05 19:09:49  akool
 *  - Support for all the new L2 frames from HiSax 3.0d (RR, UA, SABME and
 *    tei management)
 *  - CityWeekend reimplemented
 *    Many thanks to Rainer Gallersdoerfer <gallersd@informatik.rwth-aachen.de>
 *    for the tip
 *  - more providers
 *  - general clean-up
 *
 * Revision 1.29  1998/11/01 08:49:52  akool
 *  - fixed "configure.in" problem with NATION_*
 *  - DESTDIR fixes (many thanks to Michael Reinelt <reinelt@eunet.at>)
 *  - isdnrep: Outgoing calls ordered by Zone/Provider/MSN corrected
 *  - new Switch "-i" -> running on internal S0-Bus
 *  - more providers
 *  - "sonderrufnummern.dat" extended (Frag Fred, Telegate ...)
 *  - added AVM-B1 to the documentation
 *  - removed the word "Teles" from the whole documentation ;-)
 *
 * Revision 1.28  1998/10/04 12:04:05  akool
 *  - README
 *      New entries "CALLFILE" and "CALLFMT" documented
 *      Small Correction from Markus Werner <mw@empire.wolfsburg.de>
 *      cosmetics
 *
 *  - isdnrep.c
 *      Bugfix (Thanks to Arnd Bergmann <arnd@uni.de>)
 *
 *  - processor.c
 *      Patch from Oliver Lauer <Oliver.Lauer@coburg.baynet.de>
 *        Makes CHARGEMAX work without AOC-D
 *
 *      Patch from Stefan Gruendel <sgruendel@adulo.de>
 *        gcc 2.7.2.1 Optimizer-Bug workaround
 *
 * Revision 1.27  1998/10/03 18:05:55  akool
 *  - processor.c, takt_at.c : Patch from Michael Reinelt <reinelt@eunet.at>
 *    try to guess the zone of the calling/called party
 *
 *  - isdnrep.c : cosmetics (i hope, you like it, Stefan!)
 *
 * Revision 1.26  1998/09/27 11:47:28  akool
 * fix segfault of isdnlog after each RELASE
 *
 * Revision 1.25  1998/09/26 18:29:15  akool
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
 *		should be fixed soon
 *
 * Revision 1.24  1998/09/22 20:59:15  luethje
 * isdnrep:  -fixed wrong provider report
 *           -fixed wrong html output for provider report
 *           -fixed strange html output
 * kisdnlog: -fixed "1001 message window" bug ;-)
 *
 * Revision 1.23  1998/08/04 08:17:41  paul
 * Translated "CHANNEL: B1 gefordet" messages into English
 *
 * Revision 1.22  1998/06/21 11:52:52  akool
 * First step to let isdnlog generate his own AOCD messages
 *
 * Revision 1.21  1998/06/16 15:05:31  paul
 * isdnlog crashed with 1TR6 and "Unknown Codeset 7 attribute 3 size 5",
 * i.e. IE 03 which is not Date/Time
 *
 * Revision 1.20  1998/06/14 15:33:51  akool
 * AVM B1 support (Layer 3)
 * Telekom's new currency DEM 0,121 supported
 * Disable holiday rates #ifdef ISDN_NL
 * memory leak in "isdnrep" repaired
 *
 * Revision 1.19  1998/06/07 21:08:43  akool
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
 * Revision 1.18  1998/04/09 19:15:07  akool
 *  - CityPlus Implementation from Oliver Lauer <Oliver.Lauer@coburg.baynet.de>
 *  - dont change huptimeout, if disabled (via isdnctrl huptimeout isdnX 0)
 *  - Support for more Providers (TelePassport, Tele 2, TelDaFax)
 *
 * Revision 1.17  1998/03/25 20:58:34  luethje
 * isdnrep: added html feature (verbose on/off)
 * processor.c: Patch of Oliver Lauer
 *
 * Revision 1.16  1998/03/08 12:37:58  luethje
 * last changes in Wuerzburg
 *
 * Revision 1.15  1998/03/08 12:13:40  luethje
 * Patches by Paul Slootman
 *
 * Revision 1.14  1998/03/08 11:42:55  luethje
 * I4L-Meeting Wuerzburg final Edition, golden code - Service Pack number One
 *
 * Revision 1.13  1998/02/05 08:23:24  calle
 * decode also seconds in date_time if available, for the dutch.
 *
 * Revision 1.12  1997/10/08 05:37:10  calle
 * Added AVM B1 support to isdnlog, patch is from i4l@tenere.saar.de.
 *
 * Revision 1.11  1997/09/07 00:43:12  luethje
 * create new error messages for isdnrep
 *
 * Revision 1.10  1997/08/22 12:31:21  fritz
 * isdnlog now handles chargeint/non-chargeint Kernels automatically.
 * Manually setting of CONFIG_ISDNLOG_OLD_I4L no more needed.
 *
 * Revision 1.9  1997/06/22 23:03:25  luethje
 * In subsection FLAGS it will be checked if the section name FLAG is korrect
 * isdnlog recognize calls abroad
 * bugfix for program starts
 *
 * Revision 1.8  1997/05/29 17:07:22  akool
 * 1TR6 fix
 * suppress some noisy messages (Bearer, Channel, Progress) - can be reenabled with log-level 0x1000
 * fix from Bodo Bellut (bodo@garfield.ping.de)
 * fix from Ingo Schneider (schneidi@informatik.tu-muenchen.de)
 * limited support for Info-Element 0x76 (Redirection number)
 *
 * Revision 1.7  1997/05/28 21:22:53  luethje
 * isdnlog option -b is working again ;-)
 * isdnlog has new \$x variables
 * README completed
 *
 * Revision 1.6  1997/04/20 22:52:14  luethje
 * isdnrep has new features:
 *   -variable format string
 *   -can create html output (option -w1 or ln -s isdnrep isdnrep.cgi)
 *    idea and design from Dirk Staneker (dirk.staneker@student.uni-tuebingen.de)
 * bugfix of processor.c from akool
 *
 * Revision 1.5  1997/03/31 20:50:59  akool
 * fixed the postgres95 part of isdnlog
 *
 * Revision 1.4  1997/03/30 15:42:10  akool
 * Ignore invalid time from VSt
 *
 * Revision 1.3  1997/03/29 09:24:25  akool
 * CLIP presentation enhanced, new ILABEL/OLABEL operators
 *
 * Revision 1.2  1997/03/20 22:42:33  akool
 * Some minor enhancements.
 *
 * Revision 1.1  1997/03/16 20:58:47  luethje
 * Added the source code isdnlog. isdnlog is not working yet.
 * A workaround for that problem:
 * copy lib/policy.h into the root directory of isdn4k-utils.
 *
 * Revision 2.6.36  1997/02/10  09:30:43  akool
 * MAXCARDS implemented
 *
 * Revision 2.6.30  1997/02/05  20:14:46  akool
 * Dual-Teles Mode implemented
 *
 * Revision 2.6.24  1997/01/15  19:21:46  akool
 * AreaCode 0.99 added
 *
 * Revision 2.6.20  1997/01/05  20:02:46  akool
 * q931dmp added
 * Automatische Erkennung "-r" -> "-R"
 *
 * Revision 2.6.19  1997/01/04  15:21:46  akool
 * Korrektur bzgl. ISDN_CH
 * Danke an Markus Maeder (mmaeder@cyberlink.ch)
 *
 * Revision 2.6.17  1997/01/03  16:26:46  akool
 * BYTEMAX implemented
 *
 * Revision 2.6.15  1997/01/02  20:02:46  akool
 * Hopefully fixed b2c() to suppress faulty messages in processbytes()
 * CONNECTMAX implemented
 *
 * Revision 2.6.11  1996/12/31  15:11:46  akool
 * general cleanup
 *
 * Revision 2.6.6  1996/11/27  22:12:46  akool
 * CHARGEMAX implemented
 *
 * Revision 2.60  1996/11/03  09:31:46  akool
 * mit -DCHARGEINT wird Ende jedes "echten" AOC-D angezeigt
 *
 * Revision 2.3.28  1996/05/06  22:18:46  akool
 * "huptimeout" handling implemented (-hx)
 *
 * Revision 2.3.24  1996/05/04  23:03:46  akool
 * Kleiner Fix am ASN.1 Parser von Bernhard Kruepl
 * i/o byte Handing redesigned
 *
 * Revision 2.3.23  1996/04/28  12:44:46  akool
 * PRT_SHOWIMON eingefuehrt
 *
 * Revision 2.3.21  1996/04/26  11:43:46  akool
 * Faelschliche DM 0,12 Meldung an xisdn unterdrueckt
 *
 * Revision 2.3.19  1996/04/25  21:44:46  akool
 * -DSELECT_FIX, new Option "-M"
 * Optionen "-i" und "-c" entfernt
 *
 * Revision 2.3.17  1996/04/23  00:25:46  akool
 * isdn4kernel-1.3.93 voll implementiert
 *
 * Revision 2.3.16  1996/04/22  22:58:46  akool
 * Temp. Fix fuer isdn4kernel-1.3.91 implementiert
 *
 * Revision 2.3.15  1996/04/22  21:25:46  akool
 * general cleanup
 *
 * Revision 2.3.13  1996/04/18  20:36:46  akool
 * Fehlerhafte Meldung der Durchsatzrate auf unbenutztem Kanal unterdrueckt
 *
 * Revision 2.3.11  1996/04/14  21:26:46  akool
 *
 * Revision 2.3.4  1996/04/05  13:50:46  akool
 * NEWCPS-Handling implemented
 *
 * Revision 2.2.5  1996/03/25  19:47:46  akool
 * Fix in Exit() (sl)
 * 1TR6-Unterstuetzung fertiggestellt
 * Neuer Switch "-e" zum Unterdruecken der "tei" Angabe
 *
 * Revision 2.2.4  1996/03/24  12:17:46  akool
 * 1TR6 Causes implemented
 * 1TR6 / E-DSS1 Frames werden unterschieden
 * Pipe-Funktionalitaet reaktiviert 19-03-96 Bodo Bellut (lasagne@garfield.ping.de)
 * Alle Console-Ausgaben wieder mit \r
 * Gebuehrenauswertung fuer 1TR6 implementiert (Wim Bonis (bonis@kiss.de))
 *
 * Revision 2.23  1996/03/17  12:26:46  akool
 *
 * Revision 2.20  1996/03/11  21:15:46  akool
 * Calling/Called party decoding
 *
 * Revision 2.19  1996/03/10  19:46:46  akool
 * Alarm-Handling fuer /dev/isdnctrl0 funzt!
 *
 * Revision 2.17  1996/02/25  19:23:46  akool
 * Andy's Geburtstags-Release
 *
 * Revision 2.15  1996/02/21  20:38:46  akool
 * sl's Server-Verschmelzung
 * Gernot's Parken/Makeln
 *
 * Revision 2.15  1996/02/17  21:01:10  root
 * Nun geht auch Parken und Makeln
 *
 * Revision 2.14  1996/02/17  16:00:00  root
 * Zeitfehler weg
 *
 * Revision 2.15  1996/02/21  20:30:42  akool
 * sl's Serververschmelzung
 * Gernot's Makeln
 *
 * Revision 2.13  1996/02/15  21:03:42  akool
 * ein kleiner noch
 * Gernot's Erweiterungen implementiert
 * MSG_CALL_INFO enthaelt nun State
 *
 * Revision 2.12  1996/02/13  20:08:43  root
 * Nu geht's (oder?)
 *
 * Revision 1.4  1996/02/13  20:05:28  root
 * so nun gehts
 *
 * Revision 1.3  1996/02/13  18:08:45  root
 * Noch ein [ und ein ;
 *
 * Revision 1.2  1996/02/13  18:02:40  root
 * Haben wir's drin - erster Versuch!
 *
 * Revision 1.1  1996/02/13  14:28:14  root
 * Initial revision
 *
 * Revision 2.10  1996/02/12  20:38:16  akool
 * TEI-Handling von Gernot Zander
 *
 * Revision 2.06  1996/02/10  20:10:16  akool
 * Handling evtl. vorlaufender "0" bereinigt
 *
 * Revision 2.05  1996/02/05  21:42:16  akool
 * Signal-Handling eingebaut
 * AVON-Handling implementiert
 *
 * Revision 2.04  1996/01/31  18:30:16  akool
 * Bugfix im C/S
 * Neue Option "-R"
 *
 * Revision 2.03  1996/01/29  15:13:16  akool
 * Bugfix im C/S

 * Revision 2.02  1996/01/27  15:13:16  akool
 * Stefan Luethje's Client-/Server Anbindung implementiert
 * Bugfix bzgl. HANGUP ohne AOCE-Meldung
 *
 * Revision 2.01  1996/01/21  15:32:16  akool
 * Erweiterungen fuer Michael 'Ghandi' Herold implementiert
 * Syslog-Meldungen implementiert
 * Reread der isdnlog.conf bei SIGHUP implementiert
 * AOCD/AOCE Auswertungen fuer Oesterreich implementiert
 *
 * Revision 2.00  1996/01/10  20:10:16  akool
 * Vollstaendiges Redesign, basierend auf der "call reference"
 * WARNING: Requires Patch of 'q931.c'
 *
 * Revision 1.25  1995/11/18  14:38:16  akool
 * AOC von Anrufen auf 0130-xxx werden korrekt ausgewertet
 *
 * Revision 1.24  1995/11/12  11:08:16  akool
 * Auch die "call reference" wird (ansatzweise) ausgewertet
 * Neue Option "-x" aktiviert X11-Popup
 * Date/Time wird ausgewertet
 * AOC-D wird korrekt ausgewertet
 * Neue Option "-t" setzt Systemzeit auf die von der VSt gemeldete
 * Die "-m" Option kann nun auch mehrfach (additiv) angegeben werden
 *
 * Revision 1.23  1995/11/06  18:03:16  akool
 * "-m16" zeigt die Cause im Klartext an
 * Auch Gebuehreneinheiten > 255 werden korrekt ausgewertet
 *
 * Revision 1.22  1995/10/22  14:43:16  akool
 * General cleanup
 * "isdn.log" um 'I' == dialin / 'O' == dialout erweitert
 * Auch nicht zustande gekommene Verbindungen werden (mit cause)
 * protokolliert
 *
 * Revision 1.21  1995/10/18  21:25:16  akool
 * Option "-r" implementiert
 * Charging-Infos waehrend der Verbindung (FACILITY) werden ignoriert
 * "/etc/isdnlog.pid" wird erzeugt
 *
 * Revision 1.20  1995/10/15  17:23:16  akool
 * Volles D-Kanal Protokoll implementiert (fuer Teles-0.4d Treiber)
 *
 * Revision 1.13  1995/09/30  09:34:16  akool
 * Option "-m", Console-Meldung implementiert
 * Flush bei SIGTERM implementiert
 *
 * Revision 1.12  1995/09/29  17:21:13  akool
 * "isdn.log" um Zeiteintrag in UTC erweitert
 *
 * Revision 1.11  1995/09/28  18:51:17  akool
 * First public release
 *
 * Revision 1.1  1995/09/16  16:54:12  akool
 * Initial revision
 *
 */

#define _PROCESSOR_C_
#include "isdnlog.h"
#include "sys/times.h"
#include "asn1.h"
#include "asn1_comp.h"
#include "zone.h"
#include "telnum.h"
#ifdef CONFIG_ISDN_WITH_ABC_LCR_SUPPORT
#include <linux/isdn_dwabc.h>
#else
static void processlcr(char *p);
#endif

#define preselect pnum2prefix(preselect, cur_time)

static int    HiSax = 0, hexSeen = 0, uid = UNKNOWN, lfd = 0;
static char  *asnp, *asnm = NULL;
static int    chanused[2] = { 0, 0 };
static int    IIOCNETGPNavailable = -1; /* -1 = unknown, 0 = no, 1 = yes */

#ifdef Q931
#define Q931dmp q931dmp
#else
#define Q931dmp 0
#endif


// #define INTERFACE ((IIOCNETGPNavailable == 1) ? call[chan].interface : known[call[chan].confentry[OTHER]]->interface)
#define INTERFACE call[chan].interface


static void Q931dump(int mode, int val, char *msg, int version)
{
#ifdef Q931
  switch (mode) {
    case TYPE_STRING  : if (val == -4)
                          fprintf(stdout, "    ??  %s\n", msg);
                        else if (val == -3)
                          fprintf(stdout, "%s\n", msg);
                        else if (val == -2)
                          fprintf(stdout, "    ..  %s\n", msg);
                        else if (val == -1)
                          fprintf(stdout, "        %s\n", msg);
                        else
                          fprintf(stdout, "    %02x  %s\n", val, msg);
                        break;

    case TYPE_MESSAGE : fprintf(stdout, "\n%02x  %s\n", val, qmsg(mode, version, val));
                        break;

    case TYPE_ELEMENT : fprintf(stdout, "%02x ---> %s\n", val, qmsg(mode, version, val));
                        break;

    case TYPE_CAUSE   : fprintf(stdout, "    %02x  %s\n", val, qmsg(mode, version, val & 0x7f));
                        break;

  } /* switch */
#endif
} /* Q931dump */


static void diag(int cref, int tei, int sapi, int dialin, int net, int type, int version)
{
  char String[LONG_STRING_SIZE];
  char TmpString[LONG_STRING_SIZE];


  if (dialin != -1) {
    sprintf(String,"  DIAG> %s: %3d/%3d %3d %3d %s %s %s-> ",
      st + 4, cref, cref & 0x7f, tei, sapi,
      ((version == VERSION_1TR6) ? "1TR6" : "E-DSS1"),
      dialin ? " IN" : "OUT",
      net ? "NET" : "USR");

    if ((cref > 128) && (type == SETUP_ACKNOWLEDGE)) {
      sprintf(TmpString," *%d* ", cref);
      strcat(String, TmpString);
    } /* if */

  } /* if */

  print_msg(PRT_DEBUG_GENERAL, "%s%s\n", String, qmsg(TYPE_MESSAGE, VERSION_EDSS1, type));
} /* diag */


static char *location(int loc)
{
  switch (loc) {
    case 0x00 : return("User");                                break;
    case 0x01 : return("Private network serving local user");  break;
    case 0x02 : return("Public network serving local user");   break;
    case 0x03 : return("Transit network");                     break;
    case 0x04 : return("Public network serving remote user");  break;
    case 0x05 : return("Private network serving remote user"); break;
    case 0x07 : return("International network");               break;
    case 0x0a : return("Network beyond inter-working point");  break;
      default : return("");				       break;
  } /* switch */
} /* location */


void buildnumber(char *num, int oc3, int oc3a, char *result, int version,
		 int *provider, int *sondernummer, int *intern, int *local,
		 int dir, int who)
{
  auto char n[BUFSIZ];
  auto int  partner = ((dir && (who == CALLING)) || (!dir && (who == CALLED)));


//  *sondernummer = UNKNOWN;
  *intern = 0;
  *local = 0;

  if (Q931dmp) {
    register char *ps;
    auto     char  s[BUFSIZ];


    ps = s + sprintf(s, "Type of number: ");

    switch (oc3 & 0x70) {
      case 0x00 : sprintf(ps, "Unknown");                break;
      case 0x10 : sprintf(ps, "International");          break;
      case 0x20 : sprintf(ps, "National");               break;
      case 0x30 : sprintf(ps, "Network specific");       break;
      case 0x40 : sprintf(ps, "Subscriber");             break;
      case 0x60 : sprintf(ps, "Abbreviated");            break;
      case 0x70 : sprintf(ps, "Reserved for extension"); break;
    } /* switch */

    Q931dump(TYPE_STRING, oc3, s, version);

    ps = s + sprintf(s, "Numbering plan: ");

    switch (oc3 & 0x0f) {
      case 0x00 : sprintf(ps, "Unknown");                break;
      case 0x01 : sprintf(ps, "ISDN/telephony");         break;
      case 0x03 : sprintf(ps, "Data");                   break;
      case 0x04 : sprintf(ps, "Telex");                  break;
      case 0x08 : sprintf(ps, "National standard");      break;
      case 0x09 : sprintf(ps, "Private");                break;
      case 0x0f : sprintf(ps, "Reserved for extension"); break;
    } /* switch */

    Q931dump(TYPE_STRING, -1, s, version);

    if (oc3a != -1) {
      ps = s + sprintf(s, "Presentation: ");

      switch (oc3a & 0x60) {
        case 0x00 : sprintf(ps, "allowed");                                     break;
        case 0x20 : sprintf(ps, "restricted");                                  break;
        case 0x40 : sprintf(ps, "Number not available due to internetworking"); break;
        case 0x60 : sprintf(ps, "Reserved for extension");                      break;
      } /* switch */

      Q931dump(TYPE_STRING, oc3a, s, version);

      ps = s + sprintf(s, "Screening indicator: ");

      switch (oc3a & 0x03) {
        case 0x00 : sprintf(ps, "User provided, not screened");        break;
        case 0x01 : sprintf(ps, "User provided, verified and passed"); break;
        case 0x02 : sprintf(ps, "User provided, verified and failed"); break;
        case 0x03 : sprintf(ps, "Network provided");                   break;
      } /* switch */

      Q931dump(TYPE_STRING, -1, s, version);

    } /* if */

    sprintf(s, "\"%s\"", num);
    Q931dump(TYPE_STRING, -2, s, version);
  } /* if */

  strcpy(n, num);
  strcpy(result, "");

  *intern = ((strlen(num) < interns0) || !isdigit(*num));

  if (trim && !*intern) {
    if (dir && (who == CALLING))
      num += min(trimi, strlen(num));
    else if (!dir && (who == CALLED))
      num += min(trimo, strlen(num));

    print_msg(PRT_DEBUG_DECODE, " TRIM> \"%s\" -> \"%s\" (trimi=%d, trimo=%d, %s, %s, %s)\n",
      n, num, trimi, trimo, (dir ? "DIALIN" : "DIALOUT"), (who ? "CALLED" : "CALLING"), (partner ? "PARTNER" : "MYSELF"));
  } /* if */

  if (*num && !dir && (who == CALLED)) {
    char *amt = amtsholung;

    while (amt && *amt) {
      int len = strchr(amt, ':') ? strchr(amt, ':') - amt : strlen(amt);

      if (len && !strncmp(num, amt, len)) {
        if (Q931dmp) {
          auto char s[BUFSIZ], c;

          c = num[len];
          num[len] = 0;

          sprintf(s, "Amtsholung: %s", num);
          num[len] = c;

          Q931dump(TYPE_STRING, -2, s, version);
        } /* if */
        num += len;

        break;
      } /* if */

      amt += len + (strchr(amt, ':') ? 1 : 0);
    } /* while */
  } /* if */

#if  0  /* Fixme: delete */
  if (!dir && (who == CALLED) && !memcmp(num, vbn, strlen(vbn))) { /* Provider */
    register int l, c;

    l = strlen(vbn);
    if (num[l] == '0') /* dreistellige Verbindungsnetzbetreiberkennzahl? */
      l += 3;
    else
      l += 2;

    c = num[l];
    num[l] = 0;
    *provider = atoi(num + strlen(vbn));

    /* die dreistelligen Verbindungsnetzbetreiberkennzahlen werden
       intern erst mal mit einem Offset von 100 verarbeitet
       "010001 Netnet" -> "001" + 100 -> 101

       Das geht gut, solange nur die ersten 99 der dreistelligen
       vergeben werden ...
    */

    if (l == 6) /* Fixme: German specific */
      *provider += 100;

    if (l == 7) /* Fixme: German specific */
      *provider += 200;

    num[l] = c;
    num += l;
    if (Q931dmp) {
      auto char s[BUFSIZ];
      if (*provider < 100)
        sprintf(s, "Via provider \"%s%02d\", %s", vbn, *provider, getProvider(*provider));
      else
	sprintf(s, "Via provider \"%s%03d\", %s", vbn, *provider - 100, getProvider(*provider));
      Q931dump(TYPE_STRING, -1, s, version);
    } /* if */
  } /* if */
#else
  if (!dir && (who == CALLED) && !*intern) { /* split Provider */
    int l;
    /* cool Kool :-) */
    num += (l=provider2prefix(num, provider));

    if (l && Q931dmp) {
      auto char s[BUFSIZ];
      auto char prov[TN_MAX_PROVIDER_LEN];
      prefix2provider(*provider, prov);
      sprintf(s, "Via provider \"%s\", %s", prov, getProvider(*provider));
      Q931dump(TYPE_STRING, -1, s, version);
    } /* if */
  } /* if */
#endif

  if (!*intern) {
    if (*provider == UNKNOWN)
      *provider = preselect;

    if (*num && !dir && (who == CALLED) && getSpecial(num) && (*sondernummer == UNKNOWN))
      *sondernummer = strlen(num);
  } /* if */

  if (Q931dmp) {
    auto char s[BUFSIZ];


    if (*sondernummer != UNKNOWN) {
      sprintf(s, "(Sonderrufnummer %s, len=%d)", num, *sondernummer);
      Q931dump(TYPE_STRING, -1, s, version);
    } /* if */

    if (*intern)
      Q931dump(TYPE_STRING, -1, "(Interne Nummer)", version);
  } /* if */

  if ((*sondernummer == UNKNOWN) && !*intern) {
    switch (oc3 & 0x70) { /* Calling party number Information element, Octet 3 - Table 4-11/Q.931 */
      case 0x00 : if (*num) {                  /* 000 Unknown */
                    if (*num != '0') {
		      /* in NL the MSN contains myarea w/o leading zero
		         so myarea get's prepended again */
		      if (memcmp(myarea, num, strlen(myarea)) == 0)
		        strcpy(result, mycountry);
		      else
                        strcpy(result, mynum);
                      *local = 1;
                    }
                    else {
                      if (num[1] != '0') /* Falls es doch Ausland ist -> nichts machen!!! */
                        strcpy(result, mycountry);
                      else
                        strcpy(result, countryprefix);

                      while (*num == '0')
                        num++;
                    } /* else */
                  } /* if */
                  break;

      case 0x10 : if (version != VERSION_1TR6)
                    strcpy(result, countryprefix);  /* 001 International */
                  break;

      case 0x20 : if (version != VERSION_1TR6) {
                    strcpy(result, mycountry);    /* 010 National */

                    while (*num == '0')
                      num++;
		  } /* if */
                  break;

      case 0x30 : break;                       /* 011 Network specific number */

      case 0x40 : if (*num != '0') {           /* 100 Subscriber number */
                    strcpy(result, mynum);
                    *local = 1;
		  }
                  else {
                    strcpy(result, mycountry);

                    while (*num == '0')
                      num++;
                  } /* else */
                  break;

      case 0x60 : break;                       /* 110 Abbreviated number */

      case 0x70 : break;                       /* 111 Reserved for extension */
    } /* switch */
  } /* if */

  if (*num)
    strcat(result, num);
  else
    strcpy(result, "");

  print_msg(PRT_DEBUG_DECODE, " DEBUG> %s: num=\"%s\", oc3=%s(%02x), result=\"%s\", sonder=%d, intern=%d, local=%d, partner=%d\n",
    st + 4, n, i2a(oc3, 8, 2), oc3 & 0x70, result, *sondernummer, *intern, *local, partner);
} /* buildnumber */


char *ns(char *num)
{
  auto int i1 = 0, i2 = UNKNOWN, i3 = 0, i4 = 0;


  if (++retnum == MAXRET)
    retnum = 0;

  buildnumber(num, 0, 0, retstr[retnum], VERSION_EDSS1, &i1, &i2, &i3, &i4, 0, 1);
  return(num /* retstr[retnum] */);
} /* ns */


void aoc_debug(int val, char *s)
{
  print_msg(PRT_DEBUG_DECODE, " DEBUG> %s: %s\n", st + 4, s);

  if (Q931dmp)
    Q931dump(TYPE_STRING, val, s, VERSION_EDSS1);
} /* aoc_debug */


/*
    currency_mode   := AOC_UNITS | AOC_AMOUNT
    currency_factor :=
    currency        := " EUR" | "GBP" | "NOK" | "DKK" | ...
*/


static int parseRemoteOperationProtocol(char **asnp, struct Aoc *aoc)
{
  char msg[255];
  char *p = msg;
  char *asne = *asnp + strlen(*asnp);

  *asnp += 3;
  while (*asnp < asne) {
    *p++ = strtol(*asnp, NIL, 16);
    *asnp += 3;
  }
  ParseASN1(msg, p, 0);
  if (ParseComponent(aoc, msg, p) < 0)
    return 0;

  return 1;
} /* parseRemoteOperationProtocol */


static int facility(int l, char* p)
{
  auto   int  c;
  static struct Aoc  aoc;


  asnp = p;
  aoc.type = 0;

  if (asnp == NULL)
    return(AOC_OTHER);

  c = strtol(asnp += 3, NIL, 16);              /* Ext/Spare/Profile */

  memset(&aoc, 0, sizeof(aoc));

  switch (c) {                                 /* Remote Operation Protocol */
    case 0x91 : aoc_debug(c, "Remote Operation Protocol");

		if (parseRemoteOperationProtocol(&asnp, &aoc)) {
		  switch (aoc.type) {
		    case 33 : // AOCD Currency
                              if (aoc.multiplier)
				currency_factor = aoc.multiplier;

                              if (*aoc.currency)
			        currency = aoc.currency;

			      if (aoc.type_of_charging_info != 1) // if type_of_charging_info = 1 (total), treat AOCD as AOCE
				aoc.amount *= -1;
			      // fall trough

		    case 35 : // AOCE Currency
                              if (aoc.multiplier)
				currency_factor = aoc.multiplier;

                              if (*aoc.currency)
			        currency = aoc.currency;

			      return(aoc.amount);

		    case 34 : // AOCD ChargingUnits
			      aoc.amount *= -1;
			      // fall through

		    case 36 : // AOCE ChargingUnits
			      return(aoc.amount);

		    default : asnm = aoc.msg;
			      return(AOC_OTHER);
		  } /* switch */
		}
		else {
                  asnm = aoc.msg;
		  return(AOC_OTHER);
		} /* else */
                break;

    case 0x92 : aoc_debug(c, "CMIP Protocol");
                break;

    case 0x93 : aoc_debug(c, "ACSE Protocol");
                break;

      default : aoc_debug(c, "UNKNOWN Protocol");
		return(AOC_OTHER);
  } /* switch */

  return(AOC_OTHER);
} /* facility */


static int AOC_1TR6(int l, char *p)
{
  auto   int  Units = 0;
  auto   int  digit = 0;


if (mycountrynum == CCODE_NL) {
  /*
   *  NL ISDN: N40*<Units>#, with Units coded in ASCII.
   *  e.g. 30 units: N40*30#
   *  We don't know what the 'N40' stands for... skip it.
   *  Unit happens to be NLG 0.15, even though the charging is per NLG 0.01
   *  therefore this is always only an indication.
   */

  p += 9;
  l -= 3;
  aoc_debug(-1, "AOC_INITIAL_NL");
}
else if (mycountrynum == CCODE_CH) {
  /*
   * "FR. 0.10"
   *
   *
   */
  p += 9;
  l -= 3; /* Thanks to Markus Maeder (mmaeder@cyberlink.ch) */
  aoc_debug(-1, "AOC_INITIAL_CH");
}
else if (mycountrynum == CCODE_DE) {
  aoc_debug(-1, "AOC_INITIAL_1TR6");
}

  while (l--) {
    digit = strtol(p += 3, NIL, 16) ;

    if ((digit >= '0') && (digit <= '9')) {
      Units = Units * 10;
      Units += (digit - '0'); /* Units are in ASCII */
    } /* if */
  } /* while */

  currency_mode = AOC_AMOUNT;
  return(Units);
} /* AOC_1TR6 */


static int detach()
{
  if (replay)
    return(1);

  if (!close(sockets[ISDNCTRL].descriptor)) {
    if (!*isdnctrl2 || !close(sockets[ISDNCTRL2].descriptor)) {
      if (!close(sockets[ISDNINFO].descriptor)) {
        return(1);
      }
      else {
        print_msg(PRT_DEBUG_CS, "cannot close /dev/isdninfo: %s\n",strerror(errno));
        Exit(33);
      } /* else */
    }
    else {
      print_msg(PRT_DEBUG_CS, "cannot close \"%s\": %s\n", isdnctrl2, strerror(errno));
      Exit(39);
    } /* else */
  }
  else {
    print_msg(PRT_DEBUG_CS, "cannot close \"%s\": %s\n", isdnctrl, strerror(errno));
    Exit(31);
  } /* else */

  return(0);
} /* detach */


static int attach()
{
  if (replay)
    return(1);

  if ((sockets[ISDNCTRL].descriptor = open(isdnctrl, O_RDONLY | O_NONBLOCK)) < 0) {
    print_msg(PRT_DEBUG_CS, "cannot open \"%s\": %s\n", isdnctrl, strerror(errno));
    Exit(30);
  } /* if */

  if (*isdnctrl2)
    if ((sockets[ISDNCTRL2].descriptor = open(isdnctrl2, O_RDONLY | O_NONBLOCK)) < 0) {
      print_msg(PRT_DEBUG_CS, "cannot open \"%s\": %s\n", isdnctrl2, strerror(errno));
      Exit(38); /* cannot (re)open "/dev/isdnctrl2" */
    } /* if */

  sockets[ISDNINFO].descriptor = open("/dev/isdn/isdninfo", O_RDONLY | O_NONBLOCK);
  if (sockets[ISDNINFO].descriptor < 0)
    sockets[ISDNINFO].descriptor = open("/dev/isdninfo", O_RDONLY | O_NONBLOCK);
  if (sockets[ISDNINFO].descriptor < 0) {
    print_msg(PRT_DEBUG_CS, "cannot open /dev/isdninfo: %s\n", strerror(errno));
    Exit(32);
  } /* if */

  return(0);
} /* attach */


static void chargemaxAction(int chan, double charge_overflow)
{
  register int   cc = 0, c = call[chan].confentry[OTHER];
  auto     char  cmd[BUFSIZ], msg[BUFSIZ];


  sprintf(cmd, "%s/dontstop", confdir());

  if (access(cmd, F_OK)) {
    sprintf(cmd, "%s/%s", confdir(), STOPCMD);

    if (!access(cmd, X_OK)) {

      sprintf(cmd, "%s/%s %s %s %s",
        confdir(), STOPCMD, double2str((charge_overflow), 6, 3, DEB),
        known[c]->who,
        double2str(known[c]->scharge, 6, 3, DEB));

      sprintf(msg, "CHARGEMAX exhausted: %s", cmd);
      info(chan, PRT_ERR, STATE_AOCD, msg);

      (void)detach();

      cc = replay ? 0 : system(cmd);

      (void)attach();

      sprintf(msg, "CHARGEMAX exhausted: result = %d", cc);
      info(chan, PRT_ERR, STATE_AOCD, msg);
    } /* if */
    else
    {
      sprintf(msg, "CHARGEMAX exhausted: stop script `%s' doesn't exist! - NO ACTION! (%s)", cmd, strerror(errno));
      info(chan, PRT_ERR, STATE_AOCD, msg);
    }
  }
  else {
    sprintf(msg, "CHARGEMAX exhausted - NO ACTION!! - %s exists!", cmd);
    info(chan, PRT_ERR, STATE_AOCD, msg);
  } /* else */
} /* chargemaxAction */


static void emergencyStop(int chan, int strength)
{
  register int   cc = 0, c = call[chan].confentry[OTHER];
  register char *p;
  auto     char  cmd[BUFSIZ], msg[BUFSIZ];


  if (strength == 1) {
    if (c == -1)
      strength++;
    else if (*INTERFACE < '@')
      strength++;
  } /* if */

  sprintf(cmd, "%s/%s", confdir(), RELOADCMD);

  if (strength == 2)
    if (access(cmd, X_OK))
      strength++;

  switch (strength) {
    case 1 : cc = replay ? 0 : ioctl(sockets[ISDNCTRL].descriptor, IIOCNETHUP, INTERFACE);

             if (cc < 0)
               p = "failed";
             else if (cc)
               p = "not connected";
             else
               p = "hung up";

             sprintf(msg, "EMERGENCY-STOP#%d: no traffic since %d EH: hangup %s = %s\007\007",
               strength, call[chan].aoce - call[chan].traffic, INTERFACE, p);
             break;

    case 2 : (void)detach();

	     cc = replay ? 0 : system(cmd);

	     (void)attach();

             sprintf(msg, "EMERGENCY-STOP#%d: no traffic since %d EH: reload = %d\007\007",
               strength, call[chan].aoce - call[chan].traffic, cc);
             break;

    case 3 : if (replay)
               cc = 0;
             else {
               if ((cc = ioctl(sockets[ISDNCTRL].descriptor, IIOCSETGST, 1)) < 0)
                 ;
               else
                 cc = ioctl(sockets[ISDNCTRL].descriptor, IIOCSETGST, 0);
             } /* if */

             sprintf(msg, "EMERGENCY-STOP#%d: no traffic since %d EH: system off = %d\007\007",
               strength, call[chan].aoce - call[chan].traffic, cc);

             break;

    case 4 : sprintf(msg, "EMERGENCY-STOP#%d: no traffic since %d EH: REBOOT!!\007\007",
               strength, call[chan].aoce - call[chan].traffic);

             info(chan, PRT_ERR, STATE_AOCD, msg);

             if (!replay) {
               Exit(-9);               /* cleanup only! */
               system(REBOOTCMD);
             } /* if */
             break;
  } /* switch */

  info(chan, PRT_ERR, STATE_AOCD, msg);

} /* emergencyStop */


static int expensive(int bchan)
{
  return( (ifo[bchan].u & ISDN_USAGE_OUTGOING) &&
        (((ifo[bchan].u & ISDN_USAGE_MASK) == ISDN_USAGE_NET) ||
         ((ifo[bchan].u & ISDN_USAGE_MASK) == ISDN_USAGE_MODEM)));
} /* expensive */


static void decode(int chan, register char *p, int type, int version, int tei)
{
  register char     *pd, *px, *py;
  register int       element, l, l1, c, oc3, oc3a, n, sxp = 0, warn;
  auto	   int	     loc, cause;
  auto     char      s[BUFSIZ], s1[BUFSIZ];
  auto     char      sx[10][BUFSIZ];
  auto     int       sn[10];
  auto     struct tm tm;
  auto	   time_t    t;
  auto     double    tx, err, tack;


  while (1) {

    if (!*(p - 1) || !*(p + 2))
      break;

    element = strtol(p += 3, NIL, 16);

    if (element < 128) {

      l = strtol(p += 3, NIL, 16);

      if (Q931dmp) {
        auto char s[BUFSIZ];

        Q931dump(TYPE_ELEMENT, element, NULL, version);

        sprintf(s, "length=%d", l);
        Q931dump(TYPE_STRING, l, s, version);
      } /* if */

      if ((l > 50) || (l < 0)) {
	sprintf(s, "Invalid length %d -- complete frame ignored!", l);
        info(chan, PRT_SHOWNUMBERS, STATE_RING, s);
        return;
      } /* if */

      pd = qmsg(TYPE_ELEMENT, version, element);

      if (strncmp(pd, "UNKNOWN", 7) == 0) {
        register char *p1 = p, *p2;
        register int   i, c;
        auto     char  s[LONG_STRING_SIZE];


        p2 = s;
        p2 += sprintf(p2, "UNKNOWN ELEMENT %02x:", element);

        for (i = 0; i < l; i++)
          p2 += sprintf(p2, " %02x", (int)strtol(p1 += 3, NIL, 16));

        p2 += sprintf(p2, " [");
        p1 = p;

        for (i = 0; i < l; i++) {
          c = (int)strtol(p1 += 3, NIL, 16);
          p2 += sprintf(p2, "%c", isgraph(c) ? c : ' ');
        } /* for */

        p2 += sprintf(p2, "], length=%d -- complete frame ignored!", l);
        info(chan, PRT_SHOWNUMBERS, STATE_RING, s);
        return;
      }
      else
        print_msg(PRT_DEBUG_DECODE, " DEBUG> %s: ELEMENT %02x:%s (length=%d)\n", st + 4, element, pd, l);

      /* changing 0x28 to 0x2800 for special case prevents much complication */
      /* later; 0x28 means / does different things in different countries    */
      if (element == 0x28 && mycountrynum!=CCODE_NL && mycountrynum!=CCODE_CH)
	element = 0x2800;

      switch (element) {
        case 0x08 : /* Cause */
                    if (version == VERSION_1TR6) {
                      switch (l)  {
                         case 0 : call[chan].cause = 0;
                                  break;

                         case 1 : call[chan].cause = strtol(p += 3, NIL, 16) & 0x7f;
                                  break;

                         case 2 : call[chan].cause = strtol(p += 3, NIL, 16) & 0x7f;
                                  c = strtol(p += 3, NIL, 16); /* Sometimes it 0xc4 or 0xc5 */
                                  break;

                        default : p += (l * 3);
                                  print_msg(PRT_ERR, "Wrong Cause (more than two bytes)\n");
                                  break;
                      } /* switch l */

                      info(chan, PRT_SHOWCAUSE, STATE_CAUSE, qmsg(TYPE_CAUSE, version, call[chan].cause));

                      if (sound) {
                        if (call[chan].cause == 0x3b) /* "User busy" */
                          ringer(chan, RING_BUSY);
                        else
                          ringer(chan, RING_ERROR);
                      } /* if */
                    }
                    else { /* E-DSS1 */
                      c = strtol(p + 3, NIL, 16);

                      if (Q931dmp) {
                        register char *ps;
                        auto     char  s[BUFSIZ];


                        ps = s + sprintf(s, "Coding: ");

                        switch (c & 0xf0) {
                          case 0x00 :
                          case 0x80 : sprintf(ps, "CCITT standardisierte Codierung");     break;
                          case 0x20 :
                          case 0xa0 : sprintf(ps, "Reserve");                             break;
                          case 0x40 :
                          case 0xc0 : sprintf(ps, "reserviert fuer nationale Standards"); break;
                          case 0x60 :
                          case 0xe0 : sprintf(ps, "Standard bzgl. Localierung");          break;
                            default : sprintf(ps, "UNKNOWN #%d", c & 0xf0);               break;
                        } /* switch */

                        Q931dump(TYPE_STRING, c, s, version);

                        ps = s + sprintf(s, "Location: %s", location(c & 0x0f));

                        Q931dump(TYPE_STRING, -1, s, version);
                      } /* if */

		      py = location(loc = (c & 0x0f));

                      c = strtol(p + 6, NIL, 16);
		      cause = c & 0x7f;

		      if ((tei != call[chan].tei) && (chan == 6)) { /* AK:26-Nov-98 */
                        if (Q931dmp) {
                          auto char s[256];

                          Q931dump(TYPE_CAUSE, c, NULL, version);

                          sprintf(s, "IGNORING CAUSE: tei=%d, call.tei=%d, chan=%d", tei, call[chan].tei, chan);
			  Q931dump(TYPE_STRING, -2, s, version);
                        }
                        p += (l * 3);
                        break;
		      } /* if */

                      /* Remember only the _first_ cause
			 except this was "Normal call clearing", "No user responding"
                         or "non-selected user clearing"
                      */

                      if ((call[chan].cause == -1) || /* The first cause */
                          (call[chan].cause == 16) || /* "Normal call clearing" */
                          (call[chan].cause == 18) || /* "No user responding" */
                          (call[chan].cause == 26)) { /* "non-selected user clearing" */
                        call[chan].cause = cause;
                        call[chan].loc = loc;
                      } /* if */

                      if (Q931dmp)
                        Q931dump(TYPE_CAUSE, c, NULL, version);

                      if (HiSax || (
                          (call[chan].cause != 0x10) &&  /* "Normal call clearing" */
                          (call[chan].cause != 0x1a) &&  /* "non-selected user clearing" */
                          (call[chan].cause != 0x1f) &&  /* "Normal, unspecified" */
                          (call[chan].cause != 0x51))) { /* "Invalid call reference value" <- dies nur Aufgrund eines Bug im Teles-Treiber! */
                        sprintf(s, "%s (%s)", qmsg(TYPE_CAUSE, version, call[chan].cause), py);

                        if (tei == call[chan].tei)
                          info(chan, PRT_SHOWCAUSE, STATE_CAUSE, s);
                        else if (other) {
                          auto char sx[256];

                          sprintf(sx, "TEI %d : %s", tei, s);
                          info(chan, PRT_SHOWCAUSE, STATE_CAUSE, sx);
                        } /* else */

                        if (sound) {
                          if (call[chan].cause == 0x11) /* "User busy" */
                            ringer(chan, RING_BUSY);
                          else if ((call[chan].cause != 0x10) &&
                                   (call[chan].cause != 0x1a) &&
                                   (call[chan].cause != 0x1f) &&
                                   (call[chan].cause != 0x51))
                            ringer(chan, RING_ERROR);
                        } /* if */
                      } /* if */

                      p += (l * 3);
                    } /* else */

                    break;

        case 0x2d : /* SUSPEND ACKNOWLEDGE (Parkweg) */
                    p += (l * 3);
                    break;

        case 0x2e : /* RESUME ACKNOWLEDGE (Parkran) */
                    p += (l * 3);
                    /* ggf. neuer Channel kommt gleich mit */
                    break;

        case 0x33 : /* makel resume acknowledge (Makelran) */
                    p += (l * 3);
                    /* ggf. neuer Channel kommt gleich mit */
                    break;

	case 0x2800: /* DISPLAY ... z.b. Makelweg, AOC-E ... */
                    {
                      auto     char  s[BUFSIZ];
                      register char *ps = s;

                      while (l--)
                        *ps++ = strtol(p += 3, NIL, 16);
                      *ps = 0;

                      if (Q931dmp)
			Q931dump(TYPE_STRING, -2, s, version);
		      else
                        info(chan, PRT_SHOWNUMBERS, STATE_RING, s);
                    }
                    break;

        case 0x02 : /* Facility AOC-E on 1TR6 */
        case 0x1c : /* Facility AOC-D/AOC-E on E-DSS1 */
        case 0x28 : /* DISPLAY: Facility AOC-E on E-DSS1 in NL, CH */
		    /* 0x28 for non-NL, non-CH is changed to 0x2800 above */
                    if ((element == 0x02) && (version == VERSION_1TR6)) {
                      n = AOC_1TR6(l, p);           /* Wieviele Einheiten? */

                      if (type == AOCD_1TR6) {
                        n = -n;  /* Negativ: laufende Verbindung */
                                 /* ansonsten wars ein AOCE */
                        print_msg(PRT_DEBUG_DECODE, " DEBUG> %s: 1TR6 AOCD %i\n", st + 4, n);
                      } /* if */
                    }
                    else {
		      /* maybe better to check for element == 0x28 */
		      if (mycountrynum==CCODE_NL || mycountrynum==CCODE_CH) {
				n = AOC_1TR6(l, p);
		      }
		      else {
				n = facility(l, p);
		      }
                      if (n == AOC_OTHER) {
                        if (asnm && *asnm) {
                          (void)iprintf(s1, -1, mlabel, "", asnm, "\n");
                          print_msg(PRT_SHOWNUMBERS, "%s", s1);
                        } /* if */
                      }
                      else {

                        /* Dirty-Hack: Falls auch AOC-E als AOC-D gemeldet wird:

                           Ist Fehler in der VSt! Wird gerne bei nachtraeglicher
                           Beantragung von AOC-D falsch eingestellt :-(
                           -> Telekom treten!
                        */

                        if ((type != FACILITY) && (n < 0)) {
                          aoc_debug(-1, "DIRTY-HACK: AOC-D -> AOC-E");
                          n = -n;
                        } /* if */

#if 1 /* AK:24-Apr-99 */
                        if (n < 0) { /* AOC-D */
                          if (call[chan].aoce == UNKNOWN) /* Firsttime */
                            call[chan].aoce = 1;
			  else
                            call[chan].aoce++;
                        }
                        else if (currency_mode == AOC_UNITS)
                          call[chan].aoce = n;

                        call[chan].aocpay = abs(n) * currency_factor;

                        if (n < 0) {
                          tx = cur_time - call[chan].connect;

			  if (!ehInterval || !call[chan].lasteh ||
			      (cur_time - call[chan].lasteh) >= ehInterval) {
			    call[chan].lasteh=cur_time;
                            if (tx)
                              sprintf(s, "%d.EH %s %s (%s)",
                                abs(call[chan].aoce),
                                currency,
                                double2str(call[chan].aocpay, 6, 3, DEB),
                                double2clock(tx));
                            else
                              sprintf(s, "%d.EH %s %s",
                                abs(call[chan].aoce),
                                currency,
                                double2str(call[chan].aocpay, 6, 3, DEB));

                            info(chan, PRT_SHOWAOCD, STATE_AOCD, s);
			  }
                        } /* if */
#endif

                        if (n < 0)
                          sprintf(s, "aOC-D=%d", -n);
                        else if (!n)
                          sprintf(s, "aOC-E=FREE OF CHARGE");
                        else
                          sprintf(s, "aOC-E=%d", n);
                        aoc_debug(-1, s);

                        if (!n) {
			  /* Fixme: DTAG is specific to Germany */
			  /* Only DTAG sends AOCD */
                          if (mycountrynum == CCODE_DE &&
			      call[chan].provider == DTAG)
                            info(chan, PRT_SHOWAOCD, STATE_AOCD, "Free of charge");
                        }
                        else if (n < 0) {
                          tx = cur_time - call[chan].connect;

                          if ((c = call[chan].confentry[OTHER]) > -1) {
			    tack = call[chan].Rate.Duration;
                            err  = call[chan].tick - tx;
                            call[chan].tick += tack;

                            if (message & PRT_SHOWTICKS)
                              sprintf(s, "%d.EH %s %s (%s %d) C=%s",
                                abs(call[chan].aoce),
                                currency,
                                double2str(call[chan].aocpay, 6, 3, DEB),
                                tx ? double2clock(tx) : "", (int)err,
                                double2clock(call[chan].tick - tx) + 4);
                            else {
                              if (tx)
                                sprintf(s, "%d.EH %s %s (%s)",
                                  abs(call[chan].aoce),
                                  currency,
                                  double2str(call[chan].aocpay, 6, 3, DEB),
                                  double2clock(tx));
                              else
                                sprintf(s, "%d.EH %s %s",
                                  abs(call[chan].aoce),
                                  currency,
                                  double2str(call[chan].aocpay, 6, 3, DEB));
                            } /* else */
                          }
                          else if (-n > 1) { /* try to guess Gebuehrenzone */
                            err = 0;
                            px = "";

                            if (message & PRT_SHOWTICKS)
                              sprintf(s, "%d.EH %s %s (%s %d %s?) C=%s",
                                abs(call[chan].aoce),
                                currency,
                                double2str(call[chan].aocpay, 6, 3, DEB),
                                tx ? double2clock(tx) : "", (int)err, px,
                                double2clock(call[chan].tick - tx) + 4);
                            else {
                              if (tx)
                                sprintf(s, "%d.EH %s %s (%s)",
                                  abs(call[chan].aoce),
                                  currency,
                                  double2str(call[chan].aocpay, 6, 3, DEB),
                                  double2clock(tx));
                              else
                                sprintf(s, "%d.EH %s %s",
                                  abs(call[chan].aoce),
                                  currency,
                                  double2str(call[chan].aocpay, 6, 3, DEB));
                            } /* else */
                          }
                          else {
                            sprintf(s, "%d.EH %s %s",
                              abs(call[chan].aoce),
                              currency,
                              double2str(call[chan].aocpay, 6, 3, DEB));
                          } /* else */

#if 0
                          info(chan, PRT_SHOWAOCD, STATE_AOCD, s);
#endif

                          if (sound)
                            ringer(chan, RING_AOCD);

                          /* kostenpflichtiger Rausruf (wg. FACILITY) */
                          /* muss mit Teles-Karte sein, da eigene MSN bekannt */
                          /* seit 2 Gebuehrentakten kein Traffic mehr! */

                          if (!replay && watchdog && ((c = call[chan].confentry[OTHER]) > -1)) {
                            if ((type == FACILITY) && (version == VERSION_EDSS1) && expensive(call[chan].bchan) && (*INTERFACE > '@')) {
                              if (call[chan].aoce > call[chan].traffic + watchdog + 2)
                                emergencyStop(chan, 4);
                              else if (call[chan].aoce > call[chan].traffic + watchdog + 1)
                                emergencyStop(chan, 3);
                              else if (call[chan].aoce > call[chan].traffic + watchdog)
                                emergencyStop(chan, 2);
                              else if (call[chan].aoce > call[chan].traffic + watchdog - 1)
                                emergencyStop(chan, 1);
                            } /* if */
                          } /* if */
                        }
                      } /* if */
                    } /* if */

                    p += (l * 3);
                    break;

	case 0x03 : /* Date/Time 1TR6   */
        case 0x29 : /* Date/Time E-DSS1 */
                    if ((element == 0x03) && (version == VERSION_1TR6)) {
			if (l != 17)	/* 1TR6 date/time is always 17? */
				/* "Unknown Codeset 7 attribute 3 size 5" */
				goto UNKNOWN_ELEMENT;
			tm.tm_mday  = (strtol(p+=3,NIL,16)-'0') * 10;
			tm.tm_mday +=  strtol(p+=3,NIL,16)-'0';
			p += 3;	/* skip '.' */
			tm.tm_mon   = (strtol(p+=3,NIL,16)-'0') * 10;
			tm.tm_mon  +=  strtol(p+=3,NIL,16)-'0' - 1;
			p += 3;	/* skip '.' */
			tm.tm_year  = (strtol(p+=3,NIL,16)-'0') * 10;
			tm.tm_year +=  strtol(p+=3,NIL,16)-'0';
			if (tm.tm_year < 70)
			    tm.tm_year += 100;
			p += 3; /* skip '-' */
			tm.tm_hour  = (strtol(p+=3,NIL,16)-'0') * 10;
			tm.tm_hour +=  strtol(p+=3,NIL,16)-'0';
			p += 3; /* skip ':' */
			tm.tm_min   = (strtol(p+=3,NIL,16)-'0') * 10;
			tm.tm_min  +=  strtol(p+=3,NIL,16)-'0';
			p += 3; /* skip ':' */
			tm.tm_sec   = (strtol(p+=3,NIL,16)-'0') * 10;
			tm.tm_sec  +=  strtol(p+=3,NIL,16)-'0';
		    }
		    else if ((element == 0x29) && (version != VERSION_1TR6)) {
			tm.tm_year  = strtol(p += 3, NIL, 16);
			if (tm.tm_year < 70)
			  tm.tm_year += 100;
			tm.tm_mon   = strtol(p += 3, NIL, 16) - 1;
			tm.tm_mday  = strtol(p += 3, NIL, 16);
			tm.tm_hour  = strtol(p += 3, NIL, 16);
			tm.tm_min   = strtol(p += 3, NIL, 16);
			if (l > 5)
			  tm.tm_sec = strtol(p += 3, NIL, 16);
			else
			  tm.tm_sec = 0;
		    }
		    else {
			goto UNKNOWN_ELEMENT; /* no choice... */
		    }
                    tm.tm_wday  = tm.tm_yday = 0;
                    tm.tm_isdst = -1;

                    t = mktime(&tm);

                    if (t != (time_t)-1) {
                      call[chan].time = t;

                      if (settime) {
                        auto time_t     tn;
			auto struct tms tms;

			time(&tn);

			if (labs(tn - call[chan].time) > 61) {
                          (void)stime(&call[chan].time);

                          /* Nicht gerade sauber, sollte aber all zu
                             grosse Spruenge verhindern! */

                          if (replay)
                            cur_time = tt = tto = call[chan].time;
                          else {
                            time(&cur_time);
                            tt = tto = times(&tms);
                          } /* else */

                          set_time_str();

                        } /* if */

                        if (settime == 1)
                          settime--;
                      } /* if */
                    } /* if */

                    sprintf(s, "Time:%s", (t == (time_t)-1) ? "INVALID - ignored" : ctime(&call[chan].time));
                    if ((px = strchr(s, '\n')))
                      *px = 0;

                    if (Q931dmp) {
                      sprintf(s1, "Y=%02d M=%02d D=%02d H=%02d M=%02d",
                        tm.tm_year,
			tm.tm_mon + 1,
			tm.tm_mday,
			tm.tm_hour,
			tm.tm_min);
                      Q931dump(TYPE_STRING, -2, s1, version);
                      Q931dump(TYPE_STRING, -2, s + 5, version);
                    } /* if */
                    info(chan, PRT_SHOWTIME, STATE_TIME, s);
                    break;


        case 0x4c : /* COLP */
                    oc3 = strtol(p += 3, NIL, 16);

                    if (oc3 < 128) { /* Octet 3a : Screening indicator */
                      oc3a = strtol(p += 3, NIL, 16);
                      l--;
                    }
                    else
                      oc3a = -1;

                    pd = s;

                    while (--l)
                      *pd++ = strtol(p += 3, NIL, 16);

                    *pd = 0;

                    if (ignoreCOLP && !Q931dmp) /* FIXME */
                      break;

                    if (!*s) {
                      info(chan, PRT_SHOWNUMBERS, STATE_RING, "COLP *INVALID* -- ignored!");
                      break;
                    } /* if */

                    if (dual && !*s)
                      strcpy(s, call[chan].onum[CALLED]);
                    else
                      strcpy(call[chan].onum[CALLED], s);

                    /* bei "national" numbers evtl. fuehrende "0" davor */
                    if (((oc3 & 0x70) == 0x20) && (*s != '0')) {
                      sprintf(s1, "0%s", s);
                      strcpy(s, s1);
                    } /* if */

                    buildnumber(s, oc3, oc3a, call[chan].num[CALLED], version, &call[chan].provider, &call[chan].sondernummer[CALLED], &call[chan].intern[CALLED], &call[chan].local[CALLED], 0, CALLED);

                    if (!dual)
                      strcpy(call[chan].vnum[CALLED], vnum(chan, CALLED));

                    if (Q931dmp && (*call[chan].vnum[CALLED] != '?') && *call[chan].vorwahl[CALLED]
			&& oc3 && ((oc3 & 0x70) != 0x40)) {
                      auto char s[BUFSIZ];

                      sprintf(s, "%s %s/%s, %s",
                        call[chan].areacode[CALLED],
                        call[chan].vorwahl[CALLED],
                        call[chan].rufnummer[CALLED],
                        call[chan].area[CALLED]);

                      Q931dump(TYPE_STRING, -2, s, version);
                    } /* if */

                    sprintf(s1, "COLP %s", call[chan].vnum[CALLED]);
                    info(chan, PRT_SHOWNUMBERS, STATE_RING, s1);

                    break;


        case 0x6c : /* Calling party number */
                    oc3 = strtol(p += 3, NIL, 16);

                    if (oc3 < 128) { /* Octet 3a : Screening indicator */
                      oc3a = strtol(p += 3, NIL, 16);
                      l--;
                    }
                    else
                      oc3a = -1;

                    /* Screening-Indicator:
                       -1 : UNKNOWN
                        0 : User-provided, not screened
                        1 : User-provided, verified and passed
                        2 : User-provided, verified and failed
                        3 : Network provided
                    */

                    pd = s;

                    while (--l)
                      *pd++ = strtol(p += 3, NIL, 16);

                    *pd = 0;

                    warn = 0;

                    if (*call[chan].onum[CALLING]) { /* another Calling-party? */
                      if (strcmp(call[chan].onum[CALLING], s)) { /* different! */

                        if (ignoreCOLP && !Q931dmp) /* FIXME */
                          break;

                        if ((call[chan].screening == 3) && ((oc3a & 3) < 3)) { /* we believe the first one! */
                          strcpy(call[chan].onum[CLIP], s);
                          buildnumber(s, oc3, oc3a, call[chan].num[CLIP], version, &call[chan].provider, &call[chan].sondernummer[CLIP], &call[chan].intern[CLIP], &call[chan].local[CLIP], 0, 0);
                          strcpy(call[chan].vnum[CLIP], vnum(6, CLIP));
                          if (Q931dmp && (*call[chan].vnum[CLIP] != '?') && *call[chan].vorwahl[CLIP]
			      && oc3 && ((oc3 & 0x70) != 0x40)) {
                            auto char s[BUFSIZ];

                            sprintf(s, "%s %s/%s, %s",
                              call[chan].areacode[CLIP],
                              call[chan].vorwahl[CLIP],
                              call[chan].rufnummer[CLIP],
                              call[chan].area[CLIP]);

                            Q931dump(TYPE_STRING, -2, s, version);
                          } /* if */

                          sprintf(s1, "CLIP %s", call[chan].vnum[CLIP]);
                          info(chan, PRT_SHOWNUMBERS, STATE_RING, s1);

                          break;
                        }
                        else {
                          warn = 1;

			  strcpy(call[chan].onum[CLIP],      call[chan].onum[CALLING]);
			  strcpy(call[chan].num[CLIP],       call[chan].num[CALLING]);
			  strcpy(call[chan].vnum[CLIP],      call[chan].vnum[CALLING]);
			  call[chan].confentry[CLIP] = call[chan].confentry[CALLING];
			  strcpy(call[chan].areacode[CLIP],  call[chan].areacode[CALLING]);
			  strcpy(call[chan].vorwahl[CLIP],   call[chan].vorwahl[CALLING]);
			  strcpy(call[chan].rufnummer[CLIP], call[chan].rufnummer[CALLING]);
			  strcpy(call[chan].alias[CLIP],     call[chan].alias[CALLING]);
			  strcpy(call[chan].area[CLIP],      call[chan].area[CALLING]);

                          /* fall thru, and overwrite ... */
                        } /* else */
                      } /* else */
		    } /* else */

                    call[chan].screening = (oc3a & 3);

                    strcpy(call[chan].onum[CALLING], s);
                    buildnumber(s, oc3, oc3a, call[chan].num[CALLING], version, &call[chan].provider, &call[chan].sondernummer[CALLING], &call[chan].intern[CALLING], &call[chan].local[CALLING], call[chan].dialin, CALLING);

                    strcpy(call[chan].vnum[CALLING], vnum(chan, CALLING));
                    if (Q931dmp && (*call[chan].vnum[CALLING] != '?') && *call[chan].vorwahl[CALLING]
			&& oc3 && ((oc3 & 0x70) != 0x40)) {
                      auto char s[BUFSIZ];

                      sprintf(s, "%s %s/%s, %s",
                        call[chan].areacode[CALLING],
                        call[chan].vorwahl[CALLING],
                        call[chan].rufnummer[CALLING],
                        call[chan].area[CALLING]);

                      Q931dump(TYPE_STRING, -2, s, version);
                    } /* if */

		    if (callfile && call[chan].dialin) {
		      FILE *cl = fopen(callfile, "a");

		    /* Fixme: what is short for 'Calling Party Number'? */
		    sprintf(s1, "CPN %s", call[chan].num[CALLING]);
		    info(chan, PRT_SHOWNUMBERS, STATE_RING, s1);

		      if (cl != NULL) {
			iprintf(s1, chan, callfmt);
			fprintf(cl, "%s\n", s1);
			fclose(cl);
		      } /* if */
		    } /* if */

                    if (warn) {
                      sprintf(s1, "CLIP %s", call[chan].vnum[CLIP]);
                      info(chan, PRT_SHOWNUMBERS, STATE_RING, s1);
                    } /* if */

                    break;


        case 0x70 : /* Called party number */
                    oc3 = strtol(p += 3, NIL, 16);

                    pd = s;

                    while (--l)
                      *pd++ = strtol(p += 3, NIL, 16);

                    *pd = 0;

                    if (dual && ((type == INFORMATION) || ((type == SETUP) && OUTGOING))) { /* Digit's beim waehlen mit ISDN-Telefon */
                      strcat(call[chan].digits, s);
                      strcpy(call[chan].onum[CALLED], s);
                      call[chan].oc3 = oc3;
                      if (Q931dmp)
                        buildnumber(s, oc3, -1, call[chan].num[CALLED], version, &call[chan].provider, &call[chan].sondernummer[CALLED], &call[chan].intern[CALLED], &call[chan].local[CALLED], call[chan].dialin, CALLED);

                      buildnumber(call[chan].digits, oc3, -1, call[chan].num[CALLED], version, &call[chan].provider, &call[chan].sondernummer[CALLED], &call[chan].intern[CALLED], &call[chan].local[CALLED], call[chan].dialin, CALLED);

		      strcpy(call[chan].vnum[CALLED], vnum(chan, CALLED));

                      if (dual > 1) {
                        auto char sx[BUFSIZ];

                        if (*call[chan].vorwahl[CALLED])
			  sprintf(sx, "DIALING %s [%s] %s %s/%s, %s",
                            s, call[chan].digits,
                            call[chan].areacode[CALLED],
                            call[chan].vorwahl[CALLED],
                            call[chan].rufnummer[CALLED],
                            call[chan].area[CALLED]);
                        else
                          sprintf(sx, "DIALING %s [%s]", s, call[chan].digits);

			info(chan, PRT_SHOWNUMBERS, STATE_RING, sx);
                      } /* if */
		    }
                    else {
                      strcpy(call[chan].onum[CALLED], s);
                      buildnumber(s, oc3, -1, call[chan].num[CALLED], version, &call[chan].provider, &call[chan].sondernummer[CALLED], &call[chan].intern[CALLED], &call[chan].local[CALLED], call[chan].dialin, CALLED);

                      strcpy(call[chan].vnum[CALLED], vnum(chan, CALLED));
                      if (Q931dmp && (*call[chan].vnum[CALLED] != '?') && *call[chan].vorwahl[CALLED]
			  && oc3 && ((oc3 & 0x70) != 0x40)) {
                        auto char s[BUFSIZ];

			sprintf(s, "%s %s/%s, %s",
                          call[chan].areacode[CALLED],
                          call[chan].vorwahl[CALLED],
                          call[chan].rufnummer[CALLED],
                          call[chan].area[CALLED]);

                        Q931dump(TYPE_STRING, -2, s, version);
                      } /* if */

                      /* This message comes before bearer capability */
                      /* So dont show it here, show it at Bearer capability */

                      if (version != VERSION_1TR6) {
                        if (call[chan].knock)
			  info(chan, PRT_SHOWNUMBERS, STATE_RING, "********************");

                        sprintf(s, "RING (%s)", call[chan].service);
			info(chan, PRT_SHOWNUMBERS, STATE_RING, s);

                        if (call[chan].knock) {
			  info(chan, PRT_SHOWNUMBERS, STATE_RING, "NO FREE B-CHANNEL !!");
			  info(chan, PRT_SHOWNUMBERS, STATE_RING, "********************");
#ifdef Q931
			  if (!q931dmp) {
#endif
			    call[chan].connect = call[chan].disconnect = cur_time;
                            call[chan].cause = -2;
			    logger(chan);
#ifdef Q931
			  } /* if */
#endif
                        } /* if */

			if (sound)
                          ringer(chan, RING_RING);
                      } /* if */
                    } /* else */
                    break;


        case 0x74 : /* Redirecting number */
        case 0x76 : /* Redirection number */

                    oc3 = strtol(p += 3, NIL, 16);

                    pd = s;

                    while (--l)
                      *pd++ = strtol(p += 3, NIL, 16);

                    *pd = 0;

                    strcpy(call[chan].onum[REDIR], s);
                    buildnumber(s, oc3, -1, call[chan].num[REDIR], version, &call[chan].provider, &call[chan].sondernummer[REDIR], &call[chan].intern[REDIR], &call[chan].local[REDIR], 0, 0);

                    strcpy(call[chan].vnum[REDIR], vnum(chan, REDIR));
                    if (Q931dmp && (*call[chan].vnum[REDIR] != '?') && *call[chan].vorwahl[REDIR]
			&& oc3 && ((oc3 & 0x70) != 0x40)) {
                      auto char s[BUFSIZ];

                      sprintf(s, "%s %s/%s, %s",
                        call[chan].areacode[REDIR],
                        call[chan].vorwahl[REDIR],
                        call[chan].rufnummer[REDIR],
                        call[chan].area[REDIR]);

                      Q931dump(TYPE_STRING, -2, s, version);
                    } /* if */
                    break;


        case 0x01 : /* Bearer capability 1TR6 */
                    if (l > 0)
                      call[chan].bearer = strtol(p + 3, NIL, 16);
                    else
                      call[chan].bearer = 1; /* Analog */

                    px = s;
                    if (!Q931dmp)
                      px += sprintf(px, "RING (");

                    px += sprintf(px, "%s", qmsg(TYPE_SERVICE, version, call[chan].bearer));

                    if (Q931dmp) {
                      Q931dump(TYPE_STRING, call[chan].bearer, s, version);

                      if (l > 1) {
                        c = strtol(p + 6, NIL, 16);
                        sprintf(s1, "octet 3a=%d", c);
                        Q931dump(TYPE_STRING, c, s1, version);
                      } /* if */
                    } /* if */

                    px += sprintf(px, ")");
                    info(chan, PRT_SHOWNUMBERS, STATE_RING, s);

                    if (sound)
                      ringer(chan, RING_RING);

                    p += (l * 3);
                    break;


        case 0x04 : /* Bearer capability E-DSS1 */

                    clearchan(chan, 0);

                    pd = p + (l * 3);
                    l1 = l;

                    sxp = 0;

                    if (--l1 < 0) {
                      p = pd;
                      goto escape;
                    } /* if */

                    c = strtol(p += 3, NIL, 16);       /* Octet 3 */

                    px = sx[sxp];
                    *px = 0;
                    sn[sxp] = c;

                    if (!Q931dmp)
                        px += sprintf(px, "BEARER: ");

                    /* Mapping from E-DSS1 Bearer capability to 1TR6 Service Indicator: */

                    switch (c & 0x1f) {
                      case 0x00 : px += sprintf(px, "Speech");                           /* "CCITT Sprache" */
                                  call[chan].si1 = 1;
                                  call[chan].si11 = 1;
                                  strcpy(call[chan].service, "Speech");
                                  break;

                      case 0x08 : px += sprintf(px, "Unrestricted digital information"); /* "uneingeschr„nkte digitale Information" */
                                  call[chan].si1 = 7;
                                  call[chan].si11 = 0;
                                  strcpy(call[chan].service, "Data");
                                  break;

                      case 0x09 : px += sprintf(px, "Restricted digital information");  /* "eingeschr„nkte digitale Information" */
                                  call[chan].si1 = 2;
                                  call[chan].si11 = 0;
                                  strcpy(call[chan].service, "Fax G3");
                                  break;

                      case 0x10 : px += sprintf(px, "3.1 kHz audio");                   /* "3,1 kHz audio" */
                                  call[chan].si1 = 1;
                                  call[chan].si11 = 0;
                                  strcpy(call[chan].service, "3.1 kHz audio");
                                  break;

                      case 0x11 : px += sprintf(px, "Unrestricted digital information with tones/announcements"); /* "uneingeschr„nkte digitale Ton-Inform." */
                                  call[chan].si1 = 3;
                                  call[chan].si11 = 0;
                                  break;

                      case 0x18 : px += sprintf(px, "Video");                           /* "Video" */
                                  call[chan].si1 = 4;
                                  call[chan].si11 = 0;
                                  strcpy(call[chan].service, "Fax G4");
                                  break;

                        default : px += sprintf(px, "Service %d", c & 0x1f);
                                  sprintf(call[chan].service, "Service %d", c & 0x1f);
                                  break;
                    } /* switch */

                    switch (c & 0x60) {
                      case 0x00 : px += sprintf(px, ", CCITT standardized coding");        break;
                      case 0x20 : px += sprintf(px, ", ISO/IEC");                          break;
                      case 0x40 : px += sprintf(px, ", National standard");                break;
                      case 0x60 : px += sprintf(px, ", Standard defined for the network"); break;
                    } /* switch */

                    if (--l1 < 0) {
                      p = pd;
                      goto escape;
                    } /* if */

                    c = strtol(p += 3, NIL, 16);       /* Octet 4 */
                    px = sx[++sxp];
                    *px = 0;
                    sn[sxp] = c;

                    switch (c & 0x1f) {
                      case 0x10 : px += sprintf(px, "64 kbit/s");     break;
                      case 0x11 : px += sprintf(px, "2 * 64 kbit/s"); break;
                      case 0x13 : px += sprintf(px, "384 kbit/s");    break;
                      case 0x15 : px += sprintf(px, "1536 kbit/s");   break;
                      case 0x17 : px += sprintf(px, "1920 kbit/s");   break;

                      case 0x18 : oc3 = strtol(p += 3, NIL, 16); /* Octet 4.1 */
                                  px += sprintf(px, ", %d kbit/s", 64 * oc3 & 0x7f);
                                  break;

                    } /* switch */

                    switch (c & 0x60) {
                      case 0x00 : px += sprintf(px, ", Circuit mode"); break;
                      case 0x40 : px += sprintf(px, ", Packet mode");  break;
                    } /* switch */

                    if (--l1 < 0) {
                      p = pd;
                      goto escape;
                    } /* if */

                    c = strtol(p += 3, NIL, 16);

                    if ((c & 0x60) == 0x20) { /* User information layer 1 */
                      int ch = ' ';

                      do {
                        switch (ch) {
                          case ' ' : px = sx[++sxp]; /* Octet 5 */
                                     *px = 0;
                                     sn[sxp] = c;

                                     switch (c & 0x1f) {
                                       case 0x01 : px += sprintf(px, "CCITT standardized rate adaption V.110/X.30");               break;
                                       case 0x02 : px += sprintf(px, "G.711 u-law");                                               break;
                                       case 0x03 : px += sprintf(px, "G.711 A-law");                                               break;
                                       case 0x04 : px += sprintf(px, "G.721 32 kbit/s ADPCM (I.460)");                             break;
                                       case 0x05 : px += sprintf(px, "H.221/H.242");                                               break;
                                       case 0x07 : px += sprintf(px, "Non-CCITT standardized rate adaption");                      break;
                                       case 0x08 : px += sprintf(px, "CCITT standardized rate adaption V.120");                    break;
                                       case 0x09 : px += sprintf(px, "CCITT standardized rate adaption X.31, HDLC flag stuffing"); break;
                                     } /* switch */

                                     break;

                          case 'a' : px = sx[++sxp]; /* Octet 5a */
                                     *px = 0;
                                     sn[sxp] = c;

                                     switch (c & 0x1f) {
                                       case 0x01 : px += sprintf(px, "0.6 kbit/s");       break;
                                       case 0x02 : px += sprintf(px, "1.2 kbit/s");       break;
                                       case 0x03 : px += sprintf(px, "2.4 kbit/s");       break;
                                       case 0x04 : px += sprintf(px, "3.6 kbit/s");       break;
                                       case 0x05 : px += sprintf(px, "4.8 kbit/s");       break;
                                       case 0x06 : px += sprintf(px, "7.2 kbit/s");       break;
                                       case 0x07 : px += sprintf(px, "8 kbit/s");         break;
                                       case 0x08 : px += sprintf(px, "9.6 kbit/s");       break;
                                       case 0x09 : px += sprintf(px, "14.4 kbit/s");      break;
                                       case 0x0a : px += sprintf(px, "16 kbit/s");        break;
                                       case 0x0b : px += sprintf(px, "19.2 kbit/s");      break;
                                       case 0x0c : px += sprintf(px, "32 kbit/s");        break;
                                       case 0x0e : px += sprintf(px, "48 kbit/s");        break;
                                       case 0x0f : px += sprintf(px, "56 kbit/s");        break;
                                       case 0x15 : px += sprintf(px, "0.1345 kbit/s");    break;
                                       case 0x16 : px += sprintf(px, "0.100 kbit/s");     break;
                                       case 0x17 : px += sprintf(px, "0.075/1.2 kbit/s"); break;
                                       case 0x18 : px += sprintf(px, "1.2/0.075 kbit/s"); break;
                                       case 0x19 : px += sprintf(px, "0.050 kbit/s");     break;
                                       case 0x1a : px += sprintf(px, "0.075 kbit/s");     break;
                                       case 0x1b : px += sprintf(px, "0.110 kbit/s");     break;
                                       case 0x1c : px += sprintf(px, "0.150 kbit/s");     break;
                                       case 0x1d : px += sprintf(px, "0.200 kbit/s");     break;
                                       case 0x1e : px += sprintf(px, "0.300 kbit/s");     break;
                                       case 0x1f : px += sprintf(px, "12 kbit/s");        break;
                                     } /* switch */

                                     switch (c & 0x40) {
                                       case 0x00 : px += sprintf(px, ", Synchronous");    break;
                                       case 0x40 : px += sprintf(px, ", Asynchronous");   break;
                                     } /* switch */

                                     switch (c & 0x20) {
                                       case 0x00 : px += sprintf(px, ", In-band negotiation not possible"); break;
                                       case 0x20 : px += sprintf(px, ", In-band negotiation possible");     break;
                                     } /* switch */

                                     break;

                          case 'b' : px = sx[++sxp]; /* Octet 5b */
                                     *px = 0;
                                     sn[sxp] = c;

                                     switch (c & 0x60) {
                                       case 0x20 : px += sprintf(px, "8 kbit/s");  break;
                                       case 0x40 : px += sprintf(px, "16 kbit/s"); break;
                                       case 0x60 : px += sprintf(px, "32 kbit/s"); break;
                                     } /* switch */

                                     break;
                        } /* switch */

                        ch = (ch == ' ') ? 'a' : ch + 1;

                        if (--l1 < 0) {
                          p = pd;
                          goto escape;
                        } /* if */

                        c = strtol(p += 3, NIL, 16);
                      } while (!(c & 0x80));
                    } /* if */

                    if ((c & 0x60) == 0x40) { /* User information layer 2 */
                      px = sx[++sxp];
                      *px = 0;
                      sn[sxp] = c;

                      switch (c & 0x1f) {
                        case 0x02 : px += sprintf(px, "Q.931/I.441");        break;
                        case 0x06 : px += sprintf(px, "X.25, packet layer"); break;
                      } /* switch */

                      if (--l1 < 0) {
                        p = pd;
                        goto escape;
                      } /* if */

                      c = strtol(p += 3, NIL, 16);
                    } /* if */

                    if ((c & 0x60) == 0x60) { /* User information layer 3 */
                      px = sx[++sxp];
                      *px = 0;
                      sn[sxp] = c;

                      switch (c & 0x1f) {
                        case 0x02 : px += sprintf(px, "Q.931/I.451");        break;
                        case 0x06 : px += sprintf(px, "X.25, packet layer"); break;
                      } /* switch */

                    } /* if */

escape:             for (c = 0; c <= sxp; c++)
                      if (Q931dmp)
                        Q931dump(TYPE_STRING, sn[c], sx[c], version);
                      else
                        if (*sx[c])
                          info(chan, PRT_SHOWBEARER, STATE_RING, sx[c]);

                    p = pd;

                    break;


        case 0x18 : /* Channel identification */
                    c = strtol(p + 3, NIL, 16);

                    sxp = 0;
                    px = sx[sxp];
                    *px = 0;
                    sn[sxp] = c;

                    if (!Q931dmp)
                      px += sprintf(px, "CHANNEL: ");

                    switch (c) {
                      case 0x80 : px += sprintf(px, "BRI, none requested");
                                  call[chan].knock = 1;			  break;
                      case 0x81 : px += sprintf(px, "BRI, B1 requested"); break;
                      case 0x82 : px += sprintf(px, "BRI, B2 requested"); break;
                      case 0x83 : px += sprintf(px, "BRI, any channel");  break;
                      case 0x89 : px += sprintf(px, "BRI, B1 needed");    break;
                      case 0x8a : px += sprintf(px, "BRI, B2 needed");    break;
                      case 0x84 : px += sprintf(px, "BRI, D requested");  break;
                      case 0x8c : px += sprintf(px, "BRI, D needed");     break;
                      case 0xa0 : px += sprintf(px, "PRI, no channel");   break;
                      case 0xa1 : px += sprintf(px, "PRI, channel to be indicated later");
									  break;
                      case 0xa3 : px += sprintf(px, "PRI, indicated channel requested");
									  break;
                      case 0xa9 : px += sprintf(px, "PRI, indicated channel needed");
									  break;
                      case 0xac : px += sprintf(px, "PRI, D needed");     break;
                      case 0xe0 : px += sprintf(px, "no channel");        break;
                      case 0xe1 : px += sprintf(px, "channel to be indicated later");
									  break;
                      case 0xe3 : px += sprintf(px, "any channel");       break;
                      case 0xe9 : px += sprintf(px, "Nur der nachst. angegeb. Kanal ist akzeptabel"); break;
                    } /* switch */

                    if (Q931dmp)
                      Q931dump(TYPE_STRING, sn[0], sx[0], version);
                    else
                      info(chan, PRT_SHOWBEARER, STATE_RING, sx[0]);

                    if (c == 0x8a)
                      call[chan].channel = 2;
                      /* Jetzt eine 1 fuer Kanal 1 und 2 fuer die 2.
                         0 heisst unbekannt. chan muss dann spaeter
                         auf channel - 1 gesetzt werden.
                         Beim Parken bleibt der Kanal belegt (bei mir jedenfalls)
                         und neue Verbindungen kriegen vom Amt den anderen */
                    else if (c == 0x89)
                      call[chan].channel = 1;
                    else
                      call[chan].channel = 0;
                    p += (l * 3);
                    break;


        case 0x1e : /* Progress indicator */
                    sxp = 0;

                    px = sx[sxp];
                    *px = 0;

                    c = strtol(p + 3, NIL, 16);
                    sn[sxp] = c;

		    if (!Q931dmp)
		      px += sprintf(px, "PROGRESS: ");
		    px += sprintf(px, "%s", location(c & 0x0f));

                    if (l > 1) {
                      px = sx[++sxp];
                      *px = 0;

                      if (!Q931dmp)
                        px += sprintf(px, "PROGRESS: ");

                      c = strtol(p + 6, NIL, 16);
                      sn[sxp] = c;

                      switch (c) {
#ifdef LANG_DE
                        case 0x81 : px += sprintf(px, "Der Ruf verlaeuft nicht vom Anfang bis zum Ende im ISDN"); break;
                        case 0x82 : px += sprintf(px, "Zieladresse ist kein ISDN-Anschluss");                     break;
                        case 0x83 : px += sprintf(px, "(Ab)Sendeadresse ist kein ISDN-Anschluss");                break;
                        case 0x84 : px += sprintf(px, "Ruf ist zum ISDN zurueckgekehrt");                         break;
                        case 0x88 : px += sprintf(px, "Inband Information available");                            break;
#else
                        case 0x81 : px += sprintf(px, "call is not end-to-end ISDN"); break;
                        case 0x82 : px += sprintf(px, "destination address is non ISDN");                     break;
                        case 0x83 : px += sprintf(px, "origination address is not an ISDN connection"); break;
                        case 0x84 : px += sprintf(px, "call has returned to the ISDN");                         break;
                        case 0x88 : px += sprintf(px, "inband information available");                            break;
#endif
                      } /* switch */
                    } /* if */

                    for (c = 0; c <= sxp; c++)
                      if (Q931dmp)
                        Q931dump(TYPE_STRING, sn[c], sx[c], version);
                      else if (*sx[c])
                        info(chan, PRT_SHOWBEARER, STATE_RING, sx[c]);

                    p += (l * 3);
                    break;


        case 0x27 : /* Notification indicator */
                    sxp = 0;
                    px = sx[sxp];
                    *px = 0;

                    c = strtol(p + 3, NIL, 16);
                    sn[sxp] = c;

                    if (!Q931dmp)
                      px += sprintf(px, "NOTIFICATION: ");

                    switch (c) {
                      case 0x80 : px += sprintf(px, "Nutzer legt auf");                                        break;
                      case 0x81 : px += sprintf(px, "Nutzer nimmt wieder auf");                                break;
                      case 0x82 : px += sprintf(px, "Wechsel des Uebermittlungsdienstes");                     break;
                      case 0x83 : px += sprintf(px, "Discriminator for extension to ASN.1 encoded component"); break;
                      case 0x84 : px += sprintf(px, "Call completion delay");                                  break;
                      case 0xc2 : px += sprintf(px, "Conference established");                                 break;
                      case 0xc3 : px += sprintf(px, "Conference disconnected");                                break;
                      case 0xc4 : px += sprintf(px, "Other party added");                                      break;
                      case 0xc5 : px += sprintf(px, "Isolated");                                               break;
                      case 0xc6 : px += sprintf(px, "Reattached");                                             break;
                      case 0xc7 : px += sprintf(px, "Other party isolated");                                   break;
                      case 0xc8 : px += sprintf(px, "Other party reattached");                                 break;
                      case 0xc9 : px += sprintf(px, "Other party split");                                      break;
                      case 0xca : px += sprintf(px, "Other party disconnected");                               break;
                      case 0xcb : px += sprintf(px, "Conference floating");                                    break;
                      case 0xcf : px += sprintf(px, "Conference floating, served user preemted");              break;
                      case 0xcc : px += sprintf(px, "Conference disconnected, preemtion");                     break;
                      case 0xf9 : px += sprintf(px, "Remote hold");                                            break;
                      case 0xfa : px += sprintf(px, "Remote retrieval");                                       break;
                      case 0xe0 : px += sprintf(px, "Call is a waiting call");                                 break;
                      case 0xfb : px += sprintf(px, "Call is diverting");                                      break;
                      case 0xe8 : px += sprintf(px, "Diversion activated");                                    break;
                      case 0xee : px += sprintf(px, "Reverse charging");                                       break;
                    } /* switch */

                    if (Q931dmp)
                      Q931dump(TYPE_STRING, sn[0], sx[0], version);
                    else
                      info(chan, PRT_SHOWNUMBERS, STATE_RING, sx[0]);

                    p += (l * 3);
                    break;


        case 0x7d : /* High layer compatibility */
                    if (l > 1) {
                      sxp = 0;
                      px = sx[sxp];
                      *px = 0;

                      c = strtol(p + 3, NIL, 16);
                      sn[sxp] = c;

                      if (!Q931dmp)
                        px += sprintf(px, "HLC: ");

                      switch (c) {
                        case 0x91 : px += sprintf(px, "CCITT");    break;
                        case 0xb1 : px += sprintf(px, "Reserv.");  break;
                        case 0xd1 : px += sprintf(px, "national"); break;
                        case 0xf1 : px += sprintf(px, "Eigendef"); break;
                      } /* switch */

                      if (Q931dmp) {
                        px = sx[++sxp];
			*px = 0;
                      } /* if */

                      c = strtol(p + 6, NIL, 16);
                      sn[sxp] = c;

                      if (strlen(sx[sxp]))
                        px += sprintf(px, ", ");

                      switch (c) {
                        case 0x81 : px += sprintf(px, "Telefonie");                                        break;
                        case 0x84 : px += sprintf(px, "Fax Gr.2/3 (F.182)");                               break;
                        case 0xa1 : px += sprintf(px, "Fax Gr.4 (F.184)");                                 break;
                        case 0xa4 : px += sprintf(px, "Teletex service,basic and mixed-mode");             break;
                        case 0xa8 : px += sprintf(px, "Teletex service,basic and processab.-mode of Op."); break;
                        case 0xb1 : px += sprintf(px, "Teletex service,basic mode of operation");          break;
                        case 0xb2 : px += sprintf(px, "Syntax based Videotex");                            break;
                        case 0xb3 : px += sprintf(px, "International Videotex interworking via gateway");  break;
                        case 0xb5 : px += sprintf(px, "Telex service");                                    break;
                        case 0xb8 : px += sprintf(px, "Message Handling Systems (MHS)(X.400)");            break;
                        case 0xc1 : px += sprintf(px, "OSI application (X.200)");                          break;
                        case 0xde :
                        case 0x5e : px += sprintf(px, "Reserviert fuer Wartung");                          break;
                        case 0xdf :
                        case 0x5f : px += sprintf(px, "Reserviert fuer Management");                       break;
                        case 0xe0 : px += sprintf(px, "Audio visual");                                     break;
                          default : px += sprintf(px, "unknown: %d", c);                                   break;
                      } /* switch */

                      if ((c == 0x5e) || (c == 0x5f)) {
		        if (Q931dmp) {
                          px = sx[++sxp];
                          *px = 0;
                        } /* if */

                        c = strtol(p + 9, NIL, 16);
                        sn[sxp] = c;

                        if (strlen(sx[sxp]))
                          px += sprintf(px, ", ");

                        switch (c) {
                          case 0x81 : px += sprintf(px, "Telefonie G.711");                                                  break;
                          case 0x84 : px += sprintf(px, "Fax Gr.4 (T.62)");                                                  break;
                          case 0xa1 : px += sprintf(px, "Document Appl. Profile for Fax Gr4 (T.503)");                       break;
                          case 0xa4 : px += sprintf(px, "Doc.Appl.Prof.for formatted Mixed-Mode(T501)");                     break;
                          case 0xa8 : px += sprintf(px, "Doc.Appl.Prof.for Processable-form (T.502)");                       break;
                          case 0xb1 : px += sprintf(px, "Teletex (T.62)");                                                   break;
                          case 0xb2 : px += sprintf(px, "Doc.App.Prof. for Videotex interworking between Gateways (T.504)"); break;
                          case 0xb5 : px += sprintf(px, "Telex");                                                            break;
                          case 0xb8 : px += sprintf(px, "Message Handling Systems (MHS)(X.400)");                            break;
                          case 0xc1 : px += sprintf(px, "OSI application (X.200)");                                          break;
                        } /* case */
                      } /* if */

                      for (c = 0; c <= sxp; c++)
                        if (Q931dmp)
                          Q931dump(TYPE_STRING, sn[c], sx[c], version);
                        else if (*sx[c])
                          info(chan, PRT_SHOWNUMBERS, STATE_RING, sx[c]);

                    } /* if */

                    p += (l * 3);
                    break;


        default   : {
                      register char *p1, *p2;
                      register int  i;
UNKNOWN_ELEMENT:      p1 = p; p2 = s;

                      for (i = 0; i < l; i++)
                        p2 += sprintf(p2, "%02x ", (int)strtol(p1 += 3, NIL, 16));

                      p2 += sprintf(p2, "\"");
                      p1 = p;

                      for (i = 0; i < l; i++) {
                        c = (int)strtol(p1 += 3, NIL, 16);
                        p2 += sprintf(p2, "%c", isgraph(c) ? c : ' ');
                      } /* for */

                      p2 += sprintf(p2, "\"");

                      if (allflags & PRT_DEBUG_DECODE)
                        print_msg(PRT_DEBUG_DECODE, " DEBUG> %s: ELEMENT=0x%02x :%s\n", st + 4, element, s);
                      if (Q931dmp)
                        Q931dump(TYPE_STRING, -4, s, version);
                    } /* if */

                    p += (l * 3);
                    break;
      } /* switch */

    }
    else if (Q931dmp) {
      if (version == VERSION_1TR6) {
        switch ((element >> 4) & 7) {
          case 1 : sprintf(s, "%02x ---> Shift %d (cs=%d, cs_fest=%d)", element, element & 0xf, element & 7, element & 8);
                   break;

          case 3 : sprintf(s, "%02x ---> Congestion level %d", element, element & 0xf);
                   break;

          case 2 : if (element == 0xa0)
                     sprintf(s, "%02x ---> More data", element);
                   else if (element == 0xa1)
                     sprintf(s, "%02x ---> Sending complete", element);
                   break;

         default : sprintf(s, "%02x ---> Reserved %d", element, element);
                   break;
        } /* switch */

        Q931dump(TYPE_STRING, -3, s, version);
      }
      else if (version == VERSION_EDSS1) {
        switch ((element >> 4) & 7) {
          case 1 : sprintf(s, "%02x ---> Shift %d", element, element & 0xf);
                   break;

          case 3 : sprintf(s, "%02x ---> Congestion level %d", element, element & 0xf);
                   break;

          case 5 : sprintf(s, "%02x ---> Repeat indicator %d", element, element & 0xf);
                   break;

          case 2 : if (element == 0x90)
                     sprintf(s, "%02x ---> Umschaltung in eine andere Codegruppe %d\n", element, element);
                   if (element == 0xa0)
                     sprintf(s, "%02x ---> More data", element);
                   else if (element == 0xa1)
                     sprintf(s, "%02x ---> Sending complete", element);
                   break;

         default : sprintf(s, "%02x ---> Reserved %d\n", element, element);
                   break;
        } /* switch */

        Q931dump(TYPE_STRING, -3, s, version);
      } /* else */
    } /* else */
  } /* while */
} /* decode */

/* --------------------------------------------------------------------------
   call reference:


    1 ..  63 DIALIN's    - ist dabei 8. Bit gesetzt, meine Antwort an VSt
                           (cref=1 : VSt->User // cref=129 : User->VSt)

   64 .. 127 DIALOUT's   - ist dabei 8. Bit gesetzt, Antwort der VSt an mich
                           (cref=64 : User->VSt // cref=192 : VSt->User)

   kommt ein SETUP ACKNOWLEDGE mit cref > 128, beginnt ein DIALOUT (!)
   _nicht_ mit der Teles-Karte

   kommt ein CALL PROCEEDING mit cref > 191, beginnt ein DIALOUT
   mit der 2. Teles-Karte

   folgt danach sofort ein SETUP, ist das ein Selbstanruf!


   DIALOUT's erhalten vom Teles-Treiber staendig eine um jeweils 1
   erhoehte call references
   War die letzte cref also < 127, und die naechste = 64, bedeutet dies
   einen Reload des Teles-Treibers!
-------------------------------------------------------------------------- */


void dotrace(void)
{
  register int  i;
  auto     char s[BUFSIZ];


  print_msg(PRT_NORMAL, ">>>>>>> TRACE (CR=next, q=quit, d=dump, g=go):");
  fgets(s, BUFSIZ, stdin);

  if (*s == 'q')
    exit(0);
  else if (*s == 'g')
    trace = 0;
  else if (*s == 'd') {

    print_msg(PRT_NORMAL, "chan=%d\n", chan);

    for (i = 0; i < MAXCHAN; i++) {
      if (call[i].state) {
        print_msg(PRT_NORMAL, "call[%d]:", i);
        print_msg(PRT_NORMAL, "state=%d, cref=%d, dialin=%d, cause=%d\n",
          call[i].state, call[i].cref, call[i].dialin, call[i].cause);
        print_msg(PRT_NORMAL, "\taoce=%d, channel=%d, dialog=%d, bearer=%d\n",
          call[i].aoce, call[i].channel, call[i].dialog, call[i].bearer);
        print_msg(PRT_NORMAL, "\tnum[0]=\"%s\", num[1]=\"%s\"\n",
          call[i].num[0], call[i].num[1]);
        print_msg(PRT_NORMAL, "\tvnum[0]=\"%s\", vnum[1]=\"%s\"\n",
          call[i].vnum[0], call[i].vnum[1]);
        print_msg(PRT_NORMAL, "\tconfentry[0]=%d,  confentry[1]=%d\n",
          call[i].confentry[0], call[i].confentry[1]);
        print_msg(PRT_NORMAL, "\ttime=%d, connect=%d, disconnect=%d, duration=%d\n",
          (int)call[i].time, (int)call[i].connect, (int)call[i].disconnect, (int)call[i].duration);
      } /* if */
    } /* for */

    dotrace();

  } /* if */
} /* dotrace */


static int b2c(register int b)
{
  register int i;


  for (i = 0; i < chans; i++)
    if ((call[i].bchan == b) && call[i].dialog)
      return(i);

  return(-1);
} /* b2c */


/* NET_DV since 'chargeint' field exists */
#define	NETDV_CHARGEINT		0x02

static void huptime(int chan, int setup)
{
  register int                c = call[chan].confentry[OTHER];
  auto     isdn_net_ioctl_cfg cfg;
  auto     int                oldchargeint = 0, newchargeint = 0;
  auto     int                oldhuptimeout, newhuptimeout;
  auto     char               sx[BUFSIZ], why[BUFSIZ];


  if (replay)
    net_dv = 4;

  if (hupctrl && (c != UNKNOWN) && (*INTERFACE > '@') /* && expensive(call[chan].bchan) */) {
    memset(&cfg, 0, sizeof(cfg)); /* clear in case of older kernel */

    strcpy(cfg.name, INTERFACE);

    if (replay || (ioctl(sockets[ISDNCTRL].descriptor, IIOCNETGCF, &cfg) >= 0)) {
#if NET_DV >= NETDV_CHARGEINT
      if (net_dv >= NETDV_CHARGEINT)
        call[chan].chargeint = oldchargeint = cfg.chargeint;
#endif
      call[chan].huptimeout = oldhuptimeout = cfg.onhtime;

      if (!oldhuptimeout && !replay) {
        sprintf(sx, "HUPTIMEOUT %s is *disabled* - unchanged", INTERFACE);
        info(chan, PRT_SHOWNUMBERS, STATE_HUPTIMEOUT, sx);
	return;
      } /* if */

      if (call[chan].tarifknown)
        /*
         * The Linklevel dont like a CHARGEINT < 10
         * so we (sadely) use 11 instead
         *
         */
	newchargeint = max(11, (int)(call[chan].Rate.Duration + 0.5));
      else
        newchargeint = UNKNOWN;

      *why = 0;

#if NET_DV >= NETDV_CHARGEINT
      if (net_dv >= NETDV_CHARGEINT) {
        if (hup1 && hup2)
          newhuptimeout = (newchargeint < 20) ? hup1 : hup2;
        else
          newhuptimeout = oldhuptimeout;

        /* der erste Versuch, dem einmaligen Verbindungsentgelt
           (DM 0,06/Anwahl) zu entkommen ... */
        if (call[chan].Rate.Basic) /* wenn es eine Grundgebuehr gibt (z.b. T-Online eco) */
          newhuptimeout = hup3;
      }
      else
#endif
        /* for old kernels/kernel headers use old behaviour: hangup is charge
         * time minus -h param */
        if (hup1) {
          newhuptimeout = newchargeint - hup1;
          oldchargeint = newchargeint;
        }
        else
          newhuptimeout = oldhuptimeout;

      if (((oldchargeint != newchargeint) || (oldhuptimeout != newhuptimeout)) && (newchargeint != UNKNOWN)) {
#if NET_DV >= NETDV_CHARGEINT
        if (net_dv >= NETDV_CHARGEINT)
          call[chan].chargeint = cfg.chargeint = newchargeint;
#endif
        call[chan].huptimeout = cfg.onhtime = newhuptimeout;

        if (replay || (ioctl(sockets[ISDNCTRL].descriptor, IIOCNETSCF, &cfg) >= 0)) {
          sprintf(sx, "CHARGEINT %s %d (was %d)%s%s",
            INTERFACE, newchargeint, oldchargeint, (*why ? " - " : ""), why);

          info(chan, PRT_INFO, STATE_HUPTIMEOUT, sx);

            sprintf(sx, "HUPTIMEOUT %s %d (was %d)",
              INTERFACE, newhuptimeout, oldhuptimeout);

            info(chan, PRT_INFO, STATE_HUPTIMEOUT, sx);
          } /* if */
      }
      else {
        sprintf(sx, "CHARGEINT %s still %d%s%s", INTERFACE,
          oldchargeint, (*why ? " - " : ""), why);
        info(chan, PRT_SHOWNUMBERS, STATE_HUPTIMEOUT, sx);

        sprintf(sx, "HUPTIMEOUT %s still %d", INTERFACE, oldhuptimeout);
        info(chan, PRT_SHOWNUMBERS, STATE_HUPTIMEOUT, sx);
      } /* else */
    } /* if */
  } /* if */
} /* huptime */


static void oops(int where)
{
  auto   char              s[BUFSIZ];
  auto   isdn_ioctl_struct ioctl_s;
  auto   int               cmd;
  static int               loop = 0;


  if (!replay) {
    strcpy(ioctl_s.drvid, ifo[0].id);
    ioctl_s.arg = 4;
    cmd = 1;

    if ((++loop == 2) || (ioctl(sockets[ISDNCTRL].descriptor, IIOCDRVCTL + cmd, &ioctl_s) < 0)) {
      info(0, PRT_ERR, STATE_AOCD, "FATAL: Please enable D-Channel logging with:");
      sprintf(s, "FATAL: \"hisaxctrl %s 1 4\"", ifo[0].id);
      info(0, PRT_ERR, STATE_AOCD, s);
      sprintf(s, "FATAL: and restart isdnlog! (#%d)\007", where);
      info(0, PRT_ERR, STATE_AOCD, s);

      Exit(34);
    } /* if */
  } /* if */

  sprintf(s, "WARNING \"hisaxctrl %s 1 4\" called! (#%d)", ifo[0].id, where);
  info(0, PRT_ERR, STATE_AOCD, s);

} /* oops */

#ifdef __i386__
# if IIOCNETGPN == -1
extern int iiocnetgpn();
# endif
#endif

static int findinterface(void)
{
#ifdef IIOCNETGPN
  register char *p;
  auto	   FILE *iflst;
  auto	   char  s[255], name[10], sx[BUFSIZ];
  union {
     struct {
	char name[10];
	char phone[20];
	int  outgoing;
     }	   netdv5_phone;
     struct {
	char name[10];
	char phone[32];
	int  outgoing;
     }	   netdv6_phone;
#if NET_DV != 0x05 && NET_DEV != 0x06
     isdn_net_ioctl_phone netdvX_phone;
#endif
  }	   phone;
  auto	   int rc, chan, l1, l2, lmin, lmax, ldiv, match;

  if ((iflst = fopen("/proc/net/dev", "r")) == NULL)
    return(-1);

  while (fgets(s, sizeof(s), iflst)) {
    if ((p = strchr(s, ':')) == NULL)
      continue;

    *p = 0;
    sscanf(s, "%9s", name);
    if (*name != 'i') /* only ipppX is this ok?  -lt */
      continue;

    memset(&phone, 0, sizeof(phone));
    if (net_dv == 0x05 || net_dv == 0x06)
      /* don't need strncpy, name is null term'ed and same size as phone.name */
      strcpy(phone.netdv5_phone.name, name);
    else /* assume version when compiled */
#if NET_DV != 0x05 && NET_DEV != 0x06
      strncpy(phone.netdvX_phone.name, name, sizeof(phone.netdvX_phone.name)-1);
#else
      {
      fclose(iflst);
      return -1; /* can't happen? Versions should have been checked */
      }
#endif
/* call emulation for 2.0 kernels */
#ifdef __i386__
# if IIOCNETGPN == -1
     /* IIOCDBGVAR works on isdnctrl */
    rc = iiocnetgpn(sockets[ISDNCTRL].descriptor, &phone);
# else
    rc = ioctl(sockets[ISDNINFO].descriptor, IIOCNETGPN, &phone);
# endif
#else
    rc = ioctl(sockets[ISDNINFO].descriptor, IIOCNETGPN, &phone);
#endif
    if (rc) {
      if (errno == EINVAL) {
	info(chan, PRT_SHOWNUMBERS, STATE_HUPTIMEOUT, "Sorry, IIOCNETGPN not available in your kernel (2.2.12 or higher is required)");
	fclose(iflst);
	return(0);
      }
      continue;	/* check next interface in /proc/net/dev */
    }

    /* 'name' connected from/to 'phone.phone' */

    /* .phone *will* be NULL-terminated, barring any bugs in kernel code */
    if (net_dv == 0x05 || net_dv == 0x06)
      p = phone.netdv5_phone.phone;	/* same offset in 0x05 and 0x06 */
#if NET_DV != 0x05 && NET_DEV != 0x06
    else
      p = phone.netdvX_phone.phone;	/* unknown offset */
#endif
    l1 = strlen(p);

    for (chan = 0; chan < MAXCHAN; chan++) {
      l2 = strlen(call[chan].onum[OTHER]);
      if (!l2)
	continue;	/* check next channel */

      lmin = min(l1, l2);
      lmax = max(l1, l2);
      ldiv = lmax - lmin;

      if (lmin == lmax)
	match = !strcmp(p, call[chan].onum[OTHER]);
      else if (l1 > l2)
	match = !strcmp(p + ldiv, call[chan].onum[OTHER]);
      else
	match = !strcmp(p, call[chan].onum[OTHER] + ldiv);

      if (match) {
	strcpy(call[chan].interface, name);
	/* "fnum" -> Fritz-Num: phonenumber that Fritz (Elfert) */
	/* alias Link-Level maintains internally (that is, for  */
	/* example possibly with leading "R")                   */
	strcpy(call[chan].fnum, p);

	sprintf(sx, "INTERFACE %s %s %s", call[chan].interface,
	  (call[chan].dialin ? "called by" : "calling"),
	  call[chan].fnum);
	info(chan, PRT_SHOWNUMBERS, STATE_HUPTIMEOUT, sx);

	break;
      } /* if */
    } /* for */
  } /* while */

  fclose(iflst);

  return(1);
#else
  return(0);
#endif
} /* findinterface */


static void processbytes()
{
  register int     bchan, chan, change = 0;
  auto     char    sx[BUFSIZ], sy[BUFSIZ], sz[BUFSIZ];
  auto     time_t  DiffTime = (time_t)0;
  auto     int     hup = 0, eh = 0, hx;
#if SHOWTICKS
  auto     double  tack;
#endif
#if RATE_PER_SAMPLE
  auto     double  DiffTime2;
#endif


  for (bchan = 0; bchan < chans; bchan++)
    if (((ifo[bchan].u & ISDN_USAGE_MASK) == ISDN_USAGE_NET) ||
        ((ifo[bchan].u & ISDN_USAGE_MASK) == ISDN_USAGE_MODEM)) {

#if FUTURE
      if (!hexSeen)
        oops(1);
#endif

      if ((chan = b2c(bchan)) != -1) {

        *sy = 0;

        if (io[bchan].i > call[chan].ibytes) {
          call[chan].libytes = call[chan].ibytes;
          call[chan].ibytes = io[bchan].i;
          change++;
        } /* if */

        if (io[bchan].o > call[chan].obytes) {
          call[chan].lobytes = call[chan].obytes;
          call[chan].obytes = io[bchan].o;
          change++;
        } /* if */

        if (change) {
          call[chan].traffic = call[chan].aoce;

          if (!hexSeen)
            oops(3);
        } /* if */

#if 0 /* Fixme: why the hell should we call huptime() here? */
        if (fullhour) /* zu jeder vollen Stunde HANGUP-Timer neu setzen (aendern sich um: 9:00, 12:00, 18:00, 21:00, 2:00, 5:00 Uhr) */
          huptime(chan, bchan, 0);
#endif

        DiffTime = cur_time - call[chan].connect;

        if (call[chan].chargeint && DiffTime) {
          hup = (int)(call[chan].chargeint - (DiffTime % call[chan].chargeint) - 2);

          if (hup < 0)
            hup = 0;

          eh = (DiffTime / (time_t)call[chan].chargeint) + 1;

          if (ifo[bchan].u & ISDN_USAGE_OUTGOING) {
            sprintf(sy, "  H#%d=%3ds", eh, hup);

            hx = max(call[chan].chargeint, call[chan].huptimeout);

            if (hx > call[chan].chargeint) {
              hup = (int)(hx - (DiffTime % hx) - 2);

              if (hup < 0)
                hup = 0;

              eh = (DiffTime / (time_t)hx) + 1;
              sprintf(sz, " (#%d=%3ds)", eh, hup);
              strcat(sy, sz);
            } /* if */
          } /* if */
        } /* if */

        if (DiffTime) {
          call[chan].ibps = (double)(call[chan].ibytes / (double)(DiffTime));
          call[chan].obps = (double)(call[chan].obytes / (double)(DiffTime));
        }
        else
          call[chan].ibps = call[chan].obps = 0.0;

        if (change && (call[chan].ibytes + call[chan].obytes)) {

          sprintf(sx, "I=%s %s/s  O=%s %s/s%s",
            double2byte((double)call[chan].ibytes),
            double2byte((double)call[chan].ibps),
            double2byte((double)call[chan].obytes),
            double2byte((double)call[chan].obps),
            sy);
#if SHOWTICKS
          if ((message & PRT_SHOWTICKS) && (tack = call[chan].tick - (double)(cur_time - call[chan].connect)) > 0.0)
            sprintf(sy, " C=%s", double2clock(tack) + 4);
          else
            *sy = 0;

          sprintf(sx, "I=%s %s/s  O=%s %s/s%s",
            double2byte((double)call[chan].ibytes),
            double2byte((double)call[chan].ibps),
            double2byte((double)call[chan].obytes),
            double2byte((double)call[chan].obps),
            sy);
#endif

          info(chan, PRT_SHOWBYTE, STATE_BYTE, sx);

#if RATE_PER_SAMPLE
          if ((DiffTime2 = ((double)(tt - tto) / (double)CLK_TCK))) {
            auto long   ibytes = call[chan].ibytes - call[chan].libytes;
            auto long   obytes = call[chan].obytes - call[chan].lobytes;
            auto double ibps = (double)ibytes / (double)DiffTime2;
            auto double obps = (double)obytes / (double)DiffTime2;


            sprintf(sx, "I=%s %s/s  O=%s %s/s (%4.4gs)",
              double2byte(ibytes),
              double2byte(ibps),
              double2byte(obytes),
              double2byte(obps),
              (double)DiffTime2);

            info(chan, PRT_SHOWBYTE, STATE_BYTE, sx);
          } /* if */
#endif
        }
        else if (DiffTime) {
          sprintf(sx, "I=%s %s/s  O=%s %s/s%s",
            double2byte((double)call[chan].ibytes),
            double2byte((double)call[chan].ibps),
            double2byte((double)call[chan].obytes),
            double2byte((double)call[chan].obps),
            sy);

            info(chan, PRT_SHOWBYTE, STATE_BYTE, sx);
        } /* else */
      } /* if */
    } /* if */
} /* processbytes */


static void processinfo(char *s)
{
  register char *p;
  register int   j, k, chan, version;
  auto     char  sx[BUFSIZ];


  if (verbose & VERBOSE_INFO)
    print_msg(PRT_LOG, "%s\n", s);


  if (!memcmp(s, "idmap:", 6)) {
    j = sscanf(s + 7, "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s",
      ifo[ 0].id, ifo[ 1].id, ifo[ 2].id, ifo[ 3].id,
      ifo[ 4].id, ifo[ 5].id, ifo[ 6].id, ifo[ 7].id,
      ifo[ 8].id, ifo[ 9].id, ifo[10].id, ifo[11].id,
      ifo[12].id, ifo[13].id, ifo[14].id, ifo[15].id, ifo[16].id);

    if (!newcps && (j == 17)) {
      newcps = 1;

      for (chans = 0; chans < 17; chans++)
        if (!strcmp(ifo[chans].id, "-"))
          break;

      if (!Q931dmp) {
        print_msg(PRT_NORMAL, "(ISDN subsystem with ISDN_MAX_CHANNELS > 16 detected, ioctl(IIOCNETGPN) is %savailable)\n",
          (IIOCNETGPNavailable = findinterface()) ? "" : "un");
        print_msg(PRT_NORMAL, "isdn.conf:%d active channels, %d MSN/SI entries\n", chans, mymsns);

        if (dual) {
          if (hfcdual)
            print_msg(PRT_NORMAL, "(watching \"%s\" as HFC/echo mode)\n", isdnctrl);
          else
            print_msg(PRT_NORMAL, "(watching \"%s\" and \"%s\")\n", isdnctrl, isdnctrl2);
        } /* if */
      } /* if */

      /*
       * Ab "ISDN subsystem Rev: 1.21/1.20/1.14/1.10/1.6" gibt's den ioctl(IIOCGETDVR)
       *
       * Letzte Version davor war "ISDN subsystem Rev: 1.18/1.18/1.13/1.9/1.6"
       */

      if (!replay) {
        if ((version = ioctl(sockets[ISDNINFO].descriptor, IIOCGETDVR)) != -EINVAL) {
#ifdef NET_DV
          int my_net_dv = NET_DV;
#else
          int my_net_dv = 0;
#endif

          tty_dv = version & 0xff;
          version = version >> 8;
          net_dv = version & 0xff;
          version = version >> 8;
          inf_dv = version & 0xff;

          print_msg(PRT_NORMAL, "(Data versions: iprofd=0x%02x  net_cfg=0x%02x  /dev/isdninfo=0x%02x)\n", tty_dv, net_dv, inf_dv);

          if (/* Abort if kernel version is greater, since struct has probably
               * become larger and would overwrite our stack */
              (net_dv > my_net_dv && net_dv != 6 && my_net_dv != 5) ||
	      /* don't know what net_dv > 0x06 will do... */
	      /* findinterface() will probably have to be re-engineered */
	      (net_dv != my_net_dv && net_dv > 0x06) ||
              /* version 0x03 is special, because it changed a field in the
               * middle of the struct and thus is compatible only to itself */
	      (net_dv != my_net_dv && (net_dv == 0x03 || my_net_dv == 0x03))) {
            print_msg(PRT_ERR, "FATAL: isdn_net_ioctl_cfg version mismatch "
                      "(kernel 0x%02x, isdnlog 0x%02x). Please upgrade your Linux-Kernel and/or your I4L-utils.\n",
                      net_dv, my_net_dv);
            Exit(99);
          } /* if */
        } /* if */

        if (IIOCNETGPNavailable)
          print_msg(PRT_NORMAL, "Everything is fine, isdnlog-%s is running in full featured mode.\n", VERSION);
        else
          print_msg(PRT_NORMAL, "HINT: Please upgrade to Linux-2.2.12 or higher for all features of isdnlog-%s\n", VERSION);
      } /* if */

      if (chans > 2) /* coming soon ;-) */
        chans = 2;
    } /* if */
  }
  else if (!memcmp(s, "chmap:", 6))
    sscanf(s + 7, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
      &ifo[ 0].ch, &ifo[ 1].ch, &ifo[ 2].ch, &ifo[ 3].ch,
      &ifo[ 4].ch, &ifo[ 5].ch, &ifo[ 6].ch, &ifo[ 7].ch,
      &ifo[ 8].ch, &ifo[ 9].ch, &ifo[10].ch, &ifo[11].ch,
      &ifo[12].ch, &ifo[13].ch, &ifo[14].ch, &ifo[15].ch);
  else if (!memcmp(s, "drmap:", 6))
    sscanf(s + 7, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
      &ifo[ 0].dr, &ifo[ 1].dr, &ifo[ 2].dr, &ifo[ 3].dr,
      &ifo[ 4].dr, &ifo[ 5].dr, &ifo[ 6].dr, &ifo[ 7].dr,
      &ifo[ 8].dr, &ifo[ 9].dr, &ifo[10].dr, &ifo[11].dr,
      &ifo[12].dr, &ifo[13].dr, &ifo[14].dr, &ifo[15].dr);
  else if (!memcmp(s, "usage:", 6))
    sscanf(s + 7, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
      &ifo[ 0].u, &ifo[ 1].u, &ifo[ 2].u, &ifo[ 3].u,
      &ifo[ 4].u, &ifo[ 5].u, &ifo[ 6].u, &ifo[ 7].u,
      &ifo[ 8].u, &ifo[ 9].u, &ifo[10].u, &ifo[11].u,
      &ifo[12].u, &ifo[13].u, &ifo[14].u, &ifo[15].u);
  else if (!memcmp(s, "flags:", 6))
    sscanf(s + 7, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
      &ifo[ 0].f, &ifo[ 1].f, &ifo[ 2].f, &ifo[ 3].f,
      &ifo[ 4].f, &ifo[ 5].f, &ifo[ 6].f, &ifo[ 7].f,
      &ifo[ 8].f, &ifo[ 9].f, &ifo[10].f, &ifo[11].f,
      &ifo[12].f, &ifo[13].f, &ifo[14].f, &ifo[15].f);
  else if (!memcmp(s, "phone:", 6)) {
    sscanf(s + 7, "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s",
      ifo[ 0].n, ifo[ 1].n, ifo[ 2].n, ifo[ 3].n,
      ifo[ 4].n, ifo[ 5].n, ifo[ 6].n, ifo[ 7].n,
      ifo[ 8].n, ifo[ 9].n, ifo[10].n, ifo[11].n,
      ifo[12].n, ifo[13].n, ifo[14].n, ifo[15].n);

    for (j = 0; j < chans; j++)
      if (ifo[j].u & ISDN_USAGE_MASK) {

#if FUTURE
        if (!hexSeen)
          oops(2);
#endif

        for (chan = 0; chan < MAXCHAN; chan++)
          if (memcmp(ifo[j].n, "???", 3) && !strcmp(ifo[j].n, (IIOCNETGPNavailable == 1) ? call[chan].fnum : call[chan].onum[OTHER])) {
            call[chan].bchan = j;

            strcpy(call[chan].id, ifo[j].id);

            if (!(ifo[j].u & ISDN_USAGE_MASK)) /* no connection */
              strcpy(call[chan].usage, (ifo[j].u & ISDN_USAGE_EXCLUSIVE) ? "Exclusive" : "Offline");
            else {
              switch (ifo[j].u & ISDN_USAGE_MASK) {
                case ISDN_USAGE_RAW   : sprintf(call[chan].usage, "%s %s", (ifo[j].u & ISDN_USAGE_OUTGOING) ? "Outgoing" : "Incoming", "Raw");
                                        break;

                case ISDN_USAGE_MODEM : sprintf(call[chan].usage, "%s %s", (ifo[j].u & ISDN_USAGE_OUTGOING) ? "Outgoing" : "Incoming", "Modem");
                                        break;

                case ISDN_USAGE_NET   : sprintf(call[chan].usage, "%s %s", (ifo[j].u & ISDN_USAGE_OUTGOING) ? "Outgoing" : "Incoming", "Net");
                                        break;

                case ISDN_USAGE_VOICE : sprintf(call[chan].usage, "%s %s", (ifo[j].u & ISDN_USAGE_OUTGOING) ? "Outgoing" : "Incoming", "Voice");
                                        break;

                case ISDN_USAGE_FAX   : sprintf(call[chan].usage, "%s %s", (ifo[j].u & ISDN_USAGE_OUTGOING) ? "Outgoing" : "Incoming", "Fax");
                                        break;
              } /* switch */
            } /* else */

#if 0 /* Fixme: why the hell should we call huptime() here? */
            huptime(chan, j, 1); /* bei Verbindungsbeginn HANGUP-Timer neu setzen */
#endif
          } /* if */
      } /* if */

    if (imon) {
      print_msg(PRT_SHOWIMON, "\n+ %s -----------------------------------------\n", st + 4);

      for (j = 0; j < chans; j++) {

        p = sx;

        p += sprintf(p, "| %s#%d : ", ifo[j].id, j & 1);

        if (!(ifo[j].u & ISDN_USAGE_MASK)) /* no connection */
          p += sprintf(p, (ifo[j].u & ISDN_USAGE_EXCLUSIVE) ? "exclusive" : "free");
        else {
          p += sprintf(p, "%s\t", (ifo[j].u & ISDN_USAGE_OUTGOING) ? "outgoing" : "incoming");

          switch (ifo[j].u & ISDN_USAGE_MASK) {
            case ISDN_USAGE_RAW   : p += sprintf(p, "raw device");
                                    break;

            case ISDN_USAGE_MODEM : p += sprintf(p, "tty emulation");
                                    break;

            case ISDN_USAGE_NET   : p += sprintf(p, "IP interface");
                                    break;

            case ISDN_USAGE_VOICE : p += sprintf(p, "Voice");
                                    break;

            case ISDN_USAGE_FAX   : p += sprintf(p, "Fax");
                                    break;
          } /* switch */

          p += sprintf(p, "\t%s", ifo[j].n);

          if ((chan = b2c(j)) != -1) {
            k = call[chan].dialin ? CALLING : CALLED;

            p += sprintf(p, " (%s/%s, %s)",
                   call[chan].vorwahl[k],
                   call[chan].rufnummer[k],
                   call[chan].area[k]);

          } /* if */
        } /* else */

        print_msg(PRT_SHOWIMON, "%s\n", sx);

      } /* for */
    } /* if */
  }
  else if (!memcmp(s, "ibytes:", 7))
    sscanf(s + 8, "%ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld",
      &io[ 0].i, &io[ 1].i, &io[ 2].i, &io[ 3].i,
      &io[ 4].i, &io[ 5].i, &io[ 6].i, &io[ 7].i,
      &io[ 8].i, &io[ 9].i, &io[10].i, &io[11].i,
      &io[12].i, &io[13].i, &io[14].i, &io[15].i);
  else if (!memcmp(s, "obytes:", 7)) {
    sscanf(s + 8, "%ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld",
      &io[ 0].o, &io[ 1].o, &io[ 2].o, &io[ 3].o,
      &io[ 4].o, &io[ 5].o, &io[ 6].o, &io[ 7].o,
      &io[ 8].o, &io[ 9].o, &io[10].o, &io[11].o,
      &io[12].o, &io[13].o, &io[14].o, &io[15].o);

    processbytes();
  } /* else */
} /* processinfo */


void clearchan(int chan, int total)
{
  register int i;


  if (total) {
    memset((char *)&call[chan], 0, sizeof(CALL));
    call[chan].tei = BROADCAST;
  }
  else
    for (i = 0; i < MAXMSNS; i++)
      *call[chan].onum[i] =
      *call[chan].num[i] =
      *call[chan].interface = 0;

  call[chan].bchan = -1;

  call[chan].cause = -1;
  call[chan].loc = -1;
  call[chan].aoce = -1;

  call[chan].provider = -1;
  call[chan].zone = -1;
  call[chan].pay = -1; /* lt for aocpay to work */

  for (i = 0; i < MAXMSNS; i++) {
    strcpy(call[chan].vnum[i], "?");

    call[chan].confentry[i] = UNKNOWN;
    call[chan].sondernummer[i] = UNKNOWN;
    call[chan].intern[i] = 0;
  } /* for */
} /* clearchan */


static void addlist(int chan, int type, int mode) /* mode :: 0 = Add new entry, 1 = change existing entry, 2 = Terminate entry, 3 = dump */
{

#define MAXLIST 1000

  typedef struct {
    int	    state;
    char   *vnum[2];
    int	    si;
    time_t  connect;
    time_t  disconnect;
    int	    cause;
    int	    uid;
  } LIST;

  static      LIST  list[MAXLIST];
  static      int   lp = -1;
  register    int   i;
  register    char *p;
  auto struct tm   *tm;
  auto	      char  s[BUFSIZ], s1[BUFSIZ];


  if (((chan == -1) || call[chan].dialin)) {
    if (mode == 0) {

      if (++lp == MAXLIST)
        lp = 0;

      list[lp].state = SETUP;
      list[lp].vnum[CALLING] = strdup(call[chan].vnum[CALLING]);
      list[lp].vnum[CALLED] = strdup(call[chan].vnum[CALLED]);
      list[lp].si = call[chan].si1;
      list[lp].connect = call[chan].connect;
      list[lp].uid = call[chan].uid;
    }
    else if ((mode == 1) || (mode == 2)) {
      for (i = lp; i >= 0; i--) {
        if (call[chan].uid == list[i].uid) {
          switch (mode) {
            case 1 : list[i].state = CONNECT;
		     break;

            case 2 : list[i].cause = call[chan].cause;
                     list[i].state = RELEASE;
		     list[i].disconnect = call[chan].disconnect;
		     break;
          } /* switch */

          break;
        } /* if */
      } /* if */
    }
    else if (mode == 3) {
      for (i = 0; i <= lp; i++) {
        tm = localtime(&list[i].connect);
	strftime(s1, 64, "%a %b %d %X", tm);

        if (!list[i].disconnect)
          list[i].disconnect = cur_time;

        switch (list[i].si) {
           case 1 : p = "Speech"; break;
           case 2 : p = "Fax G3"; break;
           case 3 : p = "Data";	  break;
           case 4 : p = "Fax G4"; break;
	   case 7 : p = "Data";	  break;
          default : p = "";       break;
        } /* switch */

	sprintf(s, "%s %s(%s) -> %s %ds %s",
          s1,
          list[i].vnum[0],
          p,
          list[i].vnum[1],
          (int)(list[i].disconnect - list[i].connect),
          qmsg(TYPE_CAUSE, VERSION_EDSS1, list[i].cause));

        print_msg(PRT_SHOWNUMBERS, "%s\n", s);
      } /* for */
    } /* else */
  } /* if */
} /* addlist */


void processRate(int chan)
{
  call[chan].Rate.start  = call[chan].connect;
  call[chan].Rate.now    = call[chan].disconnect = cur_time;

  if (call[chan].Rate.prefix == UNKNOWN)
    call[chan].tarifknown = 0;
  else if (getRate(&call[chan].Rate, NULL) == UNKNOWN)
    call[chan].tarifknown = 0;
  else {
    call[chan].tarifknown = 1;
    call[chan].pay = call[chan].Rate.Charge;
  } /* else */
} /* processRate */


static void processLCR(int chan, char *hint)
{
  auto   RATE   bestRate, bookRate, pselRate, hintRate;
  auto   char   buffer[BUFSIZ], *p;
  auto	 double pselpreis = -1.0, hintpreis = -1.0, diff;
  char   prov[TN_MAX_PROVIDER_LEN];
  auto	 int    lcr = 0;


  *hint='\0';
  *(p=buffer)='\0';

  clearRate (&pselRate);
  pselRate.prefix=preselect;
  memcpy (pselRate.src, call[chan].Rate.src, sizeof (pselRate.src));
  memcpy (pselRate.dst, call[chan].Rate.dst, sizeof (pselRate.dst));
  pselRate.start = call[chan].Rate.start;
  pselRate.now   = call[chan].Rate.now;

  hintRate = pselRate;
  hintRate.prefix=call[chan].hint;

  getLeastCost(&call[chan].Rate, &bestRate, 1, -1);
  getLeastCost(&call[chan].Rate, &bookRate, 0, -1);

  if (getRate(&pselRate, NULL) != UNKNOWN)
    pselpreis = pselRate.Charge;

  if (getRate(&hintRate, NULL) != UNKNOWN)
    hintpreis = hintRate.Charge;

  diff = call[chan].pay - bestRate.Charge;
  if (diff > 0 && (bestRate.prefix != UNKNOWN) && (bestRate.prefix != call[chan].provider)) {
    prefix2provider(bestRate.prefix, prov);
    p+=sprintf(p, "\nHINT: Cheapest booked %s:%s %s (would save %s)",
      prov, bestRate.Provider,
      printRate (bestRate.Charge),
      printRate(diff));
  }
  diff = call[chan].pay - bookRate.Charge;
  if (diff > 0 && (bookRate.prefix != UNKNOWN) && (bookRate.prefix != bestRate.prefix)) {
    prefix2provider(bookRate.prefix, prov);
    p+=sprintf(p, "\nHINT: Overall cheapest %s:%s %s (would save %s)",
      prov, bookRate.Provider,
      printRate (bookRate.Charge),
      printRate(diff));
  }
  diff = pselpreis - call[chan].pay;
  if (diff > 0 && (call[chan].provider != preselect) && (pselpreis != -1.00) && (pselpreis != call[chan].pay)) {
    prefix2provider(preselect, prov);
    p+=sprintf(p, "\nHINT: Preselect %s:%s %s (you saved %s)",
      prov, getProvider(preselect),
      printRate (pselpreis),
      printRate(diff));
      lcr++;
  }
  diff = hintpreis - call[chan].pay;
  if (diff > 0 && (call[chan].hint != UNKNOWN) && (call[chan].hint != bestRate.prefix)) {
    prefix2provider(call[chan].hint, prov);
    p+=sprintf(p, "\nHINT:  Hinted %s:%s %s (saving %s)",
      prov, getProvider(call[chan].hint),
      printRate (hintpreis),
      printRate(diff));
  }
  if (*buffer) {
    /* p+=sprintf(p, "\nHINT: LCR:%s", (bestRate.prefix == call[chan].provider) ? "OK" : "FAILED"); */
    p+=sprintf(p, "\nHINT: LCR:%s", lcr ? "OK" : "FAILED");
    sprintf (hint, "%s", buffer+1);
  }

} /* processLCR */


static void showRates(RATE *Rate, char *message)
{
  if (Rate->Basic > 0)
    sprintf(message, "CHARGE: %s + %s/%ds = %s + %s/Min (%s)",
      printRate(Rate->Basic),
      printRate(Rate->Price),
      (int)(Rate->Duration + 0.5),
      printRate(Rate->Basic),
      printRate(60 * Rate->Price / Rate->Duration),
      explainRate(Rate));
  else
    sprintf(message, "CHARGE: %s/%ds = %s/Min (%s)",
      printRate(Rate->Price),
      (int)(Rate->Duration + 0.5),
      printRate(60 * Rate->Price / Rate->Duration),
      explainRate(Rate));
} /* showRates */


static void prepareRate(int chan, char **msg, char **tip, int viarep)
{
  auto   RATE lcRate, ckRate;
  static char message[BUFSIZ];
  static char lcrhint[BUFSIZ];

  if (msg)
    *(*msg = message) = '\0';

  if (tip)
    *(*tip = lcrhint) = '\0';

  clearRate(&call[chan].Rate);

  if (call[chan].intern[CALLED]) {
    call[chan].Rate.zone = UNZONE;
    call[chan].zone = INTERN;
    call[chan].tarifknown = 0;

    if (msg)
      sprintf(message, "CHARGE: free of charge - internal call");

    return;
  } /* if */

  call[chan].Rate.prefix = call[chan].provider;

  if (call[chan].intern[CALLING]) {
    call[chan].Rate.src[0] = mycountry;
    call[chan].Rate.src[1] = myarea;
    call[chan].Rate.src[2] = "";
  }
  else {
    call[chan].Rate.src[0] = call[chan].areacode[CALLING];
    call[chan].Rate.src[1] = call[chan].vorwahl[CALLING];
    call[chan].Rate.src[2] = call[chan].rufnummer[CALLING];
  } /* else */

  if (call[chan].sondernummer[CALLED] != UNKNOWN) {
    call[chan].Rate.dst[0] = "";
    call[chan].Rate.dst[1] = call[chan].num[CALLED];
    call[chan].Rate.dst[2] = "";
  }
  else {
    call[chan].Rate.dst[0] = call[chan].areacode[CALLED];
    call[chan].Rate.dst[1] = call[chan].vorwahl[CALLED];
    call[chan].Rate.dst[2] = call[chan].rufnummer[CALLED];
  } /* else */

  if (call[chan].provider == UNKNOWN)
    return;

  if (getRate(&call[chan].Rate, msg) == UNKNOWN)
    return;

  if (call[chan].Rate.zone == FREECALL) { /* FreeCall */
    call[chan].tarifknown = 0;

    if (msg)
      sprintf(message, "CHARGE: free of charge - FreeCall");

    return;
  } /* if */

  if (call[chan].Rate.zone == UNKNOWN)
    call[chan].tarifknown = 0;
  else
    processRate(chan);

  if (viarep)
    return;

  if (msg && call[chan].tarifknown)
    showRates(&call[chan].Rate, *msg=message);

  if ((call[chan].hint = getLeastCost(&call[chan].Rate, &lcRate, 1, -1)) != UNKNOWN) {
    if (tip) {
      double diff;
      char prov[TN_MAX_PROVIDER_LEN];
      /* compute charge for LCR_DURATION seconds for used provider */
      ckRate = call[chan].Rate;
      ckRate.now = ckRate.start + LCR_DURATION;
      getRate(&ckRate, NULL);

      diff = ckRate.Charge - lcRate.Charge;
      if(diff > 0) {
        prefix2provider(lcRate.prefix, prov);
        sprintf(lcrhint, "HINT: Better use %s:%s, %s/%ds = %s/Min, saving %s/Min",
	  prov, lcRate.Provider,
	  printRate(lcRate.Price),
	  (int)(lcRate.Duration + 0.5),
	  printRate(60 * lcRate.Price / lcRate.Duration),
	  printRate(60*(diff)/lcRate.Time));
      }
    } /* if */
  } /* if */
} /* prepareRate */


#if 0
static void LCR(int chan, char *s)
{
  auto char      *why, *hint;
  auto struct tms t1, t2;
  auto long int   tr1, tr2;


  tr1 = times(&t1);

  print_msg(PRT_NORMAL, ">> LCR: OUTGOING SETUP(%s)\n", s + 5);

  print_msg(PRT_NORMAL, ">> LCR: from TEI %d, to number \"%s\", Provider=%s%d:%s, Sonderrufnummer=%d, InternalCall=%d, LocalCall=%d\n",
    call[chan].tei, call[chan].num[CALLED], vbn, call[chan].provider, getProvider(call[chan].provider),
    call[chan].sondernummer[CALLED], call[chan].intern[CALLED], call[chan].local[CALLED]);

  if (!call[chan].intern[CALLED]) {                     /* keine Hausinternen Gespr„che */
    if (!call[chan].local[CALLED]) {		        /* keine Ortsgespr„che */
      if (call[chan].sondernummer[CALLED] == UNKNOWN) { /* keine Sonderrufnummern */

        call[chan].disconnect = call[chan].connect = cur_time;
	prepareRate(chan, &why, &hint, 0);

	if (call[chan].hint == UNKNOWN)
	  print_msg(PRT_NORMAL, ">> LCR: NO ACTION: Better provider unknown :-(\n", why);
	else if (call[chan].hint == call[chan].provider)
	  print_msg(PRT_NORMAL, ">> LCR: Best provider already used!", why);
	else {
	  print_msg(PRT_NORMAL, ">> LCR: %s\n", why);
	  print_msg(PRT_NORMAL, ">> LCR: %s\n", hint);
	  tr2 = times(&t2);
	  print_msg(PRT_NORMAL, ">> LCR: FAKE! TRYING(%s%d0%s) - Time required: %8.6g s\n", vbn, call[chan].hint, call[chan].num[CALLED] + 3, (double)(tr2 - tr1) / (double)CLK_TCK);
	} /* else */
      }
      else
        print_msg(PRT_NORMAL, ">> LCR: NO ACTION: Sonderrufnummer\n");
    }
    else
      print_msg(PRT_NORMAL, ">> LCR: NO ACTION: Local call\n");
  }
  else
    print_msg(PRT_NORMAL, ">> LCR: NO ACTION: Internal call\n");
} /* LCR */
#endif

#ifdef ILP
extern void procinfo(int channel, CALL * cp, int state);
#else
void procinfo(int channel, CALL * cp, int state) {}
#endif

static void processctrl(int card, char *s)
{
  register char       *ps = s, *p;
  register int         i, c;
  register int         wegchan; /* fuer gemakelte */
  auto     int         dialin, type = 0, cref = -1, creflen, version;
  static   int         tei = BROADCAST, sapi = 0, net = 1, firsttime = 1;
  auto     char        sx[BUFSIZ], s1[BUFSIZ], s2[BUFSIZ];
  auto	   char       *why, *hint;
  auto	   char	       hints[BUFSIZ];
  static   char        last[BUFSIZ];
  auto     int         isAVMB1 = 0;
  auto     double      tx;


  hexSeen = 1;

  if (Q931dmp) {
    register int bcast = (strtol(ps + 8, NIL, 16) >> 1) == 0x7f;
    register int sr = strtol(ps + (bcast ? 20 : 23), NIL, 16);

    if (replaydev)
      fprintf(stdout, "\n\n-----[ %d ]---[ %c ]---[ %d.card ]-------------------------------------------------------------------\n\n", ++lfd, (sr > 127 ? 'S' : 'R'), card + 1);
    else
      fprintf(stdout, "\n\n-----[ %d ]---[ %c ]---[ %d.card ]---[ %s ]------------------------------------------\n\n", ++lfd, (sr > 127 ? 'S' : 'R'), card + 1, st + 4);

    if (bcast) {
      s[13] = 0;
      fprintf(stdout, "%s    %s\n\n", s + 5, s + 14);
      s[13] = ' ';
    }
    else
      fprintf(stdout, "%s\n\n", s + 5);
  } /* if */

  if (verbose & VERBOSE_CTRL)
    print_msg(PRT_LOG, "%s\n", s);

  if (!memcmp(ps, "D2", 2)) { /* AVMB1 */
    if (firsttime) {
      firsttime = 0;
      print_msg (PRT_NORMAL, "(AVM B1 driver detected (D2))");
    } /* if */
    memcpy(ps, "HEX: ", 5);
  } /* if */

  if (!memcmp(ps, "DTRC:", 5)) { /* Eicon Driver */
    if (firsttime) {
      firsttime = 0;
      print_msg (PRT_NORMAL, "(Eicon active driver detected)");
    } /* if */
    memcpy(ps, "HEX: ", 5);
  } /* if */

  if (!memcmp(ps, "HEX: ", 5)) { /* new HiSax Driver */

    if (((verbose & VERBOSE_HEX) && !(verbose & VERBOSE_CTRL)) || stdoutput)
      print_msg(PRT_LOG, "%2d %s\n", card, s);

    if (firsttime) {
      firsttime = 0;

      if (!Q931dmp)
        print_msg(PRT_NORMAL, "(HiSax driver detected)");

      HiSax = 1;
      strcpy(last, s);
    }
    else {
      if (!strcmp(last, s)) {
        if (!Q931dmp)
          return;
      }
      else
        strcpy(last, s);
    } /* else */

    if (Q931dmp) {
      register char *s1;
#if 0
      register char *s2;
#endif
      register int i = strtol(ps + 5, NIL, 16);
      register int j = strtol(ps + 8, NIL, 16);
      register int k = strtol(ps + 11, NIL, 16);
      register int l = strtol(ps + 14, NIL, 16);
      register int sapi = i >> 2;
      register int cr = (i >> 1) & 1;
      register int ea2 = i & 1;
      register int tei = j >> 1;
      register int bcast = 0;
      register int ea3 = j & 1;


#if 0
      switch (sapi) {
        case  0 : s1 = "Signalisierungsblock"; break;
        case 16 : s1 = "Paketdatenblock";      break;
        case 63 : s1 = "TEI-Verwaltungsblock"; break;
        default : s1 = "UNKNOWN sapi";         break;
      } /* switch */

      if (tei == BROADCAST) {
        s2 = "Broadcast";
        bcast = 1;
      }
      else if (tei < 64)
        s2 = "feste TEI";
      else
        s2 = "zugeteilte TEI";

      fprintf(stdout, "%02x  SAPI=%d    C/R=%d  E/A=%d [%s]\n",
        i, sapi, cr, ea2, s1);
      fprintf(stdout, "%02x  TEI=%d     E/A=%d [%s]\n",
        j, tei, ea3, s2);
#else
      fprintf(stdout, "%02x  SAPI=%d    C/R=%d  E/A=%d\n",
        i, sapi, cr, ea2);

      if (sapi == 63) {
        fprintf(stdout, "%02x  TEI Vergabe\n", j);

        if (k == 3)
          fprintf(stdout, "%02x  UI\n", k);

	switch (l = strtol(ps + 14, NIL, 16)) {
          case 0x0f : fprintf(stdout, "%02x  Management Entity Identifier\n", l); break;
        } /* switch */

        l = strtol(ps + 17, NIL, 16);
        fprintf(stdout, "%02x  Referenz Indikator\n", l);

        l = strtol(ps + 20, NIL, 16);
        fprintf(stdout, "  %02x  Referenz Indikator\n", l);

	switch (l = strtol(ps + 23, NIL, 16)) {
          case 1 : fprintf(stdout, "  %02x  TEI ANFORDERUNG\n", l);   break;
          case 2 : fprintf(stdout, "  %02x  TEI ZUWEISUNG\n", l);     break;
          case 4 : fprintf(stdout, "  %02x  TEI BITTE PRUEFEN\n", l); break;
	} /* switch */

        k = strtol(ps + 26, NIL, 16);

        if (l == 2)
          fprintf(stdout, "  %02x  ZUGEWIESENER TEI=%d\n", k, k >> 1);
        else
          fprintf(stdout, "  %02x  AKTIONS INDIKATOR\n", k);
      }
      else
        fprintf(stdout, "%02x  TEI=%d     E/A=%d\n", j, tei, ea3);
#endif

      if (sapi != 63) { /* keine TEI Vergabe */
        if (!(k & 1)) { /* I-Block */
          if (bcast)
            fprintf(stdout, "%02x  I-B  N=%d\n", k, k >> 1);
          else
            fprintf(stdout, "%02x  I-B  N=%d  %02x: N(R)=%d  P=%d\n",
              k, k >> 1, l, l >> 1, l & 1);
        }
        else if ((k & 3) == 1) { /* S-Block */
          switch (k) {
            case 01 : s1 = "RR";              break;
            case 05 : s1 = "RNR";             break;
            case 07 : s1 = "REJ";             break;
            default : s1 = "UNKNOWN S-Block"; break;
          } /* switch */

          if (bcast)
            fprintf(stdout, "%02x  %s\n", k, s1);
          else
            fprintf(stdout, "%02x  %s  %02x: N(R)=%d  P/F=%d\n",
              k, s1, l, l >> 1, l & 1);
        }
        else { /* U-Format */
          switch (k) {
            case 0x7f : s1 = "SABME P=1"; break;
            case 0x6f : s1 = "SABME P=0"; break;
            case 0x0f : s1 = "DM    F=0"; break;
            case 0x1f : s1 = "DM    F=1"; break;
            case 0x53 : s1 = "DISC  P=1"; break;
            case 0x43 : s1 = "DISC  P=0"; break;
            case 0x73 : s1 = "UA    F=1"; break;
            case 0x63 : s1 = "UA    F=0"; break;
            case 0x93 : s1 = "FRMR  F=1"; break;
            case 0x83 : s1 = "FRMR  F=0"; break;
            case 0x13 : s1 = "UI    P=1"; break;
            case 0x03 : s1 = "UI    P=0"; break;
            default   : s1 = "UNKNOWN U-Block"; break;
          } /* switch */

          fprintf(stdout, "%02x  %s\n", k, s1);
        } /* else */
      } /* if */
    } /* if */
#if 0 /* wird so ins syslog eingetragen :-( */
    if (!replay)
      if (strtol(ps + 11, NIL, 16) == 1)
        print_msg(PRT_NORMAL, "%c\b", (strtol(ps + 5, NIL, 16) == 2) ? '>' : '<');
#endif
    if (!*(ps + 13) || !*(ps + 16))
      return;

    i = strtol(ps += 5, NIL, 16) >> 1;
    net = i & 1;
    sapi = i >> 1;

    if (sapi == 63) /* AK:07-Nov-98 -- future expansion */
      return;

    tei = strtol(ps += 3, NIL, 16) >> 1;

    ps += (tei == BROADCAST) ? 1 : 4;
  }
  else  if (!memcmp(ps, "D3", 2)) { /* AVMB1 */

    if (firsttime) {
      firsttime = 0;
      print_msg(PRT_NORMAL, "(AVM B1 driver detected (D3))");
    } /* if */

    if (*(ps + 2) == '<')  /* this is our "direction flag" */
      net = 1;
    else
      net = 0;

    tei = 65;  /* we can't get a tei, so fake it */
    isAVMB1 = 1;

    ps[0] = 'h'; ps[1] = 'e'; ps[2] = 'x';  /* rewrite for the others */
  }
  else { /* Old Teles Driver */

    /* Tei wird gelesen und bleibt bis zum Ende des naechsten hex: stehen.
       Der naechste hex: -Durchlauf hat also die korrekte tei. */

    if (!memcmp(ps, "Q.931 frame network->user tei ", 30)) {
      tei = strtol(ps += 30, NIL, 10);
      net = 1;
    }
    else if (!memcmp(ps, "Q.931 frame user->network tei ", 30)) {
      tei = strtol(ps += 30, NIL, 10);
      net = 0;
    }
    else if (!memcmp(ps, "Q.931 frame network->user with tei ", 35)) {
      tei = strtol(ps += 35, NIL, 10);
      net = 1;
    }
    else if (!memcmp(ps, "Q.931 frame network->user", 25)) {
      net = 1;
      tei = BROADCAST;
    } /* else */
  } /* else */

  if (!memcmp(ps, "hex: ", 5) || !memcmp(s, "HEX: ", 5)) {
    i = strtol(ps += 5, NIL, 16);

    switch (i) {
      case 0x40 :
      case 0x41 : version = VERSION_1TR6;
		  break;

      case 0x08 : version = VERSION_EDSS1;
		  break;

      case 0xaa : version = VERSION_UNKNOWN; /* Euracom Frames */
		  return;

      default   : version = VERSION_UNKNOWN;
		  sprintf(sx, "Unexpected discriminator 0x%02x -- ignored!", i);
		  info(chan, PRT_SHOWNUMBERS, STATE_RING, sx);
		  return;
    } /* switch */

    if (Q931dmp) {
      register int crl = strtol(ps + 3, NIL, 16);
      register int crw = strtol(ps + 6, NIL, 16);


      if (crl) {
#if 0
        register int   dir = crw >> 7;
        register int   cr  = crw & 0x7f;
        register char *s1, *s2;


        if (cr < 64) {
          s1 = "Dialin";
          s2 = dir ? "User->VSt" : "VSt->User";
        }
        else {
          s1 = "Dialout";
          s2 = dir ? "VSt->User" : "User->VSt";
        } /* else */

        fprintf(stdout, "%02x  %s, PD=%02x  %02x: CRL=%d  %02x: CRW=%d  %s [%s, %s]\n",
          i, (version == VERSION_EDSS1) ? "E-DSS1" : "1TR6", i, crl,
          crl, crw, crw & 0x7f, (crw > 127) ? "Zielseite" : "Ursprungsseite", s1, s2);
#else
        fprintf(stdout, "%02x  %s, PD=%02x  %02x: CRL=%d  %02x: CRW=%d  %s\n",
          i, (version == VERSION_EDSS1) ? "E-DSS1" : "1TR6", i, crl,
          crl, crw, crw & 0x7f, (crw > 127) ? "Zielseite" : "Ursprungsseite");
#endif
      }
      else
        fprintf(stdout, "%02x  %s, PD=%02x  %02x: CRL=%d\n",
          i, (version == VERSION_EDSS1) ? "E-DSS1" : "1TR6", i, crl,
          crl);
    } /* if */

    if (bilingual && version == VERSION_1TR6) {
      print_msg(PRT_DEBUG_BUGS, " DEBUG> %s: OOPS! 1TR6 Frame? Ignored!\n", st + 4);
      goto endhex;
    } /* if */

    creflen = strtol(ps += 3, NIL, 16);

    if (creflen)
      cref = strtol(ps += 3, NIL, 16);
    else
      cref = -1;

    type = strtol(ps += 3, NIL, 16);

    if (!isAVMB1)
      dialin = (tei == BROADCAST); /* dialin (Broadcast), alle anderen haben schon eine Tei! */
    else
      dialin = (cref & 0x80);  /* first (SETUP) tells us who initiates the connection */

    /* dialin = (cref & 0x7f) < 64; */

    cref = (net) ? cref : cref ^ 0x80; /* cref immer aus Sicht des Amts */

    if (Q931dmp)
      Q931dump(TYPE_MESSAGE, type, NULL, version);

    if (allflags & PRT_DEBUG_DIAG)
      diag(cref, tei, sapi, dialin, net, type, version);

    /* leider laesst sich kein switch nehmen, da decode
       innerhalb von SETUP/A_ACK aufgerufen werden muss, sonst
       aber erst nach feststellen von chan
       Daher GOTO (urgs...) an das Ende vom if hex:.. */

    if (type == SETUP) { /* neuen Kanal, ev. dummy, wenn keiner da ist */
      chan = 5; /* den nehmen wir _nur_ dafuer! */
      clearchan(chan, 1);
      call[chan].dialin = dialin;
      call[chan].tei = tei;
      call[chan].card = card;
      call[chan].uid = ++uid;
      decode(chan, ps, type, version, tei);

#if 0
      if (OUTGOING && *call[chan].num[CALLED])
        LCR(chan, s);
#endif

      if (call[chan].channel) { /* Aha, Kanal war dabei, dann nehmen wir den gleich */
        chan = call[chan].channel - 1;

        if (chanused[chan])
          print_msg(PRT_DEBUG_BUGS, " DEBUG> %s: chan#%d already in use!\n", st + 4, chan);

        chanused[chan] = 1;

        print_msg(PRT_DEBUG_BUGS, " DEBUG> %s: Chan auf %d gesetzt\n", st + 4, chan);

        /* nicht --channel, channel muss unveraendert bleiben! */
        memcpy((char *)&call[chan], (char *)&call[5], sizeof(CALL));
        Change_Channel(5, chan);
        clearchan(5, 1);
      } /* if */

      call[chan].cref = (dialin) ? cref : (cref | 0x80); /* immer die cref, die _vom_ Amt kommt/kommen sollte */
      call[chan].dialin = dialin;
      call[chan].tei = tei;
      call[chan].card = card;
      call[chan].connect = cur_time;
      call[chan].duration = tt;
      call[chan].state = type;

      print_msg(PRT_DEBUG_BUGS, " DEBUG> %s: START CHAN#%d tei %d cref %d %d %s %s->\n",
        st + 4, chan, tei, cref, call[chan].cref,
        call[chan].dialin ? " IN" : "OUT",
        net ? "NET" : "USR");

      addlist(chan, type, 0);

      goto endhex;
    } /* if SETUP */

/* AK:13-Feb-97 ::
   Bei Rausrufen mit Creatix a/b kommt im
   SETUP : Channel identification : BRI, beliebiger Kanal
   und im
   SETUP ACKNOWLEDGE : Channel identification : BRI, B1 gefordert

   Bei Rausrufen mit Europa-10 dagegen:
   SETUP : --
   SETUP ACKNOWLEDGE : Channel identification : BRI, B1 gefordert

*/

    if ((type == SETUP_ACKNOWLEDGE) || (type == CALL_PROCEEDING)) {
      /* Kann sein, dass ein SETUP vorher kam, suchen wir mal, denkbar:
           a) SETUP in 5 (eig. rausruf): decode auf 5, dann copy nach channel
           b) nichts (rausruf fremd): decode auf 5, copy nach channel */

      chan = 5;

      if ((call[5].cref != cref) || (call[5].tei != tei)) {
        /* bei C_PROC/S_ACK ist cref _immer_ > 128 */
        /* keiner da, also leeren */
        if (isAVMB1 && (call[chan].state == SETUP))  /* direction already set for AVMB1 */
          dialin = call[chan].dialin;
        clearchan(chan, 1);
        call[chan].dialin = dialin;
        call[chan].tei = tei;
        call[chan].cref = cref;
	call[chan].card = card;
      } /* if */

      decode(chan, ps, type, version, tei);

      if (call[chan].channel) { /* jetzt muesste einer da sein */

        chan = call[chan].channel - 1;

        if (!chanused[chan]) {
          /* nicht --channel, channel muss unveraendert bleiben! */
          memcpy((char *)&call[chan], (char *)&call[5], sizeof(CALL));
          Change_Channel(5, chan);
	  addlist(chan, type, 1);
          clearchan(5, 1);
        } /* if */
      }
      else
        print_msg(PRT_DEBUG_BUGS, " DEBUG> %s: OOPS, C_PROC/S_ACK ohne channel? tei %d\n",
          st + 4, tei);

      call[chan].connect = cur_time;
      call[chan].duration = tt;
      call[chan].state = type;

      print_msg(PRT_DEBUG_BUGS, " DEBUG> %s: START CHAN#%d tei %d %s %s->\n",
        st + 4, chan, tei,
        call[chan].dialin ? " IN" : "OUT",
        net ? "NET" : "USR");
      goto endhex;
    } /* if C_PROC || S_ACK */

    if (type == AOCD_1TR6) {
      decode(chan, ps, type, version, tei);
      goto endhex;
    } /* if AOCD_1TR6 */

    /* Beim Makeln kommt Geb. Info nur mit Cref und Tei, die
       cref muessen wir dann in chan 2/3 suchen */

    /* Bei geparkten Gespr. kommen die waehrend des Parkens
       aufgelaufenen Gebuehren beim Wiederholen. */

    if ((cref != call[0].cref) && (cref != call[1].cref) &&
        (cref != call[2].cref) && (cref != call[3].cref)) {

      decode(6, ps, type, version, tei);

      /* Mit falscher cref kommt hier keiner rein, koennte
         ein RELEASE auf bereits freiem Kanal sein */
      goto endhex;
    } /* if */

    /* So, wenn wir hier ankommen, haben wir auf jeden Fall einen
       Kanal (0, 1, 2 oder 3) und eine cref. Die tei folgt evtl. erst beim
       Connect (Reinruf). Suchen wir den Kanal: */

    /* crefs absuchen. Gibt's die mehrmals, tei absuchen, dann haben wir
       ihn.
       Es kann aber sein, dass cref stimmt, aber noch keine tei da war
       (Reinruf). Dann ist aber die cref eindeutig (hoffentlich)!
       finden wir einen Kanal mit passender cref, der keine
       tei hat, haben wir ihn. Hat er eine, und sie stimmt,
       ebenso. Sonst weitersuchen. Geparkte Kanaele ignorieren
       bis zum RESUME, oder sie werden bei neuem SETUP_ACK. ueber-
       schrieben, wenn wir wen im Parken verhungen lassen haben */

    chan = -1;

    for (i = 0; ((i < 4) &&
      ((call[i].cref != cref) ||
        ((call[i].state == SUSPEND) && (type != RESUME_ACKNOWLEDGE)) ||
        ((call[i].tei != BROADCAST) && (call[i].tei != tei)))); i++);
      chan = i;

    print_msg(PRT_DEBUG_BUGS, " DEBUG> %s: Kanal %d\n", st + 4, chan);

    /* auch wenn hier schon eine tei bei ist, erst beim connect hat
       ein reingerufener Kanal eine gueltige tei */

    decode(chan, ps, type, version, tei);
    chanused[chan] = 2;

    switch (type) {

      case ALERTING            :
      case CALL_PROCEEDING     :
				 if (!Q931dmp)
				 if (dual && *call[chan].digits) {
				   strcpy(call[chan].onum[CALLED], call[chan].digits);
                                   buildnumber(call[chan].digits, call[chan].oc3, -1, call[chan].num[CALLED], version, &call[chan].provider, &call[chan].sondernummer[CALLED], &call[chan].intern[CALLED], &call[chan].local[CALLED], call[chan].dialin, CALLED);

				   strcpy(call[chan].vnum[CALLED], vnum(chan, CALLED));
				 } /* if */
                                 break;

      case CONNECT             :
      case CONNECT_ACKNOWLEDGE :

        /* Bei Rufen an die Teles kommt CONNECT und CONN.ACKN., eins reicht uns */
        if (call[chan].state == CONNECT)
          goto doppelt;

        call[chan].state = CONNECT;
        call[chan].tei = tei;
        call[chan].dialog++; /* es hat connect gegeben */
        call[chan].connect = cur_time;
        call[chan].duration = tt;
	call[chan].card = card;

        if (*call[chan].service) {
          sprintf(sx, "CONNECT (%s)", call[chan].service);
          info(chan, PRT_SHOWCONNECT, STATE_CONNECT, sx);
        }
        else
          info(chan, PRT_SHOWCONNECT, STATE_CONNECT, "CONNECT");

        if (IIOCNETGPNavailable)
	  IIOCNETGPNavailable = findinterface();

        if (OUTGOING && *call[chan].num[CALLED]) {

	  prepareRate(chan, &why, &hint, 0);
#ifndef CONFIG_ISDN_WITH_ABC_LCR_SUPPORT
	  processlcr(call[chan].num[CALLED]); /* fake input for testing */
#endif
	  if (*why)
	    info(chan, PRT_SHOWCONNECT, STATE_CONNECT, why);

	  if (*hint)
	    info(chan, PRT_SHOWCONNECT, STATE_CONNECT, hint);

	  if (call[chan].tarifknown) {
	    call[chan].ctakt = call[chan].Rate.Units;

	    sprintf(sx, "%d.CI %s (now)", call[chan].ctakt, printRate(call[chan].pay));
	    info(chan, PRT_SHOWCONNECT, STATE_CONNECT, sx);

	    call[chan].cint = call[chan].Rate.Duration;
	    call[chan].lastcint = cur_time;
	    snprintf(sx, BUFSIZ, "NEXT CI AFTER %s (%s)",
		     double2clock(call[chan].cint) + 3,
		     explainRate(&call[chan].Rate));
            info(chan, PRT_SHOWCONNECT, STATE_CONNECT, sx);

	    huptime(chan, 1);

            if ((c = call[chan].confentry[OTHER]) > -1) {
              if (!replay && (chargemax != 0.0)) {
                if (day != known[c]->day) {
                  sprintf(s1, "CHARGEMAX resetting %s's charge (day %d->%d)",
                    known[c]->who, (known[c]->day == -1) ? 0 : known[c]->day, day);

                  info(chan, PRT_SHOWCHARGEMAX, STATE_AOCD, s1);

                  known[c]->scharge += known[c]->charge;
                  known[c]->charge = 0.0;
                  known[c]->day = day;
                } /* if */

                tx = cur_time - call[chan].connect;
                sprintf(s1, "CHARGEMAX remaining=%s %s %s",
                  printRate((chargemax - known[c]->charge - call[chan].pay)),
                  (connectmax == 0.0) ? "" : double2clock(connectmax - known[c]->online - tx),
                  (bytemax == 0.0) ? "" : double2byte((double)(bytemax - known[c]->bytes)));


                info(chan, PRT_SHOWCHARGEMAX, STATE_AOCD, s1);

                if (((known[c]->charge + call[chan].pay) >= chargemax) && (*INTERFACE > '@'))
                  chargemaxAction(chan, (known[c]->charge + call[chan].pay - chargemax));
              } /* if */

              if (!replay && (connectmax != 0.0)) {
                if (month != known[c]->month) {
                  sprintf(s1, "CONNECTMAX resetting %s's online (month %d->%d)",
                    known[c]->who, (known[c]->month == -1) ? 0 : known[c]->month, month);

                  info(chan, PRT_SHOWCHARGEMAX, STATE_AOCD,s1);

                  known[c]->sonline += known[c]->online;
                  known[c]->online = 0.0;
                  known[c]->month = month;

                  known[c]->sbytes += known[c]->bytes;
                  known[c]->bytes = 0.0;
                } /* if */
              } /* if */
            } /* if */
          } /* if */
        } /* if */

        if (sound)
          ringer(chan, RING_CONNECT);

	procinfo(call[chan].channel, &call[chan], CONNECT);

doppelt:break;

      case SUSPEND_ACKNOWLEDGE :
        call[chan].state = SUSPEND;
        info(chan, PRT_SHOWHANGUP, STATE_HANGUP, "PARK");
        break;

      case RESUME_ACKNOWLEDGE :
        call[chan].state = CONNECT;
        info(chan, PRT_SHOWCONNECT, STATE_CONNECT, "RESUME");
        break;

      case MAKEL_ACKNOWLEDGE :
        wegchan = (call[2].state) ? 3 : 2;
        memcpy((char *)&call[wegchan], (char *)&call[chan], sizeof(CALL));
        Change_Channel(chan, wegchan);
	addlist(wegchan, type, 1);
        clearchan(chan, 1);
        call[wegchan].state = MAKEL_ACKNOWLEDGE;
        info(wegchan, PRT_SHOWHANGUP, STATE_HANGUP, "MAKEL");
        break;

      case MAKEL_RESUME_ACK :
        if (call[chan].channel) { /* muesste einer da sein */
          memcpy((char *)&call[call[chan].channel - 1], (char *)&call[chan], sizeof(CALL));
          call[call[chan].channel - 1].channel = chan; /* den alten merken */
          Change_Channel(chan, call[chan].channel - 1);
          chan = call[chan].channel - 1; /* chan setzen */
	  addlist(chan, type, 1);
          clearchan(call[chan].channel, 1);
          call[chan].channel = chan + 1; /* in Ordnung bringen */
          call[chan].state = CONNECT;
        } /* if */

        info(chan, PRT_SHOWCONNECT, STATE_CONNECT, "MAKELRESUME");
        break;


      case DISCONNECT          :

        if (!call[chan].state) /* Keine Infos -> Weg damit */
          break;

        call[chan].disconnect = cur_time;

        if (replay)
          call[chan].duration = (tt - call[chan].duration) * 100;
        else
          call[chan].duration = tt - call[chan].duration;

        call[chan].state = DISCONNECT;

        break;


      case RELEASE             :
      case RELEASE_COMPLETE    :

        if (!net) /* wir nehmen nur RELEASE vom Amt */
          break;

        if (!call[chan].state) /* Keine Infos -> Weg damit */
          break;

        /* Wenn's keinen CONNECT gab, hat's auch nichts gekostet.
           Falls der RELEASE aber ein Rufablehnen war und der
           CONNECT noch folgt, wird dafuer jetzt chan auf
           4 gepackt, um die schoenen Daten in 0/1/ev.4 nicht
           zu zerstoeren. Wir erkennen das an fehlender tei. */

        if (call[chan].tei == BROADCAST) {
          memcpy((char *)&call[4], (char *)&call[chan], sizeof(CALL));
          Change_Channel(chan, 4);
          chan = 4;
	  addlist(chan, type, 1);
          call[chan].tei = tei;
	  call[chan].card = card;
        } /* if */

        if (!call[chan].disconnect) {
          call[chan].disconnect = cur_time;

          if (replay)
            call[chan].duration = (tt - call[chan].duration) * 100;
          else
            call[chan].duration = tt - call[chan].duration;
        } /* if */

        if (!call[chan].dialog) {
          call[chan].duration = 0;
          call[chan].disconnect = call[chan].connect;

          print_msg(PRT_DEBUG_BUGS, " DEBUG> %s: CHAN#%d genullt (dialin=%d, state=%d, tei=%d, cref=%d)\n",
            st + 4, chan, call[chan].dialin, call[chan].state, call[chan].tei, call[chan].cref);

          print_msg(PRT_DEBUG_BUGS, " DEBUG> %s: OOPS! DURATION=0\n", st + 4);

          if (OUTGOING) {
            print_msg(PRT_DEBUG_BUGS, " DEBUG> %s: OOPS! AOCE=0\n", st + 4);
          } /* if */
        } /* if kein connect */

        if (allflags & PRT_DEBUG_BUGS) {
          strcpy(sx, ctime(&call[chan].connect));
          sx[19] = 0;

          print_msg(PRT_DEBUG_BUGS, " DEBUG> %s: LOG CHAN#%d(%s : DIAL%s : %s -> %s : %d s (%d s) : %d EH):%s\n\n",
            st + 4, chan,
            sx + 4,
            call[chan].dialin ? "IN" : "OUT",
            call[chan].num[CALLING],
            call[chan].num[CALLED],
            (int)(call[chan].disconnect - call[chan].connect),
            (int)call[chan].duration,
            call[chan].aoce,
            qmsg(TYPE_CAUSE, version, call[chan].cause));
        } /* if */

        if (OUTGOING && call[chan].duration) {
	  processRate(chan);

	  if (call[chan].tarifknown) {
	    char *h = hints;

	    processLCR(chan, h);

	    while (h && *h)
	      info(chan, PRT_SHOWHANGUP, STATE_HANGUP, strsep(&h, "\n"));
	  } /* if */
	} /* if */

	if (!Q931dmp)
	  logger(chan);

	chanused[chan] = 0;
	addlist(chan, type, 2);

	if (call[chan].dialog || any) {

	  if (call[chan].ibytes + call[chan].obytes) {
	    sprintf(s2, " I=%s O=%s",
	      double2byte((double)call[chan].ibytes),
	      double2byte((double)call[chan].obytes));
          }
          else
            *s2 = 0;

          if (call[chan].dialin)
            sprintf(sx, "HANGUP (%s%s)",
              double2clock((double)(call[chan].disconnect - call[chan].connect)), s2);
          else {
            auto int  firsttime = 1;
            register char *p = sx;

            p += sprintf(p, "HANGUP");

            if (call[chan].Rate.Units > 0) {
              if (firsttime) {
                p += sprintf(p, " (");
                firsttime = 0;
              } /* if */

              p += sprintf(p, "%d CI %s",
                call[chan].Rate.Units,
                printRate(call[chan].pay));
            } /* if */

            if (call[chan].aocpay > 0) {
              if (firsttime) {
                p += sprintf(p, " (");
                firsttime = 0;
              }
              else
                p += sprintf(p, "; ");

              p += sprintf(p, "%d EH %s %s",
                call[chan].aoce,
                currency,
                double2str(call[chan].aocpay, 6, 3, DEB));
            } /* if */

            if (call[chan].disconnect - call[chan].connect) {
              if (firsttime) {
                p += sprintf(p, " (");
                firsttime = 0;
              }
              else
                p += sprintf(p, " ");

              p += sprintf(p, "%s",
                double2clock((double)(call[chan].disconnect - call[chan].connect)));
            } /* if */

            if (*s2) {
              if (firsttime) {
                p += sprintf(p, " (");
                firsttime = 0;
              } /* if */
              p += sprintf(p, "%s", s2);
            } /* if */

            if (!firsttime)
              p += sprintf(p, ")");

#if 0
            if (call[chan].Rate.Units > 0)
              sprintf(sx, "HANGUP (%d CI %s %s%s)",
                call[chan].aoce,
                printRate(call[chan].pay),
                double2clock((double)(call[chan].disconnect - call[chan].connect)), s2);
            else if (call[chan].pay)
              sprintf(sx, "HANGUP (%s %s%s)",
                ((call[chan].pay == -1.0) ? "UNKNOWN" : printRate(call[chan].pay)),
                double2clock((double)(call[chan].disconnect - call[chan].connect)), s2);
            else if (call[chan].aocpay)
              sprintf(sx, "HANGUP (%d EH %s %s %s%s)",
                call[chan].aoce,
                currency,
                double2str(call[chan].aocpay, 6, 3, DEB),
                double2clock((double)(call[chan].disconnect - call[chan].connect)), s2);
            else
              sprintf(sx, "HANGUP (%s%s)", double2clock((double)(call[chan].disconnect - call[chan].connect)), s2);
#endif
          } /* else */

          if (!memcmp(sx, "HANGUP (        )", 17))
            sx[6] = 0;

          if ((call[chan].cause != 0x10) && (call[chan].cause != 0x1f)) { /* "Normal call clearing", "Normal, unspecified" */
            strcat(sx, " ");
            strcat(sx, qmsg(TYPE_CAUSE, version, call[chan].cause));

            if (((p = location(call[chan].loc)) != "")) {
              strcat(sx, " (");
              strcat(sx, p);
              strcat(sx, ")");
            } /* if */

          } /* if */

          info(chan, PRT_SHOWHANGUP, STATE_HANGUP, sx);

          if (((call[chan].cause == 0x22) ||  /* No circuit/channel available */
               (call[chan].cause == 0x2a)) && /* Switching equipment congestion */
	      ((call[chan].loc == 2) ||       /* Public network serving local user */
               (call[chan].loc == 3))) {      /* Transit network */
	    auto char s[BUFSIZ], s1[BUFSIZ];
	    RATE Other;

	    prepareRate(chan, NULL, NULL, 0);

	    if (getLeastCost(&call[chan].Rate, &Other, 1, call[chan].provider) != UNKNOWN) {
	      char prov[TN_MAX_PROVIDER_LEN];
	      prefix2provider(Other.prefix, prov);
	      showRates(&Other, s1);
	      sprintf(s, "OVERLOAD? Try %s:%s (%s)", prov, Other.Provider, s1);

	      info(chan, PRT_SHOWHANGUP, STATE_HANGUP, s);
	    } /* if */
	  } /* if */

	  if (OUTGOING && ((c = call[chan].confentry[OTHER]) > -1)) {
	    if (chargemax != 0.0) {
	    known[c]->charge += call[chan].pay;
	    sprintf(sx, "CHARGEMAX total=%s today=%s remaining=%s",
	      printRate(known[c]->scharge + known[c]->charge),
	      printRate(known[c]->charge),
              printRate((chargemax - known[c]->charge)));
            info(chan, PRT_SHOWCHARGEMAX, STATE_HANGUP, sx);
	    } /* if */

            if (connectmax != 0.0) {
              if (connectmaxmode == 1)
		known[c]->online += ((int)((call[chan].disconnect - call[chan].connect + 59) / 60.0)) * 60.0;
              else
                known[c]->online += call[chan].disconnect - call[chan].connect;

              sprintf(sx, "CONNECTMAX total=%s month=%s remaining=%s",
                double2clock(known[c]->sonline + known[c]->online),
                double2clock(known[c]->online),
                double2clock(connectmax - known[c]->online));
              info(chan, PRT_SHOWCHARGEMAX, STATE_HANGUP, sx);
            } /* if */

            if (bytemax != 0.0) {
              auto double byte;

              switch (bytemaxmode & 25) {
                case  8 : byte = call[chan].obytes;
                          break;

                case 16 : byte = call[chan].ibytes + call[chan].obytes;
                          break;

                default : byte = call[chan].ibytes;
                          break;
              } /* switch */

              switch (bytemaxmode & 3) {
                case 0 : known[c]->bytes += byte;
                         break;
                case 1 : known[c]->bytes += byte;
                         break;
                case 2 : known[c]->bytes += byte;
                         break;
              } /* switch */

              sprintf(sx, "BYTEMAX total=%s month=%s remaining=%s",
                double2byte((double)(known[c]->sbytes + known[c]->bytes)),
                double2byte((double)(known[c]->bytes)),
                double2byte((double)(bytemax - known[c]->bytes)));
              info(chan, PRT_SHOWCHARGEMAX, STATE_HANGUP, sx);
            } /* if */
          } /* if */

          if (sound)
            ringer(chan, RING_HANGUP);

	  procinfo(call[chan].channel, &call[chan], RELEASE);

        } /* if */

        clearchan(chan, 1);

        break;

    } /* switch */

endhex:
    tei = BROADCAST; /* Wenn nach einer tei-Zeile keine hex:-Zeile kommt, tei ungueltig machen! */

    if ((type == SETUP) && !replay) { /* fetch additional info from "/dev/isdninfo" */
      static void moreinfo(); /* soviel zu Objektorientiertem Denken ;-) */
      moreinfo();
    } /* if */

  } /* if */
} /* processctrl */


void processflow()
{
  register char  *p;
  register int    j;
  auto     char   sx[BUFSIZ];
  auto     double s;
  int      ret;
  static   int tries = 3;

  if (!(ret=ioctl(sockets[ISDNINFO].descriptor, IIOCGETCPS, &io))) {

    if (verbose & VERBOSE_FLOW) {
      p = sx;
      s = 0L;

      for (j = 0; j < chans; j++) {
        p += sprintf(p, "%ld ", io[j].i);
        s += io[j].i;
      } /* for */

      if (s > 0L)
        print_msg(PRT_LOG, "ibytes:\t%s\n", sx);

      p = sx;
      s = 0L;

      for (j = 0; j < chans; j++) {
        p += sprintf(p, "%ld ", io[j].o);
        s += io[j].o;
      } /* for */

      if (s > 0L)
        print_msg(PRT_LOG, "obytes:\t%s\n", sx);
    } /* if */

    processbytes();
  } /* if */
  else if (tries) {
    tries--;
    print_msg(PRT_ERR, "Can't read iobytes: ioctl IIOCGETCPS returned %d\n", ret);
  }
} /* processflow */


static void processlcr(char *p)
{
  auto char                        res[BUFSIZ], s[BUFSIZ];
  register char			  *pres = res;
  auto TELNUM			   destnum;
  auto RATE			   Rate, Cheap;
  auto int			   prefix, own_country = 0;
#ifdef CONFIG_ISDN_WITH_ABC_LCR_SUPPORT
  auto struct ISDN_DWABC_LCR_IOCTL i;
  auto int			   cc;
  auto char			   ji[20];
  auto char			   kenn[40];
  auto char			   cid[40];
  auto char			   eaz[40];
#endif
  auto char			   dst[40];
  auto char			   prov[TN_MAX_PROVIDER_LEN];
  auto char			   lcr_amtsholung[BUFSIZ];
  auto int			   abort = 0;

  if(!abclcr)
    return;

#ifdef CONFIG_ISDN_WITH_ABC_LCR_SUPPORT
  sscanf(p, "%s %s %s %s %s", ji, kenn, cid, eaz, dst);
#else
  strcpy(dst, p);
#endif
  if(amtsholung && *amtsholung) {
    char *delim;

    strncpy(lcr_amtsholung, amtsholung, 5);

    if ((delim = strchr(lcr_amtsholung, ':')) != NULL)
      *delim = '\0';
  }
  else if (trimo)
    strncpy(lcr_amtsholung, dst, trimo);
  else
    *lcr_amtsholung = 0;

  normalizeNumber(dst + trimo, &destnum, TN_ALL);

  sprintf(s, "ABC_LCR: Request for number %s = %s", dst + trimo, formatNumber("%l via %p", &destnum));
  info(chan, PRT_SHOWNUMBERS, STATE_RING, s);

  clearRate(&Rate);
  time(&Rate.start);
  Rate.now = Rate.start + LCR_DURATION;

  Rate.prefix = destnum.nprovider; /* old provider */
  Rate.src[0] = mycountry;
  Rate.src[1] = myarea;
  Rate.src[2] = "";

  Rate.dst[0] = destnum.country;
  Rate.dst[1] = destnum.area;
  Rate.dst[2] = destnum.msn;

  prefix = getLeastCost(&Rate, &Cheap, 1, -1);

  if (prefix != UNKNOWN) {
    (void)prefix2provider(prefix, prov);

    if (*lcr_amtsholung)
      pres += sprintf(pres, "%s", lcr_amtsholung);

    pres += sprintf(pres, "%s", prov);

    if (*destnum.country) {
      if (strcmp(mycountry, destnum.country))
        pres += sprintf(pres, "00%s", destnum.country + 1); /* skip "+" */
      else {
        pres += sprintf(pres, "0");
        own_country = 1;
      } /* else */
    }
    /* always append area */
    pres += sprintf(pres, "%s", Cheap.dst[1]);
    pres += sprintf(pres, "%s", destnum.msn);

    if (!*destnum.msn) {
      char arg[160];
      if((abclcr & 0x4) != 0x4) {
        sprintf(s, "ABC_LCR: \"%s\" is a special number, no action", destnum.area);
        abort = 1;
        goto action;
      }
      /* call external programm for changing route etc. */
      if (providerchange && *providerchange) {
        if(!paranoia_check(providerchange)) {
	  int nok;
          sprintf(arg, "%s %d %s '%s'", providerchange, prefix2pnum(prefix),
		res, getSpecialName(Cheap.dst[1]));
          nok=system(arg);
	  if(nok) {
	    sprintf(s, "ABC_LCR: '%s' returned %d, no action", providerchange, nok);
	    abort=1;
	    goto action;
	  }
        }
      } /* if */
    } /* sondernummer */

    if ((strcmp(myarea, destnum.area) == 0) && own_country && ((abclcr & 0x2) != 0x2)) {
      sprintf(s, "ABC_LCR: \"%s\" is a local number, no action", destnum.msn);
      abort = 1;
      goto action;
    } /* if */

#ifdef CONFIG_ISDN_WITH_ABC_LCR_SUPPORT
    if (strlen(res) < sizeof(i.lcr_ioctl_nr)) {
      sprintf(s, "ABC_LCR: New number \"%s\" (via %s:%s)",
        res, prov, getProvider(prefix));
    }
    else {
      sprintf(s, "ABC_LCR: Resulting new number \"%s\" too long -- aborting", res);
      abort = 1;
    } /* else */
#else
    sprintf(s, "ABC_LCR: New number \"%s\" (via %s:%s)",
      res, prov, getProvider(prefix));
#endif
  }
  else {
    sprintf(s, "ABC_LCR: Can't find cheaper booked provider");
    abort = 1;
  } /* else */

action:
#ifdef CONFIG_ISDN_WITH_ABC_LCR_SUPPORT
  memset(&i, 0, sizeof(i));

  i.lcr_ioctl_sizeof = sizeof(i);
  i.lcr_ioctl_callid = atol(cid);

  if (!abclcr) {
    strcat(s, " (ABC_LCR *disabled*, no action)");
    abort = 1;
  } /* if */

  if (abort) { /* tell ABC_LCR to dial the *original* number (and _dont_ wait 3 seconds) */
    i.lcr_ioctl_flags = 0;

    cc = ioctl(sockets[ISDNCTRL].descriptor, IIOCNETLCR, &i);
  }
  else {
    i.lcr_ioctl_flags = DWABC_LCR_FLG_NEWNUMBER;
    strcpy(i.lcr_ioctl_nr, res);

    cc = ioctl(sockets[ISDNCTRL].descriptor, IIOCNETLCR, &i);
  } /* else */
#else
  strcat(s, " (but ABC_LCR not installed - simulation)");
#endif

  info(chan, PRT_SHOWNUMBERS, STATE_RING, s);
} /* processlcr */


int morectrl(int card)
{
  register char      *p, *p1, *p2, *p3;
  static   char       s[MAXCARDS][BIGBUFSIZ * 2];
  static   char      *ps[MAXCARDS] = { s[0], s[1] };
  auto     int        n = 0;
  auto     struct tm *tm;


  if ((n = read(sockets[card ? ISDNCTRL2 : ISDNCTRL].descriptor, ps[card], BIGBUFSIZ)) > 0) {

    now();
    ps[card] += n;

    *ps[card] = 0;

    p1 = s[card];

    while ((p = p2 = strchr(p1, '\n'))) {
      *p = 0;

      while (*--p == ' ')
        *p = 0;
retry:
      if (replay) {

        if (replaydev)
          p3 = p1;
        else {
          cur_time = tt = atom(p1 + 4);

          if (cur_time == (time_t)-1) {
            now();
            replaydev++;
            goto retry;
          } /* if */

          set_time_str();

          tm = localtime(&cur_time);
          p3 = p1 + 26;

        } /* if */

	processcint();

        if (!memcmp(p3, "idmap:", 6) ||
            !memcmp(p3, "chmap:", 6) ||
            !memcmp(p3, "drmap:", 6) ||
            !memcmp(p3, "usage:", 6) ||
            !memcmp(p3, "flags:", 6) ||
            !memcmp(p3, "phone:", 6) ||
            !memcmp(p3, "ibytes:", 7) ||
            !memcmp(p3, "obytes:", 7))
          processinfo(p3);
        else if (!memcmp(p3, "HEX: ", 5) ||
                 !memcmp(p3, "hex: ", 5) ||
/*               !memcmp(p3, "D2<: ", 5) ||   Layer 2 not yet evaluated */
/*               !memcmp(p3, "D2>: ", 5) ||   Layer 2 not yet evaluated */
                 !memcmp(p3, "D3<: ", 5) ||
                 !memcmp(p3, "D3>: ", 5))
          processctrl(0, p3);
        else if (!memcmp(p3 + 3, "HEX: ", 5))
          processctrl(atoi(p3), p3 + 3);
      }
      else {
#ifdef CONFIG_ISDN_WITH_ABC_LCR_SUPPORT
        if (!memcmp(p1 + 9, "DW_ABC_LCR", 10))
          processlcr(p1);
        else
#endif
        {
          if ((((ignoreRR & 1) == 1) && (strlen(p1) < 17)) ||
              (((ignoreRR & 2) == 2) && !memcmp(p1 + 14, "AA", 2)))
            ;
          else {
            if (!memcmp(p1, "ECHO:", 5)) { /* Echo-channel from HFC card */
              if (((ignoreRR & 2) == 2) && !memcmp(p1 + 12, "01", 2))
                ;
              else {
	        memcpy(p1 + 1, "HEX", 3);
		processctrl(card + 1, p1 + 1);
              } /* else */
            }
            else {
              if (((ignoreRR & 2) == 2) && !memcmp(p1 + 11, "01", 2))
                ;
              else
                processctrl(card, p1);
            } /* else */
          } /* else */
        } /* else */
      } /* else */

      p1 = p2 + 1;
    } /* while */

    if (p1 < ps[card]) {
      n = ps[card] - p1;
      memmove(s, p1, n);
      ps[card] = s[card] + n;
    }
    else
      ps[card] = s[card];

    return(1);
  }
  else {
    alarm(0);
    return(0);
  } /* else */
} /* morectrl */


void moreinfo()
{
  register char *p, *p1, *p2;
  static   char  s[BIGBUFSIZ * 2];
  static   char *ps = s;
  auto     int   n;


  if ((n = read(sockets[ISDNINFO].descriptor, ps, BIGBUFSIZ)) > 0) {
    now();
    ps += n;

    *ps = 0;

    p1 = s;

    while ((p = p2 = strchr(p1, '\n'))) {
      *p = 0;

      while (*--p == ' ')
        *p = 0;

      processinfo(p1);

      p1 = p2 + 1;
    } /* while */

    if (p1 < ps) {
      n = ps - p1;
      memmove(s, p1, n);
      ps = s + n;
    }
    else
      ps = s;
  } /* if */
} /* moreinfo */

/*****************************************************************************/

void morekbd()
{
  auto char  s[BIGBUFSIZ * 2];
  auto char *ps = s;
  auto int   n, chan;


  if ((n = read(sockets[STDIN].descriptor, ps, BIGBUFSIZ)) > 0) {
    ps += n;

    *ps = 0;

    switch (*s) {
      case 'l' : print_msg(PRT_SHOWNUMBERS, "Recent caller's:\n");
		 addlist(-1, SETUP, 3);
		 break;

      case 'h' : print_msg(PRT_SHOWNUMBERS, "\n\t*** s)tatus, l)ist, u)p, d)own ***\n");
		 break;

      case 'u' : /* huptime(0, 0); */
		 break;

      case 'd' : /* huptime(0, 0); */
		 break;

      case 's' : now();

		 print_msg(PRT_SHOWNUMBERS, "\n\t*** %s\n", stl);

		 for (chan = 0; chan < MAXCHAN; chan++) {
		   if (call[chan].bchan == -1)
		     sprintf(s, "\t*** BCHAN#%d : FREE ***\n", chan + 1);
		   else {
		     sprintf(s, "\t*** BCHAN#%d : %d %s %s %s ***\n",
                       chan + 1,
                       call[chan].bchan,
                       call[chan].vnum[0],
                       call[chan].dialin ? "<-" : "->",
                       call[chan].vnum[1]);
		   } /* else */

		   print_msg(PRT_SHOWNUMBERS, "%s", s);
		 } /* for */
		 break;
    } /* switch */

  } /* if */
  } /* morekbd */

  /*****************************************************************************/

  static void teardown(int chan)
  {
  auto char sx[BUFSIZ];


  if (!Q931dmp)
    logger(chan);

  chanused[chan] = 0;

  call[chan].disconnect = call[chan].connect;
  call[chan].cause = 0x66; /* Recovery on timer expiry */

  addlist(chan, SETUP, 2);

  sprintf(sx, "HANGUP (Timeout)" /*, qmsg(TYPE_CAUSE, VERSION_EDSS1, call[chan].cause) */);
  info(chan, PRT_SHOWHANGUP, STATE_HANGUP, sx);

  if (sound)
    ringer(chan, RING_HANGUP);

  clearchan(chan, 1);
} /* teardown */

/*****************************************************************************/

void processcint()
{
  auto int    chan, c;
  auto char   sx[BUFSIZ], s1[BUFSIZ], hints[BUFSIZ];
  auto double dur;


  for (chan = 0; chan < chans; chan++) {

    dur = cur_time - call[chan].connect;

    /* more than 50 seconds after SETUP nothing happen? */
    if ((chanused[chan] == 1) && (dur > 50))
      teardown(chan);

    if (OUTGOING && call[chan].tarifknown) {
      processRate(chan);

      if (call[chan].ctakt != call[chan].Rate.Units) { /* naechste Einheit */
	call[chan].ctakt = call[chan].Rate.Units;
	if (!ciInterval || !call[chan].lastcint ||
	    (cur_time - call[chan].lastcint) >= ciInterval) {
	  call[chan].lastcint = cur_time;
	  sprintf(sx, "%d.CI %s (after %s) ",
	    call[chan].ctakt,
	    printRate(call[chan].pay),
	    double2clock(call[chan].Rate.Time));
	  info(chan, PRT_SHOWCONNECT, (call[chan].Rate.Duration < 30) ? STATE_BYTE : STATE_CONNECT, sx);
	}

        if ((c = call[chan].confentry[OTHER]) > -1) {
          if (!replay && (chargemax != 0.0)) {

            sprintf(s1, "CHARGEMAX remaining=%s %s %s",
              printRate((chargemax - known[c]->charge - call[chan].pay)),
              (connectmax == 0.0) ? "" : double2clock(connectmax - known[c]->online - dur),
              (bytemax == 0.0) ? "" : double2byte((double)(bytemax - known[c]->bytes)));

            info(chan, PRT_SHOWCHARGEMAX, STATE_AOCD, s1);

            if (((known[c]->charge + call[chan].pay) >= chargemax) && (*INTERFACE > '@'))
              chargemaxAction(chan, (known[c]->charge + call[chan].pay - chargemax));
          } /* if */
        } /* if */
      } /* if */

      if (call[chan].cint != call[chan].Rate.Duration) { /* Taktwechsel */
	char *h=hints;
	call[chan].cint = call[chan].Rate.Duration;

	snprintf(sx, BUFSIZ, "NEXT CI AFTER %s (%s)",
		 double2clock(call[chan].cint) + 3,
		 explainRate(&call[chan].Rate));

	info(chan, PRT_SHOWCONNECT, STATE_CONNECT, sx);

	processLCR(chan, h);
	while (h && *h)
	  info(chan, PRT_SHOWHANGUP, STATE_HANGUP, strsep(&h,"\n"));

	huptime(chan, 0);
      } /* if */

    } /* if */
  } /* for */
} /* processcint */

/*****************************************************************************/

void lcd4linux(void)
{
  static FILE *lcd = NULL;

  if (lcdfile == NULL)
    return;

  if (lcd == NULL) {
    int fd;

    if ((fd = open(lcdfile, O_WRONLY | O_NDELAY)) == -1) {
      print_msg(PRT_ERR, "lcd4linux: open(%s) failed: %s\n", lcdfile, strerror(errno));
      lcdfile = NULL;
      return;
    } /* if */

    if ((lcd = fdopen(fd, "w")) == NULL) {
      print_msg (PRT_ERR, "lcd4linux: fdopen(%s) failed: %s\n", lcdfile, strerror(errno));
      lcdfile = NULL;
      return;
    } /* if */
  } /* if */

  /* Fixme: do something useful here... */
} /* lcd4linux */
/* vim:set sw=2: */
