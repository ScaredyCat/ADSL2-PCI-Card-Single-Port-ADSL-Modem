# $Id: Makefile,v 1.52 2002/07/19 19:03:49 keil Exp $
#
# Toplevel Makefile for isdn4k-utils
#

.EXPORT_ALL_VARIABLES:

export I4LVERSION = 3.2p1

all:	do-it-all

#
# Make "config" the default target if there is no configuration file.
#

# Following line is important for lib and isdnlog (sl).
export ROOTDIR=$(shell pwd)

ifeq (.config,$(wildcard .config))
include .config
do-it-all:      subtargets
else
CONFIGURATION = config
do-it-all:      config
endif

EXTRADIRS = isdnlog/tools/zone isdnlog/tools/dest

BUILD_ONLY :=
SUBDIRS :=
ifeq ($(CONFIG_ISDNLOG),y)
	SUBDIRS := $(SUBDIRS) lib $(EXTRADIRS) isdnlog
	BUILD_ONLY := isdnlog/tools/cdb
else
	ifeq ($(CONFIG_CTRL_CONF),y)
		SUBDIRS := $(SUBDIRS) lib
	endif
endif
ifeq ($(CONFIG_ISDNCTRL),y)
	SUBDIRS := $(SUBDIRS) isdnctrl
endif
ifeq ($(CONFIG_DIVERTCTRL),y)
	SUBDIRS := $(SUBDIRS) divertctrl
endif
ifeq ($(CONFIG_IPROFD),y)
	SUBDIRS := $(SUBDIRS) iprofd
endif
ifeq ($(CONFIG_ICNCTRL),y)
	SUBDIRS := $(SUBDIRS) icn
endif
ifeq ($(CONFIG_PCBITCTL),y)
	SUBDIRS := $(SUBDIRS) pcbit
endif
ifeq ($(CONFIG_HISAXCTRL),y)
	SUBDIRS := $(SUBDIRS) hisax
endif

ifeq ($(CONFIG_RCAPID),y)
	SUBDIRS := $(SUBDIRS) capi20 capiinfo
else
	ifeq ($(CONFIG_AVMCAPICTRL),y)
		SUBDIRS := $(SUBDIRS) capi20 capiinfo
	endif
endif

ifeq ($(CONFIG_AVMCAPICTRL),y)
	SUBDIRS := $(SUBDIRS) avmb1 capiinit
endif
ifeq ($(CONFIG_ACTCTRL),y)
	SUBDIRS := $(SUBDIRS) act2000
endif
ifeq ($(CONFIG_LOOPCTRL),y)
	SUBDIRS := $(SUBDIRS) loop
endif
ifeq ($(CONFIG_EICONCTRL),y)
	SUBDIRS := $(SUBDIRS) eicon
endif
ifeq ($(CONFIG_IMON),y)
	SUBDIRS := $(SUBDIRS) imon
endif
ifeq ($(CONFIG_IMONTTY),y)
	SUBDIRS := $(SUBDIRS) imontty
endif
ifeq ($(CONFIG_IPPPSTATS),y)
	SUBDIRS := $(SUBDIRS) ipppstats
endif
ifeq ($(CONFIG_XMONISDN),y)
	SUBDIRS := $(SUBDIRS) xmonisdn
endif
ifeq ($(CONFIG_XISDNLOAD),y)
	SUBDIRS := $(SUBDIRS) xisdnload
endif
ifeq ($(CONFIG_IPPPD),y)
	SUBDIRS := $(SUBDIRS) ipppd
endif
ifeq ($(CONFIG_VBOX),y)
	SUBDIRS := $(SUBDIRS) vbox
endif
ifeq ($(CONFIG_RCAPID),y)
	SUBDIRS := $(SUBDIRS) rcapid
endif
ifeq ($(CONFIG_CAPIFAX),y)
	SUBDIRS := $(SUBDIRS) capifax
endif
ifeq ($(CONFIG_PPPDCAPIPLUGIN),y)
	SUBDIRS := $(SUBDIRS) pppdcapiplugin
endif
ifeq ($(CONFIG_GENMAN),y)
	SUBDIRS := $(SUBDIRS) doc
endif
ifeq ($(CONFIG_FAQ),y)
	SUBDIRS := $(SUBDIRS) FAQ
endif
ifeq ($(CONFIG_EUROFILE),y)
	SUBDIRS := $(SUBDIRS) eurofile
endif
ifneq ($(SUBDIRS),)
	ifeq ($(filter lib,$(SUBDIRS)),)
		SUBDIRS := lib $(SUBDIRS)
	endif
endif

subtargets: $(CONFIGURATION)
	set -e; for i in `echo $(BUILD_ONLY) $(SUBDIRS)`; do $(MAKE) -C $$i all; done

rootperm:
	@echo 'main(int argc,char**argv){unlink(argv[0]);return(getuid()==0);}'>g
	@if gcc -x c -o G g && rm -f g && ./G ; then \
		/bin/echo -e "\n\n      Need root permission for (de)installation!\n\n"; \
		exit 1; \
	fi

