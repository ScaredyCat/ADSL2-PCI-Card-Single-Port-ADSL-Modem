##
## $Id: Makefile.am,v 1.5 1998/08/30 17:32:15 michael Exp $
##

## The tcl scripts to install ############################################

EXTRA_DIST		= initmodem.tcl answercall.tcl

## Clean & Maintenance ###################################################

CLEANFILES              = *~
MAINTAINERCLEANFILES    = Makefile.in

## Local install routines ################################################

install-data-local:
			  $(mkinstalldirs) $(pkgdatadir)/tcl
			  $(INSTALL) -m 644 initmodem.tcl $(pkgdatadir)/tcl/
			  $(INSTALL) -m 644 answercall.tcl $(pkgdatadir)/tcl/
