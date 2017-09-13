###############################################################################
#
# Makefile for sdp module.
#
###############################################################################

PROJECT_ROOT = ..
include ../make/common.mk

OBJECTS = sdp_ta.o \
	sdp_tb.o \
	sdp_tc.o \
	sdp_tk.o \
	sdp_tm.o \
	sdp_mses.o \
	sdp_to.o \
	sdp_tr.o \
	sdp_sess.o \
	sdp_tt.o \
	sdp_tz.o \
	sdp_tses.o \
	sdp_tok.o \
	sdp_utl.o

TARGET = sdp
PROGFLAGS = -ansi -I../
PROGDEFS = -DUNIX

LIBTARGET = $(libdir)/lib$(TARGET).a
all: $(LIBTARGET)
include ../make/module.mk
