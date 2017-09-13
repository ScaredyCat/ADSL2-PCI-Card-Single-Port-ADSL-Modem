# tty line ISDN status monitor
#
# (c) 1995-97 Volker Götz
#
# $Id: Makefile,v 1.7 2002/07/19 21:23:53 keil Exp $

ifeq (../.config,$(wildcard ../.config))
	#
	# Automatic config with isdn4k-utils
	#
	include ../.config
	MAN8DIR         := $(CONFIG_MANDIR)/man8
else
	#
	# Manual config standalone
	#
	CONFIG_SBINDRIR := /sbin
	MAN8DIR         := /usr/man/man8
endif

INSTALL=install
INSTALL_MAN=$(INSTALL) -o 0 -g 0 -m 0644
MANPAGE=imontty.8

CCFLAGS=-O2
#CCFLAGS=-I$(ISDN_INCLUDE) -g

# nothing to change behind this line

PROGS=imontty

all: $(PROGS)

imontty: imontty.c imontty.h
	cc $(CCFLAGS) -o imontty imontty.c

rootperm:
	@echo 'main(int argc,char**argv){unlink(argv[0]);return(getuid()==0);}'>g
	@if gcc -x c -o G g && rm -f g && ./G ; then \
		echo -e "\n\n      Need root permission for (de)installation!\n\n"; \
		exit 1; \
	fi

imontty.8: imontty.8.in
	MANDATE=`grep CHECKIN $< | awk '{print $$4}'`; \
	sed \
	   -e "s#\@MANDATE\@#$${MANDATE}#g" \
	   -e "s#\@I4LVERSION\@#$${I4LVERSION}#g" \
	   < $< > $@

install-man: $(MANPAGE)
	mkdir -p $(DESTDIR)$(MAN8DIR)
	$(INSTALL_MAN) $< $(DESTDIR)$(MAN8DIR)/$(MANPAGE)

install: $(PROGS) rootperm install-man
	install -s -o 0 -g 0 -m 0755 $(PROGS) $(DESTDIR)$(CONFIG_SBINDIR)

uninstall: rootperm
	for i in $(PROGS) ; do rm -f $(DESTDIR)$(CONFIG_SBINDIR)/$$i; done

clean:
	rm -f *.o $(PROGS) *~

distclean: clean
	rm -f $(MANPAGE)

#
# dummy for isdn4k-utils package
#
config:
