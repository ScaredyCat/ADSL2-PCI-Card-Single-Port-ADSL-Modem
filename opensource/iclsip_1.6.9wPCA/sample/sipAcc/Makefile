###############################################################################
#
# Makefile for megaco module.
#
###############################################################################

PROJECT_ROOT = ../../src
include ../../src/make/common.mk

OBJECTS = sipAcc.o\
	acc_sip.o \
	acc_low.o \
	acc_adt.o \
	acc_cm.o \
	acc_sdp.o

TARGET = sipAcc

PROGFLAGS = -I../../src/
PROGDEFS = -DUNIX
PROGLIBS = -lcclsip -lpthread -lm

APPTARGET = $(bindir)/$(TARGET)
all: $(APPTARGET)
	cp ReqMsg $(bindir)
	chmod 644 $(bindir)/ReqMsg
	cp RspMsg $(bindir)
	chmod 644 $(bindir)/RspMsg
	cp IMSReqMsg $(bindir)
	chmod 644 $(bindir)/IMSReqMsg
	cp IMSRspMsg $(bindir)
	chmod 644 $(bindir)/IMSRspMsg
	cp testmsg $(bindir)
	chmod 644 $(bindir)/testmsg
include ../../src/make/module.mk
