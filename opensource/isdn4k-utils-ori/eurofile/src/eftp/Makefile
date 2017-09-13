#$Id: Makefile,v 1.4 2000/06/08 10:06:02 keil Exp $
# This Makefile uses GNU-make extensions

default: eftp


#
# If EFT_LIBDIR or EFT_INCDIR are set (the top level makefile
# does so) this makefile will generate dependencies and search pathes
# (refering to the libraries and corresponding header files).
#
# Otherwise (for stand-alone compilation), no such dependencies are generated
# and libraries and includes are only searched in the standard location only.
#

ifdef EFT_LIBDIR
EFT_LIBDIR_FLAGS := -L $(EFT_LIBDIR) 
eftp: $(EFT_LIBDIR)/libeft.a 
endif

ifdef EFT_INCDIR
EFT_INCDIR_FLAGS := -I $(EFT_INCDIR) 
eftp.o: $(EFT_INCDIR)/tdu_user.h $(EFT_INCDIR)/eft.h
endif

ifeq ($(CONFIG_EFTP_READLINE),y)
READLINE_CFLAGS=-DCONFIG_EFTP_READLINE
READLINE_LDFLAGS= -lncurses -lreadline
endif

eftp.o:	eftp.c
	$(CC) $(CFLAGS) $(EFT_INCDIR_FLAGS) -c $(READLINE_CFLAGS) eftp.c -o eftp.o

eftp: eftp.o $(READLINE_O)
	$(LINK.C) eftp.o $(EFT_LIBDIR_FLAGS) $(READLINE_LDFLAGS) -left -o eftp

install: eftp
	$(INSTALL_PROGRAM) eftp $(DESTDIR)$(BINDIR)/
ifeq ($(CONFIG_EFTP_SETUID),y)
	chmod u+s $(DESTDIR)$(BINDIR)/eftp 
endif	

clean:
	rm -f eftp *.o 

.PHONY: clean default
