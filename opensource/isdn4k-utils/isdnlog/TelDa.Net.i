#
# $Id: TelDa.Net.i,v 1.1 2000/08/01 20:31:29 akool Exp $
#
# G:15-Jan-2000
C:TelDa.Net
C:Address:Telefon-, Daten- und Fax-Transfer GmbH & Co. KG, Schuppertsgasse 30, 35083 Wetter (Hessen)
C:Address:TeDaFax, Telefon-, Daten- und Faxtransfer AG, Postfach 2206, 35010 Marburg
C:Homepage:http://www.teldafax.de
C:Hotline:0800/01030-23
C:Hotfax:0800/01030-33
C:Maintainer:Tarif Datenbank Crew <rates@gmx.de>
C:Special:Die Homepage der Tarif-Datenbank Crew: http://www.digitalprojects.com/rates
#
# ACHTUNG:
# Hierbei handelt es sich um das "CeBIT Sonderangebot" der TelDaFax
# Nach einer Anmeldung (Zugangsnummer: "CEBIT-M70F35QVYPGT76")
# kann dieser Dienst fÅr 3 Monate zum Preis von
# DM 0,029/Minute genutzt werden, danach kostet dieser Dienst
# DM 0,06/Minute
#
# Damit "isdnlog" diesen Tarif korrekt abrechnet, m¸ssen folgende Schritte
# unternommen werden:
#
# 1. In der "/usr/lib/isdn/rate-de.dat" muﬂ im Kapitel
#    "P:30,0 TelDaFax"
#    der Eintrag
#    "I:TelDa.Net.i" eingef¸gt werden.
#
# 2. Das untige Datum 1.6.2000 muﬂ durch das persˆnliche Ablaufdatum f¸r
#    dieses Sonderangebot abge‰ndert werden.
#
Z:109
A:08000103021
T:[-01.06.2000] */*=0.029/60
T:[01.06.2000]  */*=0.06/60
#####################################################################
##L $Log: TelDa.Net.i,v $
##L Revision 1.1  2000/08/01 20:31:29  akool
##L isdnlog-4.37
##L - removed "09978 Schoenthal Oberpfalz" from "zone-de.dtag.cdb". Entry was
##L   totally buggy.
##L
##L - isdnlog/isdnlog/processor.c ... added err msg for failing IIOCGETCPS
##L
##L - isdnlog/tools/cdb       ... (NEW DIR) cdb Constant Data Base
##L - isdnlog/Makefile.in     ... cdb Constant Data Base
##L - isdnlog/configure{,.in}
##L - isdnlog/policy.h.in
##L - isdnlog/FAQ                 sic!
##L - isdnlog/NEWS
##L - isdnlog/README
##L - isdnlog/tools/NEWS
##L - isdnlog/tools/dest.c
##L - isdnlog/tools/isdnrate.man
##L - isdnlog/tools/zone/Makefile.in
##L - isdnlog/tools/zone/configure{,.in}
##L - isdnlog/tools/zone/config.h.in
##L - isdnlog/tools/zone/common.h
##L - isdnlog/tools/dest/Makefile.in
##L - isdnlog/tools/dest/configure{,.in}
##L - isdnlog/tools/dest/makedest
##L - isdnlog/tools/dest/CDB_File_Dump.{pm,3pm} ... (NEW) writes cdb dump files
##L - isdnlog/tools/dest/mcdb ... (NEW) convert testdest dumps to cdb dumps
##L
##L - isdnlog/tools/Makefile ... clean:-target fixed
##L - isdnlog/tools/telnum{.c,.h} ... TELNUM.vbn was not always initialized
##L - isdnlog/tools/rate.c ... fixed bug with R:tag and isdnlog not always
##L                            calculating correct rates (isdnrate worked)
##L
##L  s. isdnlog/tools/NEWS on details for using cdb. and
##L     isdnlog/README 20.a Datenbanken for a note about databases (in German).
##L
##L  As this is the first version with cdb and a major patch there could be
##L  still some problems. If you find something let me know. <lt@toetsch.at>
##L
