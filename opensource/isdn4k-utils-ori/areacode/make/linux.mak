# *****************************************************************************
# *									      *
# *			  AREACODE Makefile for Linux			      *
# *									      *
# * (C) 1995-96  Ullrich von Bassewitz					      *
# *		 Wacholderweg 14					      *
# *		 D-70597 Stuttgart					      *
# * EMail:	 uz@ibb.schwaben.com					      *
# *									      *
# *****************************************************************************



# $Id: linux.mak,v 1.2 1998/12/09 19:21:16 akool Exp $
#
# $Log: linux.mak,v $
# Revision 1.2  1998/12/09 19:21:16  akool
# new "Areacode" lib version 2.0
# from Ullrich von Bassewitz (uz@wuschel.musoftware.de)
# now we are supporting .at .ch .de .nl .uk and .us
# ATTENTION: The database-format has changed!
#
# Revision 1.1  1997/03/03 04:21:46  fritz
# Added files in areacode/make
#
#
#


# ------------------------------------------------------------------------------
# Stuff you may want to edit

# The name of the data file after installation

DATATARGET=/usr/lib/areacodes

# Command line for the installation of the data file
INSTALL	= install -o bin -g bin -m 644

# ------------------------------------------------------------------------------
# Definitions

# Names of executables
AS = gas
AR = ar
LD = ld
ZIP = zip

# Flags for the GNU C compiler
CFLAGS	= -g -O2 -Wall

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
		-c -o areacode.o areacode.c

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

depend dep: .depend

.depend:
	@echo "Creating dependency information"
	$(CC) -MM *.c > .depend

# ------------------------------------------------------------------------------
# clean up

distclean:	zap

clean:
	-rm -f *.bak *~ *.o
	-rm -f acvers actest

zap:	clean
	-rm -f .depend
	-rm -f *.o


