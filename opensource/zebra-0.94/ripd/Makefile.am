## Process this file with automake to produce Makefile.in.

INCLUDES = @INCLUDES@ -I.. -I$(top_srcdir) -I$(top_srcdir)/lib
## chandrav added -DMULTIPATH_NUM=@MULTIPATH_NUM@
DEFS = @DEFS@ -DSYSCONFDIR=\"$(sysconfdir)/\" -DMULTIPATH_NUM=@MULTIPATH_NUM@

INSTALL_SDATA=@INSTALL@ -m 600

LDFLAGS = @LDFLAGS@ $(IFX_LDFLAGS)
CFLAGS = @CFLAGS@ $(IFX_CFLAGS)

if CONFIGURE_RIPD_STANDALONE
noinst_LIBRARIES = librip.a libkernelcomm.a
else
noinst_LIBRARIES = librip.a 
endif

sbin_PROGRAMS = ripd

if CONFIGURE_RIPD_STANDALONE
if  CONFIGURE_DEBUG_RIPD_STANDALONE
librip_a_SOURCES = \
	ripd.c rip_zebra.c rip_interface.c rip_debug.c rip_snmp.c \
	rip_peer.c rip_communication.c rip_read_config.c rip_parser.c

noinst_HEADERS = \
	ripd.h rip_debug.h rip_read_config.h
libkernelcomm_a_SOURCES = \
		../zebra/if_netlink.c ../zebra/rtread_netlink.c ../zebra/if_proc.c  ../zebra/debug.c ../zebra/ioctl.c ../zebra/interface.c ../zebra/rt_netlink.c ../zebra/connected.c 
ripd_LDADD = ./libkernelcomm.a ../lib/libzebra.a

else
librip_a_SOURCES = \
	ripd.c rip_zebra.c rip_interface.c rip_snmp.c \
	rip_peer.c rip_communication.c rip_read_config.c rip_parser.c

noinst_HEADERS = \
	ripd.h rip_read_config.h
libkernelcomm_a_SOURCES = \
		../zebra/if_netlink.c ../zebra/rtread_netlink.c ../zebra/if_proc.c  ../zebra/ioctl.c ../zebra/interface.c ../zebra/rt_netlink.c ../zebra/connected.c
ripd_LDADD = ./libkernelcomm.a ../lib/libzebra.a
endif

else
librip_a_SOURCES = \
	ripd.c rip_zebra.c rip_interface.c rip_debug.c rip_snmp.c \
	rip_routemap.c rip_peer.c rip_offset.c 

noinst_HEADERS = \
	ripd.h rip_debug.h 

ripd_LDADD = ../lib/libzebra.a

endif

ripd_SOURCES = \
	rip_main.c $(librip_a_SOURCES) 


sysconf_DATA = ripd.conf.sample

EXTRA_DIST = $(sysconf_DATA) RIPv2-MIB.txt

install-sysconfDATA: $(sysconf_DATA)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(sysconfdir)
	@list='$(sysconf_DATA)'; for p in $$list; do \
	  if test -f $(srcdir)/$$p; then \
	    echo " $(INSTALL_SDATA) $(srcdir)/$$p $(DESTDIR)$(sysconfdir)/$$p"; \
	    $(INSTALL_SDATA) $(srcdir)/$$p $(DESTDIR)$(sysconfdir)/$$p; \
	  else if test -f $$p; then \
	    echo " $(INSTALL_SDATA) $$p $(DESTDIR)$(sysconfdir)/$$p"; \
	    $(INSTALL_SDATA) $$p $(DESTDIR)$(sysconfdir)/$$p; \
	  fi; fi; \
	done
