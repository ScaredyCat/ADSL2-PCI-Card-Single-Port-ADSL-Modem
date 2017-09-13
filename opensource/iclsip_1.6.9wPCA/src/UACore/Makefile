###############################################################################
#
# Makefile for SIP UA Core module.
#
###############################################################################

PROJECT_ROOT = ..
include ../make/common.mk

OBJECTS = ua_cfg.o \
	ua_class.o \
	ua_cm.o \
	ua_content.o \
	ua_dlg.o \
	ua_evtpkg.o \
	ua_int.o \
	ua_mgr.o \
	ua_msg.o \
	ua_sdp.o \
	ua_sipmsg.o \
	ua_sub.o \
	ua_user.o
	

TARGET = UACore
PROGFLAGS = -ansi -I../
PROGDEFS = -DUNIX

LIBTARGET = $(libdir)/lib$(TARGET).a
all: $(LIBTARGET)
include ../make/module.mk

