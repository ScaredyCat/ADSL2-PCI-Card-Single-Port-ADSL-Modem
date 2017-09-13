# $Id: Makefile.in,v 1.20 2002/07/19 21:23:54 keil Exp $
#
# Makefile for isdnctrl
# (C) 1997 Fritz Elfert
#
#
SHELL			  = /bin/sh
CFLAGS			= -Wall -O2
INCLUDES		= -I.
LDFLAGS			=  
PROGRAM			= isdnctrl
MODULES			= isdnctrl.o
MANPAGES		= isdnctrl.8
SBINDIR			= '/root/danube_build/root_filesystem/sbin'
MANDIR			= '/root/danube_build/root_filesystem/usr/man'
MAN8DIR			= $(MANDIR)/man8
INSTALL			= /usr/bin/install -c
INSTALL_PROGRAM	= $(INSTALL) -o 0 -g 0 -m 0750
INSTALL_MAN		= $(INSTALL) -o 0 -g 0 -m 0644
CC				= gcc -DVERSION=\"3.2p1\"
prefix			= /usr
exec_prefix		= ${prefix}

ifeq (n,y)
COPTS		+= -DI4L_CTRL_CONF
LDFLAGS		+= -L../lib -lisdn
MODULES		+= ctrlconf.o
MANPAGES    += .isdnctrl_conf.8
endif

ifeq (n,y)
COPTS		+= -DI4L_DWABC_UDPINFO
ABCLIBDIR   = /usr/lib
ABCICLDIR   = /usr/include
endif

.SUFFIXES:
.SUFFIXES: .c .o

ifeq (n,y)
all: libisdnudp.a $(PROGRAMM)

LDFLAGS 	+= libisdnudp.a

libisdnudp.a: isdn_dwabclib.o
		ar rs libisdnudp.a isdn_dwabclib.o
		ranlib libisdnudp.a
else
all: $(PROGRAM)
endif

%.o: %.c Makefile
	$(CC) $(CFLAGS) $(INCLUDES) $(DEFS) $(COPTS) -c -o $@ $<

all: $(PROGRAM)

config:
	@./configure

Makefile: Makefile.in config.status
	./config.status

config.status: configure
	./config.status --recheck

%.8: %.man
	rm -f $<.tmp
	sed -e "s%@MAN8DIR@%$(MAN8DIR)%" -e "/@MANINC_/s/'//g" < $< > $<.tmp
ifeq (n,y)
	sed -e 's%@MANINC_CONF@%.so%' < $<.tmp > $@
else
	sed -e 's%@MANINC_CONF@%.\\\"%' < $<.tmp > $@
endif
	rm -f $<.tmp

$(PROGRAM):	$(MODULES)
	$(CC) $(CFLAGS) $(MODULES) $(LDFLAGS) -o $@

install-man: $(MANPAGES)
	$(INSTALL) -d $(DESTDIR)$(MAN8DIR)
	$(INSTALL_MAN) $^ $(DESTDIR)$(MAN8DIR)

install: $(PROGRAM) install-man
	$(INSTALL) -d $(DESTDIR)$(SBINDIR)
	$(INSTALL_PROGRAM) $(PROGRAM) $(DESTDIR)$(SBINDIR)/$(PROGRAM)
ifeq (n,y)
	$(INSTALL) -d $(DESTDIR)$(ABCLIBDIR)
	$(INSTALL) -d $(DESTDIR)$(ABCICLDIR)
	$(INSTALL_MAN) libisdnudp.a $(DESTDIR)$(ABCLIBDIR)/libisdnudp.a
	$(INSTALL_MAN) isdn_dwabclib.h $(DESTDIR)$(ABCICLDIR)/isdn_dwabclib.h 
endif

install-strip: $(PROGRAM)
	$(INSTALL) -d $(DESTDIR)$(SBINDIR)
	$(INSTALL_PROGRAM) -s $(PROGRAM) $(DESTDIR)$(SBINDIR)/$(PROGRAM)

uninstall:
	rm -f $(DESTDIR)$(SBINDIR)/$(PROGRAM)
	for i in $(MANPAGES) ; do rm $(DESTDIR)$(MAN8DIR)/$$i ; done
ifeq (n,y)
	rm -f $(DESTDIR)$(ABCLIBDIR)/libisdnudp.a $(DESTDIR)$(ABCICLDIR)/isdn_dwabclib.h
endif

clean:
	rm -f *.o *.a *~ $(PROGRAM) *.man *.8 .*.man .*.8 *.tmp .*.tmp

distclean: clean
	rm -f config.status config.cache config.log config.h Makefile
