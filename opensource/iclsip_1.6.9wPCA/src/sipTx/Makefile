###############################################################################
#
# Makefile for adt module.
#
###############################################################################

PROJECT_ROOT = ..
include ../make/common.mk

OBJECTS = CTransactionDatabase.o \
	ClientStateMachine.o \
	ServerStateMachine.o \
	Transport.o \
        sipTx.o

TARGET = sipTx
PROGFLAGS = -ansi -I../
PROGDEFS = -DUNIX

LIBTARGET = $(libdir)/lib$(TARGET).a
all: $(LIBTARGET)
include ../make/module.mk
