# Generated automatically from Makefile.in by configure.
# Makefile.in generated automatically by automake 1.2 from Makefile.am

# Copyright (C) 1994, 1995, 1996, 1997 Free Software Foundation, Inc.
# This Makefile.in is free software; the Free Software Foundation
# gives unlimited permission to copy, distribute and modify it.


SHELL = /bin/sh

srcdir = .
top_srcdir = ..
prefix = /opt/kde
exec_prefix = ${prefix}

bindir = ${exec_prefix}/bin
sbindir = ${exec_prefix}/sbin
libexecdir = ${exec_prefix}/libexec
sysconfdir = ${prefix}/etc
sharedstatedir = ${prefix}/com
localstatedir = ${prefix}/var
libdir = ${exec_prefix}/lib
infodir = ${prefix}/info
mandir = ${prefix}/man
includedir = ${prefix}/include
oldincludedir = /usr/include

pkgdatadir = $(datadir)/kisdnlog
pkglibdir = $(libdir)/kisdnlog
pkgincludedir = $(includedir)/kisdnlog

top_builddir = ..

ACLOCAL = aclocal
AUTOCONF = autoconf
AUTOMAKE = automake
AUTOHEADER = autoheader

INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_SCRIPT = ${INSTALL_PROGRAM}
transform = s,x,x,

NORMAL_INSTALL = true
PRE_INSTALL = true
POST_INSTALL = true
NORMAL_UNINSTALL = true
PRE_UNINSTALL = true
POST_UNINSTALL = true
build_alias = i586-pc-linux-gnu
build_triplet = i586-pc-linux-gnu
host_alias = i586-pc-linux-gnu
host_triplet = i586-pc-linux-gnu
target_alias = i586-pc-linux-gnu
target_triplet = i586-pc-linux-gnu
CC = gcc
CFLAGS = -g -Wall -DDEBUG
CPP = gcc -E
CXX = c++
CXXFLAGS = -g -Wall -DDEBUG
GLINC = @GLINC@
GLLIB = @GLLIB@
GMSGFMT = /usr/bin/msgfmt
KDE_INCLUDES = -I/opt/kde/include
KDE_LDFLAGS = -L/opt/kde/lib
LDFLAGS = 
LIBCOMPAT = 
LIBJPEG = -L${topdir} -ljpegkde
LIBOBJS = @LIBOBJS@
LIBSOCKET = 
LN_S = ln -s
MAKEINFO = makeinfo
MOC = /usr/bin/moc
MSGFMT = /usr/bin/msgfmt
PACKAGE = kisdnlog
PAMINC = @PAMINC@
PAMLIBPATHS = @PAMLIBPATHS@
PAMLIBS = @PAMLIBS@
QT_INCLUDES = -I/usr/lib/qt/include
QT_LDFLAGS = -L/usr/lib
RANLIB = @RANLIB@
USE_NLS = yes
VERSION = 0.1
XGETTEXT = /usr/bin/xgettext
XPMINC = @XPMINC@
XPMLIB = @XPMLIB@
X_INCLUDES = -I/usr/X11R6/include
X_LDFLAGS = -L/usr/X11R6/lib
all_includes = -I/opt/kde/include -I../.. -I../../tools -I../../connect -I../../../lib -I/usr/lib/qt/include -I/usr/X11R6/include
all_libraries = -L/opt/kde/lib ../../connect/*.o  ../../../lib/libisdn.a -L/usr/lib -L/usr/X11R6/lib
kde_appsdir = $(prefix)/share/applnk
kde_cgidir = $(prefix)/cgi-bin
kde_confdir = $(prefix)/share/config
kde_datadir = $(prefix)/share/apps
kde_htmldir = $(prefix)/share/doc/HTML
kde_icondir = $(prefix)/share/icons
kde_includes = /opt/kde/include
kde_libraries = /opt/kde/lib
kde_locale = $(prefix)/share/locale
kde_mimedir = $(prefix)/share/mimelnk
kde_minidir = $(prefix)/share/icons/mini
kde_toolbardir = $(prefix)/share/toolbar
kde_wallpaperdir = $(prefix)/share/wallpaper
qt_includes = /usr/lib/qt/include
qt_libraries = /usr/lib
topdir = /home/global/misc/develop/c/isdn4k-utils/isdnlog/client

data_DATA = isdnlog.xpm

datadir = $(prefix)/share/apps/kisdnlog/pics
mkinstalldirs = $(SHELL) $(top_srcdir)/mkinstalldirs
CONFIG_HEADER = ../config.h
CONFIG_CLEAN_FILES = 
DATA =  $(data_DATA)

DIST_COMMON =  Makefile.am Makefile.in


DISTFILES = $(DIST_COMMON) $(SOURCES) $(HEADERS) $(TEXINFOS) $(EXTRA_DIST)

TAR = tar
GZIP = --best
default: all

.SUFFIXES:
$(srcdir)/Makefile.in: Makefile.am $(top_srcdir)/configure.in $(ACLOCAL_M4) 
	cd $(top_srcdir) && $(AUTOMAKE) --gnu pixmaps/Makefile

Makefile: $(srcdir)/Makefile.in $(top_builddir)/config.status $(BUILT_SOURCES)
	cd $(top_builddir) \
	  && CONFIG_FILES=$(subdir)/$@ CONFIG_HEADERS= $(SHELL) ./config.status


install-dataDATA: $(data_DATA)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(datadir)
	@list='$(data_DATA)'; for p in $$list; do \
	  if test -f $(srcdir)/$$p; then \
	    echo " $(INSTALL_DATA) $(srcdir)/$$p $(DESTDIR)$(datadir)/$$p"; \
	    $(INSTALL_DATA) $(srcdir)/$$p $(DESTDIR)$(datadir)/$$p; \
	  else if test -f $$p; then \
	    echo " $(INSTALL_DATA) $$p $(DESTDIR)$(datadir)/$$p"; \
	    $(INSTALL_DATA) $$p $(DESTDIR)$(datadir)/$$p; \
	  fi; fi; \
	done

uninstall-dataDATA:
	$(NORMAL_UNINSTALL)
	list='$(data_DATA)'; for p in $$list; do \
	  rm -f $(DESTDIR)$(datadir)/$$p; \
	done
tags: TAGS
TAGS:


distdir = $(top_builddir)/$(PACKAGE)-$(VERSION)/$(subdir)

subdir = pixmaps

distdir: $(DISTFILES)
	here=`cd $(top_builddir) && pwd`; \
	top_distdir=`cd $(top_distdir) && pwd`; \
	cd $(top_srcdir) \
	  && $(AUTOMAKE) --include-deps --build-dir=$$here --srcdir-name=$(top_srcdir) --output-dir=$$top_distdir --gnu pixmaps/Makefile
	@for file in $(DISTFILES); do \
	  d=$(srcdir); \
	  test -f $(distdir)/$$file \
	  || ln $$d/$$file $(distdir)/$$file 2> /dev/null \
	  || cp -p $$d/$$file $(distdir)/$$file; \
	done
info:
dvi:
check: all
	$(MAKE)
installcheck:
install-exec: 
	@$(NORMAL_INSTALL)

install-data: install-dataDATA install-data-local
	@$(NORMAL_INSTALL)

install: install-exec install-data all
	@:

uninstall: uninstall-dataDATA uninstall-local

all: Makefile $(DATA)

install-strip:
	$(MAKE) INSTALL_PROGRAM='$(INSTALL_PROGRAM) -s' INSTALL_SCRIPT='$(INSTALL_PROGRAM)' install
installdirs:
	$(mkinstalldirs)  $(DESTDIR)$(datadir)


mostlyclean-generic:
	test -z "$(MOSTLYCLEANFILES)" || rm -f $(MOSTLYCLEANFILES)

clean-generic:
	test -z "$(CLEANFILES)" || rm -f $(CLEANFILES)

distclean-generic:
	rm -f Makefile $(DISTCLEANFILES)
	rm -f config.cache config.log stamp-h stamp-h[0-9]*
	test -z "$(CONFIG_CLEAN_FILES)" || rm -f $(CONFIG_CLEAN_FILES)

maintainer-clean-generic:
	test -z "$(MAINTAINERCLEANFILES)" || rm -f $(MAINTAINERCLEANFILES)
	test -z "$(BUILT_SOURCES)" || rm -f $(BUILT_SOURCES)
mostlyclean:  mostlyclean-generic

clean:  clean-generic mostlyclean

distclean:  distclean-generic clean
	rm -f config.status

maintainer-clean:  maintainer-clean-generic distclean
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."

.PHONY: default uninstall-dataDATA install-dataDATA tags distdir info \
dvi installcheck install-exec install-data install uninstall all \
installdirs mostlyclean-generic distclean-generic clean-generic \
maintainer-clean-generic clean mostlyclean distclean maintainer-clean


install-data-local:
	$(mkinstalldirs) $(appsdir)
	$(INSTALL_DATA) kisdnlog.xpm $(kde_icondir)/kisdnlog.xpm
	$(INSTALL_DATA) isdnlog.xpm $(kde_datadir)/kisdnlog/isdnlog.xpm

uninstall-local:
	rm -f $(kde_icondir)/kisdnlog.xpm
	rm -f $(kde_datadir)/kisdnlog/isdnlog.xpm

# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
