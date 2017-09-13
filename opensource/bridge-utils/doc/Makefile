
DESTDIR=
KERNEL_HEADERS=-I/Danube/3.0.5/source/kernel/opensource/linux-2.4.31/include

INSTALL=install -s

prefix=/usr
exec_prefix=${prefix}
bindir=${exec_prefix}/bin
sbindir=${exec_prefix}/sbin
mandir=${prefix}/share/man

SUBDIRS=libbridge brctl

all:

clean:

install:
	mkdir -p $(DESTDIR)$(mandir)/man8
	install -m 644 brctl.8 $(DESTDIR)$(mandir)/man8
