##
## $Id: Makefile.am,v 1.3 2002/07/19 21:23:54 keil Exp $
##

AUTOMAKE_OPTIONS	= foreign
CLEANFILES              = *~ comperr
MAINTAINERCLEANFILES    = configure aclocal.m4 Makefile.in config.h.in \
			  stamp-h.in comperr

INCLUDES = -I../capi20 $(all_includes)
CFLAGS = -Wall -O2
LDFLAGS = -L../capi20/.libs -L../capi20 $(all_libraries)
LDADD = -lcapi20

sbin_PROGRAMS = rcapid

rcapid_SOURCES = rcapid.c