install: rootperm
	set -e; for i in `echo $(SUBDIRS)`; do $(MAKE) -C $$i install; done
	@if [ -c $(DESTDIR)/dev/isdnctrl0 ] && ls -l $(DESTDIR)/dev/isdnctrl0 | egrep "[[:space:]]45,[[:space:]]+64[[:space:]]" > /dev/null; \
	then \
		/bin/echo -e '(some) ISDN devices already exist, not creating them.\nUse scripts/makedev.sh manually if necessary.'; \
	else \
		sh scripts/makedev.sh $(DESTDIR) ; \
	fi

uninstall: rootperm
	set -e; for i in `echo $(SUBDIRS)`; do $(MAKE) -C $$i uninstall; done

#
# targets clean and distclean go through ALL directories
# regardless of configured options.
#
clean:
	-set -e; \
	for i in `echo ${wildcard */GNUmakefile}`; do \
		$(MAKE) -i -C `dirname $$i` clean; \
	done;
	-set -e; \
	for i in `echo ${wildcard */Makefile}`; do \
		$(MAKE) -i -C `dirname $$i` clean; \
	done;
	for i in `echo $(BUILD_ONLY) $(EXTRADIRS)`; do \
		if [ -f $$i/Makefile ]; then $(MAKE) -i -C $$i clean; fi; \
	done;
	-rm -f *~ *.o

distclean: clean
	-$(MAKE) -C scripts/lxdialog clean
	-set -e; \
	for i in `echo ${wildcard */GNUmakefile}`; do \
		$(MAKE) -i -C `dirname $$i` distclean; \
	done;
	-set -e; \
	for i in `echo ${wildcard */Makefile}`; do \
		if [ -f $$i ] ; then \
			$(MAKE) -i -C `dirname $$i` distclean; \
		fi ; \
	done;
	for i in `echo $(BUILD_ONLY) $(EXTRADIRS)`; do \
		if [ -f $$i/Makefile ]; then $(MAKE) -i -C $$i distclean; fi; \
	done;
	-rm -f *~ .config .config.old scripts/autoconf.h .menuconfig \
		Makefile.tmp .menuconfig.log scripts/defconfig.old
	find . -name '.#*' -exec rm -f {} \;

scripts/lxdialog/lxdialog:
	@$(MAKE) -C scripts/lxdialog all

scripts/autoconf.h: .config
	perl scripts/mk_autoconf.pl

cfgerror:
	@echo ""
	@echo "WARNING! Configure in $(ERRDIR) failed, disabling package"
	@echo ""
	@sleep 1
	@cp etc/Makefile.disabled $(ERRDIR)/Makefile

# Next target makes three attempts to configure:
#  - if a configure script exists, execute it
#  - if a Makefile.in exists, make -f Makefile.in config
#  - if a Makefile already exists, make config
#
subconfig: scripts/autoconf.h
	@echo Selected subdirs: $(BUILD_ONLY) $(SUBDIRS)
	@set -e; for i in `echo $(BUILD_ONLY) $(SUBDIRS)`; do \
		if [ -x $$i/configure ] ; then \
			/bin/echo -e "\nRunning configure in $$i ...\n"; sleep 1; \
			(cd $$i; ./configure --sbindir=$(CONFIG_SBINDIR) --bindir=$(CONFIG_BINDIR) --mandir=$(CONFIG_MANDIR) --datadir=$(CONFIG_DATADIR) || $(MAKE) -C ../ ERRDIR=$$i cfgerror); \
		elif [ -f $$i/Makefile.in ] ; then \
			/bin/echo -e "\nRunning make -f Makefile.in config in $$i ...\n"; sleep 1; \
			$(MAKE) -C $$i -f Makefile.in config; \
		elif [ -f $$i/Makefile ] ; then \
			/bin/echo -e "\nRunning make config in $$i ...\n"; sleep 1; \
			$(MAKE) -C $$i config; \
		fi; \
	done

#
# Next target uses a second tempory Makefile
# because new .config has to be re-included.
#
menuconfig: scripts/lxdialog/lxdialog
	@scripts/Menuconfig scripts/config.in
	@cp Makefile Makefile.tmp
	$(MAKE) -f Makefile.tmp subconfig
	@rm -f Makefile.tmp

#
# For testing: runs Menuconfig only
#
testconfig: scripts/lxdialog/lxdialog
	@scripts/Menuconfig scripts/config.in

config: menuconfig

mrproper: distclean

archive: distclean
	@(cd .. ;\
	ln -nfs isdn4k-utils isdn4k-utils-$(I4LVERSION) ;\
	mkdir -p distisdn ;\
	tar cvhzf distisdn/isdn4k-utils-$(I4LVERSION).tar.gz isdn4k-utils-$(I4LVERSION) ;\
	rm isdn4k-utils-$(I4LVERSION) )

distarch: distclean
	(cd .. ;\
	ln -nfs isdn4k-utils isdn4k-utils-$(I4LVERSION) ;\
	mkdir -p distisdn ;\
	tar -cvhz -X isdn4k-utils/distexclude -f distisdn/isdn4k-utils-$(I4LVERSION).tar.gz \
	isdn4k-utils-$(I4LVERSION) ;\
	rm isdn4k-utils-$(I4LVERSION) )

dist: distarch
