# Makefile for the capiplugin for pppd(8).
#
# Copyright 2000 Carsten Paeth (calle@calle.in-berlin.de)
# Copyright 2000 AVM GmbH Berlin (info@avm.de)
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version
#  2 of the License, or (at your option) any later version.

vpath %.c $(TOPDIR)

CC	= gcc
INC     = -I$(TOPDIR) -I$(CAPIINC) -Ipppd
DEFS    = -DPPPVER=$(shell $(TOPDIR)/pversion $(PPPVERSION))
CFLAGS	= -O2 -Wall -fPIC $(DEFS) $(INC) -L$(CAPILIB)
LDFLAGS	= -shared -L$(CAPILIB)

ALL = capiplugin.so userpass.so

all:	$(ALL)

capiplugin.so: capiplugin.o capiconn.o
	$(CC) -o $@ $(LDFLAGS) capiplugin.o capiconn.o -lcapi20dyn

userpass.so: userpass.o
	$(CC) -o $@ $(LDFLAGS) $(CFLAGS) -nostdlib userpass.o

distclean: clean

clean:
	$(RM) *.so *.o comperr

install: $(ALL)
	$(MKDIR) $(PLUGINDIR)
	@for i in $(ALL); do \
		echo $(INSTALL) $$i $(PLUGINDIR); \
		$(INSTALL) $$i $(PLUGINDIR); \
	done

config:
	@echo nothing to configure
