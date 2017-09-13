#CC	= gcc
COPTS	= -O2 
#COPTS	= -Os
CFLAGS	= $(COPTS) -I.. -I../../include -fPIC $(IFX_CFLAGS)
LDFLAGS	= -shared -nostdlib $(IFX_LDFLAGS)
INSTALL	= install -o root

SUBDIRS := rp-pppoe pppoatm
# Uncomment the next line to include the radius authentication plugin
# SUBDIRS += radius
# remove unnecessary plugin
# PLUGINS := minconn.so passprompt.so passwordfd.so

# include dependencies if present
ifeq (.depend,$(wildcard .depend))
include .depend
endif

all:	$(PLUGINS)
	for d in $(SUBDIRS); do $(MAKE) $(MFLAGS) -C $$d all; done

%.so: %.c
	$(CC) -o $@ $(LDFLAGS) $(CFLAGS) $^

VERSION = $(shell awk -F '"' '/VERSION/ { print $$2; }' ../patchlevel.h)
LIBDIR	= $(DESTDIR)/usr/lib/pppd/$(VERSION)

install: $(PLUGINS)
	$(INSTALL) -d $(LIBDIR)
	$(INSTALL) -d $? $(LIBDIR)
	for d in $(SUBDIRS); do $(MAKE) $(MFLAGS) -C $$d INSTALL=$(INSTALL) \
				LIBDIR=$(LIBDIR) install; done

clean:
	rm -f *.o *.so *.a
	for d in $(SUBDIRS); do $(MAKE) $(MFLAGS) -C $$d clean; done

depend:
	$(CPP) -M $(CFLAGS) *.c >.depend
	for d in $(SUBDIRS); do $(MAKE) $(MFLAGS) -C $$d depend; done
