# $Id: Makefile,v 1.3 2000/06/08 10:06:01 keil Exp $

default: eftd

#
# If EFT_LIBDIR EFT_INCDIR and EFT_AUTHDIR are set (the top level makefile
# does so) this makefile will generate dependencies and search pathes
# (refering to the libraries and corresponding header files).
#
# Otherwise (for stand-alone compilation), no dependencies are generated
# and libraries and includes are only searched in the standard location only.
#

ifdef EFT_LIBDIR
EFT_LIBDIR_FLAGS := -L $(EFT_LIBDIR) 
eftd: $(EFT_LIBDIR)/libeft.a 
endif

ifdef EFT_INCDIR
EFT_INCDIR_FLAGS := -I $(EFT_INCDIR) 
eftd.o: $(EFT_INCDIR)/tdu_user.h $(EFT_INCDIR)/eft.h
endif

LIBEFTDIR = ../libeft
LIBAUTHDIR = ../libwuauth


AUTH_FLAGS =

ifdef CONFIG_EFTD_WUAUTH
AUTH_FLAGS = -DCONFIG_EFTD_WUAUTH
AUTH_LIB = -lwuauth
ifdef EFT_AUTHDIR
EFT_LIBDIR_FLAGS += -L $(EFT_AUTHDIR) 
eftd: $(EFT_AUTHDIR)/libwuauth.a 
# there is no header file for interfacing the authentification library yet
endif
endif


eftd.o:	eftd.c
	$(COMPILE.c) $(EFT_INCDIR_FLAGS) $(AUTH_FLAGS) -c eftd.c -o eftd.o

eftd: eftd.o 
	$(LINK.o) eftd.o $(EFT_LIBDIR_FLAGS) $(AUTH_LIB) -left $(CRYPTLIB) -o eftd

install: eftd
	$(INSTALL_PROGRAM) eftd $(DESTDIR)$(SBINDIR)/

.PHONY: clean default install

clean:
	rm -f eftd *.o 

