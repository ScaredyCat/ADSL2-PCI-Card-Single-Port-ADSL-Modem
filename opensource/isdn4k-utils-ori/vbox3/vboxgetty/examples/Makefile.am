##
## $Id: Makefile.am,v 1.6 1998/11/10 18:36:59 michael Exp $
##

## Other directories #####################################################

SUBDIRS			= scripts

## Configuration to install ##############################################

EXTRA_DIST		= vboxgetty.conf vboxcallerid.conf

## Clean & Maintenance ###################################################

CLEANFILES		= *~
MAINTAINERCLEANFILES	= Makefile.in

## Local install routines ################################################

install-data-local:
			  $(mkinstalldirs) $(sysconfdir)
			  $(INSTALL) -m 664 vboxgetty.conf $(sysconfdir)
			  $(INSTALL) -m 664 callerid.conf  $(sysconfdir)
