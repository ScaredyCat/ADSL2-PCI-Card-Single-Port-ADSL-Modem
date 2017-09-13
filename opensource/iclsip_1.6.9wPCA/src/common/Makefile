###############################################################################
#
# Makefile for common module.
#
###############################################################################

PROJECT_ROOT = ..
include ../make/common.mk

OBJECTS = cm_trace.o \
	cm_utl.o

TARGET = cm
PROGFLAGS = -I../
PROGDEFS = -DUNIX

LIBTARGET = $(libdir)/lib$(TARGET).a
all: $(LIBTARGET)
include ../make/module.mk
