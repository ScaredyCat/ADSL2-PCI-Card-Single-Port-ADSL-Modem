# example of /etc/isdn/isdn.conf
# copy this file to /etc/isdn/isdn.conf and edit
#
# More information: /usr/doc/packages/i4l/isdnlog/README


[GLOBAL]
COUNTRYPREFIX   = +
COUNTRYCODE     = 48
AREAPREFIX      = 0

# EDIT THIS LINE:
AREACODE        = 22

[VARIABLES]

[ISDNLOG]
LOGFILE = /var/log/isdn.log
ILABEL  = %b %e %T %ICall to tei %t from %N2 on %n2
OLABEL  = %b %e %T %Itei %t calling %N2 with %n2
REPFMTWWW       = "%X %D %17.17H %T %-17.17F %-20.20l SI: %S %9u %U %I %O"
REPFMTSHORT     = "%X%D %8.8H %T %-14.14F%U%I %O"
REPFMT  = "  %X %D %15.15H %T %-15.15F %7u %U %I %O"
CHARGEMAX       = 10.00
CURRENCY = 0.3538,PLN

COUNTRYFILE = /usr/lib/isdn/country-de.dat
RATECONF= /etc/isdn/rate.conf
RATEFILE= /usr/lib/isdn/rate-pl.dat
HOLIDAYS= /usr/lib/isdn/holiday-de.dat
#ZONEFILE= /usr/lib/isdn/zone-de-%s.cdb
DESTFILE= /usr/lib/isdn/dest.cdb

# providerselect
VBN = 010
VBNLEN = 2:3
PRESELECTED=10

