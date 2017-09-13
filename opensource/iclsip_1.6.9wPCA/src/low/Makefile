###############################################################################
#
# Makefile for low module.
#
###############################################################################

PROJECT_ROOT = ..
include ../make/common.mk

OBJECTS = cx_misc.o \
	cx_mutex.o \
	cx_mem.o \
	cx_thrd.o \
	cx_timer.o \
	cx_sock.o \
	cx_event.o

TARGET = cx
PROGFLAGS = -ansi -I../
PROGDEFS = -DUNIX

ifneq ($(IPv6), )

OBJECTS = cx_misc.o \
	cx_mutex.o \
	cx_mem.o \
	cx_thrd.o \
	cx_timer.o \
	cx_sock6.o \
	cx_event.o

PROGDEFS += -DHAS_IPv6_LIB

PROGFLAGS += -I../include

TARGET = cx6

endif

LIBTARGET = $(libdir)/lib$(TARGET).a

all: $(LIBTARGET)
include ../make/module.mk
