###############################################################################
#
# Makefile for adt module.
#
###############################################################################

PROJECT_ROOT = ..
include ../make/common.mk

OBJECTS = dx_buf.o \
	dx_hash.o \
	dx_msgq.o \
	dx_str.o \
	dx_vec.o \
        dx_lst.o

TARGET = dx
PROGFLAGS = -ansi -I../
PROGDEFS = -DUNIX

LIBTARGET = $(libdir)/lib$(TARGET).a
all: $(LIBTARGET)
include ../make/module.mk
