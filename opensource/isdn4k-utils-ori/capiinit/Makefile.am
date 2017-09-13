##
## $Id: Makefile.am,v 1.1 2000/03/17 16:19:43 calle Exp $
##

AUTOMAKE_OPTIONS	= foreign
CLEANFILES              = *~ comperr
MAINTAINERCLEANFILES    = configure aclocal.m4 Makefile.in config.h.in \
			  stamp-h.in comperr core

INCLUDES = $(all_includes)
CFLAGS = -Wall -O2 -D_GNU_SOURCE # -g
LDFLAGS = $(all_libraries)
LDADD = 

sbin_PROGRAMS = capiinit

capiinit_SOURCES = capiinit.c
