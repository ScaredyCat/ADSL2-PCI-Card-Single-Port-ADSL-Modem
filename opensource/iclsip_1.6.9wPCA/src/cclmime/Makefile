###############################################################################
#
# Makefile for sip module.
#
###############################################################################

PROJECT_ROOT = ..
include ../make/common.mk

OBJECTS = address.o \
	body.o \
	file_stream.o \
	header.o \
	iterator.o \
	list.o \
	tcp.o \
	mapfile_stream.o \
	memory_stream.o \
	message.o \
	monitor.o \
	muerror.o \
	mutil.o \
	parse822.o \
	property.o \
	memory_stream.o \
	stream.o \
	mime.o

TARGET = mime
PROGFLAGS = -I../
PROGDEFS = -DWITH_PTHREAD -DHAVE_PTHREAD_H -DUNIX

LIBTARGET = $(libdir)/lib$(TARGET).a
all: $(LIBTARGET)
include ../make/module.mk
