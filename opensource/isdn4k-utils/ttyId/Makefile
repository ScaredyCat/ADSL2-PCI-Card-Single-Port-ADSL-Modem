# $Id: Makefile,v 1.2 2000/09/14 19:31:52 paul Exp $

VERSION = 0.1

libdir          = $(exec_prefix)/lib

CC              = gcc
CCFLAGS        = -O -Wall -I.
LINK            = $(CC)

CFLAGS		=	$(CCFLAGS) \
			-DVERSION=\"$(VERSION)\"

LDFLAGS         = -L$(libdir)


MAKE		=	make
MKDIR		=	mkdir -p -m 755
STRIP		=	strip
RM		=	rm
INSTALL		=	install -c -o root -g root -m 700
INSTALL2	=	install -c -o root -g root -m 600

all:			ttyId

MAINOBJS        =	main.o pty.o utils.o profile.o emulator.o \
			communicate.o

ttyId:			$(MAINOBJS)
			$(CC) $(MAINOBJS) -o ./ttyId

clean:			ignore	
			@find . -name '*.o' -exec $(RM) {} ';'

distclean:		clean
			rm -f ttyId

ignore:
