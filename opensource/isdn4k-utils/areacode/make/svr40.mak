# *****************************************************************************
# *									      *
# *			  AREACODE Makefile for SVR4			      *
# *									      *
# * (C) 1995-96  Ullrich von Bassewitz					      *
# *		 Wacholderweg 14					      *
# *		 D-70597 Stuttgart					      *
# * EMail:	 uz@ibb.schwaben.com					      *
# *									      *
# *                                                                           *
# * SVR40 Port by Felix Blank (felix@tasha.muc.de)                            *
# *                                                                           *
# *****************************************************************************



# $Id: svr40.mak,v 1.1 1997/03/03 04:21:46 fritz Exp $
#
# $Log: svr40.mak,v $
# Revision 1.1  1997/03/03 04:21:46  fritz
# Added files in areacode/make
#
#
#


# ------------------------------------------------------------------------------
# Stuff you may want to edit

# The name of the data file after installation
DATATARGET=/usr/local/lib/areacodes

# Command line for the installation of the data file
INSTALL	= install -o bin -g bin -m 644

# ------------------------------------------------------------------------------
# Definitions

# Names of executables
AS = gas
AR = ar
LD = ld
ZIP = zip
CC = gcc

# Flags for the GNU C compiler
CFLAGS=-O2 -Wall

# Name of the data file
DATASOURCE=areacode.dat

# ------------------------------------------------------------------------------
# Implicit rules

.c.o:
	gcc $(CFLAGS) -c $<

# ------------------------------------------------------------------------------
#

ifeq (.depend,$(wildcard .depend))
all:	actest acvers 
include .depend
else
all:	depend
endif


actest:		areacode.o actest.o
		gcc -o actest areacode.o actest.o

acvers:		acvers.o
		gcc -o acvers acvers.o

areacode.o:	areacode.h areacode.c
		gcc $(CFLAGS) -DDATA_FILENAME="\"$(DATATARGET)\"" \
		-DCHARSET_ISO -c -o areacode.o areacode.c

install:	areacode.o acvers
		@if [ `id -u` != 0 ]; then				      \
		    echo "";						      \
		    echo 'Do "make install" as root';			      \
		    echo "";						      \
		    false;						      \
		fi
		@if [ -f $(DATATARGET) ]; then				      \
		    NewVersion=`./acvers $(DATASOURCE) | awk '{ print $$3 }'`;\
		    OldVersion=`./acvers $(DATATARGET) | awk '{ print $$3 }'`;\
		    echo "Current datafile build number:  $$OldVersion";      \
		    echo "Build number of new datafile:   $$NewVersion";      \
		    if [ $$NewVersion -gt $$OldVersion ]; then                \
			echo "Installing new datafile";			      \
			$(INSTALL) $(DATASOURCE) $(DATATARGET);		      \
		    else						      \
			echo "Installed datafile is same or newer, skipping...";\
		    fi;							      \
		else							      \
		    echo "Installing new datafile";			      \
		    $(INSTALL) $(DATASOURCE) $(DATATARGET);		      \
		fi

# ------------------------------------------------------------------------------
# Create a dependency file

depend dep:
	@echo "Creating dependency information"
	$(CC) -MM *.c > .depend

# ------------------------------------------------------------------------------
# clean up

distclean:	zap

clean:
	-rm *.bak *~

zap:	clean
	-rm *.o
	-rm .depend


