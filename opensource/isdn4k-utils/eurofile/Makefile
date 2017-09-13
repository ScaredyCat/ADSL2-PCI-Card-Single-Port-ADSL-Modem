# $Id: Makefile,v 1.14 2000/06/08 10:06:01 keil Exp $

-include Rules.make


SHELL = /bin/sh

CVS = cvs -z 4

SCRIPTS = scripts/eftd.sh scripts/eftp.sh scripts/check_system
AUTO_CONFIGURED_FILES = Rules.make scripts/eftd.sh scripts/eftp.sh examples/eft.conf 

#
#additions to the CHANGES file shall automatically increase the compiled-in
#version string 
#
E4L_EXTRA_VERSION_DEP := $(shell pwd)/CHANGES
export E4L_EXTRA_VERSION_DEP

CONF_DEPS = ./configure $(AUTO_CONFIGURED_FILES) 

#this target is required by the isdn4k-utils root makefile's default target
all: $(CONF_DEPS)
	make -C src

really_all:	$(CONF_DEPS)
	make -C src all

config src/config.h $(AUTO_CONFIGURES_FILES): configure 
	./configure
	chmod a+x scripts/eftd.sh scripts/eftp.sh

configure: configure.in
	autoconf

permissions: $(SCRIPTS)
	chmod a+x $(SCRIPTS)

clean:
	rm -f  xx[0-9][0-9] CHANGES.tmp
	make -C src clean

distclean: clean
	rm -f config.* *~ '#'*'#' $(AUTO_CONFIGURED_FILES)
	make -C src distclean
	rm -f  Rules.make eftp4linux*.tar.gz
	rm -f `find . \( -name core -o -name '*.orig' -o -name '*.rej' -o \
		-name '*~' -o -name '*.bak' -o -name '#*#' -o -name '.*.orig'\
		-o -name '.*.rej' -o -name '.SUMS' -o -size 0 \) -print` TAGS

configureclean: distclean
	rm -f configure
#
# This (dummy) target is required by the isdn4k-utils
# root makefile but not implemented yet. 
#
install: 
	make -C src install
ifeq ($(CONFIG_EFT_INSTALL_EFTD_SH),y)
	cd scripts  &&  $(INSTALL_SCRIPT) eftd.sh $(DESTDIR)$(SYSVINIT_RC_DIR)/
endif
ifeq ($(CONFIG_EFT_INSTALL_CONF),y)
	cd examples  &&  $(INSTALL_DATA) eft.conf $(DESTDIR)$(I4LCONFDIR)/	
endif
	@if test -f /etc/ftpaccess; then test -e $(DESTDIR)$(I4LCONFDIR)/eftaccess || ln -sf /etc/ftpaccess $(DESTDIR)$(I4LCONFDIR)/eftaccess; fi
	@if test -f /etc/ftpusers; then test -e $(DESTDIR)$(I4LCONFDIR)/eftusers || ln -sf /etc/ftpusers $(DESTDIR)$(I4LCONFDIR)/eftusers; fi

commit: distclean configure
	$(CVS) commit .

update:
	$(CVS) update -dP .

new_version:
	@echo current version is `grep E4L_VERSION src/eft/include/tdu_user.h|sed 's/^.*VERSION//'`
	@if [ -z "$(NEW_VERSION)" ]; then echo "You must define the variable NEW_VERSION when making a new version!"; exit 1; fi
	@echo new version is $(NEW_VERSION)
# Make a new release. The variable NEW_VERSION must be set in the
# make command's arguments or environment.
# : update
release: new_version commit
	make -C src new_version
	rm -f xx[0-9][0-9]
	csplit CHANGES '/$$Log:/+1'
	mv -f xx00 CHANGES.tmp
	echo '--------------' >> CHANGES.tmp
	echo "Version $(NEW_VERSION)"  >> CHANGES.tmp
	echo '--------------' >> CHANGES.tmp
	cat xx[0-9][0-9] >> CHANGES.tmp
	rm -f xx[0-9][0-9]
	mv -f CHANGES CHANGES.old
	mv CHANGES.tmp CHANGES
	$(CVS) commit -m "eftp4linux $(NEW_VERSION)" .
	$(CVS) tag `echo eftp4linux-$(NEW_VERSION)|sed s/\\\\./_/g` .
	tar -czf ../eftp4linux-$(NEW_VERSION).tar.gz .

.PHONY: all config permissions clean distclean release configureclean commit update default
