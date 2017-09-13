# Makefile for the capiplugin for pppd(8).
#
# Copyright 2000 Carsten Paeth (calle@calle.in-berlin.de)
# Copyright 2000 AVM GmbH Berlin (info@avm.de)
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version
#  2 of the License, or (at your option) any later version.

export TOPDIR=$(shell pwd)
export CAPIINC=$(TOPDIR)/../capi20
export CAPILIB=$(TOPDIR)/../capi20
export INSTALL=$(TOPDIR)/install-sh -c
export INSTALLDATA=$(TOPDIR)/install-sh -c -o root -m 600
export MKDIR=$(TOPDIR)/mkinstalldirs

PPPSRCDIRS=/src/isdn/pppd

ifeq ($(PPPVERSIONS),)
PPPVERSIONS = 2.3.11 2.4.0 2.4.1 2.4.1b1 2.4.1b2
endif

PEERDIR=${DESTDIR}/etc/ppp/peers/isdn
DRDSLDIR=${DESTDIR}/etc/drdsl
PEERS= arcor otelo talkline avm avm-ml leased
DRDSLFILES = adsl.conf

ALL = capiplugin.so userpass.so

all: pversion
	@for i in $(PPPVERSIONS) ; do \
	    echo $(MAKE) -C ppp-$$i PPPVERSION=$$i $@ ; \
	    $(MAKE) -C ppp-$$i PPPVERSION=$$i $@ ; \
	done

distclean: clean

clean:
	@for i in $(PPPVERSIONS) ; do \
	    echo $(MAKE) -C ppp-$$i PPPVERSION=$$i $@ ; \
	    $(MAKE) -C ppp-$$i PPPVERSION=$$i $@ ; \
	done
	$(RM) comperr pversion

install-peers:
	$(MKDIR) $(PEERDIR)
	@for i in $(PEERS); do \
		echo $(INSTALLDATA) peers/$$i $(PEERDIR); \
		$(INSTALLDATA) peers/$$i $(PEERDIR); \
	done

install-drdsl:
	$(MKDIR) $(DRDSLDIR)
	@for i in $(DRDSLFILES); do \
		if [ -f $(DRDSLDIR)/$$i ] ; then \
		   echo $(DRDSLDIR)/$$i already exists ; \
		else \
		  echo $(INSTALLDATA) examples/$$i $(DRDSLDIR); \
		  $(INSTALLDATA) examples/$$i $(DRDSLDIR); \
		fi \
	done

install-man:
	for i in ${DESTDIR}/usr/share/man ${DESTDIR}/usr/man; do \
		if [ -d $$i/man8 ] ; then \
			echo $(INSTALLDATA) capiplugin.8 $$i/man8; \
			$(INSTALLDATA) capiplugin.8 $$i/man8; \
		fi ; \
	done

install: pversion
	@for i in $(PPPVERSIONS) ; do \
	    echo $(MAKE) -C ppp-$$i PPPVERSION=$$i $@ ; \
	    $(MAKE) -C ppp-$$i PPPVERSION=$$i $@ ; \
	done
	$(MAKE) install-peers
	$(MAKE) install-drdsl
	$(MAKE) install-man

srcsetup:
	@for v in $(PPPVERSIONS) ; do \
	    $(MKDIR) ppp-$$v/pppd ; \
	    echo $(INSTALL) Makefile.template ppp-$$v/Makefile ; \
	    $(INSTALL) Makefile.template ppp-$$v/Makefile ; \
	    for h in pppd.h patchlevel.h ; do \
	        echo $(INSTALL) $(PPPSRCDIRS)/ppp-$$v/pppd/$$h ppp-$$v/pppd ; \
	        $(INSTALL) $(PPPSRCDIRS)/ppp-$$v/pppd/$$h ppp-$$v/pppd ; \
	    done \
	done

config:
	@echo nothing to configure
