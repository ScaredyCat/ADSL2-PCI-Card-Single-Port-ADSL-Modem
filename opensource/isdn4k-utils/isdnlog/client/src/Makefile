# Generated automatically from Makefile.in by configure.
# Makefile.in generated automatically by automake 1.2 from Makefile.am

# Copyright (C) 1994, 1995, 1996, 1997 Free Software Foundation, Inc.
# This Makefile.in is free software; the Free Software Foundation
# gives unlimited permission to copy, distribute and modify it.

# this 10 paths are KDE specific. Use them:
# kde_htmldir       Where your docs should go to. (contains lang subdirs)
# kde_appsdir       Where your application file (.kdelnk) should go to. 
# kde_icondir       Where your icon should go to.
# kde_minidir       Where your mini icon should go to.
# kde_datadir       Where you install application data. (Use a subdir)
# kde_locale        Where translation files should go to.(contains lang subdirs)
# kde_cgidir        Where cgi-bin executables should go to.
# kde_confdir       Where config files should go to.
# kde_mimedir       Where mimetypes should go to.
# kde_toolbardir    Where general toolbar icons should go to.
# kde_wallpaperdir  Where general wallpapers should go to.

# just set the variable


SHELL = /bin/sh

srcdir = .
top_srcdir = ..
prefix = /opt/kde
exec_prefix = ${prefix}

bindir = ${exec_prefix}/bin
sbindir = ${exec_prefix}/sbin
libexecdir = ${exec_prefix}/libexec
datadir = ${prefix}/share
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

APPSDIR = $(kde_appsdir)/Utilities
# set the include path for X, qt and KDE
INCLUDES= -I/opt/kde/include -I../.. -I../../tools -I../../connect -I../../../lib -I/usr/lib/qt/include -I/usr/X11R6/include
# claim, which subdirectories you want to install
# SUBDIRS = doc

bin_PROGRAMS = 	kisdnlog

kisdnlog_SOURCES	= functions.cpp kcalls.cpp kcurcalls.cpp koldcalls.cpp kchan.cpp kconnect.cpp khost.cpp kisdnlog.h klogo.cpp klog.cpp klogwin.cpp kmenu.cpp kmessage.cpp ksplit.cpp kthruput.cpp main.cpp message.cpp message.h messagenr.h mkisdnlog.cpp

