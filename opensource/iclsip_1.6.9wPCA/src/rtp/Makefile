###############################################################################
#
# Makefile for rtp module.
#
###############################################################################

PROJECT_ROOT = ..
include ../make/common.mk

OBJECTS = rtp.o \
	rtcp.o \
	rtp_helper.o

TARGET = rtp
PROGFLAGS =
PROGDEFS =

LIBTARGET = $(libdir)/lib$(TARGET).a
all: $(LIBTARGET)
include ../make/module.mk
