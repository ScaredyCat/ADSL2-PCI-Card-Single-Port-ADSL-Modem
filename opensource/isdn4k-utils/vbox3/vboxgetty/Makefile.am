##
## $Id: Makefile.am,v 1.7 1998/11/10 18:36:25 michael Exp $
##

## other directories to process ###########################################

SUBDIRS			= examples docs

## system admin executables to build ######################################

sbin_PROGRAMS		= vboxgetty

## vboxgetty ##############################################################

DEFS		       += -DSYSCONFDIR='"$(sysconfdir)"' -DPKGDATADIR='"$(pkgdatadir)"' -DLOGDIR='"$(packagelogdir)"' -DLOCKDIR='"$(packagelockdir)"' -DPIDDIR='"$(packagepiddir)"'

INCLUDES		= $(all_includes) $(tcl_include)

noinst_LIBRARIES	= libvboxmodem.a

libvboxmodem_a_SOURCES	= libvboxmodem.c libvboxmodem.h

vboxgetty_LDADD		= $(tcl_library) -L. -lvboxmodem

vboxgetty_SOURCES	= log.c log.h \
			  modem.c modem.h \
			  rc.c rc.h \
			  vboxrc.c vboxrc.h \
			  stringutils.c stringutils.h \
			  tclscript.c tclscript.h \
			  vboxgetty.c vboxgetty.h \
			  voice.c voice.h \
			  control.c control.h \
			  lock.c lock.h \
			  audio.c audio.h \
			  breaklist.c breaklist.h

## extra files to distribute ##############################################

EXTRA_DIST		= AUTHORS COPYING

## cleanup ################################################################

CLEANFILES		= *~
MAINTAINERCLEANFILES	= Makefile.in
