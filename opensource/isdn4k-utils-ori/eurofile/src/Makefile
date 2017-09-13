# $Id: Makefile,v 1.4 2002/07/19 21:23:53 keil Exp $

SHELL = /bin/sh


-include ../Rules.make

CURRENT_DIR := $(shell pwd)

#
# passed down to sub-makefiles to communicate location of eft
# related libraries and header files  
#
EFT_LIBDIR  = $(CURRENT_DIR)/eft
EFT_INCDIR  = $(CURRENT_DIR)/eft/include
EFT_AUTHDIR = $(CURRENT_DIR)/wuauth

CFLAGS += -g -Wall

export EFT_LIBDIR EFT_INCDIR EFT_AUTHDIR CONFIG_EFTP_READLINE CONFIG_EFTD_WUAUTH CRYPTLIB CFLAGS  INSTALL INSTALL_PROGRAM INSTALL_SCRIPT INSTALL_DATA SYSVINIT_RC_DIR prefix exec_prefix bindir sbindir BINDIR SBINDIR CONFIG_EFTP_SETUID

LIBDIRS    = eft  wuauth
CLIENTDIRS = eftp
SERVERDIRS = eftd 
TESTDIRS   = testing
EFTDIRS    = $(CLIENTDIRS) $(SERVERDIRS) 

BASIC_SUBDIRS := $(LIBDIRS) $(EFTDIRS)
ALL_SUBDIRS := $(BASIC_SUBDIRS) $(TESTDIRS)


ifeq ($(CONFIG_EFTP),y)
APP_SUBDIRS += eftp
endif

ifeq ($(CONFIG_EFTD),y)
APP_SUBDIRS += eftd
endif

DEFAULT_SUBDIRS := $(LIBDIRS)
DEFAULT_SUBDIRS += $(APP_SUBDIRS)

default: $(DEFAULT_SUBDIRS) 	
#	echo $(DEFAULT_SUBDIRS)
#	echo $(MAKEFLAGS)
#

all: $(ALL_SUBDIRS)

$(CLIENTDIRS): eft

$(SERVERDIRS) testing: $(LIBDIRS)

$(ALL_SUBDIRS): FORCE
	$(MAKE) -C $@

FORCE:

clean:
	for i in $(ALL_SUBDIRS); do $(MAKE) -C $$i clean; done

distclean: clean
	rm -f config.h `find . \( -name '*~' -o -name '#*#' -o -name core \)`

install: $(APP_SUBDIRS) 
ifneq ($(APP_SUBDIRS),)
	for i in $(APP_SUBDIRS); do $(MAKE) -C $$i install; done
endif

new_version:
	make -C eft new_version

.PHONY: default all clean distclean install FORCE new_version

