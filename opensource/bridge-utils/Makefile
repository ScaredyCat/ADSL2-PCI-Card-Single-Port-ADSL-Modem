
DESTDIR=
KERNEL_HEADERS=-I/Danube/3.0.5/source/kernel/opensource/linux-2.4.31/include

INSTALL=install -s

prefix=/usr
exec_prefix=${prefix}
bindir=${exec_prefix}/bin
sbindir=${exec_prefix}/sbin
mandir=${prefix}/share/man

#SUBDIRS=libbridge brctl doc
#IFX TC modify
SUBDIRS=libbridge brctl

all:
	for x in $(SUBDIRS); do (cd $$x && $(MAKE) ); done

clean:
	for x in $(SUBDIRS); do (cd $$x && $(MAKE) clean ); done

distclean:	clean
	rm -f config.log
	rm -f config.cache
	rm -f config.status

maintainer-clean: distclean
	rm -f Makefile
	rm -f brctl/Makefile
	rm -f libbridge/Makefile
	rm -f doc/Makefile

install:
	for x in $(SUBDIRS); do (cd $$x && $(MAKE) install ); done

