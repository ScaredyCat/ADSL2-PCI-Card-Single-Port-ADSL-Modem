##
## Makefile for library for isdn4linux
##

##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2, or (at your option)
## any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

ifndef ROOTDIR
export ROOTDIR=$(shell pwd)
PREFIXDIR =.
else
PREFIXDIR =$(ROOTDIR)
endif

######################################################################
# USER CONFIGURATION AREA
######################################################################

CFLAGS      = -g -Wall -pipe #-O6
DEFS        =
ifndef _CC
export _CC  = gcc
endif
CC          = $(_CC)

export I4LCONFDIR

######################################################################
# DON'T EDIT BELOW THIS LINE
######################################################################

SUBDIRS  = 

DEFS    += -I$(PREFIXDIR)

%.o: %.c
	$(CC) $(CFLAGS) $(DEFS) -c -o $@ $<

all: .depend libisdn.a

depend dep: .depend

.depend:
		$(CPP) -M $(CFLAGS) $(DEFS) *.c >.depend

clean:
		-rm -f *.o *.a

distclean: clean
		-rm -f .depend policy.h

install: depend all

deinstall: distclean

libisdn.a: conffile.o isdntools.o libtools.o 
		ar rs libisdn.a isdntools.o conffile.o libtools.o 
		ranlib libisdn.a

ifeq (.depend,$(wildcard .depend))
include .depend
#HEADERS := $(HEADERS) .depend
endif

