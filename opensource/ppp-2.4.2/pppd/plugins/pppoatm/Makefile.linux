#***********************************************************************
#
# Makefile
#
# Makefile for pppoatm plugin.
#
# This program may be distributed according to the terms of the GNU
# General Public License, version 2 or (at your option) any later version.
#
# $Id: Makefile.linux,v 1.2 2002/08/22 02:55:20 clive Exp $
#***********************************************************************

COPTS=-O2 -fPIC -Wall
CFLAGS=$(COPTS) -I../.. -I../../../include/linux -fPIC $(IFX_CFLAGS)
LDFLAGS= -shared -nostdlib $(IFX_LDFLAGS)

ifdef MIPS
LIBATMPATH=../../../../../common/lib
endif

LIBATM=atm

all: pppoatm.so

pppoatm.so: pppoatm.c
	$(CC) -o $@ $(LDFLAGS) $(CFLAGS) $^ -l$(LIBATM)

install: pppoatm.so
	$(INSTALL) -d -m 755 $(LIBDIR)
	$(INSTALL) -c -m 4550 $? $(LIBDIR)

clean:
	rm -f *.o *.so
