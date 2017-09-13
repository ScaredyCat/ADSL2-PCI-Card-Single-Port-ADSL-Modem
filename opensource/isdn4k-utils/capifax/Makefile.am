##
## $Id: Makefile.am,v 1.3 2000/03/03 15:54:12 calle Exp $
##

AUTOMAKE_OPTIONS	= foreign
CLEANFILES              = *~ comperr config.cache config.status config.log
MAINTAINERCLEANFILES    = configure aclocal.m4 Makefile.in config.h.in \
			  stamp-h.in

INCLUDES = -I../capi20 $(all_includes)
CFLAGS = -Wall -O2
LDFLAGS = -L../capi20/.libs -L../capi20 $(all_libraries)
LDADD = -lcapi20

bin_PROGRAMS = capifax capifaxrcvd

common = c20msg.c capi.c connect.c contr.c data.c id.c init.c fax.c

capifax_SOURCES = $(common) capifax.c

capifaxrcvd_SOURCES = $(common) capifaxrcvd.c

