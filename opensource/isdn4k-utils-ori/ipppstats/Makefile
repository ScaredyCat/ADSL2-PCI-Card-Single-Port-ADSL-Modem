#
# pppstats makefile
# $Id: Makefile.in,v 1.5 2002/07/19 19:03:54 keil Exp $
#
SBINDIR      = '/sbin'
MANDIR       = '/usr/man'
CC           = gcc
INSTALL      = /usr/bin/install -c
INSTALL_SBIN = $(INSTALL) -m 755 -o 0 -g 0
INSTALL_DATA = $(INSTALL) -m 644 -o 0 -g 0
INSTALL_DIR  = $(INSTALL) -m 755 -o 0 -g 0 -d

PPPSTATSRCS  = ipppstats.c
PPPSTATOBJS  = ipppstats.o

CFLAGS       = -fomit-frame-pointer -O2

all: ipppstats

uninstall:
	rm -f $(DESTDIR)$(SBINDIR)/ipppstats $(DESTDIR)$(MANDIR)/man8/ipppstats.8

install: ipppstats ipppstats.man
	$(INSTALL_DIR) $(DESTDIR)$(SBINDIR)
	$(INSTALL_DIR) $(DESTDIR)$(MANDIR)/man8
	$(INSTALL_SBIN) ipppstats $(DESTDIR)$(SBINDIR)/ipppstats
	$(INSTALL_DATA) ipppstats.man $(DESTDIR)$(MANDIR)/man8/ipppstats.8

ipppstats: Makefile $(PPPSTATSRCS)
	$(CC) $(CFLAGS) -o ipppstats ipppstats.c

clean:
	rm -f ipppstats *~ *.o #* core

config:
	@./configure

Makefile: Makefile.in config.status
	./config.status

config.status: configure
	./config.status --recheck

distclean: clean
	rm -f Makefile config.status config.cache config.log ipppstats.man

depend:
	cpp -M $(CFLAGS) $(PPPSTATSRCS) >.depend
#	makedepend $(CFLAGS) $(PPPSTATSRCS)