# the library search path
kisdnlog_LDFLAGS = -L/opt/kde/lib ../../connect/*.o  ../../../lib/libisdn.a -L/usr/lib -L/usr/X11R6/lib

# the libraries to link against. Be aware of the order. First the libraries,
# that depend on the following ones.
kisdnlog_LDADD   = -lkdecore -lkdeui -lqt -lX11 -lXext 

# this option you can leave out. Just, if you use "make dist", you need it
noinst_HEADERS = 

# just to make sure, automake makes them 
BUILTSOURCES =	mkisdnlog.cpp

# if you "make distclean", this files get removed. If you want to remove
# them while "make clean", use CLEANFILES
DISTCLEANFILES = $(BUILTSOURCES)
mkinstalldirs = $(SHELL) $(top_srcdir)/mkinstalldirs
CONFIG_HEADER = ../config.h
CONFIG_CLEAN_FILES = 
PROGRAMS =  $(bin_PROGRAMS)


DEFS = -DHAVE_CONFIG_H -I. -I$(srcdir) -I..
CPPFLAGS = 
LIBS = 
kisdnlog_OBJECTS =  functions.o kcalls.o kcurcalls.o koldcalls.o kchan.o \
kconnect.o khost.o klogo.o klog.o klogwin.o kmenu.o kmessage.o ksplit.o \
kthruput.o main.o message.o mkisdnlog.o
kisdnlog_DEPENDENCIES = 
CXXCOMPILE = $(CXX) $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CXXFLAGS)
CXXLINK = $(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@
HEADERS =  $(noinst_HEADERS)

DIST_COMMON =  Makefile.am Makefile.in


DISTFILES = $(DIST_COMMON) $(SOURCES) $(HEADERS) $(TEXINFOS) $(EXTRA_DIST)

TAR = tar
GZIP = --best
DEP_FILES =  .deps/functions.P .deps/kcalls.P .deps/kchan.P \
.deps/kconnect.P .deps/kcurcalls.P .deps/khost.P .deps/klog.P \
.deps/klogo.P .deps/klogwin.P .deps/kmenu.P .deps/kmessage.P \
.deps/koldcalls.P .deps/ksplit.P .deps/kthruput.P .deps/main.P \
.deps/message.P .deps/mkisdnlog.P
CXXMKDEP = $(CXX) -M $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CXXFLAGS)
SOURCES = $(kisdnlog_SOURCES)
OBJECTS = $(kisdnlog_OBJECTS)

default: all

.SUFFIXES:
.SUFFIXES: .c .cpp .o
$(srcdir)/Makefile.in: Makefile.am $(top_srcdir)/configure.in $(ACLOCAL_M4) 
	cd $(top_srcdir) && $(AUTOMAKE) --gnu src/Makefile

Makefile: $(srcdir)/Makefile.in $(top_builddir)/config.status $(BUILT_SOURCES)
	cd $(top_builddir) \
	  && CONFIG_FILES=$(subdir)/$@ CONFIG_HEADERS= $(SHELL) ./config.status


mostlyclean-binPROGRAMS:

clean-binPROGRAMS:
	test -z "$(bin_PROGRAMS)" || rm -f $(bin_PROGRAMS)

distclean-binPROGRAMS:

maintainer-clean-binPROGRAMS:

install-binPROGRAMS: $(bin_PROGRAMS)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(bindir)
	@list='$(bin_PROGRAMS)'; for p in $$list; do \
	  if test -f $$p; then \
	    echo "  $(INSTALL_PROGRAM) $$p $(DESTDIR)$(bindir)/`echo $$p|sed '$(transform)'`"; \
	     $(INSTALL_PROGRAM) $$p $(DESTDIR)$(bindir)/`echo $$p|sed '$(transform)'`; \
	  else :; fi; \
	done

uninstall-binPROGRAMS:
	$(NORMAL_UNINSTALL)
	list='$(bin_PROGRAMS)'; for p in $$list; do \
	  rm -f $(DESTDIR)$(bindir)/`echo $$p|sed '$(transform)'`; \
	done

.c.o:
	$(COMPILE) -c $<

mostlyclean-compile:
	rm -f *.o core

clean-compile:

distclean-compile:
	rm -f *.tab.c

maintainer-clean-compile:

kisdnlog: $(kisdnlog_OBJECTS) $(kisdnlog_DEPENDENCIES)
	@rm -f kisdnlog
	$(CXXLINK) $(kisdnlog_LDFLAGS) $(kisdnlog_OBJECTS) $(kisdnlog_LDADD) $(LIBS)
.cpp.o:
	$(CXXCOMPILE) -c $<

tags: TAGS

ID: $(HEADERS) $(SOURCES)
	here=`pwd` && cd $(srcdir) && mkid -f$$here/ID $(SOURCES) $(HEADERS)

TAGS:  $(HEADERS) $(SOURCES)  $(TAGS_DEPENDENCIES)
	tags=; \
	here=`pwd`; \
	test -z "$(ETAGS_ARGS)$(SOURCES)$(HEADERS)$$tags" \
	  || (cd $(srcdir) && etags $(ETAGS_ARGS) $$tags  $(SOURCES) $(HEADERS) -o $$here/TAGS)

mostlyclean-tags:

clean-tags:

distclean-tags:
	rm -f TAGS ID

maintainer-clean-tags:

distdir = $(top_builddir)/$(PACKAGE)-$(VERSION)/$(subdir)

subdir = src

distdir: $(DISTFILES)
	here=`cd $(top_builddir) && pwd`; \
	top_distdir=`cd $(top_distdir) && pwd`; \
	cd $(top_srcdir) \
	  && $(AUTOMAKE) --include-deps --build-dir=$$here --srcdir-name=$(top_srcdir) --output-dir=$$top_distdir --gnu src/Makefile
	@for file in $(DISTFILES); do \
	  d=$(srcdir); \
	  test -f $(distdir)/$$file \
	  || ln $$d/$$file $(distdir)/$$file 2> /dev/null \
	  || cp -p $$d/$$file $(distdir)/$$file; \
	done

MKDEP = gcc -M $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)

DEPS_MAGIC := $(shell mkdir .deps > /dev/null 2>&1 || :)
-include .deps/.P
.deps/.P: $(BUILT_SOURCES)
	echo > $@

-include $(DEP_FILES)

mostlyclean-depend:

clean-depend:

distclean-depend:

maintainer-clean-depend:
	rm -rf .deps

.deps/%.P: %.c
	@echo "Computing dependencies for $<..."
	@o='o'; \
	test -n "$o" && o='$$o'; \
	$(MKDEP) $< >$@.tmp \
	  && sed "s,^\(.*\)\.o:,\1.$$o \1.l$$o $@:," < $@.tmp > $@ \
	  && rm -f $@.tmp

.deps/%.P: %.cpp
	@echo "Computing dependencies for $<..."
	@o='o'; \
	$(CXXMKDEP) $< >$@.tmp \
	  && sed "s,^\(.*\)\.o:,\1.$$o \1.l$$o $@:," < $@.tmp > $@ \
	  && rm -f $@.tmp
info:
dvi:
check: all
	$(MAKE)
installcheck:
install-exec: install-binPROGRAMS
	@$(NORMAL_INSTALL)

install-data: install-data-local
	@$(NORMAL_INSTALL)

install: install-exec install-data all
	@:

uninstall: uninstall-binPROGRAMS uninstall-local

all: Makefile $(PROGRAMS) $(HEADERS)

install-strip:
	$(MAKE) INSTALL_PROGRAM='$(INSTALL_PROGRAM) -s' INSTALL_SCRIPT='$(INSTALL_PROGRAM)' install
installdirs:
	$(mkinstalldirs)  $(bindir)


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
mostlyclean:  mostlyclean-binPROGRAMS mostlyclean-compile \
		mostlyclean-tags mostlyclean-depend mostlyclean-generic

clean:  clean-binPROGRAMS clean-compile clean-tags clean-depend \
		clean-generic mostlyclean

distclean:  distclean-binPROGRAMS distclean-compile distclean-tags \
		distclean-depend distclean-generic clean
	rm -f config.status

maintainer-clean:  maintainer-clean-binPROGRAMS maintainer-clean-compile \
		maintainer-clean-tags maintainer-clean-depend \
		maintainer-clean-generic distclean
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."

.PHONY: default mostlyclean-binPROGRAMS distclean-binPROGRAMS \
clean-binPROGRAMS maintainer-clean-binPROGRAMS uninstall-binPROGRAMS \
install-binPROGRAMS mostlyclean-compile distclean-compile clean-compile \
maintainer-clean-compile tags mostlyclean-tags distclean-tags \
clean-tags maintainer-clean-tags distdir mostlyclean-depend \
distclean-depend clean-depend maintainer-clean-depend info dvi \
installcheck install-exec install-data install uninstall all \
installdirs mostlyclean-generic distclean-generic clean-generic \
maintainer-clean-generic clean mostlyclean distclean maintainer-clean


# make messages.po. Move this one to ../po/ and "make merge" in po
messages:
	$(XGETTEXT) -C -ktranslate $(kisdnlog_SOURCES)

# just install datas here. Use install-exec-data for scripts and etc.
# the binary itself is already installed from automake
# use mkinstalldirs, not "install -d"
# don't install a list of file. Just one file per install.
# if you have more of them, create a subdirectory with an extra Makefile 
install-data-local: 
	$(mkinstalldirs) $(APPSDIR)
	$(INSTALL_DATA) Kisdnlog.kdelnk $(APPSDIR)

# remove ALL you have installed in install-data-local or install-exec-local
uninstall-local:
	-rm -f $(APPSDIR)/Kisdnlog.kdelnk

# add a dependency for every moc file to be full portable
# I've added a key binding to emacs for this. 
mkisdnlog.cpp: kisdnlog.h
	$(MOC) kisdnlog.h -o mkisdnlog.cpp

# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
